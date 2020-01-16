
#include "cio/cio.hpp"

#include "dib_reader.hpp"
#include "my_cstrcat.hpp"
#include "my_mkdir.hpp"
#include "pe_reader.hpp"
#include "png_writer.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>


namespace
{
	bool debug_flag = false;
	bool debug_verbose_flag = false;
	bool true_color = false;
	int compression_level = 2;
}

#define DEBUG_LOG(x) if (debug_flag) { (cio::err << "DEBUG: " << x) << cio::endl; }
//#define DEBUG_LOG(x) { ; }

#define DEBUG_LOG_VERBOSE(x) if (debug_verbose_flag) { (cio::err << "DEBUG: " << x) << cio::endl; }
//#define DEBUG_LOG(x) { ; }

#define ABORT_ON(cond, msg) \
	if (cond) \
	{ \
		(cio::err << "Aborting: " << msg) << cio::endl; \
		std::exit(1); \
	}

#define ABORT_ON_FILE(cond, msg) \
	if (cond) \
	{ \
		(cio::err << "Error: " << filename << ": " << msg) << cio::endl; \
		goto start_next_file; \
	}

#define ABORT_ON_FILE_ID(cond, msg) \
	if (cond) \
	{ \
		(cio::err << "Error: " << filename << '#' << id << ": " << msg) << cio::endl; \
		goto start_next_bitmap; \
	}

#define ABORT_ON_FILE_ID_INNER(cond, msg) \
	if (cond) \
	{ \
		(cio::err << "Error: " << filename << '#' << id << ": " << msg) << cio::endl; \
		goto start_next_bitmap_inner; \
	}


int main(int argc, char** argv)
{
	int argpos = 1;

	while (argpos < argc)
	{
		if (argv[argpos][0] == '-')
		{
			char flag = argv[argpos][1];

			switch (flag)
			{
				case 'd':
					debug_flag = true;
					break;

				case 'q':
					++argpos;

					if ((argc - argpos) == 0)
						goto usage_error;

					{
						char qchar = argv[argpos][0];

						ABORT_ON(qchar < '1' || qchar > '9', "Compression level must be a number between 1 and 9")

						compression_level = static_cast<int>(qchar - '0');
					}

					break;

				case 't':
					true_color = true;
					break;

				case 'v':
					debug_verbose_flag = true;
					break;

				default:
					ABORT_ON(true, argv[0] << ": invalid option -- '" << flag << '\'')
			}
		}
		else
		{
			break;
		}
		
		++argpos;
	}

	if ((argc - argpos) == 0)
		goto usage_error;

	while (argpos < argc)
	{
		// begin abortable region: start_next_file
		{
			const char* filename = argv[argpos];

			DEBUG_LOG("Processing " << filename << "...")

			cio::stream file(filename, cio::stream::mode_read);
			ABORT_ON_FILE(!file.is_open(), "Failed to open file")

			pe_reader pe(std::move(file));

			ABORT_ON_FILE(!pe.read_header(), "Invalid PE file")

			auto table = pe.read_bitmap_table();
			ABORT_ON_FILE(table.size() == 0, "No bitmaps found")
			DEBUG_LOG(table.size() << " bitmaps found");

			const char* basename;

			for (const char* p = filename; *p; ++p)
			{
				if (*p == '/' || *p == '\\')
					basename = p+1;
			}

			auto dirname = my_cstrcat("dump/", basename);
			ABORT_ON_FILE(!dirname, "Memory allocation failed")
			DEBUG_LOG("Writing to directory: " << dirname.get());
			my_mkdir(dirname.get());

			std::size_t largest_resource = 0;

			for (auto resource : table)
			{
				if (resource.second.second > largest_resource)
					largest_resource = resource.second.second;
			}

			ABORT_ON_FILE(largest_resource > 512*1024*1024, "Abnormally large resource size (>512MB)")
			std::unique_ptr<char[]> databuf(new char[largest_resource]);
			ABORT_ON_FILE(!databuf, "Memory allocation failed (" << largest_resource << " bytes)")

			for (auto resource : table)
			{
				// begin abortable region: start_next_bitmap
				{
					int id = resource.first;

					std::size_t datasize = resource.second.second;

					ABORT_ON_FILE_ID(id < 0, "Resource ID below 0")
					ABORT_ON_FILE_ID(id > 99999, "Resource ID above 99999")

					ABORT_ON_FILE_ID(datasize < 40, "DIB header too small")

					bool result = pe.read_resource(databuf.get(), resource.second);
					ABORT_ON_FILE_ID(!result, "Reading resource failed.")

					char pngname[11];
					std::sprintf(pngname, "/%i.png", id);
					auto png_out_filename = my_cstrcat(dirname.get(), pngname);
					DEBUG_LOG_VERBOSE("Creating file: " << png_out_filename.get());
					cio::stream png_out_file(png_out_filename.get(), cio::stream::mode_write);

					ABORT_ON_FILE_ID(!png_out_file.is_open(), "Failed to open " << png_out_filename.get() << " for writing.")

					dib_reader dib(databuf.get(), datasize);

					dib.start();

					const char* format_error = dib.check_format();
					ABORT_ON_FILE_ID(format_error != nullptr, "DIB format error: " << format_error)

					// Condom wrapper for png's setjmp error handling
					[&]()
					{
						// begin abortable region: start_next_bitmap_inner
						{
							std::map<png_color, char, png_color_cmp> palette_map;
							std::vector<std::unique_ptr<char[]>> pixel_rows;
							std::array<png_color, 256> palette;
							unsigned char palette_size = 0;
							std::unique_ptr<char[]> linebuf(new char[dib.width() * 4]);

							int width = dib.width();
							int height = dib.height();

							ABORT_ON_FILE_ID_INNER(!linebuf, "Failed to allocate line buffer")

							png_writer png(std::move(png_out_file));

							ABORT_ON_FILE_ID_INNER(!png.init(), "libpng initialization failed")

							// This is a setjmp call and must follow the rules of setjmp where each
							//   call to a libpng function is considered to be a longjmp.
							PNG_WRITER_CATCH(png)
							{
								ABORT_ON_FILE_ID_INNER(true, "libpng error")
							}

							// First pass generates palette entries and create pixel rows
							if (!true_color)
							{
								palette_size = 1;
								palette[0] = {0,0,0};
								palette_map.insert({palette[0], 0});
								pixel_rows.reserve(height);

								for (int y = 0; y < height; ++y)
								{
									dib.read_line(linebuf.get(), y);

									std::unique_ptr<char[]> row(new char[width]);

									for (int x = 0; x < width; ++x)
									{
										typedef unsigned int uc;
										unsigned char r = uc(uc(linebuf[x*4+0]) & 0xFF);
										unsigned char g = uc(uc(linebuf[x*4+1]) & 0xFF);
										unsigned char b = uc(uc(linebuf[x*4+2]) & 0xFF);

										png_color pixel = {r,g,b};

										auto index_it = palette_map.find(pixel);

										if (index_it == palette_map.end())
										{
											row[x] = palette_map[pixel] = char(palette_size);
											palette[palette_size] = pixel;
											++palette_size;

											if (palette_size == 0)
											{
												pixel_rows.clear();
												DEBUG_LOG(filename << ':' << id << " has more than 255 colours, generating true colour");
												goto end_palette_pass;
											}
										}
										else
										{
											row[x] = index_it->second;
										}
									}

									pixel_rows.emplace_back(std::move(row));
								}
							}
							end_palette_pass:

							DEBUG_LOG_VERBOSE("Palette size: " << int(palette_size));

							if (palette_size == 0)
							{
								// Fail case: write true-colour format
								png.write_header(compression_level, width, height, PNG_COLOR_TYPE_RGB_ALPHA);

								for (int y = 0; y < height; ++y)
								{
									dib.read_line(linebuf.get(), y);
									png.write_row(linebuf.get());
								}
							}
							else
							{
								png.write_header(compression_level, width, height, PNG_COLOR_TYPE_PALETTE, palette_size, palette.data());

								// Second pass write the paletted pixel data
								for (const auto& row : pixel_rows)
								{
									png.write_row(row.get());
								}
							}

							png.write_end();
						}
						// end abortable region: start_next_bitmap_inner
						start_next_bitmap_inner:
						;
					}();
				}
				// end abortable region: start_next_bitmap
				start_next_bitmap:
				;
			}
		}
		// end abortable region: start_next_file
		start_next_file:
		++argpos;
	}	

	return 0;

usage_error:
	cio::err << "usage: " << argv[0] << " [-d] [-q quality] [-t] [-v] filename.egf..." << cio::endl;
	return 1;
}

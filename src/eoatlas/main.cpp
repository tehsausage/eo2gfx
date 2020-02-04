
#include "cio/cio.hpp"

#include "sha256/sha256.h"
#include "sha256/sha256_to_hex.h"

#include "my_cstrcat.hpp"

#include "png_reader.hpp"
#include "png_writer.hpp"

#include <algorithm>
#include <array>
#include <memory>
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
		std::exit(1); \
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

	if ((argc - argpos) <= 1)
		goto usage_error;

	{
		const char* output_filename = argv[argpos++];

		int output_width = 0;
		int output_height = 0;

		struct Input
		{
			const char* filename;
			int rows;
			int cols;
			int x_off;
			int y_off;
			int x_adj;
			int y_adj;
		};

		std::vector<Input> inputs;
		int inputpos = 0;

		while (argpos < argc)
		{
			const char* filename = argv[argpos];

			DEBUG_LOG("Processing " << filename << "...")

			cio::stream png_in_file(filename, cio::stream::mode_read);

			ABORT_ON_FILE(!png_in_file.is_open(), "Failed to open input file")

			[&]()
			{
				png_reader png(std::move(png_in_file));

				ABORT_ON_FILE(!png.init(), "libpng initialization failed")

				// This is a setjmp call and must follow the rules of setjmp where each
				//   call to a libpng function is considered to be a longjmp.
				//PNG_READER_CATCH(png)
				//{
				//	ABORT_ON_FILE(true, "libpng error")
				//}

				ABORT_ON_FILE(!png.check_sig(), "image is not a valid PNG file")

				png_reader_info header = png.read_header();

				ABORT_ON_FILE(!header.valid, "error reading PNG header")
				ABORT_ON_FILE(header.bit_depth != 8 || header.color_type != PNG_COLOR_TYPE_RGB_ALPHA,
					"image could not be loaded in the correct format")

				int cols = static_cast<int>(header.width);
				int rows = static_cast<int>(header.height);

				int x_off = cols;
				int y_off = 0;

				int last_row = 0;
				int last_col = 0;

				bool hit_pixels = false;

				std::unique_ptr<char[]> linebuf(new char[header.rowbytes]);

				for (int y = 0; y < rows; ++y)
				{
					png.read_row(linebuf.get());

					bool all_alpha = true;

					for (int x = 0; x < cols; ++x)
					{
						unsigned char alpha = linebuf[x];

						if (alpha != 0)
						{
							all_alpha = false;
							last_col = std::max(last_col, x);
						}
						else
						{
							if (x < x_off)
							{
								DEBUG_LOG("new x off =" << x)
							}

							x_off = std::min(x_off, x);
						}
					}

					if (all_alpha)
					{
						if (!hit_pixels)
							y_off = y;
					}
					else
					{
						hit_pixels = true;
						last_row = y;
					}
				}

				int x_adj = 0;
				int y_adj = rows - last_row;

				output_width += cols;
				output_height = std::max(rows, output_height);

				DEBUG_LOG(filename << ':' << ' ' << cols << 'x' << rows)
				DEBUG_LOG("\toffset:" << ' ' << x_off << 'x' << y_off)
				DEBUG_LOG("\tadjust:" << ' ' << x_adj << 'x' << y_adj)

				inputs.emplace_back(Input{filename, rows, cols, x_off, y_off, x_adj, y_adj});
			}();

			++argpos;
			++inputpos;
		}

		// Fixme: This adds a buffer around both sides of the image for overlapping invisible pixels to be loaded in to
		// Should be calculated by the greatest of x/y_off and x/y_adj
		int SAFETY_BUFFER = 4 * 1024; // 4KB: big enough for 512 pixels either side of the image

		{
			// Exists for the debug macro output
			const char* filename = output_filename;

			DEBUG_LOG("Generating atlas: " << filename << ": " << output_width << 'x' << output_height)

			{
				std::map<png_color, char, png_color_cmp> palette_map;
				std::vector<std::unique_ptr<char[]>> pixel_rows;
				std::array<png_color, 256> palette;
				unsigned char palette_size = 0;

				int stride = output_width * 4;
				std::unique_ptr<char[]> databuf(new char[(stride * output_height) + SAFETY_BUFFER]);

				for (int i = 0; i < (stride * output_height) + SAFETY_BUFFER; ++i)
					databuf[i] = 0;

				{
					int col = 0;

					for (auto& input : inputs)
					{
						DEBUG_LOG("Adding " << input.filename << "...")

						cio::stream png_in_file(input.filename, cio::stream::mode_read);
						ABORT_ON_FILE(!png_in_file.is_open(), "Failed to open input file")

						png_reader png(std::move(png_in_file));

						ABORT_ON_FILE(!png.init(), "libpng initialization failed")

						// This is a setjmp call and must follow the rules of setjmp where each
						//   call to a libpng function is considered to be a longjmp.
						//PNG_READER_CATCH(png)
						//{
						//	ABORT_ON_FILE(true, "libpng error")
						//}

						ABORT_ON_FILE(!png.check_sig(), "image is not a valid PNG file")

						png_reader_info header = png.read_header();

						png.read_image(&databuf[SAFETY_BUFFER/2] + col*4, stride);
						col += header.width;

						png.read_end();
					}
				}

				{
					DEBUG_LOG("Calculating hash...")

					sha256_context hash_ctx{};
					char digest[32];
					char cdigest[65];
					sha256_start(&hash_ctx);
					
					std::unique_ptr<char[]> hashlinebuf(new char[stride]);

					for (int y = 0; y < output_height; ++y)
					{
						// Ignore the colour conversion data for image hashing
						// Also ignore alpha channel
						for (int i = 0; i < stride; ++i)
						{
							if (i % 4 == 3)
								hashlinebuf[i] = 0x00;
							else
								hashlinebuf[i] = *((unsigned char*)&databuf[SAFETY_BUFFER/2] + stride * y + i) & 0xF8;
						}

						sha256_update(&hash_ctx, hashlinebuf.get(), stride);
					}

					sha256_finish(&hash_ctx, digest);
					sha256_to_hex(digest, 32, cdigest);
					cdigest[64] = '\0';

					DEBUG_LOG(cdigest)

					{
						auto hash_output_filename = my_cstrcat(output_filename, ".sha256");
						DEBUG_LOG_VERBOSE("Creating file: " << hash_output_filename.get())
						cio::stream sha256_out_file(hash_output_filename.get(), cio::stream::mode_write);
						sha256_out_file.write(cdigest);
					}
				}

				{
					{
						auto offs_output_filename = my_cstrcat(output_filename, ".offs2");
						DEBUG_LOG_VERBOSE("Creating file: " << offs_output_filename.get())
						cio::stream offs_out_file(offs_output_filename.get(), cio::stream::mode_write);

						int col = 0;

						for (size_t i = 0; i < inputs.size(); ++i)
						{
							offs_out_file << col << ',' << inputs[i].cols
							                     << ',' << inputs[i].rows
							                     << ',' << inputs[i].x_off
							                     << ',' << inputs[i].y_off
							                     << ',' << inputs[i].x_adj
							                     << ',' << inputs[i].y_adj << ';';
							col += inputs[i].cols;
						}
					}
				}

				DEBUG_LOG_VERBOSE("Creating file: " << output_filename)
				cio::stream png_out_file(output_filename, cio::stream::mode_write);

				ABORT_ON_FILE(!png_out_file.is_open(), "Failed to open " << output_filename << " for writing.")

				png_writer png(std::move(png_out_file));

				ABORT_ON_FILE(!png.init(), "libpng initialization failed")

				// This is a setjmp call and must follow the rules of setjmp where each
				//   call to a libpng function is considered to be a longjmp.
				//PNG_WRITER_CATCH(png)
				//{
				//	ABORT_ON_FILE(true, "libpng error")
				//}

				// First pass generates palette entries and create pixel rows
				if (!true_color)
				{
					palette_size = 1;
					palette[0] = {0,0,0};
					palette_map.insert({palette[0], 0});
					pixel_rows.reserve(output_height);

					for (int y = 0; y < output_height; ++y)
					{
						const char* linebuf = &databuf[SAFETY_BUFFER/2] + stride * y;

						std::unique_ptr<char[]> row(new char[output_width]);

						for (int x = 0; x < output_width; ++x)
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
									DEBUG_LOG(output_filename << " has more than 255 colours, generating true colour")
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

				DEBUG_LOG_VERBOSE("Palette size: " << int(palette_size))

				if (palette_size == 0)
				{
					// Fail case: write true-colour format
					png.write_header(compression_level, output_width, output_height, PNG_COLOR_TYPE_RGB_ALPHA);

					for (int y = 0; y < output_height; ++y)
					{
						png.write_row(&databuf[SAFETY_BUFFER/2] + stride * y);
					}
				}
				else
				{
					png.write_header(compression_level, output_width, output_height, PNG_COLOR_TYPE_PALETTE, palette_size, palette.data());

					// Second pass write the paletted pixel data
					for (const auto& row : pixel_rows)
					{
						png.write_row(row.get());
					}
				}

				png.write_end();
			}
		}
	}

	return 0;

usage_error:
	cio::err << "usage: " << argv[0] << " [-d] [-v] output.png input.png..." << cio::endl;
	return 1;
}


#include "cio/cio.hpp"

#include "my_cstrcat.hpp"

#include "png_reader.hpp"
#include "dib_writer.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <map>
#include <stack>
#include <vector>

namespace
{
	bool debug_flag = false;
	bool debug_verbose_flag = false;
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
		(cio::err << "Error: " << input_filename << ": " << msg) << cio::endl; \
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

	const char* format_name;
	const char* input_filename;
	const char* output_filename;

	dib_writer::format_t format;

	if ((argc - argpos) <= 0)
		goto usage_error;

	format_name = argv[argpos++];

	if ((argc - argpos) <= 0)
		goto usage_error;

	input_filename = argv[argpos++];

	if ((argc - argpos) <= 0)
		goto usage_error;

	output_filename = argv[argpos++];


	if (std::strcmp(format_name, "555") == 0)
		format = dib_writer::format_555;
	else if (std::strcmp(format_name, "565") == 0)
		format = dib_writer::format_565;
	else if (std::strcmp(format_name, "888") == 0)
		format = dib_writer::format_888;
	else
	{
		cio::err << "unknown format. supported formats: 555, 565, 888" << cio::endl;
		return 1;
	}

	[&]()
	{
		DEBUG_LOG("Converting " << input_filename << "...")
		cio::stream png_in_file(input_filename, cio::stream::mode_read);
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

		ABORT_ON_FILE(!header.valid, "error reading PNG header")
		ABORT_ON_FILE(header.bit_depth != 8 || header.color_type != PNG_COLOR_TYPE_RGB_ALPHA,
			"image could not be loaded in the correct format")

		int cols = static_cast<int>(header.width);
		int rows = static_cast<int>(header.height);

		// BMP lines are stored in reverse order
		std::stack<std::unique_ptr<char[]>> lines;

		for (int y = 0; y < rows; ++y)
		{
			DEBUG_LOG_VERBOSE("Reading line " << y)
			std::unique_ptr<char[]> linebuf(new char[header.rowbytes + 4]);
			png.read_row(linebuf.get());

			lines.push(std::move(linebuf));
		}

		dib_writer dib;
		cio::stream outfile(output_filename, "wb");

		DEBUG_LOG("Writing BMP header (size=0,start=0)")
		outfile.write("BM\0\0\0\0\0\0\0\0\0\0\0\0", 14);

		DEBUG_LOG("Writing DIB header (width=" << cols << ",height=" << rows << ",format=" << format_name <<")")

		dib.write_header(outfile, cols, rows, format);

		int32_t pixel_start = static_cast<int32_t>(outfile.tell());

		for (; !lines.empty(); lines.pop())
		{
			auto&& line = lines.top();

			DEBUG_LOG_VERBOSE("Writing line " << lines.size() - 1)

			dib.write_line(outfile, line.get());
		}

		int32_t total_size = static_cast<int32_t>(outfile.tell());

		DEBUG_LOG("Writing BMP size (size=" << total_size << ")")
		outfile.seek(2);
		dib_writer::write_u32_le(outfile, total_size);

		DEBUG_LOG("Writing BMP offset (offset=" << pixel_start << ")")
		outfile.seek(10);
		dib_writer::write_u32_le(outfile, pixel_start);

		outfile.close();
	}();

	return 0;

usage_error:
	cio::err << "usage: " << argv[0] << " [-d] [-v] format input.png output.bmp" << cio::endl;
	return 1;
}

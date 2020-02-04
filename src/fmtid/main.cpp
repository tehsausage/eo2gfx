
#include "cio/cio.hpp"

#include "my_cstrcat.hpp"

#include "png_reader.hpp"
#include "png_writer.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <map>
#include <vector>

namespace
{
	bool debug_flag = false;
	bool debug_verbose_flag = false;
	const char* s_555 = "555";
	const char* s_565 = "565";
	const char* s_888 = "888";

}

static unsigned char bpp5table[64];
static ptrdiff_t bpp5tablesize;
static unsigned char bpp6table[128];
static ptrdiff_t bpp6tablesize;

void init_tables()
{
	for (int i = 0; i < 32; ++i)
	{
		int x = i * 510 / 31;
		int lsb = x & 1;

		bpp5table[bpp5tablesize++] = (x + lsb) / 2;

		//if (lsb)
		//	bpp5table[bpp5tablesize++] = x / 2 + 1;
	}

	for (int i = 0; i < 64; ++i)
	{
		int x = i * 510 / 63;
		int lsb = x & 1;

		bpp6table[bpp6tablesize++] = (x + lsb) / 2;

		//if (lsb)
		//	bpp6table[bpp6tablesize++] = x / 2 + 1;
	}
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
	if ((argc - argpos) <= 0)
		goto usage_error;

	init_tables();

	{
		while (argpos < argc)
		{
			const char* filename = argv[argpos];

			DEBUG_LOG_VERBOSE("Processing " << filename << "...")

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

				const char* fmt_name = s_555;

				ABORT_ON(header.rowbytes < cols * 4U, "image not decoded in to correct format")

				std::unique_ptr<char[]> linebuf(new char[header.rowbytes + 4]);
				linebuf[header.rowbytes] = 0xA5;
				linebuf[header.rowbytes + 1] = 0xA5;
				linebuf[header.rowbytes + 2] = 0xA5;
				linebuf[header.rowbytes + 3] = 0xA5;

				for (int y = 0; y < rows; ++y)
				{
					png.read_row(linebuf.get());

					unsigned char row_good = 1;
					unsigned char row_good2 = 1;

					for (int x = 0; x < cols; ++x)
					{
						unsigned char r = (unsigned char)linebuf[x * 4];
						unsigned char g = (unsigned char)linebuf[x * 4 + 1];
						unsigned char b = (unsigned char)linebuf[x * 4 + 2];
						unsigned char a = (unsigned char)linebuf[x * 4 + 3];

						if (!a)
							continue;

						unsigned char r_good = 0;
						unsigned char g_good = 0;
						unsigned char g_good2 = 0;
						unsigned char b_good = 0;

						for (size_t i = 0; i < sizeof bpp5table; ++i)
						{
							r_good |= (r == bpp5table[i]);
						}

						for (size_t i = 0; i < sizeof bpp5table; ++i)
						{
							g_good |= (g == bpp5table[i]);
						}

						for (size_t i = 0; i < sizeof bpp6table; ++i)
						{
							g_good2 |= (g == bpp6table[i]);
						}

						for (size_t i = 0; i < sizeof bpp5table; ++i)
						{
							b_good |= (b == bpp5table[i]);
						}

						row_good &= (r_good & g_good & b_good);
						row_good2 &= (r_good & g_good2 & b_good);

					}

					if (!row_good)
					{
						if (fmt_name == s_555)
						{
							fmt_name = s_565;
						}
					}

					if (!row_good && !row_good2)
					{
						fmt_name = s_888;
						break;
					}
				}

				DEBUG_LOG_VERBOSE(filename << ':' << ' ' << cols << 'x' << rows << ": " << fmt_name)

				cio::out << filename << ' ' << fmt_name << cio::endl;
			}();

			++argpos;
		}

	}

	return 0;

usage_error:
	cio::err << "usage: " << argv[0] << " [-d] [-v] input.png..." << cio::endl;
	return 1;
}

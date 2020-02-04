
#include "cio/cio.hpp"

#include "my_cstrcat.hpp"

#include "dib_reader.hpp"

#include <cassert>
#include <cstring>
#include <memory>

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

	const char* input_filename;

	if ((argc - argpos) <= 0)
		goto usage_error;

	input_filename = argv[argpos++];

	[&]()
	{
		DEBUG_LOG_VERBOSE("Examining " << input_filename << "...")
		cio::stream bmp_in_file(input_filename, cio::stream::mode_read);
		ABORT_ON_FILE(!bmp_in_file.is_open(), "Failed to open input file")
		char buf[4];
		ABORT_ON_FILE(!bmp_in_file.read(buf, 2), "read failed")

		ABORT_ON_FILE(std::memcmp(buf, "BM", 2) != 0, "'BM' magic bytes not found")

		ABORT_ON_FILE(!bmp_in_file.read(buf, 4), "read failed")
		long bmp_size = int_pack_32(buf);
		DEBUG_LOG_VERBOSE("bmp_size = " << bmp_size)

		ABORT_ON_FILE(!bmp_in_file.skip(4), "seek failed")
		ABORT_ON_FILE(!bmp_in_file.read(buf, 4), "read failed")
		long pixel_offset = int_pack_32(buf);
		DEBUG_LOG_VERBOSE("pixel_offset = " << pixel_offset)

		long dib_size = bmp_size - 14;

		ABORT_ON(dib_size <= 0, "DIB size (" << dib_size << ") is too small")

		std::unique_ptr<char[]> dib_data(new char[dib_size]);
		long dib_bytes_read = bmp_in_file.read(dib_data.get(), dib_size);
		ABORT_ON_FILE(!dib_bytes_read, "read failed")

		dib_reader dib(dib_data.get(), dib_size);

		dib.start();

		auto c2str = [](dib_reader::Compression c)
		{
			static char buf[12];
			if (c == dib_reader::Compression::RGB) return "RGB";
			else if (c == dib_reader::Compression::BitFields) return "BitFields";
			else return "Other";
		};

		cio::out << "DIB Size: " << dib_size;
			cio::out << " (bytes read = " << dib_bytes_read << ")" << cio::endl;
		cio::out << "DIB Header Size: " << dib.header_size() << cio::endl;
		cio::out << cio::endl;

		cio::out << "Width: " << dib.width() << cio::endl;
		cio::out << "Height: " << dib.height() << cio::endl;
		cio::out << "Res: " << dib.hres() << 'x' << dib.vres() << cio::endl;
		cio::out << "Depth: " << dib.depth() << cio::endl;
		cio::out << "Stride: " << dib.stride() << cio::endl;
		cio::out << "Compression: " << c2str(dib.compression());
			cio::out << " (" << dib.compression() << ")" << cio::endl;
		cio::out << cio::endl;
		cio::out << "Palette size: " << dib.palette_size() << cio::endl;
		cio::out << "Image data size: " << dib.image_size();
			cio::out << " (actual = " << (dib_size - dib.header_size() - dib.palette_size()) << ")";
			cio::out << " (expected = " << (dib.stride() * dib.height()) << ")" << cio::endl;
		cio::out << "Stride: " << dib.stride() << cio::endl;

		if (dib.header_size() >= 52 || dib.compression() == dib_reader::BitFields)
		{
			cio::out << "Bitfield masks: " << cio::endl;
			printf("\tR = %08x\n", dib.red_mask());
			printf("\tG = %08x\n", dib.green_mask());
			printf("\tB = %08x\n", dib.blue_mask());

			if (dib.palette_size() >= 16)
				printf("\tA = %08x\n", dib.alpha_mask());
		}
	}();

	return 0;

usage_error:
	cio::err << "usage: " << argv[0] << " [-d] [-v] input.bmp" << cio::endl;
	return 1;
}

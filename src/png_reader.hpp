#ifndef PNG_READER_HPP
#define PNG_READER_HPP

#include "cio/cio.hpp"
#include "tidy_ptr.hpp"

#include <png.h>

#include <cstdint>
#include <utility>

struct png_reader_info
{
	bool valid;
	png_uint_32 width;
	png_uint_32 height;
	int bit_depth;
	int color_type;
	int number_of_passes;
	png_uint_32 rowbytes;
};

struct png_reader
{
	private:
		cio::stream file;
		tidy_ptr<png_struct> png_ptr;
		tidy_ptr<png_info> info_ptr;

		png_reader_info info;
		int sigbytes;

	public:
		png_reader(cio::stream&& file)
			: file(std::move(file))
			, sigbytes(0)
		{ }

		png_reader(const png_reader&) = delete;
		png_reader(png_reader&& other) = default;
		png_reader& operator=(const png_reader&) = delete;
		png_reader& operator=(png_reader&& other) = default;

		bool init();
		bool check_sig();
		png_reader_info read_header();
		void read_row(char* data);
		void read_image(char* data, int stride);
		void read_end();

		cio::stream& get_file()
		{
			return file;
		}

		cio::stream&& finish()
		{
			return std::move(file);
		}

		png_structp get_png_ptr() { return png_ptr.get(); }

		~png_reader();
};

#define PNG_READER_CATCH(png) if (setjmp(png_jmpbuf(png.get_png_ptr())) != 0)

#endif // PNG_READER_HPP

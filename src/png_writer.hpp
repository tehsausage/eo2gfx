#ifndef PNG_WRITER_HPP
#define PNG_WRITER_HPP

#include "cio/cio.hpp"
#include "tidy_ptr.hpp"

#include <png.h>

#include <cstdint>
#include <utility>

struct png_writer
{
	private:
		cio::stream file;
		tidy_ptr<png_struct> png_ptr;
		tidy_ptr<png_info> info_ptr;

	public:
		png_writer(cio::stream&& file)
			: file(std::move(file))
		{ }

		png_writer(const png_writer&) = delete;
		png_writer(png_writer&&) = default;
		png_writer& operator=(const png_writer&) = delete;
		png_writer& operator=(png_writer&&) = default;

		bool init();
		void write_header(int compression_level, int width, int height, int format = PNG_COLOR_TYPE_RGB_ALPHA, int palette_size = 0, png_color* palette_data = nullptr);
		void write_row(const char* data);
		void write_end();

		cio::stream& get_file()
		{
			return file;
		}

		cio::stream&& finish()
		{
			return std::move(file);
		}

		png_structp get_png_ptr() { return png_ptr.get(); }

		~png_writer();
};

struct png_color_cmp
{
	bool operator()(const png_color& a, const png_color& b) const
	{
		std::uint_fast32_t a_ = (std::uint_fast32_t(a.red) << 16) | (std::uint_fast32_t(a.green) << 8)
		                      | std::uint_fast32_t(a.blue);

		std::uint_fast32_t b_ = (std::uint_fast32_t(b.red) << 16) | (std::uint_fast32_t(b.green) << 8)
		                      | std::uint_fast32_t(b.blue);

		return a_ < b_;
	}
};

#define PNG_WRITER_CATCH(png) if (setjmp(png_jmpbuf(png.get_png_ptr())) != 0)

#endif // PNG_WRITER_HPP

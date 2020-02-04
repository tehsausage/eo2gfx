#ifndef DIB_WRITER_HPP
#define DIB_WRITER_HPP

#include "cio/cio.hpp"

#include <cstdint>
#include <utility>

#include "int_pack.hpp"

// writes v3 format
class dib_writer
{
	private:
		int           rs, gs, bs, as;
		std::uint32_t rm, gm, bm, am;

		int* rtable = nullptr;
		int* gtable = nullptr;
		int* btable = nullptr;
		int* atable = nullptr;

	public:
		// made these public so I can use them for BMP header writing :smugshrug:
		static void write_u16_le(cio::stream& os, std::uint16_t value) noexcept
		{
			char buf[2] = {
				char(std::uint32_t(value     ) & 0xFFU),
				char(std::uint32_t(value >> 8) & 0xFFU)
			};

			os.write(buf, sizeof buf);
		}

		static void write_u24_le(cio::stream& os, std::uint32_t value) noexcept
		{
			char buf[3] = {
				char(std::uint32_t(value      ) & 0xFFU),
				char(std::uint32_t(value >>  8) & 0xFFU),
				char(std::uint32_t(value >> 16) & 0xFFU)
			};

			os.write(buf, sizeof buf);
		}

		static void write_u32_le(cio::stream& os, std::uint32_t value) noexcept
		{
			char buf[4] = {
				char(std::uint32_t(value      ) & 0xFFU),
				char(std::uint32_t(value >>  8) & 0xFFU),
				char(std::uint32_t(value >> 16) & 0xFFU),
				char(std::uint32_t(value >> 24) & 0xFFU)
			};

			os.write(buf, sizeof buf);
		}

		enum Compression
		{
			RGB = 0,
			RLE8 = 1,
			RLE4 = 2,
			BitFields = 3,
			JPEG = 4,
			PNG = 5
		};

		enum format_t
		{
			format_555,
			format_565,
			format_888
		};

		format_t m_format;
		int m_bpp;
		int m_width;
		int m_stride;

		// Writes the DIB header and sets the output format
		void write_header(cio::stream& os, int width, int height, format_t format);

		// linebuf is an 8-bit RGBA image
		// linebuf must be at least m_width*m_bpp bytes
		// lines must be written from the bottom up
		void write_line(cio::stream& os, const char* linebuf);
};

#endif // DIB_WRITER_HPP

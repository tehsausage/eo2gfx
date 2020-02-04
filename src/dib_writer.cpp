
#include "dib_writer.hpp"

#include <cmath>
#include <cstddef>
#include <cstring>

#define NUM_CONVERT_TABLES 8

static bool convert_table_init = false;

static int convert_table[NUM_CONVERT_TABLES * 256];

static int* get_convert_table(int bit)
{
	return &convert_table[bit * 256];
}

static void decode_bitfield(std::uint32_t m, int& shift_out, std::uint32_t& mask_out)
{
	int shift = 0;

	if (m == 0)
	{
		shift_out = 0;
		mask_out = 0;
		return;
	}

#ifdef __GNUC__
	shift = __builtin_ctz(m);
	m >>= shift;
#else
	while ((m & 1) == 0)
	{
		m >>= 1;
		++shift;
	}
#endif

	shift_out = shift;
	mask_out = m;
}

static void generate_scale_table(int* table, int entries)
{
	int tblsize = entries - 1;

	for (int i = 0; i < 256; ++i)
	{
		table[i] = (i * tblsize * 2 + 255) / 510;
	}
}

void dib_writer::write_header(cio::stream& os, int width, int height, format_t format)
{
	Compression compression = RGB;

	as = am = 0;

	if (format == format_555)
	{
		decode_bitfield(0x00007C00U, rs, rm);
		decode_bitfield(0x000003E0U, gs, gm);
		decode_bitfield(0x0000001FU, bs, bm);
		m_bpp = 2;
	}
	else if (format == format_565)
	{
		decode_bitfield(0x0000F800U, rs, rm);
		decode_bitfield(0x000007E0U, gs, gm);
		decode_bitfield(0x0000001FU, bs, bm);
		compression = BitFields;
		m_bpp = 2;
	}
	else if (format == format_888)
	{
		decode_bitfield(0x00FF0000U, rs, rm);
		decode_bitfield(0x0000FF00U, gs, gm);
		decode_bitfield(0x000000FFU, bs, bm);
		m_bpp = 3;
	}

	for (int i = 0; i < NUM_CONVERT_TABLES; ++i)
	{
		std::uint32_t mask = ~(0xFFFFFFFFU << (i+1)) & 0xFFFFFFFFU;

		int* table = get_convert_table(i);
		int entries = (1 << (i+1));

		if (!convert_table_init)
			generate_scale_table(table, entries);

		if (rm == mask) rtable = table;
		if (gm == mask) gtable = table;
		if (bm == mask) btable = table;
		if (am == mask) atable = table;
	}

	m_format = format;
	m_width = width;
	m_stride = (((width * m_bpp) - 1) | 0x3) + 1;

	convert_table_init = true;

	// sketchy bmp v1/v2 format (doesn't include alpha bitmask):
	// write_u32_le(os, (compression == Compression::BitFields) ? 52 : 40); // 0: header_size
	// bmp v3+:
	write_u32_le(os, 56); // 0: header_size
	write_u32_le(os, width); // 4: width
	write_u32_le(os, height); // 8: height
	write_u16_le(os, 1); // 12: color_planes
	write_u16_le(os, m_bpp * 8); // 14: bit depth
	write_u32_le(os, uint32_t(compression)); // 16: compression
	write_u32_le(os, m_stride * height); // 20: image_size
	write_u32_le(os, 0); // 24: horizontal dpi (ppm)
	write_u32_le(os, 0); // 28: vertical dpi (ppm)
	write_u32_le(os, 0); // 32: palette_size (#colours)
	write_u32_le(os, 0); // 36: important colours (#colours)

	// bitfields are not optional in v3 format
	{
		std::uint32_t bmr = std::uint32_t(rm) << rs;
		std::uint32_t bmg = std::uint32_t(gm) << gs;
		std::uint32_t bmb = std::uint32_t(bm) << bs;
		std::uint32_t bma = std::uint32_t(am) << as;

		write_u32_le(os, bmr);
		write_u32_le(os, bmg);
		write_u32_le(os, bmb);
		write_u32_le(os, bma);
	}
}

void dib_writer::write_line(cio::stream& os, const char* linebuf)
{
	for (int i = 0; i < m_width; i++)
	{
		std::uint32_t r = std::uint8_t(linebuf[i * 4 + 0]);
		std::uint32_t g = std::uint8_t(linebuf[i * 4 + 1]);
		std::uint32_t b = std::uint8_t(linebuf[i * 4 + 2]);
		std::uint32_t a = std::uint8_t(linebuf[i * 4 + 3]);

		if (m_bpp == 3)
		{
			std::uint32_t pixel = std::uint32_t(r << 16) | std::uint32_t(g << 8) | std::uint32_t(b);
			write_u24_le(os, pixel);
		}
		else
		{
			std::uint16_t pixel = 0;

			pixel |= std::uint16_t(std::uint16_t(rtable[std::uint8_t(r)]) << rs);
			pixel |= std::uint16_t(std::uint16_t(gtable[std::uint8_t(g)]) << gs);
			pixel |= std::uint16_t(std::uint16_t(btable[std::uint8_t(b)]) << bs);

			if (!a)
				pixel = 0;

			write_u16_le(os, pixel);
		}
	}

	for (int i = m_width * m_bpp; i < m_stride; ++i)
	{
		os.put('\0');
	}
}

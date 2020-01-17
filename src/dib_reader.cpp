
#include "dib_reader.hpp"

#include <cmath>
#include <cstddef>
#include <cstring>

#define NUM_CONVERT_TABLES 8

static bool convert_table_init = false;

static int convert_table[(2 << (NUM_CONVERT_TABLES + 2)) - 2];

static int* get_convert_table(int bit)
{
	return &convert_table[(2 << bit) - 2];
}

static void decode_bitfield(uint32_t m, int& shift_out, uint32_t& mask_out)
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
	int i;

	int tblsize = entries - 1;

	for (i = 0; i < entries; ++i)
	{
		// This formula rounds down
		// e.g. 12 * 255 / 31 = 98.709  ->  98
		//table[i] = i * 255 / tblsize;

		// This formula rounds up from half
		// e.g. 19 * 255 / 31 = 156.29   ->  156
		//      20 * 255 / 31 = 164.516  ->  165
		int x = i * 510 / tblsize;
		// If lsb is set after (x/(y*2)), then (x/y) would have had a decimal
		//   component that is 0.5 or greater.
		int lsb = x & 1;
		// Adding the lsb bumps up the result by 1 after divison by 2
		table[i] = (x + lsb) / 2;
	}
}

const char* dib_reader::check_format() const
{
	if (width() < 0)
		return "Image width less than zero";

	if (width() > 0x40000000 || height() < -0x40000000 || height() > 0x40000000)
		return "Image dimensions out of bounds";

	if (depth() != 16 && depth() != 24 && depth() != 32)
		return "Unsupported bit depth";

	if (compression() != RGB && compression() != BitFields)
		return "Unsupported compression";

	constexpr int maxmask = (1 << NUM_CONVERT_TABLES) - 1;

	if (rm > maxmask || gm > maxmask || bm > maxmask || am > maxmask)
		return "Bit mask too long";

	return nullptr;
}

void dib_reader::start()
{
	if (compression() == BitFields)
	{
		decode_bitfield(red_mask(),   rs, rm);
		decode_bitfield(green_mask(), gs, gm);
		decode_bitfield(blue_mask(),  bs, bm);
		decode_bitfield(alpha_mask(), as, am);
	}
	else
	{
		as = am = 0;

		switch (depth())
		{
			case 16:
				decode_bitfield(0x00007C00U, rs, rm);
				decode_bitfield(0x000003E0U, gs, gm);
				decode_bitfield(0x0000001FU, bs, bm);
				break;

			case 24:
			case 32:
				decode_bitfield(0x00FF0000U, rs, rm);
				decode_bitfield(0x0000FF00U, gs, gm);
				decode_bitfield(0x000000FFU, bs, bm);
				break;
		}
	}

	for (int i = 0; i < NUM_CONVERT_TABLES; ++i)
	{
		uint32_t mask = ~(0xFFFFFFFFU << (i+1)) & 0xFFFFFFFFU;

		int* table = get_convert_table(i);
		int entries = (1 << (i+1));

		if (!convert_table_init)
			generate_scale_table(table, entries);

		if (rm == mask) rtable = table;
		if (gm == mask) gtable = table;
		if (bm == mask) btable = table;
		if (am == mask) atable = table;
	}

	convert_table_init = true;
}

void dib_reader::read_line(char* outbuf, int row)
{
	int line = ((height() < 0) ? row : height() - 1 - row);

	const char *linebuf = data() + stride() * line;

	for (int i = 0; i < width(); i++)
	{
		std::size_t pixel_offset = linebuf - data_ptr;
		uint32_t pixel;

		if (pixel_offset < data_size)
			pixel = read_u32_le(pixel_offset);
		else
			pixel = 0;

		char r = static_cast<char>(rtable[((pixel >> rs) & rm)]);
		char g = static_cast<char>(gtable[((pixel >> gs) & gm)]);
		char b = static_cast<char>(btable[((pixel >> bs) & bm)]);
		char a = static_cast<char>((pixel != 0) * 0xFF);

		outbuf[0] = r;
		outbuf[1] = g;
		outbuf[2] = b;
		outbuf[3] = a;

		linebuf += bpp();
		outbuf += 4;
	}
}

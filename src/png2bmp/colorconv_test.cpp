#include "cio/cio.hpp"

#include <cassert>
#include <cstdint>

#define NUM_CONVERT_TABLES 8

static int convert_up_table[(2 << (NUM_CONVERT_TABLES + 2)) - 2];

static int* get_convert_up_table(int bit)
{
	return &convert_up_table[(2 << bit) - 2];
}

static void generate_scale_up_table(int* table, int entries)
{
	int tblsize = entries - 1;

	for (int i = 0; i < entries; ++i)
	{
		table[i] = (i * 510 + tblsize) / tblsize / 2;
	}
}

static int convert_down_table[NUM_CONVERT_TABLES * 256];

static int* get_convert_down_table(int bit)
{
	return &convert_down_table[bit * 256];
}

static void generate_scale_down_table(int* table, int entries)
{
	int tblsize = entries - 1;

	for (int i = 0; i < 256; ++i)
	{
		table[i] = (i * tblsize * 2 + 255) / 510;
	}
}

#define ABORT_ON(cond, msg) \
	if (cond) \
	{ \
		(cio::err << "[bits=" << (i+1) << ",val=" << ii << "] Error: " << msg) << cio::endl; \
		dump_info(); \
		std::exit(1); \
	}

int main()
{
	cio::out << "Generating scale-up tables..." << cio::endl;

	for (int i = 0; i < NUM_CONVERT_TABLES-1; ++i)
	{
		int* table = get_convert_up_table(i);
		int entries = (1 << (i+1));

		generate_scale_up_table(table, entries);
	}

	cio::out << "Generating scale-down tables..." << cio::endl;

	for (int i = 0; i < NUM_CONVERT_TABLES-1; ++i)
	{
		int* table = get_convert_down_table(i);
		int entries = (1 << (i+1));

		generate_scale_down_table(table, entries);
	}

	for (int i = 0; i < NUM_CONVERT_TABLES-1; ++i)
	{
		cio::out << "Testing table " << i << "..." << cio::endl;
		int* up_table = get_convert_up_table(i);
		int* down_table = get_convert_down_table(i);

		int entries = (1 << (i+1));

		for (int ii = 0; ii < entries; ++ii)
		{
			int val8 = up_table[ii];

			auto dump_info = [&]()
			{
				auto in_range = [](int val) { return val >= 0 && val <= 255; };

				cio::err << "---" << cio::endl;
				cio::err << "   IN: ";

				for (int a = ii - 4; a < ii + 4; ++a)
				{
					if (a >= 0 && a < entries)
					{
						if (a == ii)
							cio::err.put('[');

						cio::err << a << "=>" << (in_range(a) ? up_table[a] : -1);

						if (a == ii)
							cio::err.put(']');

						cio::err.put(' ');
					}
				}

				cio::err << cio::endl;

				cio::err << "---" << cio::endl;

				cio::err << "  OUT: ";

				for (int a = val8 - 40; a < val8 + 40; ++a)
				{
					if (a >= 0 && a < 256)
					{
						if (a == val8)
							cio::err.put('[');

						cio::err << a << "=>" << (in_range(a) ? down_table[a] : -1);

						if (a == val8)
							cio::err.put(']');

						cio::err.put(' ');
					}
				}

				cio::err << cio::endl;

				cio::err << "---" << cio::endl;
			};

			ABORT_ON(val8 < 0 || val8 > 255, "Up-converted value " << val8 << " out of range: [0,255]")

			int valdown = down_table[val8];

			ABORT_ON(valdown < 0 || valdown > 255, "Down-converted value " << valdown << " out of range: [0,255]")

			ABORT_ON(valdown != ii, "Down-converted value does not match: " << ii << " => " << val8 << " => " << valdown)
		}
	}
}

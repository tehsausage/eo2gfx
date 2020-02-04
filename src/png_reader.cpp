
#include "png_reader.hpp"

#include <vector>

bool png_reader::init()
{
	png_ptr.reset(png_create_read_struct(
		PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr
	));
	
	if (!png_ptr)
		return false;

	info_ptr.reset(png_create_info_struct(png_ptr.get()));
	
	if (!info_ptr)
	{
		auto pp = png_ptr.get();
		png_destroy_read_struct(&pp, nullptr, nullptr);
		png_ptr = nullptr;
		return false;
	}

	return true;
}

bool png_reader::check_sig()
{
	char sig[8];

	sigbytes = static_cast<int>(file.read(sig, 8));

	if (sigbytes != 8)
		return false;

	if (png_sig_cmp(reinterpret_cast<png_bytep>(sig), 0, sigbytes) != 0)
		return false;

	return true;
}

png_reader_info png_reader::read_header()
{
	int interlace_type = 0;

	png_init_io(png_ptr.get(), file.get());

	if (sigbytes)
		png_set_sig_bytes(png_ptr.get(), sigbytes);

	png_read_info(png_ptr.get(), info_ptr.get());

	png_get_IHDR(png_ptr.get(), info_ptr.get(),
		&info.width, &info.height, &info.bit_depth, &info.color_type,
		&interlace_type, nullptr, nullptr
	);

	if (interlace_type != 0)
	{
		info.number_of_passes = png_set_interlace_handling(png_ptr.get());
	}
	else
	{
		info.number_of_passes = 1;
	}

	// We want everything transformed in to RGBA format when loaded

	if (info.color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(png_ptr.get());
		info.color_type = PNG_COLOR_TYPE_RGB;
	}

    if (info.color_type == PNG_COLOR_TYPE_GRAY)
    {
    	if (info.bit_depth < 8)
    	{
	    	png_set_expand_gray_1_2_4_to_8(png_ptr.get());
	    	info.bit_depth = 8;
	    }

		png_set_gray_to_rgb(png_ptr.get());

    	info.color_type = PNG_COLOR_TYPE_RGB;
    }

    if (png_get_valid(png_ptr.get(), info_ptr.get(), PNG_INFO_tRNS))
    {
    	png_set_tRNS_to_alpha(png_ptr.get());
    }

    if (info.bit_depth == 16)
    {
    	png_set_strip_16(png_ptr.get());
    	info.bit_depth = 8;
    }

    if (info.bit_depth < 8)
    {
    	png_set_packing(png_ptr.get());
    	info.bit_depth = 8;
    }

    if (info.color_type == PNG_COLOR_TYPE_RGB)
    {
    	png_set_filler(png_ptr.get(), 0xFF, PNG_FILLER_AFTER);
    	info.color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    }

	png_read_update_info(png_ptr.get(), info_ptr.get());

	info.rowbytes = png_get_rowbytes(png_ptr.get(), info_ptr.get());
	info.valid = true;

	return info;
}

void png_reader::read_row(char* data)
{
	png_read_rows(png_ptr.get(), reinterpret_cast<png_bytepp>(&data), NULL, 1);
}

void png_reader::read_image(char* data, int stride)
{
	std::vector<png_bytep> row_pointers(info.height);

	for (png_uint_32 i = 0; i < info.height; ++i)
	{
		row_pointers[i] = reinterpret_cast<png_bytep>(&data[i * stride]);
	}

	for (int pass = 0; pass < info.number_of_passes; ++pass)
	{
		png_read_rows(png_ptr.get(), reinterpret_cast<png_bytepp>(&row_pointers[0]), NULL, info.height);
	}
}

void png_reader::read_end()
{
	png_read_end(png_ptr.get(), info_ptr.get());
}

png_reader::~png_reader()
{
	if (png_ptr)
	{
		auto pp = png_ptr.get();
		auto ip = info_ptr.get();
		png_destroy_read_struct(&pp, &ip, nullptr);
	}
}

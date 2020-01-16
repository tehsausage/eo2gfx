
#include "png_writer.hpp"

bool png_writer::init()
{
	png_ptr.reset(png_create_write_struct(
		PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr
	));
	
	if (!png_ptr)
		return false;

	info_ptr.reset(png_create_info_struct(png_ptr.get()));
	
	if (!info_ptr)
	{
		auto pp = png_ptr.get();
		png_destroy_write_struct(&pp, nullptr);
		png_ptr = nullptr;
		return false;
	}

	return true;
}

void png_writer::write_header(int compression_level, int width, int height, int format, int palette_size, png_color* palette_data)
{
	png_init_io(png_ptr.get(), file.get());

	png_set_compression_level(png_ptr.get(), compression_level);

	png_set_IHDR(png_ptr.get(), info_ptr.get(),
		width, height, 8, format,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);

	if (palette_size > 0)
	{
		png_byte trans[1] = {0};

		png_set_PLTE(png_ptr.get(), info_ptr.get(), palette_data, palette_size);
		png_set_tRNS(png_ptr.get(), info_ptr.get(), trans, 1, nullptr);
	}

	png_write_info(png_ptr.get(), info_ptr.get());
}

void png_writer::write_row(const char* data)
{
	png_write_row(png_ptr.get(), reinterpret_cast<png_bytep>(const_cast<char*>(data)));
}

void png_writer::write_end()
{
	png_write_end(png_ptr.get(), info_ptr.get());
}

png_writer::~png_writer()
{
	if (png_ptr)
	{
		auto pp = png_ptr.get();
		auto ip = info_ptr.get();
		png_destroy_write_struct(&pp, &ip);
	}
}
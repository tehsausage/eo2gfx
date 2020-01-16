#ifndef DIB_READER_HPP
#define DIB_READER_HPP

#include "cio/cio.hpp"

#include <cstdint>
#include <utility>

#include "int_pack.hpp"

class dib_reader
{
	private:
		const char* data_ptr;
		std::size_t data_size;

		int      rs, gs, bs, as;
		uint32_t rm, gm, bm, am;

		int* rtable = nullptr;
		int* gtable = nullptr;
		int* btable = nullptr;
		int* atable = nullptr;

		std::uint16_t read_u16_le(std::size_t offset) const noexcept
		{
			char a = data_ptr[offset];
			char b = data_ptr[offset + 1];

			return int_pack_16(a, b);
		}

		std::uint32_t read_u32_le(std::size_t offset) const noexcept
		{
			char a = data_ptr[offset];
			char b = data_ptr[offset + 1];
			char c = data_ptr[offset + 2];
			char d = data_ptr[offset + 3];

			return int_pack_32(a, b, c, d);
		}

	public:
		enum Compression
		{
			RGB = 0,
			RLE8 = 1,
			RLE4 = 2,
			BitFields = 3,
			JPEG = 4,
			PNG = 5
		};

		// The buffer pointed to by data_ptr must be at least 40 bytes
		dib_reader(const char* data_ptr, std::size_t data_size)
			: data_ptr(reinterpret_cast<const char*>(data_ptr))
			, data_size(data_size)
		{ }

		bool          v2_format()    const noexcept { return header_size() >= 52; }
		bool          v3_format()    const noexcept { return header_size() >= 56; }
		
		std::int32_t  header_size()  const noexcept { return read_u32_le(0); }
		std::int32_t  width()        const noexcept { return read_u32_le(4); }
		std::int32_t  height()       const noexcept { return read_u32_le(8); }
		std::int16_t  color_planes() const noexcept { return 1; }
		std::int16_t  depth()        const noexcept { return read_u16_le(14); }
		Compression   compression()  const noexcept { return Compression(read_u32_le(16)); }
		std::uint32_t image_size()   const noexcept { return read_u32_le(20); }
		std::size_t   palette_size() const noexcept { return (!v2_format() && compression() == BitFields) * 12; }
		const char*   data()         const noexcept { return reinterpret_cast<const char*>(data_ptr + header_size() + palette_size()); }
		const char*   palette()      const noexcept { return reinterpret_cast<const char*>(data_ptr + header_size()); }
		const char*   raw_data()     const noexcept { return reinterpret_cast<const char*>(data_ptr); }
		
		std::int16_t  bpp()          const noexcept { return static_cast<std::int16_t>(depth() >> 3); }
		std::int32_t  stride()       const noexcept { return width() * bpp() + ((4U - (width() * bpp())) & 3); }

		std::uint32_t red_mask()     const noexcept {
			return v2_format()
				? read_u32_le(40)
				: read_u32_le(header_size());
		}

		std::uint32_t green_mask()   const noexcept {
			return v2_format()
				? read_u32_le(44)
				: read_u32_le(header_size() + 4);
		}

		std::uint32_t blue_mask()    const noexcept {
			return v2_format()
				? read_u32_le(48)
				: read_u32_le(header_size() + 8);
		}

		std::uint32_t alpha_mask()   const noexcept {
			return v3_format()
				? read_u32_le(52)
				: 0;
		}

		// Returns a pointer to a human readable string describing what's wrong with the file
		// Returns nullptr if the format is acceptable
		const char* check_format() const;

		void start();

		// outbuf must be at least line_size() bytes
		void read_line(char* outbuf, int row);
};

#endif // DIB_READER_HPP

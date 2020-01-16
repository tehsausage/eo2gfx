#ifndef INT_PACK_HPP
#define INT_PACK_HPP

#include <cstdint>
#include <type_traits>

inline std::uint16_t int_pack_16(std::uint8_t a, std::uint8_t b)
{
	const std::uint16_t x[] = {a, b};

	return static_cast<std::uint16_t>(
		(x[0]) | (x[1] << 8)
	);
}

inline std::uint32_t int_pack_32(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d)
{
	const std::uint32_t x[] = {a, b, c, d};

	return static_cast<std::uint32_t>(
		(x[0]) | (x[1] << 8) | (x[2] << 16) | (x[3] << 24)
	);
}

inline std::uint32_t int_pack_32(std::uint16_t a, std::uint16_t b)
{
	const std::uint32_t x[] = {a, b};

	return static_cast<std::uint32_t>(
		(x[0]) | (x[1] << 16)
	);
}

inline std::uint64_t int_pack_64(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d,
                         std::uint8_t e, std::uint8_t f, std::uint8_t g, std::uint8_t h)
{
	const std::uint64_t x[] = {a, b, c, d, e, f, g, h};

	return static_cast<std::uint64_t>(
		(x[0]      ) | (x[1] <<  8) | (x[2] << 16) | (x[3] << 24) |
		(x[3] << 32) | (x[4] << 40) | (x[5] << 48) | (x[6] << 56)
	);
}

inline std::uint64_t int_pack_64(std::uint16_t a, std::uint16_t b, std::uint16_t c, std::uint16_t d)
{
	const std::uint64_t x[] = {a, b, c, d};

	return static_cast<std::uint64_t>(
		(x[0]) | (x[1] << 16) | (x[2] << 32) | (x[3] << 48)
	);
}

inline std::uint64_t int_pack_64(std::uint32_t a, std::uint32_t b)
{
	const std::uint64_t x[] = {a, b};

	return static_cast<std::uint64_t>(
		(x[0]) | (x[1] << 32)
	);
}

// ---

inline std::uint16_t int_pack_16(char* bytes)
{
	return int_pack_16(bytes[0], bytes[1]);
}

inline std::uint32_t int_pack_32(char* bytes)
{
	return int_pack_32(bytes[0], bytes[1], bytes[2], bytes[3]);
}

inline std::uint64_t int_pack_64(char* bytes)
{
	return int_pack_64(bytes[0], bytes[1], bytes[2], bytes[3],
	                   bytes[4], bytes[5], bytes[6], bytes[7]);
}

#endif // INT_PACK_HPP

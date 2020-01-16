
#include "my_cstrcat.hpp"

#include <algorithm>
#include <cstring>

std::unique_ptr<char[]> my_cstrcat(const char* a, const char* b)
{
	std::unique_ptr<char[]> result;

	std::size_t alen = std::strlen(a);
	std::size_t blen = std::strlen(b);
	std::size_t length = alen + blen;

	result.reset(new char[length + 1]);

	// Memory allocation failed
	if (!result)
		return result;

	std::copy(a, a + alen, result.get());
	std::copy(b, b + blen, result.get() + alen);
	result.get()[length] = '\0';

	return result;
}

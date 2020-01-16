
#include "sha256_to_hex.h"

void sha256_to_hex(const char* digest, size_t length, char* cdigest)
{
	for (size_t i = 0; i < length; ++i)
	{
		cdigest[i*2]   = "0123456789abcdef"[((digest[i] >> 4) & 0x0F)];
		cdigest[i*2+1] = "0123456789abcdef"[((digest[i]) & 0x0F)];
	}
}

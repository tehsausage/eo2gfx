
#ifndef SHA256_TO_HEX_H_INCLUDED
#define SHA256_TO_HEX_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>

void sha256_to_hex(const char* digest, size_t length, char* cdigest);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SHA256_TO_HEX_H_INCLUDED */

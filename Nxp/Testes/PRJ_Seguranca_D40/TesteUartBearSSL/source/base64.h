
#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>

size_t base64Encode(unsigned char *pu8Destino, const unsigned char *pu8Origem, unsigned short u16TamanhoOrigem);
size_t base64Decode(unsigned char *pu8Destino, const unsigned char *pu8Origem, unsigned short u16TamanhoOrigem);
size_t base64EncodedLength(unsigned int uiTamanhoOrigem);
size_t base64DecodedLength(const unsigned char *pu8Origem, int n);

#endif

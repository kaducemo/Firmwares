
#ifndef BASE64_H
#define BASE64_H

unsigned short base64Encode(unsigned char *pu8Destino, const unsigned char *pu8Origem, unsigned short u16TamanhoOrigem);
unsigned short base64Decode(unsigned char *pu8Destino, const unsigned char *pu8Origem, unsigned short u16TamanhoOrigem);
unsigned int   base64EncodedLength(unsigned int uiTamanhoOrigem);
unsigned int   base64DecodedLength(const unsigned char *pu8Origem, int n);

#endif

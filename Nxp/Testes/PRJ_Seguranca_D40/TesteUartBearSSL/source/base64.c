#include "base64.h"

static unsigned char base64Caracter(unsigned char u8Indice)
{
    unsigned char u8Caracter;
    if(u8Indice < 26)
        u8Caracter = u8Indice + 'A';
    else if(u8Indice < 52)
        u8Caracter = u8Indice + 'a' - 26;
    else if(u8Indice < 62)
        u8Caracter = u8Indice + '0' - 52;
    else if(u8Indice < 63)      
        u8Caracter = '+';
    else if(u8Indice < 64)
        u8Caracter = '/';
    else
        u8Caracter = '=';
    return u8Caracter;
} 


static unsigned char base64Indice(unsigned char u8Caracter)
{
    unsigned char u8Indice;
    if((u8Caracter >= 'A') && (u8Caracter <= 'Z'))
        u8Indice = u8Caracter - 'A';
    else if((u8Caracter >= 'a') && (u8Caracter <= 'z'))
        u8Indice = u8Caracter - 'a' + 26;
    else if((u8Caracter >= '0') && (u8Caracter <= '9')) 
        u8Indice = u8Caracter - '0' + 52;
    else if(u8Caracter == '+')          
        u8Indice = 62;
    else if(u8Caracter == '/')          
        u8Indice = 63;
    else                                
        u8Indice = 64;
    return u8Indice;
} 


unsigned short base64Encode(unsigned char *pu8Destino, const unsigned char *pu8Origem, unsigned short u16TamanhoOrigem)
{
    unsigned char u8C1, u8C2, u8C3;
    unsigned short u16Indice;
    unsigned short u16Parcial;

    u16Indice = 0;
    u16Parcial = 3 * (u16TamanhoOrigem / 3);

    while(u16Indice < u16Parcial)
    {
        u8C1 = *pu8Origem++;
        u8C2 = *pu8Origem++;
        u8C3 = *pu8Origem++;

        *pu8Destino++ = base64Caracter(u8C1 >> 2);
        *pu8Destino++ = base64Caracter(((u8C1 & 0x03) << 4) | (u8C2 >> 4));
        *pu8Destino++ = base64Caracter(((u8C2 & 0x0F) << 2) | (u8C3 >> 6));
        *pu8Destino++ = base64Caracter(u8C3 & 0x3F);

        u16Indice += 3;
    }

    if(u16Indice < u16TamanhoOrigem)
    {
        u8C1 = *pu8Origem++;
        u8C2 = ((u16Indice + 1) < u16TamanhoOrigem) ? *pu8Origem++ : 0;

        *pu8Destino++ = base64Caracter(u8C1 >> 2);
        *pu8Destino++ = base64Caracter(((u8C1 & 0x03) << 4) | (u8C2 >> 4));
        *pu8Destino++ = ((u16Indice + 1) < u16TamanhoOrigem) ? base64Caracter((u8C2 & 0x0F) << 2) : '=';
        *pu8Destino++ = '=';
    }

    return (4 * ((u16TamanhoOrigem + 2) / 3));
}


unsigned short base64Decode(unsigned char *pu8Destino, const unsigned char *pu8Origem, unsigned short u16TamanhoOrigem)
{
    unsigned char u8D1, u8D2, u8D3, u8D4;
    unsigned short u16Indice;
    unsigned short u16Tamanho;

    u16Indice = 0;
    u16Tamanho = 0;

    while(u16Indice < u16TamanhoOrigem)
    {
        if((u8D1 = base64Indice(*pu8Origem++)) >= 64)
            break;
        if((u8D2 = base64Indice(*pu8Origem++)) >= 64)   
            break;
        *pu8Destino++ = (u8D1 << 2) | (u8D2 >> 4);  u16Tamanho++;

        if((u8D3 = base64Indice(*pu8Origem++)) >= 64)   
            break;
        *pu8Destino++ = (u8D2 << 4) | (u8D3 >> 2);  u16Tamanho++;

        if((u8D4 = base64Indice(*pu8Origem++)) >= 64)   
            break;
        *pu8Destino++ = (u8D3 << 6) | u8D4;         u16Tamanho++;

        u16Indice += 4;
    }
    return u16Tamanho;
}


unsigned int   base64EncodedLength(unsigned int uiTamanhoOrigem)
{
	return (4 * ((uiTamanhoOrigem + 2) / 3));
}


unsigned int base64DecodedLength(const unsigned char *pu8Origem, int n)
/*
 * A quantidade de Bytes na saída vai depender do número de paddings p
 * Fórmula: out = ((n / 4) * 3) - P
 */
{
    unsigned int p = 0; // Quantidade de paddings '='
    unsigned int out = 0;

    // Verifica quantos paddings existem no final
    if (n >= 2 && pu8Origem[n - 1] == '=')
        p++;
    if (n >= 2 && pu8Origem[n - 2] == '=')
        p++;

    // Aplica a fórmula
    out = ((n / 4) * 3) - p;

    return out;
}


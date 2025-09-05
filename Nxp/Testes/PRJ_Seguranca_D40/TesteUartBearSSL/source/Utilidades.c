/*
 * Utilidades.c
 *
 *  Created on: 1 de set de 2025
 *      Author: carlos.oliveira
 */

#include "Utilidades.h"

#include <stdio.h>
#include <stdlib.h>



void secure_zero(void *v, size_t n)
/* Esta a funcao limpa um buffer, retirando residuos da RAM de forma segura e nao
 * otimizavel pelo compilador.
 * @param v: vetor que se deseja limpar
 * @param n: tanho desse vetor*/
{
    volatile uint8_t *p = (volatile uint8_t *)v;
    while (n--) {
        *p++ = 0;
    }
}

char *hex_array_to_string(uint8_t *in, uint8_t len)
/*Função auxiliar para transformar um binário HEXADECIMAL em String*/
{
	if(in == NULL || len == 0)
		return NULL;

	// Cada byte vira "0xXX " (5 caracteres), mais o '\0' no final
	size_t str_len = len * 5;
	char *out = malloc(str_len);

	if(!out)
		return NULL;

	for (size_t i = 0; i < len; ++i)
	{
		sprintf(out + i * 5, "0x%02X ", in[i]);
	}

	// Remove o espaço extra no final
	if (str_len > 0)
	{
		out[str_len - 1] = '\0';
	}

	return out;
}

void delay(void)
/*Função bloqueante de Delay...*/
{
    volatile uint32_t i = 0;
    for (i = 0; i < 8000; ++i)
    {
        __asm("NOP"); /* delay */
    }
}

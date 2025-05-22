/*
 * Copyright 2016-2025 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    Teste2Sodium.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include <stdbool.h>
#include "bearSSL.h"


/* TODO: insert other include files here. */

typedef struct KeygenResult
{
    uint8_t pvBuf[BR_EC_KBUF_PRIV_MAX_SIZE];
    uint8_t pbBuf[BR_EC_KBUF_PUB_MAX_SIZE];
    br_ec_private_key pvKey;
    br_ec_public_key pbKey;
    int ok;
}Chave;

Chave chavesLocais, chavesRemotas;

uint8_t segredo1[32] = {0}, segredo2[32] = {0}; //Gera Segredos para comparacao


char *hex_array_to_string(uint8_t *in, uint8_t len)
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

int keygen_ec(int curve, Chave *ch)
{
    const char *seeder_name;
    size_t length;

    br_prng_seeder seeder = br_prng_seeder_system(&seeder_name);

    if (seeder == NULL)
    {
    	PRINTF("ERRO: sem fonte de RANDOM\n\r");
    	ch->ok = -1;
    }

    br_hmac_drbg_context rng;
    br_hmac_drbg_init(&rng, &br_sha256_vtable, NULL, 0);

    if (!seeder(&rng.vtable))
    {
    	PRINTF("Falha ao gerar funcao RANDOM\n\r");
    	ch->ok = -1;
    }

    PRINTF("Nome da funcao Seeder: %s\n\r", seeder_name);

    const br_ec_impl *impl = br_ec_get_default();

    //Gera chave privada
    ch->ok = (0 != br_ec_keygen( &rng.vtable, impl, &(ch->pvKey), ch->pvKey.x, curve));

    length = br_ec_compute_pub(impl, &(ch->pbKey), ch->pbKey.q, &(ch->pvKey));

    if (length == 0)
    {
    	PRINTF("-ERRO: Curva nao suportada\n\r");
    	ch->ok = -1;
	}

    return ch->ok;
}


int ComputeSharedSecret(br_ec_private_key *pvKey, br_ec_public_key *pbKey, uint8_t *secret)
{
	const br_ec_impl *ec_implementation = br_ec_get_default();

	br_ec_public_key tmp = *pbKey;
	if (ec_implementation->mul(tmp.q, tmp.qlen, pvKey->x, pvKey->xlen, BR_EC_secp256r1))
		memcpy(secret, tmp.q, 33);
	else
		return -1;

	return 0;
}





/* TODO: insert other definitions and declarations here. */

/*
 * @brief   Application entry point.
 */
int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    memset(&chavesLocais, 0, sizeof(Chave));
    memset(&chavesRemotas, 0, sizeof(Chave));

    //Aloca buffers para chaves remotas e Locais
    chavesLocais.pvKey.x = chavesLocais.pvBuf;
    chavesRemotas.pvKey.x = chavesRemotas.pvBuf;

    chavesLocais.pbKey.q = chavesLocais.pbBuf;
    chavesRemotas.pbKey.q = chavesRemotas.pbBuf;


    if(keygen_ec(BR_EC_secp256r1, &chavesLocais) == 1)
	{
    	PRINTF("Chaves Locais Geradas! \n\r");
    	char *strPvKey = hex_array_to_string(chavesLocais.pvKey.x, 32);

    	if (chavesLocais.ok == 1)
		{
			PRINTF("Tamanho Chave Privada Local: %d, Tamanho Chave Publica Local: %d\n", chavesLocais.pvKey.xlen, chavesLocais.pbKey.qlen);
			PRINTF("Chave Local Privada: %s\n\n\n", strPvKey);
			free(strPvKey);

			if(keygen_ec(BR_EC_secp256r1, &chavesRemotas) == 1)
			{
				PRINTF("Chaves Remotas Geradas! \n\r");
				char *strPvKey2 = hex_array_to_string(chavesRemotas.pvKey.x, 32);

				if (chavesRemotas.ok == 1)
				{
					PRINTF("Tamanho Chave Privada Remota: %d, Tamanho Chave Publica Remota: %d\n", chavesRemotas.pvKey.xlen, chavesRemotas.pbKey.qlen);
					PRINTF("Chave Remota Privada: %s\n", strPvKey2);
					free(strPvKey2);

					PRINTF("Gerando segredo compartilhado...\n\r");
					if(!ComputeSharedSecret(&chavesLocais.pvKey, &chavesRemotas.pbKey, segredo1))
					{
						char *strPvKey3 = hex_array_to_string(segredo1, 32);
						PRINTF("Segredo Gerado 1: %s\n\r", strPvKey3);
						free(strPvKey3);

						if(!ComputeSharedSecret(&chavesRemotas.pvKey, &chavesLocais.pbKey , segredo2))
						{
							char *strPvKey4 = hex_array_to_string(segredo2, 32);
							PRINTF("Segredo Gerado 2 : %s\n\r", strPvKey4);
							free(strPvKey4);
						}
					}

				} else
				{
					PRINTF("-ERRO!");
				}
			}


		} else
		{
			PRINTF("-ERRO!");
		}

	}
    else
    {
    	PRINTF("Erro na geracao de chaves locais. Preso em Loop! \n\r");
    }


    PRINTF("FIM! Preso em Loop...\n\r");

    /* Enter an infinite loop, just incrementing a counter. */
    while(1)
    {
        __asm volatile ("nop");
    }

    return 0 ;
}

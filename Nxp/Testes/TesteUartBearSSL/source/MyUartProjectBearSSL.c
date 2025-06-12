/*
 * Copyright 2016-2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    MyUartProject.c
 * @brief   Application entry point.
 * Utilizando este FW, a placa serve como uma ponte entre dois dispositivos utilizando duas UARTs.
 * Neste caso, eu coloquei na UART (UART0, TX = PTB17, RX = PTB16 ) um módulo de BLUETOOTH e na outra UART (UART4, TX = PTE24 , RX = PTE25) a qual está
 * ligada a um conversor serial-USB embarcado na própria FRDM-K64, um PC. Dessa forma é possivel transferir comandos de um PC
 * para o módulo Bluetooth e vice versa.
 */

#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include "bearssl.h"


/* TODO: insert other include files here. */
/*******************************************************************************
 * Code
 ******************************************************************************/

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



void UART0_SERIAL_RX_TX_IRQHANDLER(void)
{
    uint8_t data;

    /* If new data arrived. */
    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(UART0))
    {
        data = UART_ReadByte(UART0);
        UART_WriteByte(UART0, data);
    }
    SDK_ISR_EXIT_BARRIER;
}

void UART4_SERIAL_RX_TX_IRQHANDLER(void)
{
    uint8_t data;

    /* If new data arrived. */
    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(UART4))
    {
        data = UART_ReadByte(UART4);
        UART_WriteByte(UART0, data);
    }
    SDK_ISR_EXIT_BARRIER;
}

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


void delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 8000; ++i)
    {
        __asm("NOP"); /* delay */
    }
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


					const char *salt_hkdf = "DATAPROM";
					const char *info = "SECRETCOMM";

					uint8_t key[16] = {0};
					uint8_t iv[16] = {0};

					uint8_t plain[16] = "Hello, world!\0"; // 16 bytes
					PRINTF("Dados antes da Criptografia : %s\n\r", plain);
					uint8_t encrypted[16]= {0};
					uint8_t decrypted[16]= {0};

					PRINTF("--{ ECDH + HKDF + AES-CBC }--\n\r");

					// Derivação da chave e IV com HKDF - SHA512
//					br_hkdf_context hkdf;
//					br_hkdf_init(&hkdf, &br_sha512_vtable, salt_hkdf, strlen(salt_hkdf));
//					br_hkdf_inject(&hkdf, segredo1, sizeof(segredo1));
//					br_hkdf_flip(&hkdf);
//					br_hkdf_produce(&hkdf, info, strlen(info), key, sizeof(key));
//					br_hkdf_produce(&hkdf, info, strlen(info), iv, sizeof(iv));

					// Derivação da chave e IV com HKDF - SHA256
					br_hkdf_context hkdf;
					br_hkdf_init(&hkdf, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
					br_hkdf_inject(&hkdf, segredo1, sizeof(segredo1));
					br_hkdf_flip(&hkdf);
					br_hkdf_produce(&hkdf, info, strlen(info), key, sizeof(key));
					br_hkdf_produce(&hkdf, info, strlen(info), iv, sizeof(iv));


					uint8_t iv_original[16] = {0};
					memcpy(iv_original, iv, 16); // salva o IV original



					char *strPvKey5 = hex_array_to_string(key, 16);
					PRINTF("Chave Derivada: %s\n\r", strPvKey5);
					free(strPvKey5);

					char *strPvKey6 = hex_array_to_string(iv, 16);
					PRINTF("IV Derivada: %s\n\r", strPvKey6);
					free(strPvKey6);

					// Criptografia AES-CBC (IMPLEMENTAÇÃO BIG - Mais rápido, mas usa mais memoria
//					br_aes_big_cbcenc_keys ctx_enc;
//					br_aes_big_cbcenc_init(&ctx_enc, key, sizeof(key));
//					memcpy(encrypted, plain, 16);
//					br_aes_big_cbcenc_run(&ctx_enc, iv, encrypted, 16); //Aqui IV será modificado

					// Criptografia AES-CBC (IMPLEMENTAÇÃO SMALL - Lenta, mas usa menos memoria
					br_aes_small_cbcenc_keys  ctx_enc;
					br_aes_small_cbcenc_init(&ctx_enc, key, sizeof(key));
					memcpy(encrypted, plain, 16);
					br_aes_small_cbcenc_run(&ctx_enc, iv, encrypted, 16); //Aqui IV será modificado


					char *strPvKey7 = hex_array_to_string(encrypted, 16);
					PRINTF("Dado Criptografado: %s\n\r", strPvKey7);
					free(strPvKey7);


					// Descriptografia AES-CBC - BIG
//					br_aes_big_cbcdec_keys ctx_dec;
//					uint8_t iv_dec[16];
//					memcpy(iv_dec, iv_original, 16); // reset IV
//					memcpy(decrypted, encrypted, 16);
//					br_aes_big_cbcdec_init(&ctx_dec, key, sizeof(key));
//					br_aes_big_cbcdec_run(&ctx_dec, iv_dec, decrypted, 16);

					// Descriptografia AES-CBC - SMALL
					br_aes_small_cbcdec_keys ctx_dec;
					uint8_t iv_dec[16];
					memcpy(iv_dec, iv_original, 16); // reset IV
					memcpy(decrypted, encrypted, 16);
					br_aes_small_cbcdec_init(&ctx_dec, key, sizeof(key));
					br_aes_small_cbcdec_run(&ctx_dec, iv_dec, decrypted, 16);

					PRINTF("Dados Descriptografados: %s\n\r", decrypted);

				}
				else
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

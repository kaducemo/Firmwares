/*
 * Copyright 2016-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    TesteMbedTls.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include "fsl_rnga.h"


/* TODO: insert other include files here. */


#include <mbedtls/entropy.h>
#include "mbedtls/entropy_poll.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/ctr_drbg.h"

#include <stdio.h>
#include <string.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/aes.h>
#include <mbedtls/config.h>

/* TODO: insert other definitions and declarations here. */

#define MESSAGE "minha mensagem"

void handle_error(int ret) {
	if (ret != 0) {
		printf("Erro: %d\n", ret);
		exit(1);
	}
}


void print_hex(const char *title, const unsigned char *buf, size_t len) {
	printf("%s: ", title);
	for (size_t i = 0; i < len; i++) {
		printf("%02x", buf[i]);
	}
	printf("\n");
}




/*
 * @brief   Application entry point.
 */
int main(void) {


	mbedtls_ecdh_context ctx_server, ctx_client;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;


	unsigned char server_public_key[65], client_public_key[65];
	unsigned char shared_secret[32];
	unsigned char iv[16] = {0};
	unsigned char encrypted_message[128];
	unsigned char decrypted_message[128];
	size_t olen;

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    /* Init RNGA */
    	RNGA_Init(RNG);
    	PRINTF("INICIOU");

    	mbedtls_ecdh_init(&ctx_server);
    	mbedtls_ecdh_init(&ctx_client);
    	mbedtls_entropy_init(&entropy);

    	unsigned char buf[MBEDTLS_ENTROPY_BLOCK_SIZE];

    	int ret = mbedtls_entropy_func(&entropy, buf, sizeof(buf));

    	if (ret != 0) {
    		printf("Erro na coleta de entropia: %d\n", ret);
    		return ret;
    	}

    	print_hex("Entropia coletada", buf, sizeof(buf));
    	mbedtls_ctr_drbg_init(&ctr_drbg);



	handle_error(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0));

	// Generate server key pair
	handle_error(mbedtls_ecdh_setup(&ctx_server, MBEDTLS_ECP_DP_SECP256R1));
	handle_error(mbedtls_ecdh_gen_public(&ctx_server.grp, &ctx_server.d, &ctx_server.Q, mbedtls_ctr_drbg_random, &ctr_drbg));
	handle_error(mbedtls_ecp_point_write_binary(&ctx_server.grp, &ctx_server.Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, server_public_key, sizeof(server_public_key)));


	// Generate client key pair
	handle_error(mbedtls_ecdh_setup(&ctx_client, MBEDTLS_ECP_DP_SECP256R1));
	handle_error(mbedtls_ecdh_gen_public(&ctx_client.grp, &ctx_client.d, &ctx_client.Q, mbedtls_ctr_drbg_random, &ctr_drbg));
	handle_error(mbedtls_ecp_point_write_binary(&ctx_client.grp, &ctx_client.Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, client_public_key, sizeof(client_public_key)));


	// Compute shared secret
	handle_error(mbedtls_ecdh_read_public(&ctx_server, client_public_key, sizeof(client_public_key)));
	handle_error(mbedtls_ecdh_calc_secret(&ctx_server, &olen, shared_secret, sizeof(shared_secret), mbedtls_ctr_drbg_random, &ctr_drbg));


	// Encrypt message
	mbedtls_aes_context aes;
	mbedtls_aes_init(&aes);
	handle_error(mbedtls_aes_setkey_enc(&aes, shared_secret, 256));
	handle_error(mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, strlen(MESSAGE), iv, (unsigned char *)MESSAGE, encrypted_message));
	mbedtls_aes_free(&aes);

	print_hex("Encrypted Message", encrypted_message, strlen(MESSAGE));


	decrypted_message[strlen(MESSAGE)] = '\0';
	printf("Decrypted Message: %s\n", decrypted_message);

	mbedtls_ecdh_free(&ctx_server);
	mbedtls_ecdh_free(&ctx_client);
	mbedtls_entropy_free(&entropy);
	mbedtls_ctr_drbg_free(&ctr_drbg);

    /* Force the counter to be placed into memory. */
    volatile static int i = 0 ;
    /* Enter an infinite loop, just incrementing a counter. */
    while(1)
    {
        i++ ;
        /* 'Dummy' NOP to allow source level single stepping of
            tight while() loop */
        __asm volatile ("nop");
    }
    return 0 ;
}

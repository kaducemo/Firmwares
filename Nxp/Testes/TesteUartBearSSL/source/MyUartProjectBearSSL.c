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
#include "fsl_rnga.h"


/* TODO: insert other include files here. */
/*******************************************************************************
 * Code
 ******************************************************************************/
#define CLEAR_MSB_N_BITS(byte, n) ((byte) & (255U >> n))
#define CLEAR_LSB_N_BITS(byte, n) ((byte) & (255U << n))


typedef struct KeygenResult
{
    uint8_t pvBuf[BR_EC_KBUF_PRIV_MAX_SIZE];
    uint8_t pbBuf[BR_EC_KBUF_PUB_MAX_SIZE];
    br_ec_private_key pvKey;
    br_ec_public_key pbKey;
    int ok;
}Chave;

Chave chavesLocais, chavesRemotas;

uint8_t segredo1[65] = {0}, segredo2[32] = {0}; //Gera Segredos para comparacao

uint8_t rxSerialBuffer[512] = {0}; //Buffer utilizado na porta serial
size_t qtDadosDisponiveis = 0; //Diz quantos bytes estão disponíveis para serem lidos quando um pacote é fechado

uint8_t chavePublicaRemotaBuf[65] = {0};
br_ec_public_key chavePublicaRemota = 	{
											.curve = BR_EC_secp256r1,
											.q = chavePublicaRemotaBuf,
											.qlen = sizeof(chavePublicaRemotaBuf)
										};
uint64_t contadorMensagensTX = 0;


void UART0_SERIAL_RX_TX_IRQHANDLER(void)
{
	static uint8_t cont = 0;
	static uint8_t recebendoPacote = 0;
    /* If new data arrived. */
    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(UART0))
    {
    	rxSerialBuffer[cont] = UART_ReadByte(UART0);
    	if(rxSerialBuffer[cont] == 0x02) //Inicio de Frame
    	{
    		recebendoPacote = 1;
    		qtDadosDisponiveis = 0;
    		cont = 0;
    	}
    	if(recebendoPacote)
    	{
    		if(cont < 512-1)
    		{
    			cont++;
    			if(rxSerialBuffer[cont - 1] == 0x03)
				{
					qtDadosDisponiveis = cont-2; //Retira 0x02 e 0x03 da contagem
					cont = 0;
					recebendoPacote = 0;
				}
    		}
    		else //Estourou Buffer
    		{
    			cont = 0;
    			recebendoPacote = 0;
    			qtDadosDisponiveis = 0;
    		}
    	}
    }
    SDK_ISR_EXIT_BARRIER;
}

size_t DadosProntosParaLeitura()
{
	return qtDadosDisponiveis;
}

size_t LeDadosSerial(uint8_t **output)
{
	size_t qty = 0;

	if(qtDadosDisponiveis)
	{
		*output = &rxSerialBuffer[1]; //salva quantos bytes foram lidos
		qty = qtDadosDisponiveis;
		qtDadosDisponiveis = 0;
	}
	else
	{
		output = NULL;
		qty = 0;
	}

	return qty;
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
//    	PRINTF("ERRO: sem fonte de RANDOM\n\r");
    	ch->ok = -1;
    }

    br_hmac_drbg_context rng;
    br_hmac_drbg_init(&rng, &br_sha256_vtable, NULL, 0);

    if (!seeder(&rng.vtable))
    {
//    	PRINTF("Falha ao gerar funcao RANDOM\n\r");
    	ch->ok = -1;
    }

//    PRINTF("Nome da funcao Seeder: %s\n\r", seeder_name);

    const br_ec_impl *impl = br_ec_get_default();

    //Gera chave privada
    ch->ok = (0 != br_ec_keygen( &rng.vtable, impl, &(ch->pvKey), ch->pvKey.x, curve));

    length = br_ec_compute_pub(impl, &(ch->pbKey), ch->pbKey.q, &(ch->pvKey));

    if (length == 0)
    {
//    	PRINTF("-ERRO: Curva nao suportada\n\r");
    	ch->ok = -1;
	}

    return ch->ok;
}


int ComputeSharedSecret(br_ec_private_key *pvKey, br_ec_public_key *pbKey, uint8_t *secret)
{
	const br_ec_impl *ec_implementation = br_ec_get_default();

	br_ec_public_key tmp = *pbKey;
//	if (ec_implementation->mul(tmp.q, tmp.qlen, pvKey->x, pvKey->xlen, BR_EC_secp256r1))
//		memcpy(secret, tmp.q, 33);
//	else
//		return -1;
//
	ec_implementation->mul(tmp.q, tmp.qlen, pvKey->x, pvKey->xlen, BR_EC_secp256r1);

	if (tmp.qlen == 65) {
	    memcpy(secret, tmp.q, 65);
	    return 0;
	}
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

uint8_t* apply_pkcs7_padding(const uint8_t *input, size_t len, size_t *padded_len_out)
{
    size_t pad_len = 16 - (len % 16);
    size_t padded_len = len + pad_len;

    uint8_t *padded = malloc(padded_len);
    if (!padded) return NULL;

    memcpy(padded, input, len);
    memset(padded + len, (uint8_t)pad_len, pad_len);

    *padded_len_out = padded_len;
    return padded;
}

uint8_t* CodificaDados7bits(const uint8_t *input, size_t len_input, size_t *len_out)
{
	uint8_t *output = NULL;
	*len_out = ((len_input*8) % 7) ? ((len_input*8) / 7) + 1: (len_input*8) / 7;

	output = malloc (*len_out);
	if(output == NULL)
	{
		*len_out = 0;
		return output;
	}
	else
	{
		memset(output,0,*len_out);
		int j = *len_out - 1;
		uint8_t msb = 0, lsb = 0, shift = 1;


		for(int i  = len_input - 1; i >= 0 ; i--)
		{
			msb = input[i];
			msb >>= shift;
			lsb = input[i];

			lsb = CLEAR_MSB_N_BITS(lsb, (8-shift));
			lsb <<= (7 - shift);


			output[j] |= 0x80;
			output[j] |= msb;
			j--;
			output[j] |= 0x80;
			output[j] |= lsb;

			if((shift + 1) % 8)
			{
				shift++;

			}
			else
			{
				shift = 1;
				j--;
			}
		}
	}
	return output;

}

uint8_t* DecodificaDados7bits(const uint8_t* input, size_t len_input, size_t* len_out)
{
    if (!input || len_input == 0 || !len_out) return NULL;

    size_t total_bits = len_input * 7; // 7 bits úteis por byte
    *len_out = total_bits / 8;         // número de bytes originais
    uint8_t* output = malloc(*len_out);
    if (!output) {
        *len_out = 0;
        return NULL;
    }

    memset(output, 0, *len_out);

    int in_index = len_input - 1;
    int out_index = *len_out - 1;
    uint8_t shift = 1;

    for(out_index = *len_out-1; out_index>= 0 ; out_index--)
    {
    	output[out_index] = input[in_index] << shift;
    	output[out_index] = CLEAR_LSB_N_BITS(output[out_index], shift);
    	//uint8_t aux = (input[in_index -1] & 0b01111111) >> (7-shift) ;
    	output[out_index] |= (input[in_index -1] & 0b01111111) >> (7-shift);
    	in_index--;
    	if((shift + 1) % 8)
		{
			shift++;

		}
		else
		{
			shift = 1;
			in_index--;
		}
    }

    return output;
}

uint8_t *CriaQuadroCodificado(uint8_t *input, size_t q, size_t *qOutput)
{
	uint8_t *output = NULL, *codificado = NULL;
	size_t qOut = 0;
	*qOutput = 0;

	codificado = CodificaDados7bits(input, q, &qOut);

	if(!qOut)
		return NULL;


	output = malloc(qOut+2);
	if(output == NULL)	return NULL;

	*qOutput =  qOut+2;

	memset(output, 0, *qOutput);

	output[0] = 0x02;// Insere Headers
	output[(*qOutput)-1] = 0x03;

	memcpy(&output[1], codificado, qOut);
	free(codificado);

	return output;
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

    const char *salt_hkdf = "DATAPROM_SALT";
    char info[18] = {'D','A','T','A','S','E','C','R','E','T','\0','\0','\0','\0','\0','\0','\0','\0'};

    uint8_t key[32] = {0};
	uint8_t iv[16] = {0};
	uint8_t *encrypted = NULL;
	//uint8_t encrypted[16] = {0};
	uint8_t decrypted[16] = {0};
	//uint8_t plain[16] = "DESAFIO, DP40\0"; // 16 bytes
	uint8_t plain[] = "DESAFIO, DP40 - Agora o desafio eh muito maior!!!"; // 16 bytes

	size_t tamPacoteRx = 0, tamPacoteTx = 0;
	size_t tamPacoteDecodifcado = 0;


    VOLTA:
    memset(chavePublicaRemotaBuf, 0, sizeof(chavePublicaRemotaBuf));
    memset(&chavesLocais, 0, sizeof(Chave));
    memset(segredo1, 0, sizeof(segredo1));

    memset(key, 0, sizeof(key));
    memset(iv, 0, sizeof(iv));


    //Aloca buffers para chaves remotas e Locais
	chavesLocais.pvKey.x = chavesLocais.pvBuf;
	chavesLocais.pbKey.q = chavesLocais.pbBuf;

	chavesRemotas.pbKey = chavePublicaRemota;

	while(!DadosProntosParaLeitura()); //Espera para receber a chave pública

	uint8_t *pacoteRecebido;
	uint8_t *pacoteEnviado;
	uint8_t *pacoteDecodificado;
	tamPacoteRx = LeDadosSerial(&pacoteRecebido);


	if(!tamPacoteRx)
		while(true); //Erro

	pacoteDecodificado = DecodificaDados7bits(pacoteRecebido, tamPacoteRx, &tamPacoteDecodifcado);
	if(tamPacoteDecodifcado != 65)
		while(true); //Erro

	memcpy(chavePublicaRemotaBuf, pacoteDecodificado, tamPacoteDecodifcado);
	free(pacoteDecodificado); //Libera a memória alocada para o pacote


    if(keygen_ec(BR_EC_secp256r1, &chavesLocais) == 1) // Gera chaves Locais
	{

    	pacoteEnviado = CriaQuadroCodificado(chavesLocais.pbKey.q, chavesLocais.pbKey.qlen, &tamPacoteTx);

    	UART_WriteBlocking(UART0, pacoteEnviado, tamPacoteTx);
    	free(pacoteEnviado);

    	if(!ComputeSharedSecret(&chavesLocais.pvKey, &chavesRemotas.pbKey, segredo1))
    	{

    		memcpy(&info[10], &contadorMensagensTX, sizeof(contadorMensagensTX)); // Atualiza INFO a cada mensagem enviada

    		// Produz Key
			br_hkdf_context hkdfKey;
			br_hkdf_init(&hkdfKey, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
			br_hkdf_inject(&hkdfKey, segredo1, sizeof(segredo1));
			br_hkdf_flip(&hkdfKey);
			br_hkdf_produce(&hkdfKey, info, sizeof(info)- sizeof(contadorMensagensTX), key, sizeof(key));	 //Produz Chave


			//Produz IV
			br_hkdf_context hkdfIV;
			br_hkdf_init(&hkdfIV, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
			br_hkdf_inject(&hkdfIV, segredo1, sizeof(segredo1));
			br_hkdf_flip(&hkdfIV);
			br_hkdf_produce(&hkdfIV, info, sizeof(info), iv, sizeof(iv));

			uint8_t iv_original[16] = {0};
			memcpy(iv_original, iv, 16); // salva o IV original

			size_t padded_len_out = 0;
			uint8_t *paddedPlain = apply_pkcs7_padding(plain, sizeof(plain), &padded_len_out);

			br_aes_small_cbcenc_keys  ctx_enc;
			br_aes_small_cbcenc_init(&ctx_enc, key, sizeof(key));
			encrypted = malloc(padded_len_out);
			if(encrypted == NULL)
				while(true);

			memcpy(encrypted, paddedPlain, padded_len_out);

			br_aes_small_cbcenc_run(&ctx_enc, iv, encrypted, padded_len_out); //Aqui IV será modificado

			/*Anexa iterador ao quadro codificado*/
			size_t tam_contador_encrypted = sizeof(contadorMensagensTX) + padded_len_out;
			uint8_t *contador_encrypted = malloc(tam_contador_encrypted); // Aloca memória para adicionar Iterador aos dados criptgrafados
			memcpy(contador_encrypted, &contadorMensagensTX, sizeof(contadorMensagensTX)); //Anexa contador Codificado no inicio da mensagem
			memcpy(&contador_encrypted[sizeof(contadorMensagensTX)], encrypted, padded_len_out); //Anexa Dados Encriptados


			/*Cria pacote codificado em 7 bits e adiciona headers de comunicação*/
			pacoteEnviado = CriaQuadroCodificado(contador_encrypted, tam_contador_encrypted, &tamPacoteTx);

			/*Envia */
			UART_WriteBlocking(UART0, pacoteEnviado, tamPacoteTx);
			free(encrypted);
			free(pacoteEnviado);
			free(contador_encrypted);
			free(paddedPlain);
			contadorMensagensTX++;


//			UART_WriteBlocking(UART0, paddedPlain, padded_len_out);
//			UART_WriteBlocking(UART0, encrypted, padded_len_out);
//			UART_WriteBlocking(UART0, encrypted, sizeof(encrypted));

			/*Libera a memoria alocada*/
//			free(paddedPlain);
//			free(encrypted);

//			br_aes_small_cbcdec_keys ctx_dec;
//			uint8_t iv_dec[16];
//			memcpy(iv_dec, iv_original, 16); // reset IV
//			memcpy(decrypted, encrypted, 16);
//			br_aes_small_cbcdec_init(&ctx_dec, key, sizeof(key));
//			br_aes_small_cbcdec_run(&ctx_dec, iv_dec, decrypted, 16);

			asm("NOP");
			asm("NOP");
			asm("NOP");

    	}
    	else
    	{
    		while(true);
    	}
	}

    goto VOLTA;

/*=========================================================================================*/
/*=========================================================================================*/
/*=========================================================================================*/

    return 0 ;
}

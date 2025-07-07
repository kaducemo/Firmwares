/*
 * Copyright 2016-2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    MyUartProjectBearSSL.c
 * @brief   Application entry point.
 * PoC realizada com Criptografia. Deve ser gravado na placa FRDM-K64. Utilizado e conjunto
 * com um Software que se comunica com essa placa através da UART0. Neste FW, é trocadas
 * chaves publicas, gerados segredos de curvas elipticas (P256), utilizado HKDF, para geração
 * de Chaves e IVs para AES-256, finalmente é trocado um desafio/solução com o SW para validar
 * todo o canal.
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
#define CLEAR_MSB_N_BITS(byte, n) ((byte) & (255U >> n)) //Macros auxiliares
#define CLEAR_LSB_N_BITS(byte, n) ((byte) & (255U << n))


typedef struct KeygenResult
{
    uint8_t pvBuf[BR_EC_KBUF_PRIV_MAX_SIZE];
    uint8_t pbBuf[BR_EC_KBUF_PUB_MAX_SIZE];
    br_ec_private_key pvKey;
    br_ec_public_key pbKey;
    int ok;
}Chave;

Chave chavesLocais, chavesRemotas; // Chaves Publicas e Privadas (locais e remotas)

uint8_t segredo1[65] = {0}, segredo2[32] = {0}; //Gera Segredos para comparacao

uint8_t rxSerialBuffer[512] = {0}; //Buffer utilizado na porta serial
size_t qtDadosDisponiveis = 0; //Diz quantos bytes estão disponíveis para serem lidos quando um pacote é fechado

uint8_t chavePublicaRemotaBuf[65] = {0}; //Buffer chave Publica remota
br_ec_public_key chavePublicaRemota = 	{
											.curve = BR_EC_secp256r1,
											.q = chavePublicaRemotaBuf,
											.qlen = sizeof(chavePublicaRemotaBuf)
										};
uint64_t contadorMensagensTX = 0; //Conta o fluxo de mensagens. Importante para geração da IVs.


void UART0_SERIAL_RX_TX_IRQHANDLER(void)
/*Interrupçao UART0*/
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
					qtDadosDisponiveis = cont;
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
/*Diz o tamanho do quadro que está disponível para leitura*/
{
	return qtDadosDisponiveis;
}

size_t LeDadosSerial(uint8_t **output)
/*Retorna um ponteiro para o quadro recebido na UART0 (Caso haja algum)
 * output: Ponteiro retornado (não precisa ser deslocado (buffer estático)
 * return: tamanho do quadro recebido
 * */
{
	size_t qty = 0;

	if(qtDadosDisponiveis)
	{
		*output = &rxSerialBuffer[0]; //salva quantos bytes foram lidos
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
/*Interrupção UART4 (NÃO ESTÁ SENDO UTILZADO*/
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

int keygen_ec(int curve, Chave *ch)
/*Gera um par de chaves(publica e privada) para a curva selecionada*/
{
    const char *seeder_name;
    ch->ok = 1;

    const br_ec_impl *impl = br_ec_get_default(); //Inicia contexto ECDHE

    /*Inicia gerador de sememntes (ENTROPIA)*/
    br_prng_seeder seeder = br_prng_seeder_system(&seeder_name);
    if (seeder == NULL)
	/*Erro. Não conseguiu iniciar o gerador de sementes (ENTROPIA)*/
    {

    	ch->ok = -1;
    	memset(ch,0,sizeof(Chave)); //Limpa toda estrutura de chaves antes de retornar
    	return ch->ok;
    }

    //Inicia Gerador Deterministico SHA256
    br_hmac_drbg_context rng;
    br_hmac_drbg_init(&rng, &br_sha256_vtable, NULL, 0);
    if (!seeder(&rng.vtable))
	/*Erro. Não conseguiu iniciar o gerador detereministico SHA256*/
    {
    	ch->ok = -1;
    	memset(ch,0,sizeof(Chave)); //Limpa toda estrutura de chaves antes de retornar
    	return ch->ok;
    }

    //Gera chave privada ECDHE
    if (!br_ec_keygen( &rng.vtable, impl, &(ch->pvKey), ch->pvKey.x, curve))
	/*Erro. Geracao da chave privada*/
    {
    	ch->ok = -1;
		memset(ch,0,sizeof(Chave)); //Limpa toda estrutura de chaves antes de retornar
		return ch->ok;
    }

    //Gera chave publica ECDHE a partir da chave privada
    if (!br_ec_compute_pub(impl, &(ch->pbKey), ch->pbKey.q, &(ch->pvKey)))
    	/*Erro. Geracao da chave publica*/
    {
    	ch->ok = -1;
    	memset(ch,0,sizeof(Chave)); //Limpa toda estrutura de chaves antes de retornar
    	return ch->ok;
	}

    //Limpa toda estrutura do gerador Deterministico
    memset(&rng,0,sizeof(br_hmac_drbg_context));

    return ch->ok;
}


int ComputeSharedSecret(br_ec_private_key *pvKey, br_ec_public_key *pbKey, uint8_t *secret)
/*Retorna o segredo da curva eliptica BR_EC_secp256r1
 *
 * pvKey: CHAVE PRIVADA
 * pbKey: CHAVE PUBLICA
 * secret: Secredo compartilhado calculado
 * return: 0 = segredo gerado com sucesso, -1 = erro
 * */
{
	const br_ec_impl *ec_implementation = br_ec_get_default();

	br_ec_public_key tmp = *pbKey;
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
/*Função bloqueante de Delay...*/
{
    volatile uint32_t i = 0;
    for (i = 0; i < 8000; ++i)
    {
        __asm("NOP"); /* delay */
    }
}

uint8_t* apply_pkcs7_padding(const uint8_t *input, size_t len, size_t *padded_len_out)
/* Aplica PADDING PKCS7 em um vertor de entrada
 * input: dados que se deseja codificar para (8 bits)
 * len_input: tamanho dos dados de entrada
 * len_out: tamanho dos dados decodificados na saida
 * return: ponteiro para dados codificados em 7 bits alocados dinamicamente (deve
 * ser desalocado manualmente após uso)
 *
 * */
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

/**
 * Remove o padding PKCS#7 de um buffer e retorna uma nova área de memória com os dados originais.
 *
 * @param input        Dados de entrada com padding.
 * @param padded_len   Tamanho total dos dados (incluindo padding).
 * @param output_len   Ponteiro para armazenar o tamanho dos dados sem o padding.
 * @return             Ponteiro para nova região com dados decodificados, ou NULL se o padding for inválido.
 *                     O ponteiro retornado deve ser liberado com free().
 */
uint8_t* remove_pkcs7_padding(const uint8_t *input, size_t padded_len, size_t *output_len) {
    if (padded_len == 0 || input == NULL || output_len == NULL) return NULL;

    uint8_t pad_len = input[padded_len - 1];
    if (pad_len == 0 || pad_len > 16 || pad_len > padded_len) return NULL;

    // Verifica se todos os bytes de padding estão corretos
    for (size_t i = 0; i < pad_len; i++) {
        if (input[padded_len - 1 - i] != pad_len) return NULL;
    }

    size_t unpadded_len = padded_len - pad_len;
    uint8_t *output = malloc(unpadded_len);
    if (!output) return NULL;

    memcpy(output, input, unpadded_len);
    *output_len = unpadded_len;
    return output;
}


uint8_t* CodificaDados7bits(const uint8_t *input, size_t len_input, size_t *len_out)
/* Codifica um vetor de uint8 em um outro vetor de 7bits com o msb setado.
 * input: dados que se deseja codificar para (8 bits)
 * len_input: tamanho dos dados de entrada
 * len_out: tamanho dos dados decodificados na saida
 * return: ponteiro para dados codificados em 7 bits alocados dinamicamente (desalocar manualmente)
 * */
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
/* Descodifica o vetor de 7bits (com o msb setado) para um vetor tradicional de 8bits
 * input: dados que se deseja retornar para o formato original (8 bits)
 * len_input: tamanho dos dados de entrada
 * len_out: tamanho dos dados decodificados na saida
 * return: ponteiro para dados decodificados alocados dinamicamente (desalocar manualmente)
 * */
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
/* Cria quadro padronizado de  comunicação com os dados de entrada
 * input: dados que se deseja criar o quadro
 * q: tamanho dos dados de entrada
 * qOutput: tamanho dos dados codificados na saida
 * return: ponteiro para dados codificados alocados dinamicamente (desalocar manualmente)
 * */
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

uint8_t *ObtemDadosQuadroCodificado(uint8_t *input, size_t q, size_t *qOutput)
/* Retira os dados do quadro padronizado de comunicação (já retirando o header)
 * input: quadro codificado com o header
 * q: tamanho total do quadro codificado
 * qOutput: tamanho dos dados decodificados
 * return: ponteiro para dados decodificados alocados dinamicamente (desalocar manualmente)
 * */
{
	uint8_t *pacoteDecodificado = DecodificaDados7bits(&input[1], q-2, qOutput);
	return pacoteDecodificado;
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
	uint8_t plain[] = "DESAFIO, DP40 - Agora o desafio eh muito maior!!!";
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

	pacoteDecodificado = ObtemDadosQuadroCodificado(pacoteRecebido, tamPacoteRx, &tamPacoteDecodifcado);

	if(tamPacoteDecodifcado != 65)
		while(true); //Erro (não bate com tamanho da chave pública)

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

			/*Envia Desafio*/
			UART_WriteBlocking(UART0, pacoteEnviado, tamPacoteTx);

			/*Desaloca memorias*/
			free(encrypted);
			free(pacoteEnviado);
			free(contador_encrypted);
			free(paddedPlain);

			contadorMensagensTX++;

			while(!DadosProntosParaLeitura()); //Espera para receber a Solucao do Desafio

			tamPacoteRx = LeDadosSerial(&pacoteRecebido);

			if(!tamPacoteRx)
				while(true); //Erro

			pacoteDecodificado = ObtemDadosQuadroCodificado(pacoteRecebido, tamPacoteRx, &tamPacoteDecodifcado);

			if(!tamPacoteDecodifcado)
				while(true); //Erro Decodificacao

			/* Salva Contador de Mensagens e solucao criptografada*/
			size_t tamSolucaoCripto = tamPacoteDecodifcado - sizeof(uint64_t);
			uint8_t *solucaoCripto  = malloc(tamSolucaoCripto);

			if(solucaoCripto == NULL)
				while(true);

			memcpy(&contadorMensagensTX, pacoteDecodificado, sizeof(uint64_t)); //Salva Contador
			memcpy(solucaoCripto, &pacoteDecodificado[sizeof(uint64_t)], tamSolucaoCripto); //Salva Mensagem
			free(pacoteDecodificado); //Libera a memória alocada para o pacote Decodificado

			/* Atualiza INFO com o contador de Mensagens*/
			memcpy(&info[10], &contadorMensagensTX, sizeof(contadorMensagensTX));

			/* Produz IV */
			br_hkdf_context hkdfSol;
			br_hkdf_init(&hkdfSol, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
			br_hkdf_inject(&hkdfSol, segredo1, sizeof(segredo1));
			br_hkdf_flip(&hkdfSol);
			br_hkdf_produce(&hkdfSol, info, sizeof(info), iv, sizeof(iv));

			/* Descriptografa */
			br_aes_small_cbcdec_keys ctx_dec;
			br_aes_small_cbcdec_init(&ctx_dec, key, sizeof(key));
			br_aes_small_cbcdec_run(&ctx_dec, iv, solucaoCripto, tamSolucaoCripto);

			/* Retira Padding PKCS7*/
			size_t tamSolucao = 0;
			uint8_t *solucao = remove_pkcs7_padding(solucaoCripto, tamSolucaoCripto, &tamSolucao);
			asm("NOP");
			asm("NOP");
			asm("NOP");
			free(solucaoCripto);
			free(solucao);

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

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
#include "PubKey.h"
#include "PrvKey.h"
#include "PSKLib.h"
#include "base64.h"


/* TODO: insert other include files here. */
/*******************************************************************************
 * Code
 ******************************************************************************/

// Tamanhos padrão para GCM
#define GCM_IV_LEN   	12
#define GCM_TAG_LEN  	16
#define AES_KEY_LEN  	32  // AES-256


#define ECDH_SEC_LEN 	65  //Tamanho do Secredo ECDH formato não compactado
#define HKDF_INFO_LEN	18

#define OK	1
#define NOK 0

#define CLEAR_MSB_N_BITS(byte, n) ((byte) & (255U >> n)) //Macros auxiliares
#define CLEAR_LSB_N_BITS(byte, n) ((byte) & (255U << n))


uint8_t rxSerialBuffer[512] = {0}; //Buffer utilizado na porta serial
size_t qtDadosDisponiveis = 0; //Diz quantos bytes estão disponíveis para serem lidos quando um pacote é fechado
uint64_t contadorMensagens = 0; //Conta o fluxo de mensagens. Importante para geração da IVs.


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


int keygen_ec(int curve, PrvKey *prKey, PubKey *pbKey)
/*Gera um par de chaves(publica e privada) para a curva selecionada*/
{
    const char *seeder_name;

    prKey->ok = OK;
    pbKey->ok = OK;

    const br_ec_impl *impl = br_ec_get_default(); //Inicia contexto ECDHE

    /*Inicia gerador de sememntes (ENTROPIA)*/
    br_prng_seeder seeder = br_prng_seeder_system(&seeder_name);

    if (seeder == NULL)
	/*Erro. Não conseguiu iniciar o gerador de sementes (ENTROPIA)*/
    {
    	prKey->ok = NOK;
		pbKey->ok = NOK;

		memset(prKey,0,sizeof(PrvKey));
		memset(pbKey,0,sizeof(PubKey));

    	return NOK;
    }

    //Inicia Gerador Deterministico SHA256
    br_hmac_drbg_context rng;
    br_hmac_drbg_init(&rng, &br_sha256_vtable, NULL, 0);

    if (!seeder(&rng.vtable))
	/*Erro. Não conseguiu iniciar o gerador detereministico SHA256*/
    {
    	prKey->ok = NOK;
		pbKey->ok = NOK;

		memset(prKey,0,sizeof(PrvKey));
		memset(pbKey,0,sizeof(PubKey));

		return NOK;
    }

    //Gera chave privada ECDHE
    if (!br_ec_keygen( &rng.vtable, impl, &(prKey->pvKey), prKey->pvKey.x, curve))
	/*Erro. Geracao da chave privada*/
    {
    	prKey->ok = NOK;
		pbKey->ok = NOK;

		memset(prKey,0,sizeof(PrvKey));
		memset(pbKey,0,sizeof(PubKey));

		return NOK;
    }

    //Gera chave publica ECDHE a partir da chave privada
    if (!br_ec_compute_pub(impl, &(pbKey->pbKey), pbKey->pbKey.q, &(prKey->pvKey)))
    	/*Erro. Geracao da chave publica*/
    {
    	prKey->ok = NOK;
		pbKey->ok = NOK;

		memset(prKey,0,sizeof(PrvKey));
		memset(pbKey,0,sizeof(PubKey));

		return NOK;
	}

    //Limpa toda estrutura do gerador Deterministico
    memset(&rng,0,sizeof(br_hmac_drbg_context));

    return OK;
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

//	codificado = CodificaDados7bits(input, q, &qOut);
//	if(!qOut)

	codificado = malloc(base64EncodedLength(q));
	if(!codificado)
		return NULL;

	qOut = base64Encode(codificado, input, q);


	output = malloc(qOut+2);
	if(output == NULL)	return NULL;

	*qOutput =  qOut+2;

	memset(output, 0, *qOutput);

	output[0] = 0x02;// Insere Headers
	output[(*qOutput)-1] = 0x03;

	memcpy(&output[1], codificado, qOut);
	secure_zero(codificado, qOut);
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

uint8_t *RetiraDadosDeQuadroRecebido(uint8_t *dadosRx, size_t lenRx, uint8_t cripto, uint8_t *ikm, uint64_t *contador, size_t *lenOut)
/* Esta função retorna os dados de um quadro recebido. Esse quadro por padrão
 * está codificado em 7 bits e possui header e ainda pode estar criptografado.
 * Os dados retornados devem ser desalocados manualmente após seu uso.
 * @param dadosRx(in): Dados recebidos e que devem ser desempacotados
 * @param lenRx(in): tamanho do vetor dos dados Recebidos
 * @param cripto(in): Diz se quadro recebido está criptografado (1 == Criptografado, 0 == Não criptografado).
 * @param secret(in): (utilizado para criptografia) Segredo compartilhado gerado pelo ECDH
 * @param contador(out): (utilizado para criptografia)É o contador de mensagens. Será atualizado a cada chamada.
 * @param lenOut(out): tamanho do vetor de saída (dados empacotados)
 * */
{
	uint8_t *output = NULL;
	const char *salt_hkdf = "DATAPROM_SALT";
	char info[HKDF_INFO_LEN] = {'D','A','T','A','S','E','C','R','E','T','\0','\0','\0','\0','\0','\0','\0','\0'};
	uint8_t iv[GCM_IV_LEN] = {0}; // IV GCM 12 bytes
	uint8_t key[AES_KEY_LEN] = {0}; //Chave GCM

	if(cripto)
	{
		if( dadosRx != NULL && lenRx != 0 && contador != NULL && lenOut != NULL)
		//Verifica integridade dos dados de entrada
		{
			size_t tamPacoteDecodifcado = 0;
			uint8_t *pacoteDecodificado = ObtemDadosQuadroCodificado(dadosRx, lenRx, &tamPacoteDecodifcado);

			if(!tamPacoteDecodifcado)
				while(true); //Erro Decodificacao

			/* Salva Contador de Mensagens e solucao criptografada*/
			size_t tamSolucaoCripto = tamPacoteDecodifcado - sizeof(uint64_t) - GCM_TAG_LEN;
			uint8_t *solucaoCripto  = malloc(tamSolucaoCripto);

			if(solucaoCripto == NULL)
				while(true);

			memcpy(contador, pacoteDecodificado, sizeof(uint64_t)); //Salva Contador
			memcpy(solucaoCripto, &pacoteDecodificado[sizeof(uint64_t)], tamSolucaoCripto); //Salva Mensagem

			/* Atualiza INFO com o contador de Mensagens*/
			memcpy(&info[sizeof(info) - sizeof(uint64_t)], contador, sizeof(uint64_t));

			/* Produz Key Solucao*/
			br_hkdf_context hkdfSolKey;
			br_hkdf_init(&hkdfSolKey, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
			br_hkdf_inject(&hkdfSolKey, ikm, ECDH_SEC_LEN);
			br_hkdf_flip(&hkdfSolKey);
			br_hkdf_produce(&hkdfSolKey, info, sizeof(info), key, sizeof(key));

			/* Produz IV Solucao*/
			br_hkdf_context hkdfSolIV;
			br_hkdf_init(&hkdfSolIV, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
			br_hkdf_inject(&hkdfSolIV, ikm, ECDH_SEC_LEN);
			br_hkdf_flip(&hkdfSolIV);
			br_hkdf_produce(&hkdfSolIV, info, sizeof(info), iv, sizeof(iv));

			// Inicializa AES-GCM
			br_aes_ct64_ctr_keys aes_ctx;
			br_aes_ct64_ctr_init(&aes_ctx, key, AES_KEY_LEN);
			br_gcm_context gcm;
			br_gcm_init(&gcm, &aes_ctx.vtable, &br_ghash_ctmul64);

			 // Prepara buffer de decifração (Retira tamanho contador e TAG
			size_t tamSolucao = tamSolucaoCripto;
			*lenOut = tamSolucao;
			output = malloc(tamSolucao);
			if (!output) while(true);

			// Copia os dados cifrados para saída, pois será decifrado in-place
			memcpy(output, solucaoCripto, tamSolucao);

			// Processo GCM
			br_gcm_reset(&gcm, iv, GCM_IV_LEN);
			br_gcm_aad_inject(&gcm, NULL, 0); // sem AAD
			br_gcm_run(&gcm, 0, output, tamSolucao); // decifra in-place
			br_gcm_flip(&gcm);

			// Verifica TAG
			uint8_t tag_calculada[GCM_TAG_LEN];
			br_gcm_get_tag(&gcm, tag_calculada);

			if (memcmp(&pacoteDecodificado[tamPacoteDecodifcado-GCM_TAG_LEN], tag_calculada, GCM_TAG_LEN) != 0) {
				// TAG inválida → dados corrompidos ou adulterados
				secure_zero(output, tamSolucao);
				free(output);
				output =  NULL;
				*lenOut = 0;
			}

			secure_zero(pacoteDecodificado, tamPacoteDecodifcado);
			secure_zero(solucaoCripto, tamSolucaoCripto);
			free(pacoteDecodificado); //Libera a memória alocada
			free(solucaoCripto);
		}
	}
	else
	{
		if(dadosRx != NULL && lenRx != 0)
		//Verifica integridade dos dados de entrada
		{
			//Gera Quadro sem header e retira codificação 7 bits
			output = ObtemDadosQuadroCodificado(dadosRx, lenRx, lenOut);
		}
	}
	return output;
}

uint8_t *GeraQuadroParaEnvio(uint8_t *dados, size_t lenIn, uint8_t cripto, uint8_t *ikm ,uint64_t *contador, size_t *lenOut)
/*Esta função retorna um quadro pronto para ser transmitido. Esses dados podem ou não serem criptografados. *
 * O quadro retornado deve ser desalocado manualmente após seu uso.
 * @param dados(in): Dados para serem empacotados
 * @param lenIn(in): tamanho do vetor de dados de entrada
 * @param cripto(in): Diz se dados vão ou não serem criptografados (0 não criptografa, qualquer outro valor criptografa)
 * @param secret(in): (utilizado para criptografia)Segredo (alta entropia) gerado por ECDH
 * @param contador(in): (utilizado para criptografia)É o contador de mensagens. Não deve nunca se repetir
 * @param lenOut(out): tamanho do vetor de saída (dados empacotados)
 * */

{
	uint8_t *output = NULL;
	const char *salt_hkdf = "DATAPROM_SALT";
	char info[HKDF_INFO_LEN] = {'D','A','T','A','S','E','C','R','E','T','\0','\0','\0','\0','\0','\0','\0','\0'};
	uint8_t iv[GCM_IV_LEN] = {0}; // IV GCM 12 bytes
	uint8_t key[AES_KEY_LEN] = {0}; //Chave GCM
	uint8_t tag[GCM_TAG_LEN] = {0};
	uint8_t *encrypted = NULL;


	*lenOut = 0;

	if(cripto)
	// Vai criptografar
	{
		if(dados != NULL && lenIn != 0 && ikm != NULL && contador != NULL)
		//Verifica integridade dos dados de entrada
		{
			memcpy(&info[HKDF_INFO_LEN - sizeof(uint64_t)], contador, sizeof(uint64_t)); // Atualiza INFO a cada mensagem enviada

			// Produz Key AES256-GCM
			br_hkdf_context hkdfKey;
			br_hkdf_init(&hkdfKey, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
			br_hkdf_inject(&hkdfKey, ikm, ECDH_SEC_LEN);
			br_hkdf_flip(&hkdfKey);
			br_hkdf_produce(&hkdfKey, info, sizeof(info), key, AES_KEY_LEN);	 //Produz Chave


			// Produz IV GCM
			br_hkdf_context hkdfIV;
			br_hkdf_init(&hkdfIV, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
			br_hkdf_inject(&hkdfIV, ikm, ECDH_SEC_LEN);
			br_hkdf_flip(&hkdfIV);
			br_hkdf_produce(&hkdfIV, info, sizeof(info), iv, sizeof(iv));

			//Inicializa contexto AES-GCM
			br_gcm_context gcm;
			br_aes_ct64_ctr_keys aes_ctx;
			br_aes_ct64_ctr_init(&aes_ctx, key, AES_KEY_LEN);
			br_gcm_init(&gcm, &aes_ctx.vtable, &br_ghash_ctmul64);
			encrypted = malloc(lenIn);
			if(encrypted == NULL)
				return output;

			//Criptografa Dados
			memcpy(encrypted, dados, lenIn);
			br_gcm_reset(&gcm, iv, GCM_IV_LEN);
			br_gcm_aad_inject(&gcm, NULL, 0); // nenhum AAD, se desejar pode adicionar depois
			br_gcm_run(&gcm, 1, encrypted, lenIn);
			br_gcm_flip(&gcm);
			br_gcm_get_tag(&gcm, tag);

			//Junta Dados, Contador e TAG no Quadro: [Contador de Mensagens(LSB) | Dados Cript | TAG (MSB)]
			size_t lenQuadro = sizeof(uint64_t) + lenIn + GCM_TAG_LEN;
			uint8_t *quadro = malloc(lenQuadro);
			if (!quadro) { //Erro de alocacao
				secure_zero(encrypted, lenIn);
				free(encrypted);
				return output;
			}
			memcpy(quadro, contador, sizeof(uint64_t));
			memcpy(&quadro[sizeof(uint64_t)], encrypted, lenIn);
			memcpy(&quadro[sizeof(uint64_t) + lenIn], tag, GCM_TAG_LEN);

			//Gera Quadro codificado com 7bits e Headers
			output = CriaQuadroCodificado(quadro, lenQuadro, lenOut);

			//Libera alocacoes intermediarias
			secure_zero(encrypted, lenIn);
			secure_zero(quadro, lenQuadro);
			free(encrypted);
			free(quadro);

		}
	}
	else
	// Sem criptografia
	{
		if(dados != NULL && lenIn != 0)
		//Verifica integridade dos dados de entrada
		{
			//Gera Quadro codificado com 7bits e Headers
			output = CriaQuadroCodificado(dados, lenIn, lenOut);
		}
	}
	return output;
}


/* TODO: insert other definitions and declarations here. */

/*
 * @brief   Application entry point.
 */
int main(void) {

	PrvKey *chaveLocalPrivada = NULL;
	PubKey *chaveLocalPublica = NULL, *chaveRemotaPublica = NULL;

	uint8_t segredo1[ECDH_SEC_LEN] = {0}; //Gera Segredos para comparacao

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

	uint8_t plain[] = "DESAFIO, DP40 - Agora o desafio eh muito maior!!!";
	size_t tamPacoteRx = 0, tamPacoteTx = 0;
	size_t tamPacoteDecodifcado = 0;

VOLTA:

	if (chaveLocalPrivada != NULL)		Destroy_PrvKey(chaveLocalPrivada);
	chaveLocalPrivada = New_PrvKey();

	if (chaveLocalPublica != NULL)		Destroy_PubKey(chaveLocalPublica);
	chaveLocalPublica = New_PubKey();

	if (chaveRemotaPublica != NULL)		Destroy_PubKey(chaveRemotaPublica);
	chaveRemotaPublica = New_PubKey();

    memset(segredo1, 0, sizeof(segredo1));



    //Aloca buffers para chaves remotas e Locais
	chaveRemotaPublica->pbKey.curve = BR_EC_secp256r1;
	chaveRemotaPublica->pbKey.qlen = sizeof(chaveRemotaPublica->pbBuf);

	while(!DadosProntosParaLeitura()); //Espera para receber a chave pública

	uint8_t *pacoteRecebido;
	uint8_t *pacoteEnviado;
	uint8_t *pacoteDecodificado;

	tamPacoteRx = LeDadosSerial(&pacoteRecebido);
	if(!tamPacoteRx)	while(true); //Erro

	pacoteDecodificado = RetiraDadosDeQuadroRecebido(pacoteRecebido, tamPacoteRx, 0, NULL, NULL, &tamPacoteDecodifcado);
	if(tamPacoteDecodifcado != 65)	while(true); //Erro (não bate com tamanho da chave pública)

	memcpy(chaveRemotaPublica->pbBuf, pacoteDecodificado, tamPacoteDecodifcado);
	secure_zero(pacoteDecodificado, tamPacoteDecodifcado);
	free(pacoteDecodificado); //Libera a memória alocada para o pacote

	if(keygen_ec(BR_EC_secp256r1, chaveLocalPrivada, chaveLocalPublica) == 1) // Gera chaves Locais (Publica e Privada
	{
    	/* == Enviando a Chave Publica  == */

    	// Empacota Chave publica sem Criptografia
    	pacoteEnviado = GeraQuadroParaEnvio(chaveLocalPublica->pbKey.q, chaveLocalPublica->pbKey.qlen, 0,NULL ,NULL, &tamPacoteTx);
    	if(pacoteEnviado == NULL || tamPacoteTx == 0)	while(true);

    	// Envia a Chave publica
    	UART_WriteBlocking(UART0, pacoteEnviado, tamPacoteTx);

    	// Desaloca o Pacote
    	secure_zero(pacoteEnviado, tamPacoteTx);
    	free(pacoteEnviado);

    	if(!ComputeSharedSecret(&chaveLocalPrivada->pvKey, &chaveRemotaPublica->pbKey, segredo1)) //Gera o Segredo Compartilhado
    	{


    		/* == Enviando o Desafio == */

    		// Produz IKM
			uint8_t ikm[ECDH_SEC_LEN];
			if(!psk_ikm_get(1, (char *)segredo1, (char *)ikm)) //Gera IKM com segredo e código do controlador #1
				while(true);

			//Cria um quadro criptografado contendo o Desafio
			pacoteEnviado = GeraQuadroParaEnvio(plain, sizeof(plain), 1, ikm, &contadorMensagens, &tamPacoteTx);

			/*Envia o Desafio*/
			UART_WriteBlocking(UART0, pacoteEnviado, tamPacoteTx);
			contadorMensagens++;

			/*Desaloca pacote*/
			secure_zero(pacoteEnviado, tamPacoteTx);
			free(pacoteEnviado);

			/* == Recebe a Solução == */

			//Espera para receber a Solucao do Desafio
			while(!DadosProntosParaLeitura()); //Espera para receber a Solucao do Desafio
			tamPacoteRx = LeDadosSerial(&pacoteRecebido);
			if(!tamPacoteRx)	while(true); //Erro

			//Decifra pacote da solucão
			pacoteDecodificado = RetiraDadosDeQuadroRecebido(pacoteRecebido, tamPacoteRx, 1, ikm, &contadorMensagens, &tamPacoteDecodifcado);
			if(!tamPacoteDecodifcado)	while(true); //Erro Decodificacao

			/*Desaloca solucao*/
			secure_zero(pacoteDecodificado, tamPacoteDecodifcado);
			free(pacoteDecodificado);
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

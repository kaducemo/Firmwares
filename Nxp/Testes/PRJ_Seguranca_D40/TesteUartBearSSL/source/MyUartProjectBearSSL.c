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
#include "Utilidades.h"
#include "DatapromFrame.h"
#include "PubKey.h"
#include "PrvKey.h"
#include "PSKLib.h"
#include "base64.h"
#include "OpcodesDP.h"


/* TODO: insert other include files here. */
/*******************************************************************************
 * Code
 ******************************************************************************/

#define DP40_CODIGO				5


#define UART_RX_BUFFER_LEN		512

uint8_t plain[] = "DESAFIO, DP40 - Agora o desafio eh muito maior!!!";

PrvKey *chaveLocalPrivada = NULL;
PubKey *chaveLocalPublica = NULL, *chaveRemotaPublica = NULL;



uint8_t rxSerialBuffer[UART_RX_BUFFER_LEN] = {0}; //Buffer utilizado na porta serial
size_t qtDadosDisponiveis = 0; //Diz quantos bytes estão disponíveis para serem lidos quando um pacote é fechado
uint64_t contadorMensagens = 0; //Conta o fluxo de mensagens. Importante para geração da IVs.

uint8_t segredo[ECDH_SEC_LEN] = {0}; //Segredo ECDH
uint8_t ikm[ECDH_SEC_LEN] = {0};


void UART0_SERIAL_RX_TX_IRQHANDLER(void){
	/*Interrupçao UART0*/

	static size_t cont = 0;
	static uint8_t recebendoPacote = 0;

    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(UART0))  {
    	// Recebeu dados

    	if (recebendoPacote) {
    		// Recebendo um FRAME
    		rxSerialBuffer[cont++] = UART_ReadByte(UART0);
    		if(rxSerialBuffer[cont-1] == STX) {
    			//Inicio de Frame

				qtDadosDisponiveis = 0;
				rxSerialBuffer[0] = STX;
				cont = 1;
			}
			else if(rxSerialBuffer[cont-1] == ETX)	{
				//Final de Frame

				qtDadosDisponiveis = cont;
				cont = 0;
				recebendoPacote = 0;
			}
    		if(cont >= UART_RX_BUFFER_LEN) {
				//Estourou Buffer (ERRO)

				cont = 0;
				recebendoPacote = 0;
				qtDadosDisponiveis = 0;
    		}

    	}
    	else {
    		// Esperando receber STX para iniciar FRAME

    		rxSerialBuffer[0] = UART_ReadByte(UART0);
    		if(rxSerialBuffer[0] == STX) {
    			//Inicio de Frame

				recebendoPacote = 1;
				qtDadosDisponiveis = 0;
				cont = 1;
			}
    	}
    }
    SDK_ISR_EXIT_BARRIER;
}

void UART4_SERIAL_RX_TX_IRQHANDLER(void){
	/*Interrupção UART4 (NÃO ESTÁ SENDO UTILZADO*/

    uint8_t data;

    /* If new data arrived. */
    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(UART4))
    {
        data = UART_ReadByte(UART4);
        UART_WriteByte(UART0, data);
    }
    SDK_ISR_EXIT_BARRIER;
}

size_t dados_prontos_uart(){
	/*Diz o tamanho do quadro que está disponível para leitura*/

	return qtDadosDisponiveis;
}

size_t dados_prontos_eth(){
	/*Diz o tamanho do quadro que está disponível para leitura*/
	return 0;
}

size_t le_dados_uart(uint8_t **output) {
	/*Retorna um ponteiro para o quadro recebido na UART0 (Caso haja algum)
	 * output: Ponteiro retornado (não precisa ser deslocado (buffer estático)
	 * return: tamanho do quadro recebido
	 * */

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

size_t le_dados_eth(uint8_t **output)
/*Retorna um ponteiro para o quadro recebido na UART0 (Caso haja algum)
 * output: Ponteiro retornado (não precisa ser deslocado (buffer estático)
 * return: tamanho do quadro recebido
 * */
{
	size_t qty = 0;
	return qty;
}

bool le_chave()
//Diz se chave foi pressionada
{
	bool ret = false;
	return ret;
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




void processa_evento_chave()

{
	//Processa pressionamento da chave
}


void processa_dados_rx(uint8_t interface, uint8_t *dados, size_t q, uint8_t *ikm) {
	//Processa dados recebidos atraves de uma interface
	//Interface == 0: UART, 1 == EHT

	DP_Frame_t *frameResposta = NULL, *frameRecebido = NULL;
	uint8_t *frameRespostaVetorizado = NULL;
	size_t tamanhoRespostaVetorizada = 0;

	if(!interface) {
		//UART

		frameRecebido =  ObtemFrameDoVetor(dados, q, ikm);

		if(frameRecebido)
		{
			switch (frameRecebido->op)
			{
				case MENSAGEM_INICIAL_GSM_80: //Resposta do Antares
				{
					break;
				}

				case ENVIA_IDENTIFICACAO_8D: // Programador deseja se conectar
				{
					uint8_t auxEnd[3] = {0};

					uint8_t versaoSW[4] = {'T', '3', '0', '1'};
					uint8_t codigoControlador[6] = {'0', '0', '0', '0', '0', '0' + DP40_CODIGO};
					uint8_t descricao[128] = {0};
					const char *minhaDescricao = "Controlador DP40A - FW Seguro";
					memcpy(descricao, minhaDescricao, strlen(minhaDescricao) + 1);
					uint8_t versaoTabela[2] = {0x30 | 0x80, 0x01 | 0x80};
					uint8_t versaoProtocolo[2] = {0x30 | 0x80, 0x02 | 0x80};
					size_t tamDados = sizeof(versaoSW) + sizeof(codigoControlador) + sizeof(descricao) + sizeof(versaoTabela) + sizeof(versaoProtocolo);
					uint8_t *dados = malloc(tamDados);
					memcpy(&dados[0], versaoSW, 4);
					memcpy(&dados[0+4], codigoControlador, 6);
					memcpy(&dados[0+4+6], descricao, 128);
					memcpy(&dados[0+4+6+128], versaoTabela, 2);
					memcpy(&dados[0+4+6+128+2], versaoProtocolo, 2);


					VetorizaCodigoDoControlador(DP40_CODIGO, auxEnd);

					frameResposta = ConstroiFrameQNS(auxEnd, ENVIA_IDENTIFICACAO_8D, dados, tamDados ); //Constroi Quadro Nao Seguro

					frameRespostaVetorizado = VetorizaQuadro(frameResposta, &tamanhoRespostaVetorizada);
					UART_WriteBlocking(UART0, frameRespostaVetorizado, tamanhoRespostaVetorizada);

					//Destroi chaves antigas e cria novas
					secure_zero(segredo, ECDH_SEC_LEN);
					if (chaveLocalPrivada != NULL)
					{
						Destroy_PrvKey(chaveLocalPrivada);
						chaveLocalPrivada = NULL;
					}
					chaveLocalPrivada = New_PrvKey();

					if (chaveLocalPublica != NULL)
					{
						Destroy_PubKey(chaveLocalPublica);
						chaveLocalPublica = NULL;
					}
					chaveLocalPublica = New_PubKey();

					// Cria novas chaves Locais Publicas e Privadas
					if(keygen_ec(BR_EC_secp256r1, chaveLocalPrivada, chaveLocalPublica) != 1)
					{
						//Houve problema na geracao, apaga as chaves
						if (chaveLocalPublica != NULL)
						{
							Destroy_PubKey(chaveLocalPublica);
							chaveLocalPublica = NULL;
						}
						if (chaveLocalPrivada != NULL)
						{
							Destroy_PrvKey(chaveLocalPrivada);
							chaveLocalPrivada = NULL;
						}
					}
					contadorMensagens = 0;


					break;
				}

				case TROCA_DADOS_SEGUROS_B5: //Troca de dados segura
				{
					uint8_t auxEnd[3] = {0};


					contadorMensagens = frameRecebido->iterador > contadorMensagens ? frameRecebido->iterador : contadorMensagens;
					VetorizaCodigoDoControlador(DP40_CODIGO, auxEnd);

					if(frameRecebido->dados[0] == SOLICITA_DATA_E_HORA_86)
					{
						uint8_t dados[8] = {frameRecebido->dados[0], 0x81, 0x8F, 0xBB, 0x80, 0x81, 0x81, 0x87};
						frameResposta = ConstroiFrameQS(auxEnd, dados, sizeof(dados), &contadorMensagens, ikm);
						frameRespostaVetorizado = VetorizaQuadro(frameResposta, &tamanhoRespostaVetorizada);
						UART_WriteBlocking(UART0, frameRespostaVetorizado, tamanhoRespostaVetorizada);
					}
					else
					{
						asm("NOP");
						asm("NOP");
					}
					break;
				}
				case SOLICITA_DATA_E_HORA_86:
				{
					uint8_t auxEnd[3] = {0};
					VetorizaCodigoDoControlador(DP40_CODIGO, auxEnd);
					uint8_t dados[7] = {0x81, 0x8F, 0xBB, 0x80, 0x81, 0x81, 0x87};
					frameResposta = ConstroiFrameQNS(auxEnd, SOLICITA_DATA_E_HORA_86, dados, sizeof(dados));
					frameRespostaVetorizado = VetorizaQuadro(frameResposta, &tamanhoRespostaVetorizada);
					UART_WriteBlocking(UART0, frameRespostaVetorizado, tamanhoRespostaVetorizada);

					break;
				}

				case TROCA_CHAVES_PUBLICA_B6: //Troca de chaves ECDH
				{
					uint8_t auxEnd[3] = {0};
					VetorizaCodigoDoControlador(DP40_CODIGO, auxEnd);


					uint8_t *dadosDecodificadosB64= malloc(base64DecodedLength(frameRecebido->dados, frameRecebido->tamanho));
					if(dadosDecodificadosB64 == NULL)
						break;
					size_t tamPacoteDecodifcado64 = base64Decode(dadosDecodificadosB64, frameRecebido->dados, frameRecebido->tamanho);

					if(tamPacoteDecodifcado64 != 65)
					{
						destroy_obj((void **)&dadosDecodificadosB64, tamPacoteDecodifcado64);
						break;
					}

					if(chaveRemotaPublica != NULL)
					{
						Destroy_PubKey(chaveRemotaPublica);
						chaveRemotaPublica = NULL;
					}

					chaveRemotaPublica = New_PubKey(); //Aloca Espaco para nova chave publica remota
					if(chaveRemotaPublica == NULL)
						break;


					memcpy(chaveRemotaPublica->pbBuf, dadosDecodificadosB64, tamPacoteDecodifcado64); //Copia para o buffer da chave
					chaveRemotaPublica->pbKey.curve = BR_EC_secp256r1;
					chaveRemotaPublica->pbKey.qlen = sizeof(chaveRemotaPublica->pbBuf);

					destroy_obj((void **)&dadosDecodificadosB64, tamPacoteDecodifcado64);	 //Libera memoria da chave decodificada

					if(chaveLocalPrivada != NULL && chaveLocalPublica != NULL) // As chaves já existem
					{
						/* == Enviando a Chave Publica  == */

						//Codifica a chave Publica em B64
						size_t tamChavePublicaB64 = base64EncodedLength(chaveLocalPublica->pbKey.qlen);
						uint8_t *chavePublicaB64 = malloc(tamChavePublicaB64);
						if(chavePublicaB64 == NULL) //Nao conseguiu alocar
							break;

						secure_zero(chavePublicaB64, tamChavePublicaB64);
						base64Encode(chavePublicaB64, chaveLocalPublica->pbKey.q, chaveLocalPublica->pbKey.qlen);


						DP_Frame_t *frameResposta = ConstroiFrameQNS(auxEnd, TROCA_CHAVES_PUBLICA_B6, chavePublicaB64, tamChavePublicaB64 ); //Constroi Quadro Nao Seguro
						destroy_obj((void **)&chavePublicaB64, tamChavePublicaB64); //Libera a memoria do objeto alocado para chaveB64


						frameRespostaVetorizado = VetorizaQuadro(frameResposta, &tamanhoRespostaVetorizada);
						if(!tamanhoRespostaVetorizada)
							break;

						if(!ComputeSharedSecret(&chaveLocalPrivada->pvKey, &chaveRemotaPublica->pbKey, segredo)) //Gera o Segredo Compartilhado
						{
							// Produz IKM
							if(!psk_ikm_get(1, (char *)segredo, (char *)ikm)) //Gera IKM com segredo e código do controlador #1
								while(true);
						}

						UART_WriteBlocking(UART0, frameRespostaVetorizado, tamanhoRespostaVetorizada);
					}
					break;
				}
			}
		}

		DestrutorFrames(&frameResposta);
		DestrutorFrames(&frameRecebido);
		if(frameRespostaVetorizado != NULL)
		{
			destroy_obj((void **)&frameRespostaVetorizado, tamanhoRespostaVetorizada);
		}
	}
	else {
		//ETH

	}

}

/* TODO: insert other definitions and declarations here. */

/*
 * @brief   Application entry point.
 */
int main(void) {

	size_t tamPacoteRx = 0;
	uint8_t *pacoteRecebido;

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

	while(true)
	{

		/*TODO Verifica se chave foi pressionada (envia Mensagem Inicial Antanres*/
		if(le_chave())
		{
			asm("NOP");
		}

		/*TODO Verifca quadro recebido pela ETH*/
		if(dados_prontos_eth())
		{
			tamPacoteRx = le_dados_eth(&pacoteRecebido);
			processa_dados_rx(1, pacoteRecebido, tamPacoteRx, ikm);
		}

		/*Verifica se  houve quadro recebido na UART*/
		if(dados_prontos_uart())
		{
			tamPacoteRx = le_dados_uart(&pacoteRecebido);
			processa_dados_rx(0, pacoteRecebido, tamPacoteRx, ikm);
		}

	}

/*=========================================================================================*/
/*=========================================================================================*/
/*=========================================================================================*/

    return 0 ;
}

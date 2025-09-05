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

#define DP40_CODIGO				1


#define UART_RX_BUFFER_LEN		512

uint8_t plain[] = "DESAFIO, DP40 - Agora o desafio eh muito maior!!!";





uint8_t rxSerialBuffer[UART_RX_BUFFER_LEN] = {0}; //Buffer utilizado na porta serial
size_t qtDadosDisponiveis = 0; //Diz quantos bytes estão disponíveis para serem lidos quando um pacote é fechado
uint64_t contadorMensagens = 0; //Conta o fluxo de mensagens. Importante para geração da IVs.


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

	if(!interface) {
		//UART

		DP_Frame_t *frame =  ObtemFrameDoVetor(dados, q, ikm);

		switch (frame->op)
		{
			case MENSAGEM_INICIAL_GSM_80: //Resposta do Antares
			{
				break;
			}

			case ENVIA_IDENTIFICACAO_8D: // Programador deseja se conectar
			{
				uint8_t versaoSW[4] = {'T', '3', '0', '1'};
				uint8_t codigoControlador[6] = {'0', '0', '0', '0', '0', '1'};
				uint8_t descricao[128] = {0};
				uint8_t versaoTabela[2] = {0x30 | 0x80, 0x00 | 0x80};
				uint8_t versaoProtocolo[2] = {0x30 | 0x80, 0x00 | 0x80};
				size_t tamDados = sizeof(versaoSW) + sizeof(codigoControlador) + sizeof(descricao) + sizeof(versaoTabela) + sizeof(versaoProtocolo);
				uint8_t *dados = malloc(tamDados);
				memcpy(&dados[0], versaoSW, 4);
				memcpy(&dados[0+4], codigoControlador, 6);
				memcpy(&dados[0+4+6], descricao, 128);
				memcpy(&dados[0+4+6+128], versaoTabela, 2);
				memcpy(&dados[0+4+6+128+2], versaoProtocolo, 2);

				uint8_t auxEnd[3] = {0};
				VetorizaCodigoDoControlador(DP40_CODIGO, auxEnd);

				DP_Frame_t *frame = ConstroiFrameQNS(auxEnd, ENVIA_IDENTIFICACAO_8D, dados, tamDados ); //Constroi Quadro Nao Seguro


				break;
			}

			case TROCA_DADOS_SEGUROS_B5: //Troca de dados segura
			{
				break;
			}

			case TROCA_CHAVES_PUBLICA_B6: //Troca de chaves ECDH
			{
				break;
			}

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

	size_t tamPacoteRx = 0, tamPacoteTx = 0;
	size_t tamPacoteDecodifcado = 0;

	uint8_t *pacoteRecebido;
	uint8_t *pacoteEnviado;
	uint8_t *pacoteDecodificado;

	PrvKey *chaveLocalPrivada = NULL;
	PubKey *chaveLocalPublica = NULL, *chaveRemotaPublica = NULL;


	uint8_t segredo1[ECDH_SEC_LEN] = {0}; //Gera Segredos para comparacao
	uint8_t ikm[ECDH_SEC_LEN] = {0};

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

//	uint8_t plain[] = "DESAFIO, DP40 - Agora o desafio eh muito maior!!!";
//	size_t tamPacoteRx = 0, tamPacoteTx = 0;
//	size_t tamPacoteDecodifcado = 0;

//	uint8_t end[3] = {0x00, 0x01, 0x02};
//	uint8_t op = 0x8D;
//	uint8_t dados[] = {0x81, 0x82, 0x83, 0x84, 0x85, 0x86};
//	uint64_t meuIterador = 128000;


//	DP_Frame_t *frame = ConstroiFrameQNS(end, op, dados, sizeof(dados) ); //Constroi Quadro Nao Seguro
//	size_t tamVetor = 0;
//	uint8_t *quadroVetorizado = VetorizaQuadro(frame, &tamVetor);
//	DP_Frame_t *frameRecuperado = ObtemFrameDoVetor(quadroVetorizado, tamVetor, NULL);
//
//	DestrutorFrames(&frame);
//	DestrutorFrames(&frameRecuperado);
//	free(quadroVetorizado);
//
//	secure_zero(segredo1, sizeof(segredo1)); //Limpa segredo1
//	segredo1[0] = 0x04;
//	uint8_t meuIkm[ECDH_SEC_LEN];
//	if(!psk_ikm_get(end[2], (char *)segredo1, (char *) meuIkm)) //Gera IKM com segredo e código do controlador #end[2]
//		while(true);
//	frame = ConstroiFrameQS(end, dados, sizeof(dados), &meuIterador, meuIkm);
//	asm("NOP");
//	quadroVetorizado = VetorizaQuadro(frame, &tamVetor);
//	asm("NOP");
//	frameRecuperado = ObtemFrameDoVetor(quadroVetorizado, tamVetor, meuIkm);
//	DestrutorFrames(&frame);
//	DestrutorFrames(&frameRecuperado);
//	free(quadroVetorizado);

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








//Comentei tudo 1X DENTRO DOS VOLTA

//VOLTA:
//
//	if (chaveLocalPrivada != NULL)		Destroy_PrvKey(chaveLocalPrivada);
//	chaveLocalPrivada = New_PrvKey();
//
//	if (chaveLocalPublica != NULL)		Destroy_PubKey(chaveLocalPublica);
//	chaveLocalPublica = New_PubKey();
//
//	if (chaveRemotaPublica != NULL)		Destroy_PubKey(chaveRemotaPublica);
//	chaveRemotaPublica = New_PubKey();
//
//    memset(segredo1, 0, sizeof(segredo1));
//
//
//
//    //Aloca buffers para chaves remotas e Locais
//	chaveRemotaPublica->pbKey.curve = BR_EC_secp256r1;
//	chaveRemotaPublica->pbKey.qlen = sizeof(chaveRemotaPublica->pbBuf);
//
//	while(!dados_prontos_uart()); //Espera para receber a chave pública
//
////	uint8_t *pacoteRecebido;
////	uint8_t *pacoteEnviado;
////	uint8_t *pacoteDecodificado;
//
//	tamPacoteRx = le_dados_uart(&pacoteRecebido);
//	if(!tamPacoteRx)
//		while(true); //Erro
//
//	pacoteDecodificado = RetiraDadosDeQuadroRecebido(&pacoteRecebido[1], tamPacoteRx-2, 0, NULL, NULL, &tamPacoteDecodifcado);
//	if(tamPacoteDecodifcado != 65)
//		while(true); //Erro (não bate com tamanho da chave pública)
//
//	memcpy(chaveRemotaPublica->pbBuf, pacoteDecodificado, tamPacoteDecodifcado);
//	secure_zero(pacoteDecodificado, tamPacoteDecodifcado);
//	free(pacoteDecodificado); //Libera a memória alocada para o pacote
//
//	if(keygen_ec(BR_EC_secp256r1, chaveLocalPrivada, chaveLocalPublica) == 1) // Gera chaves Locais (Publica e Privada
//	{
//    	/* == Enviando a Chave Publica  == */
//
//    	// Empacota Chave publica sem Criptografia
//    	pacoteEnviado = GeraQuadroParaEnvio(chaveLocalPublica->pbKey.q, chaveLocalPublica->pbKey.qlen, 0,NULL ,NULL, &tamPacoteTx);
//    	if(pacoteEnviado == NULL || tamPacoteTx == 0)	while(true);
//
//    	// Envia a Chave publica
//    	UART_WriteBlocking(UART0, pacoteEnviado, tamPacoteTx);
//
//    	// Desaloca o Pacote
//    	secure_zero(pacoteEnviado, tamPacoteTx);
//    	free(pacoteEnviado);
//
//    	if(!ComputeSharedSecret(&chaveLocalPrivada->pvKey, &chaveRemotaPublica->pbKey, segredo1)) //Gera o Segredo Compartilhado
//    	{
//
//
//    		/* == Enviando o Desafio == */
//
//    		// Produz IKM
////			uint8_t ikm[ECDH_SEC_LEN];
//			if(!psk_ikm_get(1, (char *)segredo1, (char *)ikm)) //Gera IKM com segredo e código do controlador #1
//				while(true);
//
//			//Cria um quadro criptografado contendo o Desafio
//			pacoteEnviado = GeraQuadroParaEnvio(plain, sizeof(plain), 1, ikm, &contadorMensagens, &tamPacoteTx);
//
//			/*Envia o Desafio*/
//			UART_WriteBlocking(UART0, pacoteEnviado, tamPacoteTx);
//			contadorMensagens++;
//
//			/*Desaloca pacote*/
//			secure_zero(pacoteEnviado, tamPacoteTx);
//			free(pacoteEnviado);
//
//			/* == Recebe a Solução == */
//
//			//Espera para receber a Solucao do Desafio
//			while(!dados_prontos_uart()); //Espera para receber a Solucao do Desafio
//			tamPacoteRx = le_dados_uart(&pacoteRecebido);
//			if(!tamPacoteRx)	while(true); //Erro
//
//			//Decifra pacote da solucão
//			pacoteDecodificado = RetiraDadosDeQuadroRecebido(&pacoteRecebido[1], tamPacoteRx-2, 1, ikm, &contadorMensagens, &tamPacoteDecodifcado);
//			if(!tamPacoteDecodifcado)	while(true); //Erro Decodificacao
//
//			/*Desaloca solucao*/
//			secure_zero(pacoteDecodificado, tamPacoteDecodifcado);
//			free(pacoteDecodificado);
//    	}
//    	else
//    	{
//    		while(true);
//    	}
//	}
//
//    goto VOLTA;

/*=========================================================================================*/
/*=========================================================================================*/
/*=========================================================================================*/

    return 0 ;
}

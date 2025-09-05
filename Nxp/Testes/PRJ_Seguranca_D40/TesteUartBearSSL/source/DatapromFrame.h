/*
 * DatapromFrame.h
 *
 *  Created on: 1 de set de 2025
 *      Author: carlos.oliveira
 */

#ifndef DATAPROMFRAME_H_
#define DATAPROMFRAME_H_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include "Utilidades.h"


#define ECDH_SEC_LEN 		65  //Tamanho do Secredo ECDH formato não compactado
#define HKDF_INFO_SALT_LEN	16 // Tamanho dos vetores INFO e SALT

// Tamanhos padrão para GCM
#define GCM_IV_LEN   		12
#define GCM_TAG_LEN  		16
#define AES_KEY_LEN  		16  // AES-128

#define MSG_BY_SESSION		4 //Diz a quantidade de mensagens até a sessão seja trocada

#define END_OFFSET				1
#define OP_OFFSET				4
#define RES_OFFSET				5
#define B64_ITERADOR_OFFSET		6
#define B64_TAG_OFFSET			18
#define B64_DATA_OFFSET			42

#define LEN_MINIMAL_QNS			7	//Tamanho do quadro nao seguro minimo (sem dados)
#define LEN_MINIMAL_QS			48  //Tamanho do quadro nao seguro minimo (sem dados, somente opcode cripto)



typedef struct {
	//Campos comuns
    uint8_t endereco[3]; // Endereco Controlador (AREA/CONTROLADOR/SUBCONTROLADOR)
    uint8_t op; //OPCODE NAO SEGURO

    // Utilizado apenas em Quadros Seguros (QS)
    uint8_t res; // Reservado
    uint64_t iterador; // Cotador de pacotes
    uint8_t tag[GCM_TAG_LEN]; // Tag GCM

    // Utilizado apenas em Quadros Nao Seguros (QNS)
    uint8_t *dados;   // Dados (criptografados ou não)
    uint16_t tamanho; // Tamanho dos dados em bytes

} DP_Frame_t;

uint8_t GeraChecksum(uint8_t *in, uint16_t q);
uint8_t CalculaChecksum(DP_Frame_t *frame);
void LimpaMSB(uint8_t *vet, size_t q);
void SetaMSB(uint8_t *vet, size_t q);
uint8_t ObtemCodigoControladorDoVetor(uint8_t *end);
void *VetorizaCodigoDoControlador(uint8_t cod, uint8_t *out);

DP_Frame_t *ObtemFrameDoVetor(uint8_t *vet, size_t q, uint8_t *ikm);

DP_Frame_t *ConstroiFrameQNS(uint8_t *end, uint8_t op, uint8_t *dados, size_t q ); //Constroi Quadro Nao Seguro
DP_Frame_t *ConstroiFrameQS(uint8_t *end, uint8_t *dados, size_t q, uint64_t *iterador, uint8_t *ikm); // Constroi Quadro Seguro
void DestrutorFrames(DP_Frame_t **frame);
uint8_t *VetorizaQuadro(DP_Frame_t *frame, size_t *qOut);


uint8_t *RetiraDadosDeQuadroRecebido(uint8_t *dadosRx, size_t lenRx, uint8_t cripto, uint8_t *ikm, uint64_t *contador, size_t *lenOut);
uint8_t *GeraQuadroParaEnvio(uint8_t *dados, size_t lenIn, uint8_t cripto, uint8_t *ikm ,uint64_t *contador, size_t *lenOut);


uint8_t *CriaQuadroCodificado(uint8_t *dados, size_t q, size_t *qOutput);
uint8_t *ObtemDadosQuadroCodificado(uint8_t *input, size_t q, size_t *qOutput);

uint8_t* CodificaDados7bits(const uint8_t *input, size_t len_input, size_t *len_out);
uint8_t* DecodificaDados7bits(const uint8_t* input, size_t len_input, size_t* len_out);




#endif /* DATAPROMFRAME_H_ */

/*
 * DatapromFrame.c
 *
 *  Created on: 1 de set de 2025
 *      Author: carlos.oliveira
 */


#include "DatapromFrame.h"
#include "OpcodesDP.h"
#include "bearssl.h"
#include "PSKLib.h"
#include "base64.h"

#include "FreeRTOS.h"



uint8_t GeraChecksum(uint8_t *in, uint16_t q)
/*Gera o checksum de um vetor segundo calculo do protocolo
 * CKS = ~XOR(IN[0..(q-1)]) | 0x80
 * */
{
	uint16_t i;
    uint8_t checksum;

    for(checksum = 0, i=0 ; i<q ; i++)
        checksum ^= in[i];

    return (~checksum) | 0x80;
}

uint8_t CalculaChecksum(DP_Frame_t *frame)
{
	uint8_t ret = 0;
	return ret;
}

void LimpaMSB(uint8_t *vet, size_t q)
{
	if(vet != NULL)
	{
		for(size_t i = 0; i < q; i++)
			vet[i] &= 0x7F;
	}
}

void SetaMSB(uint8_t *vet, size_t q)
{
	if(vet != NULL)
	{
		for(size_t i = 0; i < q; i++)
			vet[i] |= 0x80;
	}
}

bool ObtemIdControladorDoVetor(uint8_t *end,  uint8_t *out)
/* Pega um vetor no formato padrão do protocolo e devolve a
 * ID do controlador (em out - um vetor de 3 posições) no seguinte formato:
 * [Area(LSB)][Rede][Codigo(MSB)]
 * Return: True(1) = convertido, False(0) = Nao conseguiu converter*/
{
	bool ret = false;
	if(out != NULL && end != NULL)
	{
		uint8_t copy_end[3] = {end[0], end[1], end[2]};
		copy_end[0] &= 0x7F; //Limpa primeiro bit
		copy_end[1] &= 0x7F;
		copy_end[2] &= 0x7F;

		out[0] = out[1] = out[2] = 0;
		//Pega o codigo do controlador
		out[2] |= (copy_end[1] & 0x03) << 4;
		out[2] |= (copy_end[2] & 0x78) >> 3 ;

		//Pega a rede
		out[1] |= (copy_end[0] & 0x01) << 5;
		out[1] |= (copy_end[1] & 0x7C) >> 2;

		//Pega a area
		out[0] |= (copy_end[0] & 0x7E) >>  1;

		ret = true;
	}


	return ret;
}

bool VetorizaIdDoControlador(uint8_t cod, uint8_t rede, uint8_t area, uint8_t *out)
/* Pega os valores de identificação do controlador no formato binário
 * e devolve ID do controlador (em out - vetor 3 posições) no formato
 * do vetor do protocolo (mas sem o 1o. bit setado)
 *Return:  Return: True(1) = convertido, False(0) = Nao conseguiu converter
 **/
{
	bool ret = false;
	//Salva Codigo no vetor
	if(out != NULL)
	{
		out[0] = out[1] = out[2] = 0;

		//Salva Código no vetor
		out[1] = (cod>>4) & 0x03;
		out[2] = (cod<<3) & 0x78;

		//Salva Rede no vetor
		out[0] |= ((rede >> 5) & 0x01);
		out[1] |= ((rede << 2) & 0x7C);

		//Salva Area no vetor
		out[0] |= (area << 1) & 0x7E;

		out[0] |= 0x80;
		out[1] |= 0x80;
		out[2] |= 0x80;

		ret = true;
	}
	return ret;
}


DP_Frame_t *ObtemFrameDoVetor(uint8_t *vet, size_t q, uint8_t *ikm, uint8_t salt_id)
/* @brief: Utilizar essa função para obter um quandro(estrutura) a partir de um vetor de entrada
 * @param vet (in): Vetor de entrada
 * @param q (in): tamanho do vetor de entrada
 * @param ikm (in): parametro opcional, é o IKM utilizado para descriptografar os daods
 * @param sald_id (in): Diz se o salt deve ser concatenado com a ID
* Return: Retorna um Frame que pode ser ou não seguro
* */
{
	DP_Frame_t *ret = NULL;
	//Verifica Compatibilidade/Integridade do vetor
	if((vet[0] == STX) && (vet[q-1] == ETX) && (vet[q-2] == GeraChecksum(&vet[1],q-3)))	{
		ret = pvPortMalloc(sizeof(DP_Frame_t));
//		ret = malloc(sizeof(DP_Frame_t));
		if(ret == NULL)
			return ret;
		secure_zero(ret, sizeof(DP_Frame_t));

		LimpaMSB(&vet[END_OFFSET],3);
		ret->endereco[0] = vet[END_OFFSET+0];
		ret->endereco[1] = vet[END_OFFSET+1];
		ret->endereco[2] = vet[END_OFFSET+2];

		ret->op = vet[OP_OFFSET];

		if(ret->op == TROCA_DADOS_SEGUROS_B5) {
			// Quadro seguro (possui campos adicionais)
			if(q > LEN_MINIMAL_QS && ikm != NULL) {
				//Entao existe dados (obrigatorio)
				size_t tamDadosB64 = (q-2) - B64_DATA_OFFSET;

				//Necessario obter o tamanho dos dados depois de decodificados do B64
				LimpaMSB(&vet[B64_DATA_OFFSET], tamDadosB64);
				ret->tamanho =  base64DecodedLength(&vet[ B64_DATA_OFFSET], tamDadosB64);

				ret->dados = pvPortMalloc(ret->tamanho);
//				ret->dados = malloc(ret->tamanho);
				if(ret->dados == NULL)
				{
					secure_zero(ret, sizeof(DP_Frame_t));
					vPortFree(ret);
//					free(ret);
					ret = NULL;
				}

				//Decodifica Dados mas mantem criptografados
				base64Decode(ret->dados, &vet[B64_DATA_OFFSET], tamDadosB64);

				// Decodfica iterador
				LimpaMSB(&vet[B64_ITERADOR_OFFSET], 12);
				base64Decode((uint8_t *)(&ret->iterador), &vet[B64_ITERADOR_OFFSET], 12);

				// Decodifica TAG
				LimpaMSB(&vet[B64_TAG_OFFSET], 24);
				base64Decode(ret->tag, &vet[B64_TAG_OFFSET], 24);

				// Descriptografa Dados
				char salt_hkdf[HKDF_SALT_LEN] 	= {'D','A','T','A','S','A','L','T','\0','\0','\0'};
				char infoKey[HKDF_INFO_LEN] 		= {'D','A','T','A','I','N','F','O','\0','\0','\0','\0','\0','\0','\0','\0'};
				char infoIV[HKDF_INFO_LEN] 		= {'D','A','T','A','I','N','F','O','\0','\0','\0','\0','\0','\0','\0','\0'};
				uint8_t iv[GCM_IV_LEN] = {0}; // IV GCM 12 bytes
				uint8_t key[AES_KEY_LEN] = {0}; //Chave GCM
				uint64_t sessao = 0;

				if(salt_id) {
					// Se necessario atualiza o Salt com o endereço
					uint8_t auxSalt[3] = {0};
					ObtemIdControladorDoVetor(&ret->endereco[0], auxSalt);
					memcpy(&salt_hkdf[8], auxSalt, 3);
				}

				//Atualiza info e salt com iterador
				sessao = (uint64_t)(ret->iterador / MSG_BY_SESSION);
				memcpy(&infoKey[sizeof(infoKey) - sizeof(uint64_t)], &sessao, sizeof(uint64_t)); // Atualiza INFO a cada mensagem enviada
				memcpy(&infoIV[sizeof(infoIV) - sizeof(uint64_t)], &(ret->iterador), sizeof(uint64_t)); // Atualiza INFO a cada mensagem enviada

				//memcpy(&salt_hkdf[sizeof(salt_hkdf) - sizeof(uint64_t)], &sessao, sizeof(uint64_t)); // Atualiza SALT


				// HKDF  - Produz Key AES256-GCM
				br_hkdf_context hkdfKey;
				br_hkdf_init(&hkdfKey, &br_sha256_vtable, salt_hkdf, sizeof(salt_hkdf));
//				br_hkdf_init(&hkdfKey, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
				br_hkdf_inject(&hkdfKey, ikm, ECDH_SEC_LEN);
				br_hkdf_flip(&hkdfKey);
				br_hkdf_produce(&hkdfKey, infoKey, sizeof(infoKey), key, AES_KEY_LEN);	 //Produz Chave


				// HKDF - Produz IV GCM
				br_hkdf_context hkdfIV;
				br_hkdf_init(&hkdfIV, &br_sha256_vtable, salt_hkdf, sizeof(salt_hkdf));
//				br_hkdf_init(&hkdfIV, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
				br_hkdf_inject(&hkdfIV, ikm, ECDH_SEC_LEN);
				br_hkdf_flip(&hkdfIV);
				br_hkdf_produce(&hkdfIV, infoIV, sizeof(infoIV), iv, sizeof(iv));


				//Inicializa contexto AES-GCM
				br_gcm_context gcm;
				br_aes_ct64_ctr_keys aes_ctx;
				br_aes_ct64_ctr_init(&aes_ctx, key, AES_KEY_LEN);
				br_gcm_init(&gcm, &aes_ctx.vtable, &br_ghash_ctmul64);


				// Processo GCM
				br_gcm_reset(&gcm, iv, GCM_IV_LEN);
				br_gcm_aad_inject(&gcm, NULL, 0); // sem AAD
				br_gcm_run(&gcm, 0, ret->dados, ret->tamanho); // decifra in-place
				br_gcm_flip(&gcm);

				// Verifica TAG
				uint8_t tag_calculada[GCM_TAG_LEN];
				br_gcm_get_tag(&gcm, tag_calculada);

				if (memcmp(ret->tag, tag_calculada, GCM_TAG_LEN) != 0) {
					// TAG inválida → dados corrompidos ou adulterados
					secure_zero(ret->dados, ret->tamanho);
					secure_zero(ret, sizeof(DP_Frame_t));
					vPortFree(ret->dados);
					vPortFree(ret);
//					free(ret->dados);
//					free(ret);
					ret = NULL;
				}

				// Retira RES
				LimpaMSB(&vet[RES_OFFSET], 1);
				ret->res = vet[RES_OFFSET];

			}
			else {
				// vetor com prolemas ou IKM nao foi passada
				secure_zero(ret, sizeof(DP_Frame_t));
				vPortFree(ret);
//				free(ret);
				ret = NULL;
			}
		}
		else if(ret->op >= MENSAGEM_INICIAL_GSM_80 && ret->op <= TROCA_CHAVES_PUBLICA_B6) {
			// Quadro nao seguro
			if(q > LEN_MINIMAL_QNS) {
				//Entao existe dados
				ret->tamanho = q - LEN_MINIMAL_QNS;
				ret->dados = pvPortMalloc(ret->tamanho);
//				ret->dados = malloc(ret->tamanho);
				if(ret->dados == NULL)
				{
					secure_zero(ret, sizeof(DP_Frame_t));
					vPortFree(ret);
//					free(ret);
					ret = NULL;
				}
				for(size_t i = 0; i < ret->tamanho; i++)
					ret->dados[i] = vet[OP_OFFSET+1 + i];
				LimpaMSB(ret->dados, ret->tamanho); // Limpa o primeiro bit antes de armazenar no quadro
			}
		}
	}
	return ret;
}

DP_Frame_t *ConstroiFrameQNS(uint8_t *end, uint8_t op, uint8_t *dados, size_t q )
/* Cria um quadro nao seguro.
 * Esse quadro eh alocado dinamicamente e precisa ser desalocado manualmente apos o uso.
 *
 * @param end(in): Endereço contendo AREA | CODIGO CONTROLADOR | CODIGO SUBCONTROLADOR
 * @param op(in): Opcode do quadro de comunicacao
 * @param dados(in): Dados que serão protegidos (Criptografados)
 * @param q(in): tamanho do vetor de dados
 * return: Estrutura de Quadro Nao Seguro preenchida
 * */
{
	DP_Frame_t *ret = NULL;
	if(end != NULL && (op >= MENSAGEM_INICIAL_GSM_80 && op <= TROCA_CHAVES_PUBLICA_B6) && op != TROCA_DADOS_SEGUROS_B5)
	// Verifica se endereco e opcode sao validos
	{
		if(q == 0 || (q && dados != NULL))
		//Verifica o restante dos parametros
		{
			ret = pvPortMalloc(sizeof(DP_Frame_t));
//			ret = malloc(sizeof(DP_Frame_t));
			if(!ret)
				return ret;
			secure_zero(ret,sizeof(DP_Frame_t));

			ret->endereco[0] = end[0];
			ret->endereco[1] = end[1];
			ret->endereco[2] = end[2];
			ret->op = op;
			ret->tamanho = q;
			ret->res = 0;
			if(ret->tamanho)
			{
				ret->dados = pvPortMalloc(ret->tamanho); //Aloca Espaco para dados Criptografados
//				ret->dados = malloc(ret->tamanho); //Aloca Espaco para dados Criptografados
				if(ret->dados == NULL)
				{
					secure_zero(ret, sizeof(DP_Frame_t));
					vPortFree(ret);
					ret = NULL;
//					free(ret);
					return ret;
				}
				memcpy(ret->dados, dados, ret->tamanho); //Copia os dados para dentro do quadro
			}
			else
			{
				ret->dados = NULL;
			}
		}
	}
	return ret;
}

DP_Frame_t *ConstroiFrameQS(uint8_t *end, uint8_t *dados, size_t q, uint64_t *iterador, uint8_t *ikm, uint8_t salt_id)
/* Cria um quadro seguro. Pré-requisito, a IKM já deve ter sido gerada.
 * Esse quadro eh alocado dinamicamente e precisa ser desalocado manualmente apos o uso.
 *
 * @param end(in): Endereço contendo AREA | CODIGO CONTROLADOR | CODIGO SUBCONTROLADOR
 * @param dados(in): Dados que serão protegidos (Criptografados)
 * @param q(in): tamanho do vetor de dados
 * @param iterador(in): Contador de mensagens (utilizado para criptografar os dados).O iterador e
 * incrementado após a geracao do quadro.
 * @param ikm (in): IKM utilzada para gerar CHAVES, IV e TAG da criptografia
 * @param sald_id (in): Diz se o salt deve ser concatenado com a ID
 * return: Estrutura de Quadro Seguro preenchida
 * */
{
	DP_Frame_t *ret = NULL;

	if(end != NULL && q && dados != NULL && ikm != NULL && iterador != NULL)
	//Verifica se existe dados para serem criptografados
	{
		char salt_hkdf[HKDF_SALT_LEN] 	= {'D','A','T','A','S','A','L','T','\0','\0', '\0'};
		char infoKey[HKDF_INFO_LEN] 	= {'D','A','T','A','I','N','F','O','\0','\0','\0','\0','\0','\0','\0','\0'};
		char infoIV[HKDF_INFO_LEN] 		= {'D','A','T','A','I','N','F','O','\0','\0','\0','\0','\0','\0','\0','\0'};
		uint8_t iv[GCM_IV_LEN] = {0}; // IV GCM 12 bytes
		uint8_t key[AES_KEY_LEN] = {0}; //Chave GCM
		uint64_t sessao = 0;

		if(salt_id) {
			// Se necessario atualiza o Salt com o endereço
			uint8_t auxSalt[3] = {0};
			ObtemIdControladorDoVetor(end, auxSalt);
			memcpy(&salt_hkdf[8], auxSalt, 3);
		}


		// Aloca memoria para o quadro
		ret = pvPortMalloc(sizeof(DP_Frame_t));
//		ret = malloc(sizeof(DP_Frame_t));
		if(!ret)
			return ret;

		ret->dados = pvPortMalloc(q); //Aloca Espaco para dados Criptografados
//		ret->dados = malloc(q); //Aloca Espaco para dados Criptografados
		if(ret->dados == NULL)
		{
			secure_zero(ret, sizeof(DP_Frame_t));
			vPortFree(ret);
//			free(ret);
			return ret;
		}

		(*iterador)++; //Incrementa contador

		//Atualiza estrutura do QS
		ret->endereco[0] = end[0];
		ret->endereco[1] = end[1];
		ret->endereco[2] = end[2];
		ret->op = TROCA_DADOS_SEGUROS_B5;
		ret->res = 0;
		ret->iterador = *iterador;
		ret->tamanho = q;
		memcpy(ret->dados, dados, ret->tamanho);



		//Atualiza info e salt com iterador
		sessao = (uint64_t)(ret->iterador / MSG_BY_SESSION);
		memcpy(&infoKey[sizeof(infoKey) - sizeof(uint64_t)], &sessao, sizeof(uint64_t)); // Atualiza INFO a cada mensagem enviada
		memcpy(&infoIV[sizeof(infoIV) - sizeof(uint64_t)], &(ret->iterador), sizeof(uint64_t)); // Atualiza INFO a cada mensagem enviada

		// Produz Key AES256-GCM
		br_hkdf_context hkdfKey;
		br_hkdf_init(&hkdfKey, &br_sha256_vtable, salt_hkdf, sizeof(salt_hkdf));
//		br_hkdf_init(&hkdfKey, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
		br_hkdf_inject(&hkdfKey, ikm, ECDH_SEC_LEN);
		br_hkdf_flip(&hkdfKey);
		br_hkdf_produce(&hkdfKey, infoKey, sizeof(infoKey), key, AES_KEY_LEN);	 //Produz Chave


		// Produz IV GCM
		br_hkdf_context hkdfIV;
		br_hkdf_init(&hkdfIV, &br_sha256_vtable, salt_hkdf, sizeof(salt_hkdf));
//		br_hkdf_init(&hkdfIV, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
		br_hkdf_inject(&hkdfIV, ikm, ECDH_SEC_LEN);
		br_hkdf_flip(&hkdfIV);
		br_hkdf_produce(&hkdfIV, infoIV, sizeof(infoIV), iv, sizeof(iv));


		//Inicializa contexto AES-GCM
		br_gcm_context gcm;
		br_aes_ct64_ctr_keys aes_ctx;
		br_aes_ct64_ctr_init(&aes_ctx, key, AES_KEY_LEN);
		br_gcm_init(&gcm, &aes_ctx.vtable, &br_ghash_ctmul64);


		//Criptografa Dados
		br_gcm_reset(&gcm, iv, GCM_IV_LEN);
		br_gcm_aad_inject(&gcm, NULL, 0); // nenhum AAD, se desejar pode adicionar depois
		br_gcm_run(&gcm, 1, ret->dados, ret->tamanho);
		br_gcm_flip(&gcm);
		br_gcm_get_tag(&gcm, ret->tag);

	}
	return ret;
}

void DestrutorFrames(DP_Frame_t **frame)
/* Destroi o quadro de comunicacao liberando as memorias alocadas
 * @param frame(in): Quadro que sera desalocado
 * */
{
	//Verifica se um frame
	if(frame != NULL && *frame != NULL)
	{
		//Se houver dados nesse quadro esses devem ser desalocados primeiro
		if ((*frame)->dados && (*frame)->tamanho != 0) {
			memset((*frame)->dados, 0, (*frame)->tamanho);
//			secure_zero((*frame)->dados, (*frame)->tamanho);
			vPortFree((*frame)->dados);
//			free((*frame)->dados);
		}
		//Limpa/libera o restante da estrutura
		memset(*frame, 0, sizeof(DP_Frame_t));
//		secure_zero(*frame, sizeof(DP_Frame_t));
		vPortFree(*frame);
//		free(*frame);
		*frame = NULL;
	}
}

uint8_t *VetorizaQuadro(DP_Frame_t *frame, size_t *qOut)
/* Usar essa funcao para produzir um vetor pronto para ser enviado.
 * A funçao aloca memoria desse buffer e precisa ser liberada após
 * seu uso.
 * @param frame(in): E a estrutura do quadro do qual sera gerado o
 * buffer.
 * @param qOut(out): Tamanho do vetor de saida gerado
 * return: Retorna NULL se falhar ou endereço do buffer alocado
 * */
{

	uint8_t *out = NULL;
	int i = 0;

	if(frame != NULL) {

		if(frame->op == TROCA_DADOS_SEGUROS_B5)	{
			//OPCODE de Quadro Seguro (QS)

			int tamDadosB64 = base64EncodedLength(frame->tamanho); //Codifica B64 os dados criptografados
			*qOut = tamDadosB64 + 44;// STX[1] | END[3] | OP[1] | RES[1] | ITERADOR[12] | TAG[24] | DADOS CRIPT B64[tamDadosB64] | CKS[1] | ETX[1]
			out = pvPortMalloc(*qOut); //Aloca Espaco para dados Criptografados
//			out = malloc(*qOut);
			if(!out){
				// Nao conseguiu alocar a memoria para o vetor
				return NULL;
			}

			out[0] = STX;
			out[END_OFFSET] 	= frame->endereco[0] 	| 0x80;
			out[END_OFFSET+1] 	= frame->endereco[1] 	| 0x80;
			out[END_OFFSET+2] 	= frame->endereco[2] 	| 0x80;
			out[OP_OFFSET] = frame->op;
			out[RES_OFFSET] = frame->res | 0x080;
			out[*qOut-1] = ETX;


			if(!base64Encode(&out[B64_ITERADOR_OFFSET], (const unsigned char *)&frame->iterador, sizeof(uint64_t))){
				// Erro ao codificar a ITERADOR em B64
				secure_zero(out, *qOut);
				vPortFree(out);
//				free(out);
			}
			for(size_t i = 0; i < 12; i++) //Seta primeiro bit
				out[6+i] |= 0x80;

			if(!base64Encode(&out[B64_TAG_OFFSET], frame->tag, GCM_TAG_LEN)){
				// Erro ao codificar a TAG em B64
				secure_zero(out, *qOut);
				vPortFree(out);
//				free(out);
			}
			for(size_t i = 0; i < 24; i++) //Seta primeiro bit
				out[B64_TAG_OFFSET+i] |= 0x80;

			if(!base64Encode(&out[B64_DATA_OFFSET], frame->dados, frame->tamanho)){
				// Erro ao codificar os dados em B64
				secure_zero(out, *qOut);
				vPortFree(out);
//				free(out);
			}
			for(size_t i = 0; i < tamDadosB64; i++) //Seta primeiro bit
				out[B64_DATA_OFFSET+i] |= 0x80;

			out[*qOut-2] = GeraChecksum(&out[END_OFFSET], *qOut-3);
		}
		else if(frame->op != 0)	{
			// OPCODEs de quadro nao seguro (QNS)
			uint8_t cks = 0;


			*qOut = frame->tamanho + 7; //STX[1] + END[3] + OP[1] + DADOS[tamanho] + CKS[1] + ETX[1]
			out = pvPortMalloc(*qOut);
//			out = malloc(*qOut);
			if(!out)
				return NULL;

			//Insere marcadores de quadro
			out[i++] = STX;
			out[*qOut-1] = ETX;

			//Insere endereco setando o primeiro bit
			out[i] = frame->endereco[0] | 0x80;
			cks ^= out[i++];
			out[i] = frame->endereco[1] | 0x80;
			cks ^= out[i++];
			out[i] = frame->endereco[2] | 0x80;
			cks ^= out[i++];

			out[i] = frame->op;
			cks ^= out[i++];

			if(frame->tamanho) {
				//Existem dados a serem transmitidos
				int j = 0; //Indice do vetor de dados
				while(j < frame->tamanho)
				{
					out[i+j] = frame->dados[j] | 0x80;
					cks ^= out[i+j];
					j++;
				}
				i += j; //Adiciona o deslocamento dos dados
			}
			out[i++] = (~cks) | 0x80;

			if(i != *qOut-1) {
				//Erro na conversao do quadro
				secure_zero(out, *qOut);
				vPortFree(out);
//				free(out);
				*qOut = 0;
				return out;
			}
		}
		else {
			//Opcode com problema
		}
	}
	return out;
}


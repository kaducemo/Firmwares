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


uint8_t ObtemCodigoControladorDoVetor(uint8_t *end)
{
	uint8_t codigo = 0;
	codigo |= (end[1] << 4) & 0x30;
	codigo |= (end[2] >> 3) & 0x0F ;
	return codigo;
}

void *VetorizaCodigoDoControlador(uint8_t cod, uint8_t *out)
{
	uint8_t codigo = 0;
	out[0] = 0;
	out[1] = (codigo>>4) & 0x03;
	out[2] = (codigo<<3) * 0x78;
}


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


DP_Frame_t *ObtemFrameDoVetor(uint8_t *vet, size_t q, uint8_t *ikm )
{
	DP_Frame_t *ret = NULL;
	//Verifica Compatibilidade/Integridade do vetor
	if((vet[0] == STX) && (vet[q-1] == ETX) && (vet[q-2] == GeraChecksum(&vet[1],q-3)))	{
		ret = malloc(sizeof(DP_Frame_t));
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

				ret->dados = malloc(ret->tamanho);
				if(ret->dados == NULL)
				{
					secure_zero(ret, sizeof(DP_Frame_t));
					free(ret);
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
				char salt_hkdf[HKDF_INFO_SALT_LEN] 	= {'D','A','T','A','S','A','L','T','\0','\0','\0','\0','\0','\0','\0','\0'};
				char info[HKDF_INFO_SALT_LEN] 		= {'D','A','T','A','I','N','F','O','\0','\0','\0','\0','\0','\0','\0','\0'};
				uint8_t iv[GCM_IV_LEN] = {0}; // IV GCM 12 bytes
				uint8_t key[AES_KEY_LEN] = {0}; //Chave GCM
				uint64_t sessao = 0;

				//Atualiza info e salt com iterador
				memcpy(&info[sizeof(info) - sizeof(uint64_t)], &(ret->iterador), sizeof(uint64_t)); // Atualiza INFO a cada mensagem enviada
				sessao = (uint64_t)(ret->iterador / MSG_BY_SESSION);
				memcpy(&salt_hkdf[sizeof(salt_hkdf) - sizeof(uint64_t)], &sessao, sizeof(uint64_t)); // Atualiza SALT


				// HKDF  - Produz Key AES256-GCM
				br_hkdf_context hkdfKey;
				br_hkdf_init(&hkdfKey, &br_sha256_vtable, salt_hkdf, strlen(salt_hkdf));
				br_hkdf_inject(&hkdfKey, ikm, ECDH_SEC_LEN);
				br_hkdf_flip(&hkdfKey);
				br_hkdf_produce(&hkdfKey, info, sizeof(info), key, AES_KEY_LEN);	 //Produz Chave


				// HKDF - Produz IV GCM
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
					free(ret->dados);
					free(ret);
					ret = NULL;
				}

				// Retira RES
				LimpaMSB(&vet[RES_OFFSET], 1);
				ret->res = vet[RES_OFFSET];

			}
			else {
				// vetor com prolemas ou IKM nao foi passada
				secure_zero(ret, sizeof(DP_Frame_t));
				free(ret);
				ret = NULL;
			}
		}
		else if(ret->op >= MENSAGEM_INICIAL_GSM_80 && ret->op <= TROCA_CHAVES_PUBLICA_B6) {
			// Quadro nao seguro
			if(q > LEN_MINIMAL_QNS) {
				//Entao existe dados
				ret->tamanho = q - LEN_MINIMAL_QNS;
				ret->dados = malloc(ret->tamanho);
				if(ret->dados == NULL)
				{
					secure_zero(ret, sizeof(DP_Frame_t));
					free(ret);
					ret = NULL;
				}
				for(size_t i = 0; i < ret->tamanho; i++)
					ret->dados[i] = vet[OP_OFFSET+1 + i];
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
			ret = malloc(sizeof(DP_Frame_t));
			if(!ret)
				return ret;
			secure_zero(ret,sizeof(DP_Frame_t));

			ret->endereco[0] = end[0];
			ret->endereco[1] = end[1];
			ret->endereco[2] = end[2];
			ret->op = op;
			ret->tamanho = q;
			if(ret->tamanho)
			{
				ret->dados = malloc(ret->tamanho); //Aloca Espaco para dados Criptografados
				if(ret->dados == NULL)
				{
					secure_zero(ret, sizeof(DP_Frame_t));
					free(ret);
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

DP_Frame_t *ConstroiFrameQS(uint8_t *end, uint8_t *dados, size_t q, uint64_t *iterador, uint8_t *ikm)
/* Cria um quadro seguro. Pré-requisito, a IKM já deve ter sido gerada.
 * Esse quadro eh alocado dinamicamente e precisa ser desalocado manualmente apos o uso.
 *
 * @param end(in): Endereço contendo AREA | CODIGO CONTROLADOR | CODIGO SUBCONTROLADOR
 * @param dados(in): Dados que serão protegidos (Criptografados)
 * @param q(in): tamanho do vetor de dados
 * @param iterador(in): Contador de mensagens (utilizado para criptografar os dados).O iterador e
 * incrementado após a geracao do quadro.
 * @param ikm (in): IKM utilzada para gerar CHAVES, IV e TAG da criptografia
 * return: Estrutura de Quadro Seguro preenchida
 * */
{
	DP_Frame_t *ret = NULL;

	if(end != NULL && q && dados != NULL && ikm != NULL && iterador != NULL)
	//Verifica se existe dados para serem criptografados
	{
		char salt_hkdf[HKDF_INFO_SALT_LEN] 	= {'D','A','T','A','S','A','L','T','\0','\0','\0','\0','\0','\0','\0','\0'};
		char info[HKDF_INFO_SALT_LEN] 		= {'D','A','T','A','I','N','F','O','\0','\0','\0','\0','\0','\0','\0','\0'};
		uint8_t iv[GCM_IV_LEN] = {0}; // IV GCM 12 bytes
		uint8_t key[AES_KEY_LEN] = {0}; //Chave GCM
		uint64_t sessao = 0;

		// Aloca memoria para o quadro
		ret = malloc(sizeof(DP_Frame_t));
		if(!ret)
			return ret;

		ret->dados = malloc(q); //Aloca Espaco para dados Criptografados
		if(ret->dados == NULL)
		{
			secure_zero(ret, sizeof(DP_Frame_t));
			free(ret);
			return ret;
		}

		//Atualiza estrutura do QS
		ret->endereco[0] = end[0];
		ret->endereco[1] = end[1];
		ret->endereco[2] = end[2];
		ret->op = TROCA_DADOS_SEGUROS_B5;
		ret->iterador = *iterador;
		ret->tamanho = q;
		memcpy(ret->dados, dados, ret->tamanho);

		(*iterador)++; //Incrementa contador

		//Atualiza info e salt com iterador
		memcpy(&info[sizeof(info) - sizeof(uint64_t)], &(ret->iterador), sizeof(uint64_t)); // Atualiza INFO a cada mensagem enviada
		sessao = (uint64_t)(ret->iterador / MSG_BY_SESSION);
		memcpy(&salt_hkdf[sizeof(salt_hkdf) - sizeof(uint64_t)], &sessao, sizeof(uint64_t)); // Atualiza SALT


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
	if(frame && *frame)
	{
		//Se houver dados nesse quadro esses devem ser desalocados primeiro
		if ((*frame)->dados && (*frame)->tamanho != 0) {
			secure_zero((*frame)->dados, (*frame)->tamanho);
			free((*frame)->dados);
		}
		//Limpa/libera o restante da estrutura
		secure_zero(*frame, sizeof(DP_Frame_t));
		free(*frame);
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
			out = malloc(*qOut);
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
				free(out);
			}
			for(size_t i = 0; i < 12; i++) //Seta primeiro bit
				out[6+i] |= 0x80;

			if(!base64Encode(&out[B64_TAG_OFFSET], frame->tag, GCM_TAG_LEN)){
				// Erro ao codificar a TAG em B64
				secure_zero(out, *qOut);
				free(out);
			}
			for(size_t i = 0; i < 24; i++) //Seta primeiro bit
				out[B64_TAG_OFFSET+i] |= 0x80;

			if(!base64Encode(&out[B64_DATA_OFFSET], frame->dados, frame->tamanho)){
				// Erro ao codificar os dados em B64
				secure_zero(out, *qOut);
				free(out);
			}
			for(size_t i = 0; i < tamDadosB64; i++) //Seta primeiro bit
				out[B64_DATA_OFFSET+i] |= 0x80;

			out[*qOut-2] = GeraChecksum(&out[END_OFFSET], *qOut-3);
		}
		else if(frame->op != 0)	{
			// OPCODEs de quadro nao seguro (QNS)
			uint8_t cks = 0;


			*qOut = frame->tamanho + 7; //STX[1] + END[3] + OP[1] + DADOS[tamanho] + CKS[1] + ETX[1]
			out = malloc(*qOut);
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
				free(out);
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
	//const char *salt_hkdf = "DATAPROM_SALT";
	char salt_hkdf[HKDF_INFO_SALT_LEN] = {'D','A','T','A','S','A','L','T','\0','\0','\0','\0','\0','\0','\0','\0'};
	char info[HKDF_INFO_SALT_LEN] = {'D','A','T','A','I','N','F','O','\0','\0','\0','\0','\0','\0','\0','\0'};
	uint8_t iv[GCM_IV_LEN] = {0}; // IV GCM 12 bytes
	uint8_t key[AES_KEY_LEN] = {0}; //Chave GCM
	uint64_t sessao = 0;

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

			/* Atualiza INFO e SALT com o contador de Mensagens*/
			memcpy(&info[sizeof(info) - sizeof(uint64_t)], contador, sizeof(uint64_t)); //Atualiza INFO
			sessao = (uint64_t)(*contador / MSG_BY_SESSION);
			memcpy(&salt_hkdf[sizeof(salt_hkdf) - sizeof(uint64_t)], &sessao, sizeof(uint64_t)); // Atualiza SALT

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
	//const char *salt_hkdf = "DATAPROM_SALT";
	char info[HKDF_INFO_SALT_LEN] = 	 {'D','A','T','A','I','N','F','O','\0','\0','\0','\0','\0','\0','\0','\0'};
	char salt_hkdf[HKDF_INFO_SALT_LEN] = {'D','A','T','A','S','A','L','T','\0','\0','\0','\0','\0','\0','\0','\0'};
	uint8_t iv[GCM_IV_LEN] = {0}; // IV GCM 12 bytes
	uint8_t key[AES_KEY_LEN] = {0}; //Chave GCM
	uint8_t tag[GCM_TAG_LEN] = {0};
	uint8_t *encrypted = NULL;
	uint64_t sessao = 0;


	*lenOut = 0;

	if(cripto)
	// Vai criptografar
	{
		if(dados != NULL && lenIn != 0 && ikm != NULL && contador != NULL)
		//Verifica integridade dos dados de entrada
		{
			memcpy(&info[sizeof(info) - sizeof(uint64_t)], contador, sizeof(uint64_t)); // Atualiza INFO a cada mensagem enviada
			sessao = (uint64_t)(*contador / MSG_BY_SESSION);
			memcpy(&salt_hkdf[sizeof(salt_hkdf) - sizeof(uint64_t)], &sessao, sizeof(uint64_t)); // Atualiza SALT
//			memcpy(&salt_hkdf[sizeof(salt_hkdf) - sizeof(uint64_t)], (uint64_t)(contador/4), sizeof(uint64_t)); // Atualiza SALT

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





uint8_t *CriaQuadroCodificado(uint8_t *dados, size_t q, size_t *qOutput)
/* Cria quadro padronizado de  comunicação com os dados de entrada
 * end: endereço que se deseja anexar no quadro (3 bytes)
 * opcode: Opcode anexado no quadro
 * dados: dados que se deseja criar o quadro (criptografados ou nao)
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

	int tam = base64EncodedLength(q);
	codificado = malloc(tam);
	if(!codificado)
		return NULL;

	qOut = base64Encode(codificado, dados, q);

	//Seta bit 1 dos dados codificados
	for(int i = 0; i < qOut; i++)
		codificado[i] |= 0x80;

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
	//	uint8_t *pacoteDecodificado = DecodificaDados7bits(&input[1], q-2, qOutput);

	for(int i = 0; i < q; i++) //retira o bit 1 de todos os dados
		input[i] &= 0x7F;

	int tam = base64DecodedLength(input, q);
	uint8_t *pacoteDecodificado = malloc(tam);
	if(!pacoteDecodificado)
		return NULL;
	*qOutput = base64Decode(pacoteDecodificado, input, q);
	return pacoteDecodificado;
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

uint8_t CalculaChecksum(DP_Frame_t *frame)
{
	uint8_t ret = 0;
	return ret;
}



/*
 * PSKLib.h
 *
 *  Created on: 25 de ago de 2025
 *      Author: carlos.oliveira
 */

#ifndef INCLUDE_PSKLIB_H_
#define INCLUDE_PSKLIB_H_



/**
 * @brief psk_lenght_get()
 * Diz o comprimento das PSKs.
 * @return Retorna o comprimento de uma PSK em bytes
 */
unsigned int psk_lenght_get();


/**
 * @brief psk_ikm_get(int index, char *in, char *ikm);
 * Chame essa função para obter o IKM, utilizado no HKDF.
 * Deve ser usada junto com o endereço do controlador e
 * o segredo compartilhado ECDH.
 * @param index (in): ID do Controlador (1-63), caso 0
 * se refere a um controlador que ainda não possui uma ID.
 * @param sec (in): Segredo ECDH nao compactado (65 bytes)
 * @param ikm (out): Vetor do IKM (65 bytes)
 * @return Retorna 0 se falhar a geração da IKM.
 */
int psk_ikm_get(int index, char *sec, char *ikm);


#endif /* INCLUDE_PSKLIB_H_ */

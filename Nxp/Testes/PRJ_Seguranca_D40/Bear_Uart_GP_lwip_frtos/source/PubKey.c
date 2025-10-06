/*
 * PubKey.c
 *
 *  Created on: 15 de ago de 2025
 *      Author: carlos.oliveira
 */


#include "PubKey.h"
#include "FreeRTOS.h"


static void CleanObj(void *v, size_t n)
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

PubKey *New_PubKey()
{
	PubKey *obj = pvPortMalloc(sizeof(PubKey));
//	PubKey *obj = malloc(sizeof(PubKey));
	if(obj)
	{
		obj->pbKey.q = obj->pbBuf; //Inicializa o buffer
		memset(obj->pbBuf, 0, sizeof(obj->pbBuf));
	}
	return obj;
}

void Destroy_PubKey(PubKey *obj)
{
	CleanObj(obj, sizeof(PubKey)); //Limpa objeto
	vPortFree(obj);
	//free(obj);// libera a memoria alocada
	obj = NULL;
}

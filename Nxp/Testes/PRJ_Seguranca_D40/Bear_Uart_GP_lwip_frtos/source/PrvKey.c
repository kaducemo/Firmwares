/*
 * PrvKey.c
 *
 *  Created on: 15 de ago de 2025
 *      Author: carlos.oliveira
 */

#include "PrvKey.h"
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

PrvKey *New_PrvKey()
{
	PrvKey *obj = pvPortMalloc(sizeof(PrvKey));
//	PrvKey *obj = malloc(sizeof(PrvKey));
	if(obj)
	{
		obj->pvKey.x = obj->pvBuf; //Inicializa o buffer
		memset(obj->pvBuf, 0, sizeof(obj->pvBuf));
	}
	return obj;
}

void Destroy_PrvKey(PrvKey *obj)
{
	CleanObj(obj, sizeof(PrvKey)); //Limpa objeto
	vPortFree(obj);
//	free(obj);// libera a memoria alocada
	obj = NULL;
}

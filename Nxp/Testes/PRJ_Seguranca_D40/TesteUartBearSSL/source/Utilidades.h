/*
 * Utilidades.h
 *
 *  Created on: 1 de set de 2025
 *      Author: carlos.oliveira
 */

#ifndef UTILIDADES_H_
#define UTILIDADES_H_

#include <stdint.h>
#include <stddef.h>


#define CLEAR_MSB_N_BITS(byte, n) ((byte) & (255U >> n)) //Macros auxiliares
#define CLEAR_LSB_N_BITS(byte, n) ((byte) & (255U << n))




#define OK	1
#define NOK 0



void secure_zero(void *v, size_t n);
char *hex_array_to_string(uint8_t *in, uint8_t len);
void delay(void);

#endif /* UTILIDADES_H_ */

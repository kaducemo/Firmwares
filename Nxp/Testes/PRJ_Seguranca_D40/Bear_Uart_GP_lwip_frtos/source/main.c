/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
//#include "main.h"
#include "minhasTask.h"

/*******************************************************************************
 * Defines
 ******************************************************************************/


#ifndef BOARD_SW2_PIN
#define BOARD_SW2_PIN  			6U
#endif

#ifndef BOARD_SW3_PIN
#define BOARD_SW3_PIN  			4U
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
extern uint8_t rxSerialBuffer[UART_RX_BUFFER_LEN]; //Buffer utilizado na porta serial
extern size_t qtDadosDisponiveis; //Diz quantos bytes estão disponíveis para serem lidos quando um pacote é fechado
extern EventGroupHandle_t connEventGroup; //Controla diversos eventos
extern QueueHandle_t rxQueue; //Queues de dados
extern QueueHandle_t txQueue;
extern SemaphoreHandle_t xSemaforoSerialRX; //Sinalização de recepçao via serial

/*******************************************************************************
 * IRQs
 ******************************************************************************/

void PORTA_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Verifica se a interrupção foi no pino SW3
    if (PORT_GetPinsInterruptFlags(PORTA) & (1U << BOARD_SW3_PIN))
    {
        // Limpa a flag
        PORT_ClearPinsInterruptFlags(PORTA, 1U << BOARD_SW3_PIN);

        if(connEventGroup !=  NULL)
        {
        	// Seta o bit 1 no EventGroup
			xEventGroupSetBitsFromISR(connEventGroup, (EVENT_BUTTON_SW3_PRESSED), &xHigherPriorityTaskWoken);
			// Se uma task de maior prioridade foi despertada, faz o context switch
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void PORTC_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Verifica se a interrupção foi no pino SW2
    if (PORT_GetPinsInterruptFlags(PORTC) & (1U << BOARD_SW2_PIN))
    {
        // Limpa a flag
        PORT_ClearPinsInterruptFlags(PORTC, 1U << BOARD_SW2_PIN);


        if(connEventGroup !=  NULL)
        {

        	// Seta o bit 1 no EventGroup
			xEventGroupSetBitsFromISR(connEventGroup, (EVENT_BUTTON_SW2_PRESSED), &xHigherPriorityTaskWoken);
			// Se uma task de maior prioridade foi despertada, faz o context switch
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void UART0_SERIAL_RX_TX_IRQHANDLER(void){
	/*Interrupçao UART0*/

	static size_t cont = 0;
	static uint8_t recebendoPacote = 0;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

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
				if(rxQueue != NULL && connEventGroup != NULL) {
					// Libera o semáforo
					xSemaphoreGiveFromISR(xSemaforoSerialRX, &xHigherPriorityTaskWoken);
					portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
				}
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

/*******************************************************************************
 * MAIN
 ******************************************************************************/
int main(void)
{
//	uint8_t test[] = {0xCB, 0x96, 0xA8, 0x8D, 0xB0, 0xB3, 0xB0, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEA, 0xC4, 0xF0, 0x86, 0xF9, 0xB8, 0xE2, 0xBB, 0xEA, 0x80, 0xAB, 0xB7, 0xDD, 0xD2, 0xC8, 0x80, 0xF0, 0xC3, 0xFB, 0xBB, 0xEA, 0x9B, 0xB2, 0x99, 0xC6, 0xCE, 0xC8, 0xA3, 0xF0, 0xE3, 0xF9, 0x83, 0xC6, 0x80, 0xAB, 0xAB, 0xD0, 0xC6, 0xCB, 0x9B, 0xEB, 0xFB, 0xE3, 0xCC, 0xC8, 0x80, 0xF0, 0xB4, 0xFB, 0xF8, 0xEA, 0xC4, 0xF0, 0xC3, 0xFB, 0xCA, 0xC8, 0x80, 0xF0, 0xAE, 0xFB, 0xFB, 0xEB, 0x80, 0xAB, 0x82, 0xD1, 0xCF, 0xF4, 0x96, 0xF3, 0xFB, 0xE3, 0xCF, 0xF4, 0xFA, 0xF0, 0xCD, 0xF0, 0xA0, 0xFE, 0xFB, 0xEB, 0x81, 0xBB, 0xFB, 0xE3, 0xC3, 0xCB, 0x9B, 0xE8, 0x80, 0xAB, 0x8A, 0xD0, 0xC1, 0xCB, 0x9B, 0xE8, 0x80, 0xA1, 0x98, 0xC6, 0xCC, 0xF0, 0xE1, 0xFE, 0x83, 0xC6, 0x81, 0xAB, 0x81, 0xD1, 0x80, 0xA3, 0xFB, 0xE3,0xFB, 0xEB, 0x80, 0xAB, 0xE6, 0xDC, 0x99, 0xE1, 0xFB, 0xEB, 0xCF, 0x83, 0x80, 0x83, 0x80};
//	volatile uint8_t meucks = GeraChecksum(test, sizeof(test));

    SYSMPU_Type *base = SYSMPU;
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

    uint8_t idProtocolo[3] = {0};
    uint8_t idConvertida[3] = {0};


#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    base->CESR &= ~SYSMPU_CESR_VLD_MASK;

    /* Initialize lwIP from thread */
    if (sys_thread_new("Inicia IF Rede", stack_init_task, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
    {
        LWIP_ASSERT("stack_init_task(): Task creation failed.", 0);
    }

    vTaskStartScheduler(); //Inicia execução das tasks

    /* Will not get here unless a task calls vTaskEndScheduler ()*/
    return 0;
}


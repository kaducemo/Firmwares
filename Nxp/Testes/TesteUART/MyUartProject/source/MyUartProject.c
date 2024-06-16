/*
 * Copyright 2016-2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    MyUartProject.c
 * @brief   Application entry point.
 * Utilizando este FW, a placa serve como uma ponte entre dois dispositivos utilizando duas UARTs.
 * Neste caso, eu coloquei na UART (UART0, TX = PTB17, RX = PTB16 ) um módulo de BLUETOOTH e na outra UART (UART4, TX = PTE24 , RX = PTE25) a qual está
 * ligada a um conversor serial-USB embarcado na própria FRDM-K64, um PC. Dessa forma é possivel transferir comandos de um PC
 * para o módulo Bluetooth e vice versa.
 */

#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"


/* TODO: insert other include files here. */
/*******************************************************************************
 * Code
 ******************************************************************************/

void UART0_SERIAL_RX_TX_IRQHANDLER(void)
{
    uint8_t data;

    /* If new data arrived. */
    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(UART0))
    {
        data = UART_ReadByte(UART0);
        UART_WriteByte(UART4, data);
    }
    SDK_ISR_EXIT_BARRIER;
}

void UART4_SERIAL_RX_TX_IRQHANDLER(void)
{
    uint8_t data;

    /* If new data arrived. */
    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(UART4))
    {
        data = UART_ReadByte(UART4);
        UART_WriteByte(UART0, data);
    }
    SDK_ISR_EXIT_BARRIER;
}


void delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 8000; ++i)
    {
        __asm("NOP"); /* delay */
    }
}
/* TODO: insert other definitions and declarations here. */

/*
 * @brief   Application entry point.
 */
int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    PRINTF("Hello World\r\n");

    /* Force the counter to be placed into memory. */
    volatile static int i = 0 ;
    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
        i++ ;
        /* 'Dummy' NOP to allow source level single stepping of
            tight while() loop */
        delay();
//        UART_WriteByte(UART4, 'A');
        //UART_WriteByte(UART0, 'B');
        __asm volatile ("nop");
    }
    return 0 ;
}

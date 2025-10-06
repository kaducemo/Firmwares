/*
 * minhasTask.h
 *
 *  Created on: 16 de set de 2025
 *      Author: carlos.oliveira
 */

#ifndef MINHASTASK_H_
#define MINHASTASK_H_

/*
 * main.h
 *
 *  Created on: 16 de set de 2025
 *      Author: carlos.oliveira
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "lwip/opt.h"

#if LWIP_NETCONN

#include "tcpecho.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "enet_ethernetif.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "peripherals.h"
#include "fsl_phy.h"
#include "fsl_rnga.h"
#include "bearssl.h"
#include "OpcodesDP.h"
#include "PSKLib.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"

#include "fsl_phyksz8081.h"
#include "fsl_enet_mdio.h"
#include "fsl_device_registers.h"

#include "base64.h"
#include "DatapromFrame.h"
#include "PrvKey.h"
#include "PubKey.h"
#include "Utilidades.h"

/*******************************************************************************
 * MACROS
 ******************************************************************************/

#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)x
#endif /* UNUSED_PARAMETER */

/* Eventos */
#define EVENT_BUTTON_SW2_PRESSED 	(1 << 0)
#define EVENT_BUTTON_SW3_PRESSED 	(1 << 1)
#define EVENT_ETH_CONNECTED 		(1 << 2)
#define EVENT_ETH_DISCONNECTED 		(1 << 3)
#define EVENT_ETH_UART_RX_DT_READY	(1 << 4)
#define CONNECTION_ETH_ERROR		(1 << 8)




#include <stdint.h>
/*******************************************************************************
 * Definições
 ******************************************************************************/

#define DP40_CODIGO				37U
#define DP40_REDE				37U
#define DP40_AREA				37U

#define PROT_VER				"0300"
#define PROT_VER_H				0x30 //Versao de Protocolo
#define PROT_VER_L				0x02

#define SW_VER					"0308"
#define SW_VER_H				0x03 //Versao FW
#define SW_VER_L				0x01

#define TAB_VER					"0300"
#define TAB_VER_H				0x30 //Versao TABELA
#define TAB_VER_L				0x00

#define DP40_DESCRIPTION		"Controlador DP40A - FW Seguro"



#define UART_RX_BUFFER_LEN		2048U


// Maximo de tentativas p/ se conectar ao servidor
#define MAX_CONNECT_ATTEMPTS	5U

/* Porta de conexao. */
#define server_PORT		0x5050 //(20560)

/* IP REMOTO */

/* IP LOCAL. */
#ifndef serverIP_ADDR0
#define serverIP_ADDR0 10U
#endif
#ifndef serverIP_ADDR1
#define serverIP_ADDR1 10U
#endif
#ifndef serverIP_ADDR2
#define serverIP_ADDR2 0U
#endif
#ifndef serverIP_ADDR3
#define serverIP_ADDR3 102U
#endif


/* IP LOCAL. */
#ifndef configIP_ADDR0
#define configIP_ADDR0 10U
#endif
#ifndef configIP_ADDR1
#define configIP_ADDR1 10U
#endif
#ifndef configIP_ADDR2
#define configIP_ADDR2 0U
#endif
#ifndef configIP_ADDR3
#define configIP_ADDR3 101U
#endif

/* MASCARA DE REDE */
#ifndef configNET_MASK0
#define configNET_MASK0 255U
#endif
#ifndef configNET_MASK1
#define configNET_MASK1 255U
#endif
#ifndef configNET_MASK2
#define configNET_MASK2 255U
#endif
#ifndef configNET_MASK3
#define configNET_MASK3 0U
#endif

/* GATEWAY. */
#ifndef configGW_ADDR0
#define configGW_ADDR0 10U
#endif
#ifndef configGW_ADDR1
#define configGW_ADDR1 10U
#endif
#ifndef configGW_ADDR2
#define configGW_ADDR2 0U
#endif
#ifndef configGW_ADDR3
#define configGW_ADDR3 100U
#endif

/* MAC  */
#ifndef configMAC_ADDR
#define configMAC_ADDR                     \
    {                                      \
        0x02, 0x12, 0x13, 0x10, 0x15, 0x11 \
    }
#endif

/* Address of PHY interface. */
#define EXAMPLE_PHY_ADDRESS BOARD_ENET0_PHY_ADDRESS

/* MDIO operations. */
#define EXAMPLE_MDIO_OPS enet_ops

/* PHY operations. */
#define EXAMPLE_PHY_OPS phyksz8081_ops

/* ENET clock frequency. */
#define EXAMPLE_CLOCK_FREQ CLOCK_GetFreq(kCLOCK_CoreSysClk)

#ifndef EXAMPLE_NETIF_INIT_FN
/*! @brief Network interface initialization function. */
#define EXAMPLE_NETIF_INIT_FN ethernetif0_init
#endif /* EXAMPLE_NETIF_INIT_FN */

/*! @brief Stack size of the temporary lwIP initialization thread. */
#define INIT_THREAD_STACKSIZE 1024

/*! @brief Priority of the temporary lwIP initialization thread. */
#define INIT_THREAD_PRIO DEFAULT_THREAD_PRIO

#endif
#endif /* MAIN_H_ */

/*******************************************************************************
 * Tipos Personalizados
 ******************************************************************************/

//Enumeração das interfaces
typedef enum  {
	IFACE_UNDEF,
	IFACE_SERIAL,
	IFACE_ENET
}iface_t;

//Enumeração tipo pacote
typedef enum  {
	PACKTYPE_UNDEF, 	  //Pacote nao definido
	PACKTYPE_SEND,		  //Envia dado e espera uma resposta
	PACKTYPE_RECEIVE,	  //Pacote com dados recebidos
	PACKTYPE_SEND_ANSWER, // Envia resposta
}PackType_t;

//Estutura utilizada para manipulação de dados atrave´s das queues
typedef struct {
	iface_t iface; //interface onde o essa queue será utilizada (SERIAL/ENET)
	PackType_t ptype; //diz a finalidade desse pacote
	uint8_t *buf; //Aponta para o primeiro byte do buffer
	uint16_t len; //Tamanho do dados no buffer
}QueueDados_t;

/*******************************************************************************
 * Variaveis publicas
 ******************************************************************************/
extern EventGroupHandle_t connEventGroup; //Controla diversos eventos
extern uint8_t rxSerialBuffer[UART_RX_BUFFER_LEN]; //Buffer utilizado na porta serial
extern size_t qtDadosDisponiveis; //Diz quantos bytes estão disponíveis para serem lidos quando um pacote é fechado
extern QueueHandle_t rxQueue; //Queues de dados
extern QueueHandle_t txQueue;
extern SemaphoreHandle_t xSemaforoSerialRX; //Sinalização de recepçao via serial

/*******************************************************************************
 * Prototipos Funções
 ******************************************************************************/
QueueDados_t *new_queue_dados(iface_t iface, PackType_t pt, uint8_t *pIn, uint16_t q);
int clean_queue_dados(QueueDados_t *qDados);
void destroy_queue_dados(QueueDados_t **qDados);

/*******************************************************************************
 * Prototipos Tasks
 ******************************************************************************/
void stack_init_task(void *arg);
void tcp_connect_task(void *arg);
void tcp_receive_task(void *arg);
void tcp_send_task(void *arg);
void process_task(void *arg);
void serial_receive_task(void *arg);


/*******************************************************************************
 * Prototipos Hooks
 ******************************************************************************/
void vApplicationIdleHook(void);
void vApplicationMallocFailedHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);


#endif /* MINHASTASK_H_ */

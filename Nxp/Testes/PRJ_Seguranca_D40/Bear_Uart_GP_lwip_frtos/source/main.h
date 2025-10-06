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

//#include "tcpecho.h"
//#include "lwip/netifapi.h"
//#include "lwip/tcpip.h"
//#include "netif/ethernet.h"
//#include "enet_ethernetif.h"
//
//#include "pin_mux.h"
//#include "clock_config.h"
//#include "board.h"
//#include "peripherals.h"
//#include "fsl_phy.h"
//#include "fsl_rnga.h"
//#include "bearssl.h"
//#include "PSKLib.h"
//
//#include "fsl_phyksz8081.h"
//#include "fsl_enet_mdio.h"
//#include "fsl_device_registers.h"



#include <stdint.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* @TEST_ANCHOR */

/* IP address configuration. */
#ifndef configIP_ADDR0
#define configIP_ADDR0 10
#endif
#ifndef configIP_ADDR1
#define configIP_ADDR1 10
#endif
#ifndef configIP_ADDR2
#define configIP_ADDR2 0
#endif
#ifndef configIP_ADDR3
#define configIP_ADDR3 101
#endif

/* Netmask configuration. */
#ifndef configNET_MASK0
#define configNET_MASK0 255
#endif
#ifndef configNET_MASK1
#define configNET_MASK1 255
#endif
#ifndef configNET_MASK2
#define configNET_MASK2 255
#endif
#ifndef configNET_MASK3
#define configNET_MASK3 0
#endif

/* Gateway address configuration. */
#ifndef configGW_ADDR0
#define configGW_ADDR0 10
#endif
#ifndef configGW_ADDR1
#define configGW_ADDR1 10
#endif
#ifndef configGW_ADDR2
#define configGW_ADDR2 0
#endif
#ifndef configGW_ADDR3
#define configGW_ADDR3 100
#endif

/* MAC address configuration. */
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

/*
 * minhasTasks.c
 *
 *  Created on: 16 de set de 2025
 *      Author: carlos.oliveira
 */

#include "minhasTask.h"


/*******************************************************************************
 * Variables Publicas
 ******************************************************************************/
EventGroupHandle_t connEventGroup = NULL; //Controla diversos eventos
uint8_t rxSerialBuffer[UART_RX_BUFFER_LEN] = {0}; //Buffer utilizado na porta serial
size_t qtDadosDisponiveis = 0; //Diz quantos bytes estão disponíveis para serem lidos quando um pacote é fechado UART
QueueHandle_t rxQueue = NULL; //Queues de dados
QueueHandle_t txQueue = NULL;
SemaphoreHandle_t xSemaforoSerialRX = NULL;

//ECDH - SERIAL
uint8_t segredo[ECDH_SEC_LEN] = {0};
uint8_t *ikm = NULL;
uint64_t contadorMensagens = 0;
PrvKey *chaveLocalPrivada = NULL;
PubKey *chaveLocalPublica = NULL, *chaveRemotaPublica = NULL;

//ECDH - ANTARES
uint8_t segredoAnt[ECDH_SEC_LEN] = {0}; //Segredo ECDH
uint8_t *ikmAnt = NULL;
uint64_t contadorMensagensAnt = 0;
PrvKey *chaveLocalPrivadaAnt = NULL;
PubKey *chaveLocalPublicaAnt = NULL, *chaveRemotaPublicaAnt = NULL;


/*******************************************************************************
 * Variables Privadas
 ******************************************************************************/

static mdio_handle_t mdioHandle = {.ops = &EXAMPLE_MDIO_OPS};
static phy_handle_t phyHandle   = {.phyAddr = EXAMPLE_PHY_ADDRESS, .mdioHandle = &mdioHandle, .ops = &EXAMPLE_PHY_OPS};

// Global handles
static struct netconn *client_conn = NULL;
static TaskHandle_t serialRxTaskHandle = NULL;
static TaskHandle_t connectTaskHandle = NULL;
static TaskHandle_t sendTaskHandle = NULL;
static TaskHandle_t receiveTaskHandle = NULL;
static TaskHandle_t processTaskHandle = NULL;
struct netbuf *buf;


/*******************************************************************************
 * Funcoes Publicas
 ******************************************************************************/

int ComputeSharedSecret(br_ec_private_key *pvKey, br_ec_public_key *pbKey, uint8_t *secret)
/*Retorna o segredo da curva eliptica BR_EC_secp256r1
 *
 * pvKey: CHAVE PRIVADA
 * pbKey: CHAVE PUBLICA
 * secret: Secredo compartilhado calculado
 * return: 0 = segredo gerado com sucesso, -1 = erro
 * */
{
	const br_ec_impl *ec_implementation = br_ec_get_default();

	br_ec_public_key tmp = *pbKey;
	ec_implementation->mul(tmp.q, tmp.qlen, pvKey->x, pvKey->xlen, BR_EC_secp256r1);

	if (tmp.qlen == 65) {
	    memcpy(secret, tmp.q, 65);
	    return 0;
	}
	else
		return -1;

	return 0;
}

int keygen_ec(int curve, PrvKey *prKey, PubKey *pbKey)
/*Gera um par de chaves(publica e privada) para a curva selecionada*/
{
    const char *seeder_name;

    prKey->ok = OK;
    pbKey->ok = OK;

    const br_ec_impl *impl = br_ec_get_default(); //Inicia contexto ECDHE

    /*Inicia gerador de sememntes (ENTROPIA)*/
    br_prng_seeder seeder = br_prng_seeder_system(&seeder_name);

    if (seeder == NULL)
	/*Erro. Não conseguiu iniciar o gerador de sementes (ENTROPIA)*/
    {
    	prKey->ok = NOK;
		pbKey->ok = NOK;

		memset(prKey,0,sizeof(PrvKey));
		memset(pbKey,0,sizeof(PubKey));

    	return NOK;
    }

    //Inicia Gerador Deterministico SHA256
    br_hmac_drbg_context rng;
    br_hmac_drbg_init(&rng, &br_sha256_vtable, NULL, 0);

    if (!seeder(&rng.vtable))
	/*Erro. Não conseguiu iniciar o gerador detereministico SHA256*/
    {
    	prKey->ok = NOK;
		pbKey->ok = NOK;

		memset(prKey,0,sizeof(PrvKey));
		memset(pbKey,0,sizeof(PubKey));

		return NOK;
    }

    //Gera chave privada ECDHE
    if (!br_ec_keygen( &rng.vtable, impl, &(prKey->pvKey), prKey->pvKey.x, curve))
	/*Erro. Geracao da chave privada*/
    {
    	prKey->ok = NOK;
		pbKey->ok = NOK;

		memset(prKey,0,sizeof(PrvKey));
		memset(pbKey,0,sizeof(PubKey));

		return NOK;
    }

    //Gera chave publica ECDHE a partir da chave privada
    if (!br_ec_compute_pub(impl, &(pbKey->pbKey), pbKey->pbKey.q, &(prKey->pvKey)))
    	/*Erro. Geracao da chave publica*/
    {
    	prKey->ok = NOK;
		pbKey->ok = NOK;

		memset(prKey,0,sizeof(PrvKey));
		memset(pbKey,0,sizeof(PubKey));

		return NOK;
	}

    //Limpa toda estrutura do gerador Deterministico
    memset(&rng,0,sizeof(br_hmac_drbg_context));

    return OK;
}


//Constroi uma estrutura para apontar dados
QueueDados_t *new_queue_dados(iface_t iface, PackType_t pt, uint8_t *pIn, uint16_t q)
{
	QueueDados_t *ret = NULL;
	ret = pvPortMalloc(sizeof(QueueDados_t));
	if(ret == NULL)
		return ret;

	ret->iface = IFACE_UNDEF;
	ret->buf = NULL;
	ret->len = 0;

	ret->buf =  pvPortMalloc(q);
	if(ret->buf == NULL)
	{
		vPortFree(ret);
		ret = NULL;
		return ret;
	}

	ret->len = q;
	ret->iface = iface;
	ret->ptype = pt;
	memcpy(ret->buf, pIn, ret->len);
	return ret;
}

//Limpa uma estrutura para apontar dados
int clean_queue_dados(QueueDados_t *qDados)
{
	int ret = 1;
	if(qDados != NULL)
	{
		if(qDados->buf != NULL && qDados->len != 0) {
			// Limpa Buffer
			memset(qDados->buf, 0, qDados->len);
			qDados->len = 0;
			qDados->iface = IFACE_UNDEF;
			qDados->ptype = PACKTYPE_UNDEF;
		}
	}
	else
		ret = 0;
	return ret;
}

//Destroi uma etrutura de dados e destroi os dados
void destroy_queue_dados(QueueDados_t **qDados)
{
	if(qDados != NULL && *qDados != NULL)
	{
		clean_queue_dados(*qDados);
		if(((*qDados)->buf) != NULL)
			vPortFree((*qDados)->buf);
		vPortFree(*qDados);
		*qDados = NULL;
	}
}

/*******************************************************************************
 * Tasks
 ******************************************************************************/


/*!
 * @brief Initializes lwIP stack.
 *
 * @param arg unused
 */
void stack_init_task(void *arg)
{
    static struct netif netif;
    ip4_addr_t netif_ipaddr, netif_netmask, netif_gw;
    ethernetif_config_t enet_config = {
        .phyHandle  = &phyHandle,
        .macAddress = configMAC_ADDR,
    };

    LWIP_UNUSED_ARG(arg);

    mdioHandle.resource.csrClock_Hz = EXAMPLE_CLOCK_FREQ;

    IP4_ADDR(&netif_ipaddr, configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3);
    IP4_ADDR(&netif_netmask, configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3);
    IP4_ADDR(&netif_gw, configGW_ADDR0, configGW_ADDR1, configGW_ADDR2, configGW_ADDR3);

    tcpip_init(NULL, NULL);

    netifapi_netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, EXAMPLE_NETIF_INIT_FN,
                       tcpip_input);
    netifapi_netif_set_default(&netif);
    netifapi_netif_set_up(&netif);

    connEventGroup = xEventGroupCreate();
	rxQueue = xQueueCreate(10, sizeof(QueueDados_t *));
	txQueue = xQueueCreate(10, sizeof(QueueDados_t *));
	xSemaforoSerialRX = xSemaphoreCreateBinary();

	if (connEventGroup == NULL || rxQueue == NULL || txQueue == NULL || xSemaforoSerialRX == NULL) {
		PRINTF(" -STACK_INIT_TASK: ERRO - Criando recursos do FreeRTOS!\r\n");
		return;
	}

	BaseType_t result = pdPASS;

	result = xTaskCreate(process_task, "ProcessTask", DEFAULT_THREAD_STACKSIZE, NULL, DEFAULT_THREAD_PRIO, &processTaskHandle);
	if (result != pdPASS)
	{
		PRINTF(" -STACK_INIT_TASK: ERRO - Criando Task Processa Eventos\r\n");
		vTaskDelete(NULL);
		taskYIELD();
	}

	result = xTaskCreate(tcp_connect_task, "ConnectTask", configMINIMAL_STACK_SIZE, NULL, DEFAULT_THREAD_PRIO, &connectTaskHandle);
	if(result != pdPASS)
	{
		PRINTF(" -STACK_INIT_TASK: ERRO - Criando tcp_connect_task\r\n");
		vTaskDelete(NULL);
		taskYIELD();
	}

	result = xTaskCreate(serial_receive_task, "SerialRxTask", configMINIMAL_STACK_SIZE, NULL, DEFAULT_THREAD_PRIO, &serialRxTaskHandle);
	if(result != pdPASS)
	{
		PRINTF(" -STACK_INIT_TASK: ERRO - Criando serial_receive_task\r\n");
		vTaskDelete(NULL);
		taskYIELD();
	}

	PRINTF(" -STACK_INIT_TASK: Interface de REDE configurada\r\n");

//    PRINTF("\r\n************************************************\r\n");
//    PRINTF(" INTERFACE LOCAL CONFIGURADA.\r\n");
//    PRINTF("************************************************\r\n");
//    PRINTF(" IPv4 local       : %u.%u.%u.%u\r\n", ((u8_t *)&netif_ipaddr)[0], ((u8_t *)&netif_ipaddr)[1],
//           ((u8_t *)&netif_ipaddr)[2], ((u8_t *)&netif_ipaddr)[3]);
//    PRINTF(" IPv4 mascara     : %u.%u.%u.%u\r\n", ((u8_t *)&netif_netmask)[0], ((u8_t *)&netif_netmask)[1],
//           ((u8_t *)&netif_netmask)[2], ((u8_t *)&netif_netmask)[3]);
//    PRINTF(" IPv4 Gateway     : %u.%u.%u.%u\r\n", ((u8_t *)&netif_gw)[0], ((u8_t *)&netif_gw)[1],
//           ((u8_t *)&netif_gw)[2], ((u8_t *)&netif_gw)[3]);
//    PRINTF("\r\n************************************************\r\n");
//        PRINTF(" CONECTANDO NO SERVIDOR...\r\n");
//        PRINTF("************************************************\r\n");
//	PRINTF(" IPv4 servidor    : %u.%u.%u.%u\r\n", serverIP_ADDR0, serverIP_ADDR1,
//			serverIP_ADDR2, serverIP_ADDR3);
//    PRINTF(" Porta            : %u\r\n", server_PORT);
//    PRINTF("************************************************\r\n");

    vTaskDelete(NULL);
}

// Task: Recebe dados da porta Serail connection
void serial_receive_task(void *arg)
{
	UBaseType_t stackRestante;
	PRINTF(" -SERIAL_RX_TASK: Iniciada\r\n");

	for (;;) {
//		stackRestante = uxTaskGetStackHighWaterMark(NULL);
//		if(stackRestante < 128)
//			PRINTF(" -SERIAL_RX_TASK: Pouca memoria disponivel STACK - %u bytes\r\n", stackRestante );
		// Espera indefinidamente pelo semáforo
		if (xSemaphoreTake(xSemaforoSerialRX, portMAX_DELAY) == pdTRUE) {
			// Semáforo recebido, executa ação
			QueueDados_t *q_dado = new_queue_dados(IFACE_SERIAL, PACKTYPE_RECEIVE, rxSerialBuffer, qtDadosDisponiveis);
			if(q_dado != NULL)
				xQueueSend(rxQueue, &q_dado, 0);

			xEventGroupSetBits(connEventGroup, EVENT_ETH_UART_RX_DT_READY);
			printf(" -SERIAL_RX_TASK: Converteu dado Serial em Queue \r\n");
		}
	}

    vTaskDelete(NULL);
}

// Task: TCP connection
void tcp_connect_task(void *arg)
{
	UNUSED_PARAMETER(arg);
	UBaseType_t stackRestante;
    ip_addr_t server_ip;
    uint16_t tentativas_conexao = MAX_CONNECT_ATTEMPTS;
    PRINTF(" -CONNECT_TASK: Iniciada\r\n");
    vTaskSuspend(NULL); // Suspende imediatamente ao iniciar
    IP4_ADDR(&server_ip, serverIP_ADDR0, serverIP_ADDR1, serverIP_ADDR2, serverIP_ADDR3); // Server IP

    while(true)
    {
//    	stackRestante = uxTaskGetStackHighWaterMark(NULL);
//    	if(stackRestante < 128)
//    		PRINTF(" -CONNECT_TASK: Pouca memoria disponivel STACK - %u bytes\r\n", stackRestante );

    	tentativas_conexao = MAX_CONNECT_ATTEMPTS;

    	if(client_conn != NULL) {
			// Conexao ativa
			err_t err;
			err = netconn_close(client_conn);
			err = netconn_delete(client_conn);

			if (err != ERR_OK) {
				xEventGroupSetBits(connEventGroup, CONNECTION_ETH_ERROR);
			}
			else {
				xEventGroupSetBits(connEventGroup, EVENT_ETH_DISCONNECTED);
				client_conn = NULL;
			}
    	}

    	else {
			// Tentando conectar ao servidor
			while (1)
			{

				if (client_conn != NULL)
				{
					netconn_delete(client_conn);
					client_conn = NULL;
				}

				client_conn = netconn_new(NETCONN_TCP);
				if (client_conn == NULL) {
					xEventGroupSetBits(connEventGroup, CONNECTION_ETH_ERROR);
					break;
				}

				if (netconn_connect(client_conn, &server_ip, server_PORT) == ERR_OK)
				{
					xEventGroupSetBits(connEventGroup, EVENT_ETH_CONNECTED);
					break;
				}
				else
				{
					if(!tentativas_conexao)
					{
						if (client_conn != NULL) {
							netconn_delete(client_conn);
							client_conn = NULL;
						}
						xEventGroupSetBits(connEventGroup, EVENT_ETH_DISCONNECTED);
						vTaskDelay(pdMS_TO_TICKS(1500));
						break;
					}
					else
					{
						tentativas_conexao--;
						PRINTF(" -CONNECT_TASK: Tentativas Restantes...%u\r\n", tentativas_conexao);
						vTaskDelay(pdMS_TO_TICKS(1500));
					}
				}
			}
		}
    	vTaskSuspend(NULL);
    }

    vTaskDelete(NULL);

}

// Task: TCP receive
void tcp_receive_task(void *arg)
{
	UNUSED_PARAMETER(arg);
	UBaseType_t stackRestante;
    void *data;
    u16_t len;
    QueueDados_t *q_enviado = NULL;
    err_t err;

    PRINTF(" -RECEIVE_TASK: Iniciada\r\n");

    while (1)
    {
//    	stackRestante = uxTaskGetStackHighWaterMark(NULL);
//    	if(stackRestante < 128)
//    		PRINTF(" -RECEIVE_TASK: Pouca memoria disponivel STACK - %u bytes\r\n", stackRestante );
    	if (client_conn != NULL) {
    		if (err = netconn_recv(client_conn, &buf) == ERR_OK)
			{
    			if(buf == NULL)
    			{
    				PRINTF("RECEIVE_TASK: ERRO - buf == NULL \r\n");
    				xEventGroupSetBits(connEventGroup, EVENT_ETH_DISCONNECTED);
					vTaskSuspend(NULL); // Suspende imediatamente ao iniciar
    			}
				netbuf_data(buf, &data, &len);
				q_enviado = new_queue_dados(IFACE_ENET, PACKTYPE_RECEIVE,(uint8_t *)data, (uint16_t)len);
				xQueueSend(rxQueue, &q_enviado, 0);
				xEventGroupSetBits(connEventGroup, EVENT_ETH_UART_RX_DT_READY);
				netbuf_delete(buf);
				buf = NULL;

			}
    		else
    		{
    			 printf("RECEIVE_TASK: ERRO - \"%s\"\n", lwip_strerr(err));
    			 xEventGroupSetBits(connEventGroup, EVENT_ETH_DISCONNECTED);
    			 vTaskSuspend(NULL); // Suspende imediatamente ao iniciar
    			 break;
    		}
		}
    	else
		{
			PRINTF("RECEIVE_TASK: ERRO - client_conn == NULL \r\n");
			xEventGroupSetBits(connEventGroup, EVENT_ETH_DISCONNECTED);
			vTaskSuspend(NULL); // Suspende imediatamente ao iniciar
			break;
		}
    }
    vTaskDelete(NULL); // Deleta task
}


// Task: TCP send
void tcp_send_task(void *arg)
{
	UNUSED_PARAMETER(arg);
	UBaseType_t stackRestante;
	QueueDados_t *q_recebido = NULL;
    TickType_t xStartTime, xNow, xLastSendTime; //Variáveis auxiliares para timeout
    const TickType_t xTimeout = pdMS_TO_TICKS(5000);  // timeout total de 5s
//    vTaskSuspend(NULL); // Suspende imediatamente ao iniciar

    PRINTF(" -SEND_TASK: Iniciada\r\n");


    while (1)
    {
//    	stackRestante = uxTaskGetStackHighWaterMark(NULL);
//		if(stackRestante < 128)
//			PRINTF(" -SEND_TASK: Pouca memoria disponivel STACK - %u bytes\r\n", stackRestante );

        if (xQueueReceive(txQueue, &q_recebido, portMAX_DELAY) == pdTRUE)
        {
			//Envia a Mensagem e Aguarda o recebimento da mesma por um timeout

			xStartTime = xTaskGetTickCount(); //Horario que recebeu o comando

			if (client_conn != NULL) {
				netconn_write(client_conn, q_recebido->buf, q_recebido->len, NETCONN_COPY);
				PRINTF(" -SEND_TASK: Enviada mensagem p/ Antares\r\n");
			} else {
				PRINTF(" -SEND_TASK: ERRO - client_conn está NULL na tcp_send_task\r\n");
				xEventGroupSetBits(connEventGroup, EVENT_ETH_DISCONNECTED);
				break;
			}

			if(q_recebido->ptype == PACKTYPE_SEND) {
			// Se pacote espera resposta

				TickType_t xNow = xTaskGetTickCount();
				TickType_t xLastSendTime = xNow;
				while (true) {

					 xNow = xTaskGetTickCount();

					// Envia a cada 1s
					if ((xNow - xLastSendTime) >= pdMS_TO_TICKS(1000)) {
						netconn_write(client_conn, q_recebido->buf, q_recebido->len, NETCONN_COPY);
						PRINTF(" -SEND_TASK: Enviada mensagem p/ Antares NOVAMENTE\r\n");
						xLastSendTime = xNow;
					}

					if (uxQueueMessagesWaiting(rxQueue) > 0) {
							PRINTF(" -SEND_TASK: Recebeu Resposta do Antares\r\n");
								break;  // sinal recebido
					}

					// Verifica se o tempo total já passou
					if ((xTaskGetTickCount() - xStartTime) > xTimeout) {
						// Timeout atingido
						PRINTF(" -SEND_TASK: ERRO - Antares NAO Respondeu!\r\n");
						break;
					}
				}
			}
        }
        destroy_queue_dados(&q_recebido);
    }
    vTaskDelete(NULL); // Deleta TASK
}



// Task: Process received data
void process_task(void *arg)
{
	UNUSED_PARAMETER(arg);
	EventBits_t uxBits;
	UBaseType_t stackRestante;

	PRINTF(" -PROCESS_TASK: Iniciada\r\n");

    while (1)
    {
//    	stackRestante = uxTaskGetStackHighWaterMark(NULL);
//    	if(stackRestante < 500)
//    		PRINTF(" -PROCESS_TASK: Pouca memoria disponivel STACK - %u bytes\r\n", stackRestante );

    	uxBits = xEventGroupWaitBits(																\
    			connEventGroup,																		\
				EVENT_BUTTON_SW2_PRESSED | EVENT_BUTTON_SW3_PRESSED | EVENT_ETH_CONNECTED | 		\
				EVENT_ETH_DISCONNECTED   |  CONNECTION_ETH_ERROR 	| EVENT_ETH_UART_RX_DT_READY, 	\
				pdTRUE,																				\
				pdFALSE,																			\
				portMAX_DELAY);


    	if (uxBits & EVENT_BUTTON_SW2_PRESSED) {
    		//Garante que as tasks não estão fazendo nada quando conecta
    		if(sendTaskHandle != NULL)
    		{
    			vTaskDelete(sendTaskHandle);
    			sendTaskHandle = NULL;
    		}
    		if(receiveTaskHandle != NULL)
    		{
    			vTaskDelete(receiveTaskHandle);
    			receiveTaskHandle = NULL;
    		}
    		if(buf != NULL)
    		{
    			netbuf_delete(buf);
    			buf = NULL;
    		}

    		vTaskResume(connectTaskHandle);
    	}

    	if (uxBits & EVENT_BUTTON_SW3_PRESSED) {
		//Garante que as tasks não estão fazendo nada quando conecta
//    		uint8_t vetor_quadro[3] = {0x80, 0x81, 0x82};
//			QueueDados_t *q_dado = new_queue_dados(IFACE_ENET, PACKTYPE_SEND ,vetor_quadro, sizeof(vetor_quadro));
//
//			if (txQueue != NULL && q_dado != NULL)
//				xQueueSend(txQueue, &q_dado, 0);

    		DP_Frame_t *frame = NULL;

    		uint8_t frame_data[17] = {(DP40_AREA | 0x80), (DP40_REDE | 0x80), (DP40_CODIGO | 0x80)};

			//Passa Versao do SW
			memcpy(&frame_data[3], SW_VER, strlen(SW_VER));

			//Passa Versao da Tabela
			memcpy(&frame_data[7], TAB_VER, strlen(TAB_VER));

			//Passa Versao Protocolo
			memcpy(&frame_data[11], PROT_VER, strlen(PROT_VER));

			//Adiciona UPS e Pluviometrico
			frame_data[15] = frame_data [16] = 0x80;

			//Converte endereco do controlador para o padrao do frame
			uint8_t auxEnd[3] = {0};
			VetorizaIdDoControlador(DP40_CODIGO, DP40_REDE, DP40_AREA, auxEnd);

			//Constroi o frame e em seguida o vetoriza
			frame = ConstroiFrameQNS(auxEnd, MENSAGEM_INICIAL_GSM_80, frame_data, sizeof(frame_data) ); //Constroi Quadro Nao Seguro
			size_t frame_len = 0;
			uint8_t *frame_vector = VetorizaQuadro(frame, &frame_len) ;

			//Envia vetor para queue e em seguida desaloca memorias intermediarias
			QueueDados_t *q_dado = new_queue_dados(IFACE_ENET, PACKTYPE_SEND,frame_vector, frame_len);
			if (txQueue != NULL && q_dado != NULL)
				xQueueSend(txQueue, &q_dado, 0);

			vPortFree(frame);
			vPortFree(frame_vector);
			frame = NULL;
			frame_vector = NULL;
	}

    	if (uxBits & EVENT_ETH_CONNECTED) {
    		PRINTF(" -PROCESS_TASK: Conectado no servidor!\r\n");
    		DP_Frame_t *frame = NULL;

    		BaseType_t result = pdPASS;
    		result = xTaskCreate(tcp_receive_task, "ReceiveTask", configMINIMAL_STACK_SIZE, NULL, DEFAULT_THREAD_PRIO, &receiveTaskHandle);
			if (result != pdPASS)
					PRINTF(" -PROCESS_TASK ERRO: Criando Task RECEBE DADOS!\r\n");
			result = xTaskCreate(tcp_send_task, "SendTask", configMINIMAL_STACK_SIZE, NULL, DEFAULT_THREAD_PRIO, &sendTaskHandle);
			if (result != pdPASS)
					PRINTF(" -PROCESS_TASK ERRO: Criando Task ENVIA DADOS!\r\n");

			/*Dados da Mensagem Inicial*/

			//Passa endereco para os dados
			uint8_t frame_data[17] = {(DP40_AREA | 0x80), (DP40_REDE | 0x80), (DP40_CODIGO | 0x80)};

			//Passa Versao do SW
			memcpy(&frame_data[3], SW_VER, strlen(SW_VER));

			//Passa Versao da Tabela
			memcpy(&frame_data[7], TAB_VER, strlen(TAB_VER));

			//Passa Versao Protocolo
			memcpy(&frame_data[11], PROT_VER, strlen(PROT_VER));

			//Adiciona UPS e Pluviometrico
			frame_data[15] = frame_data [16] = 0x80;

			//Converte endereco do controlador para o padrao do frame
			uint8_t auxEnd[3] = {0};
			VetorizaIdDoControlador(DP40_CODIGO, DP40_REDE, DP40_AREA, auxEnd);

			//Constroi o frame e em seguida o vetoriza
			frame = ConstroiFrameQNS(auxEnd, MENSAGEM_INICIAL_GSM_80, frame_data, sizeof(frame_data) ); //Constroi Quadro Nao Seguro
			size_t frame_len = 0;
			uint8_t *frame_vector = VetorizaQuadro(frame, &frame_len) ;

			//Envia vetor para queue e em seguida desaloca memorias intermediarias
			QueueDados_t *q_dado = new_queue_dados(IFACE_ENET, PACKTYPE_SEND,frame_vector, frame_len);
			if (txQueue != NULL && q_dado != NULL)
				xQueueSend(txQueue, &q_dado, 0);


    		vPortFree(frame);
			vPortFree(frame_vector);
			frame = NULL;
			frame_vector = NULL;
    	}

    	if (uxBits & EVENT_ETH_DISCONNECTED) {
    		PRINTF(" -PROCESS_TASK: Desconectado do servidor\r\n");
		}

    	if (uxBits & CONNECTION_ETH_ERROR) {
			PRINTF(" -PROCESS_TASK ERRO: Fechando conexao TCP\r\n");
		}

    	if (uxBits & EVENT_ETH_UART_RX_DT_READY) {
    		QueueDados_t *queue_recebido = NULL;
    		if (xQueueReceive(rxQueue, &queue_recebido, 0) == pdTRUE)
			{
    			DP_Frame_t *frameResposta = NULL, *frameRecebido = NULL;
				uint8_t *frameRespostaVetorizado = NULL;
				size_t tamanhoRespostaVetorizada = 0;
				uint8_t origem = IFACE_UNDEF; // Interface de origem do dado recebido


				origem = queue_recebido->iface;
				if(origem == IFACE_SERIAL)
				{
					frameRecebido =  ObtemFrameDoVetor(queue_recebido->buf, queue_recebido->len, ikm, 0);
					if(frameRecebido != NULL)
					{
						uint8_t ack = 0x06;
						UART_WriteBlocking(UART0, &ack, 1); //Envia ACK
						PRINTF(" -PROCESS_TASK: ACK: %02X, SERIAL\r\n", ack);

//						uint8_t test[] = {0x02, 0xCB, 0x96, 0xA8, 0x8D, 0xB0, 0xB3, 0xB0, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEA, 0xC4, 0xF0, 0x86, 0xF9, 0xB8, 0xE2, 0xBB, 0xEA, 0x80, 0xAB, 0xB7, 0xDD, 0xD2, 0xC8, 0x80, 0xF0, 0xC3, 0xFB, 0xBB, 0xEA, 0x9B, 0xB2, 0x99, 0xC6, 0xCE, 0xC8, 0xA3, 0xF0, 0xE3, 0xF9, 0x83, 0xC6, 0x80, 0xAB, 0xAB, 0xD0, 0xC6, 0xCB, 0x9B, 0xEB, 0xFB, 0xE3, 0xCC, 0xC8, 0x80, 0xF0, 0xB4, 0xFB, 0xF8, 0xEA, 0xC4, 0xF0, 0xC3, 0xFB, 0xCA, 0xC8, 0x80, 0xF0, 0xAE, 0xFB, 0xFB, 0xEB, 0x80, 0xAB, 0x82, 0xD1, 0xCF, 0xF4, 0x96, 0xF3, 0xFB, 0xE3, 0xCF, 0xF4, 0xFA, 0xF0, 0xCD, 0xF0, 0xA0, 0xFE, 0xFB, 0xEB, 0x81, 0xBB, 0xFB, 0xE3, 0xC3, 0xCB, 0x9B, 0xE8, 0x80, 0xAB, 0x8A, 0xD0, 0xC1, 0xCB, 0x9B, 0xE8, 0x80, 0xA1, 0x98, 0xC6, 0xCC, 0xF0, 0xE1, 0xFE, 0x83, 0xC6, 0x81, 0xAB, 0x81, 0xD1, 0x80, 0xA3, 0xFB, 0xE3,0xFB, 0xEB, 0x80, 0xAB, 0xE6, 0xDC, 0x99, 0xE1, 0xFB, 0xEB, 0xCF, 0x83, 0x80, 0x83, 0x80, 0xEC, 0x03};
//						UART_WriteBlocking(UART0, test, sizeof(test)); //Envia ACK
					}
				}
				else
					frameRecebido =  ObtemFrameDoVetor(queue_recebido->buf, queue_recebido->len, ikmAnt, 1);

				destroy_queue_dados(&queue_recebido); //Destroi o dado do queue pois já foi copiado para o quadro


				if(frameRecebido != NULL)
				{
					switch (frameRecebido->op)
					{
						case MENSAGEM_INICIAL_GSM_80: //Resposta do Antares
						{
							if(origem == IFACE_ENET)
							{
								PRINTF(" -PROCESS_TASK: Recebida MENSAGEM_INICIAL_GSM_80, TCP\r\n");

								// Apaga IKM e Libera a memoria
								if(ikmAnt != NULL)
								{
									secure_zero(ikmAnt, ECDH_SEC_LEN);
									vPortFree(ikmAnt);
									ikmAnt = NULL;
								}

								//Destroi chaves antigas e cria novas
								secure_zero(segredoAnt, ECDH_SEC_LEN);
								if (chaveLocalPrivadaAnt != NULL)
								{
									Destroy_PrvKey(chaveLocalPrivadaAnt);
									chaveLocalPrivadaAnt = NULL;
								}
								chaveLocalPrivadaAnt = New_PrvKey();

								if (chaveLocalPublicaAnt != NULL)
								{
									Destroy_PubKey(chaveLocalPublicaAnt);
									chaveLocalPublicaAnt = NULL;
								}
								chaveLocalPublicaAnt = New_PubKey();

								// Cria novas chaves Locais Publicas e Privadas
								if(keygen_ec(BR_EC_secp256r1, chaveLocalPrivadaAnt, chaveLocalPublicaAnt) != 1)
								{
									//Houve problema na geracao, apaga as chaves
									if (chaveLocalPublicaAnt != NULL)
									{
										Destroy_PubKey(chaveLocalPublicaAnt);
										chaveLocalPublicaAnt = NULL;
									}
									if (chaveLocalPrivadaAnt != NULL)
									{
										Destroy_PrvKey(chaveLocalPrivadaAnt);
										chaveLocalPrivadaAnt = NULL;
									}
								}
								contadorMensagensAnt = 0;
							}
							else
								PRINTF(" -PROCESS_TASK: Recebida MENSAGEM_INICIAL_GSM_80, SERIAL\r\n");
							break;
						}

						case ENVIA_IDENTIFICACAO_8D: //Resposta do Antares
						{
							if(origem == IFACE_ENET)
								PRINTF(" -PROCESS_TASK: Recebido ENVIA_IDENTIFICACAO, TCP\r\n");
							else
								PRINTF(" -PROCESS_TASK: Recebido ENVIA_IDENTIFICACAO, SERIAL\r\n");

							uint8_t auxEnd[3] = {0};

							const char *versaoSW = SW_VER;
							uint8_t codigoControlador[6] = {0};
							sprintf(&codigoControlador[0],"%02d",DP40_AREA);
							sprintf(&codigoControlador[2],"%02d",DP40_REDE);
							sprintf(&codigoControlador[4],"%02d",DP40_CODIGO);

							uint8_t descricao[128] = {0};
							const char *minhaDescricao = DP40_DESCRIPTION;
							memcpy(descricao, minhaDescricao, strlen(minhaDescricao) + 1);
							uint8_t versaoTabela[2] = {(TAB_VER_H | 0x80), (TAB_VER_L | 0x80)};
							uint8_t versaoProtocolo[2] = {(PROT_VER_H | 0x80), (PROT_VER_L | 0x80)};
							size_t tamDados = sizeof(versaoSW) + sizeof(codigoControlador) + sizeof(descricao) + sizeof(versaoTabela) + sizeof(versaoProtocolo);

							uint8_t *dados = pvPortMalloc(tamDados);
							memcpy(&dados[0], versaoSW, 4);
							memcpy(&dados[0+4], codigoControlador, 6);
							memcpy(&dados[0+4+6], descricao, 128);
							memcpy(&dados[0+4+6+128], versaoTabela, 2);
							memcpy(&dados[0+4+6+128+2], versaoProtocolo, 2);

							VetorizaIdDoControlador(DP40_CODIGO, DP40_REDE, DP40_AREA, auxEnd);

							frameResposta = ConstroiFrameQNS(auxEnd, ENVIA_IDENTIFICACAO_8D, dados, tamDados ); //Constroi Quadro Nao Seguro
							vPortFree(dados);
							dados = NULL;

							frameRespostaVetorizado = VetorizaQuadro(frameResposta, &tamanhoRespostaVetorizada);
							if(origem == IFACE_SERIAL)	{
								//SERIAL
								UART_WriteBlocking(UART0, frameRespostaVetorizado, tamanhoRespostaVetorizada);

//								taskENTER_CRITICAL();
//								for(uint16_t b = 0; b <tamanhoRespostaVetorizada; b++)
//									PRINTF("%u: %02X, ", b+1, frameRespostaVetorizado[b]);
//								PRINTF("\r\n");
//								taskEXIT_CRITICAL();

								// Apaga IKM e Libera a memoria
								if(ikm != NULL)
								{
									secure_zero(ikm, ECDH_SEC_LEN);
									vPortFree(ikm);
									ikm = NULL;
								}
								//Destroi chaves antigas e cria novas
								secure_zero(segredo, ECDH_SEC_LEN);
								if (chaveLocalPrivada != NULL)
								{
									Destroy_PrvKey(chaveLocalPrivada);
									chaveLocalPrivada = NULL;
								}
								chaveLocalPrivada = New_PrvKey();

								if (chaveLocalPublica != NULL)
								{
									Destroy_PubKey(chaveLocalPublica);
									chaveLocalPublica = NULL;
								}
								chaveLocalPublica = New_PubKey();

								// Cria novas chaves Locais Publicas e Privadas
								if(keygen_ec(BR_EC_secp256r1, chaveLocalPrivada, chaveLocalPublica) != 1){
									//Houve problema na geracao, apaga as chaves
									if (chaveLocalPublica != NULL)
									{
										Destroy_PubKey(chaveLocalPublica);
										chaveLocalPublica = NULL;
									}
									if (chaveLocalPrivada != NULL)
									{
										Destroy_PrvKey(chaveLocalPrivada);
										chaveLocalPrivada = NULL;
									}
								}
								contadorMensagens = 0;
							}
							else {
								//ETH
								QueueDados_t *q_dado = new_queue_dados(IFACE_ENET, PACKTYPE_SEND_ANSWER ,frameRespostaVetorizado, tamanhoRespostaVetorizada);
								if (txQueue != NULL && q_dado != NULL)
									xQueueSend(txQueue, &q_dado, 0);
							}
							break;
						}
						case TROCA_DADOS_SEGUROS_B5: //Troca de dados segura
						{
							uint8_t auxEnd[3] = {0};

							if(origem == IFACE_ENET)
							{
								PRINTF(" -PROCESS_TASK: Recebido TROCA_DADOS_SEGUROS_B5, TCP\r\n");
								contadorMensagensAnt = frameRecebido->iterador > contadorMensagensAnt ? frameRecebido->iterador : contadorMensagensAnt;
							}
							else
							{
								PRINTF(" -PROCESS_TASK: Recebido TROCA_DADOS_SEGUROS_B5, SERIAL\r\n");
								contadorMensagens = frameRecebido->iterador > contadorMensagens ? frameRecebido->iterador : contadorMensagens;
							}


							VetorizaIdDoControlador(DP40_CODIGO, DP40_REDE, DP40_AREA, auxEnd);

							if(frameRecebido->dados[0] == SOLICITA_DATA_E_HORA_86)
							{
								uint8_t dados[8] = {frameRecebido->dados[0], 0x81, 0x8F, 0xBB, 0x80, 0x81, 0x81, 0x87};

								if(origem == IFACE_ENET)
								{
									PRINTF(" -PROCESS_TASK: Recebido SOLICITA_DATA_E_HORA_86, TCP\r\n");
								}
								else
								{
									PRINTF(" -PROCESS_TASK: Recebido SOLICITA_DATA_E_HORA_86, SERIAL\r\n");
								}



								if(origem == IFACE_SERIAL)
								{
									frameResposta = ConstroiFrameQS(auxEnd, dados, sizeof(dados), &contadorMensagens, ikm, 0);
									frameRespostaVetorizado = VetorizaQuadro(frameResposta, &tamanhoRespostaVetorizada);
									UART_WriteBlocking(UART0, frameRespostaVetorizado, tamanhoRespostaVetorizada);
								}
								else
								{
									frameResposta = ConstroiFrameQS(auxEnd, dados, sizeof(dados), &contadorMensagensAnt, ikmAnt, 1);
									frameRespostaVetorizado = VetorizaQuadro(frameResposta, &tamanhoRespostaVetorizada);
									QueueDados_t *q_dado = new_queue_dados(IFACE_ENET, PACKTYPE_SEND_ANSWER,frameRespostaVetorizado, tamanhoRespostaVetorizada);
									if (txQueue != NULL && q_dado != NULL)
										xQueueSend(txQueue, &q_dado, 0);
								}
							}
							else
							{
								PRINTF(" -PROCESS_TASK: Recebido OUTRO QUADRO(CRIPTO), SERIAL\r\n");
							}
							break;
						}
						case TROCA_CHAVES_PUBLICA_B6: //Troca de chaves ECDH
						{
							uint8_t auxEnd[3] = {0};
							VetorizaIdDoControlador(DP40_CODIGO, DP40_REDE, DP40_AREA, auxEnd);

							uint8_t *dadosDecodificadosB64= pvPortMalloc(base64DecodedLength(frameRecebido->dados, frameRecebido->tamanho));
							if(dadosDecodificadosB64 == NULL)
								break;
							size_t tamPacoteDecodifcado64 = base64Decode(dadosDecodificadosB64, frameRecebido->dados, frameRecebido->tamanho);

							if(tamPacoteDecodifcado64 != 65)
							{
								destroy_obj((void **)&dadosDecodificadosB64, tamPacoteDecodifcado64);
								break;
							}

							if(origem == IFACE_ENET)
							{
								PRINTF(" -PROCESS_TASK: Recebido TROCA_CHAVES_PUBLICA_B6, TCP\r\n");

								if(chaveRemotaPublicaAnt != NULL)
								{
									Destroy_PubKey(chaveRemotaPublicaAnt);
									chaveRemotaPublicaAnt = NULL;
								}

								chaveRemotaPublicaAnt = New_PubKey(); //Aloca Espaco para nova chave publica remota
								if(chaveRemotaPublicaAnt == NULL)
									break;

								memcpy(chaveRemotaPublicaAnt->pbBuf, dadosDecodificadosB64, tamPacoteDecodifcado64); //Copia para o buffer da chave
								chaveRemotaPublicaAnt->pbKey.curve = BR_EC_secp256r1;
								chaveRemotaPublicaAnt->pbKey.qlen = sizeof(chaveRemotaPublicaAnt->pbBuf);
								destroy_obj((void **)&dadosDecodificadosB64, tamPacoteDecodifcado64);	 //Libera memoria da chave decodificada

								if(chaveLocalPrivadaAnt != NULL && chaveLocalPublicaAnt != NULL) // As chaves já existem
								{
									/* == Enviando a Chave Publica  == */

									//Codifica a chave Publica em B64
									size_t tamChavePublicaB64 = base64EncodedLength(chaveLocalPublicaAnt->pbKey.qlen);
									uint8_t *chavePublicaB64 = pvPortMalloc(tamChavePublicaB64);
									if(chavePublicaB64 == NULL) //Nao conseguiu alocar
										break;

									secure_zero(chavePublicaB64, tamChavePublicaB64);
									base64Encode(chavePublicaB64, chaveLocalPublicaAnt->pbKey.q, chaveLocalPublicaAnt->pbKey.qlen);


									DP_Frame_t *frameResposta = ConstroiFrameQNS(auxEnd, TROCA_CHAVES_PUBLICA_B6, chavePublicaB64, tamChavePublicaB64 ); //Constroi Quadro Nao Seguro
									destroy_obj((void **)&chavePublicaB64, tamChavePublicaB64); //Libera a memoria do objeto alocado para chaveB64


									frameRespostaVetorizado = VetorizaQuadro(frameResposta, &tamanhoRespostaVetorizada);
									if(!tamanhoRespostaVetorizada)
										break;

									if(!ComputeSharedSecret(&chaveLocalPrivadaAnt->pvKey, &chaveRemotaPublicaAnt->pbKey, segredoAnt)) //Gera o Segredo Compartilhado
									{
										// Produz IKM
										if(ikmAnt == NULL)
										{
											ikmAnt = pvPortMalloc(ECDH_SEC_LEN);
											if(ikmAnt == NULL)
											{
												break; //Nao conseguiu alocar ikm, entao nao responde
											}
										}
										if(!psk_ikm_get(DP40_CODIGO%10, (char *)segredoAnt, (char *)ikmAnt)) //Gera IKM com segredo e código do controlador #1
											break; //Nao conseguiu gerar IKM
									}
									QueueDados_t *q_dado = new_queue_dados(IFACE_ENET, PACKTYPE_SEND_ANSWER ,frameRespostaVetorizado, tamanhoRespostaVetorizada);
									if (txQueue != NULL && q_dado != NULL)
										xQueueSend(txQueue, &q_dado, 0);
								}

							}
							else
							{
								PRINTF(" -PROCESS_TASK: Recebido TROCA_CHAVES_PUBLICA_B6, SERIAL\r\n");

								if(chaveRemotaPublica != NULL)
								{
									Destroy_PubKey(chaveRemotaPublica);
									chaveRemotaPublica = NULL;
								}

								chaveRemotaPublica = New_PubKey(); //Aloca Espaco para nova chave publica remota
								if(chaveRemotaPublica == NULL)
									break;

								memcpy(chaveRemotaPublica->pbBuf, dadosDecodificadosB64, tamPacoteDecodifcado64); //Copia para o buffer da chave
								chaveRemotaPublica->pbKey.curve = BR_EC_secp256r1;
								chaveRemotaPublica->pbKey.qlen = sizeof(chaveRemotaPublica->pbBuf);
								destroy_obj((void **)&dadosDecodificadosB64, tamPacoteDecodifcado64);	 //Libera memoria da chave decodificada

								if(chaveLocalPrivada != NULL && chaveLocalPublica != NULL) // As chaves já existem
								{
									/* == Enviando a Chave Publica  == */

									//Codifica a chave Publica em B64
									size_t tamChavePublicaB64 = base64EncodedLength(chaveLocalPublica->pbKey.qlen);
									uint8_t *chavePublicaB64 = pvPortMalloc(tamChavePublicaB64);
									if(chavePublicaB64 == NULL) //Nao conseguiu alocar
										break;

									secure_zero(chavePublicaB64, tamChavePublicaB64);
									base64Encode(chavePublicaB64, chaveLocalPublica->pbKey.q, chaveLocalPublica->pbKey.qlen);


									DP_Frame_t *frameResposta = ConstroiFrameQNS(auxEnd, TROCA_CHAVES_PUBLICA_B6, chavePublicaB64, tamChavePublicaB64 ); //Constroi Quadro Nao Seguro
									destroy_obj((void **)&chavePublicaB64, tamChavePublicaB64); //Libera a memoria do objeto alocado para chaveB64


									frameRespostaVetorizado = VetorizaQuadro(frameResposta, &tamanhoRespostaVetorizada);
									if(!tamanhoRespostaVetorizada)
										break;

									if(!ComputeSharedSecret(&chaveLocalPrivada->pvKey, &chaveRemotaPublica->pbKey, segredo)) //Gera o Segredo Compartilhado
									{
										// Produz IKM
										if(ikm == NULL)
										{
											ikm = pvPortMalloc(ECDH_SEC_LEN);
											if(ikm == NULL)
											{
												break; //Nao conseguiu alocar ikm, entao nao responde
											}
										}
										if(!psk_ikm_get(0, (char *)segredo, (char *)ikm)) //Gera IKM com segredo e código do controlador #1
											break; //Nao conseguiu gerar IKM
									}
									UART_WriteBlocking(UART0, frameRespostaVetorizado, tamanhoRespostaVetorizada);

								}
							}
							break;
						}
						case SOLICITA_DATA_E_HORA_86:
						{
							if(origem == IFACE_ENET)
								PRINTF(" -PROCESS_TASK: Recebido SOLICITA_DATA_E_HORA_86, TCP\r\n");
							else
								PRINTF(" -PROCESS_TASK: Recebido SOLICITA_DATA_E_HORA_86, SERIAL\r\n");
							uint8_t auxEnd[3] = {0};
							VetorizaIdDoControlador(DP40_CODIGO, DP40_REDE, DP40_AREA, auxEnd);
							uint8_t dados[7] = {0x81, 0x8F, 0xBB, 0x80, 0x81, 0x81, 0x87};
							frameResposta = ConstroiFrameQNS(auxEnd, SOLICITA_DATA_E_HORA_86, dados, sizeof(dados));
							frameRespostaVetorizado = VetorizaQuadro(frameResposta, &tamanhoRespostaVetorizada);
							if(origem == IFACE_SERIAL)
								UART_WriteBlocking(UART0, frameRespostaVetorizado, tamanhoRespostaVetorizada);
							else
							{
								QueueDados_t *q_dado = new_queue_dados(IFACE_ENET, PACKTYPE_SEND_ANSWER ,frameRespostaVetorizado, tamanhoRespostaVetorizada);
								if (txQueue != NULL && q_dado != NULL)
									xQueueSend(txQueue, &q_dado, 0);
							}


							break;
						}
					}
				}

//				taskENTER_CRITICAL();

				DestrutorFrames(&frameResposta);
				DestrutorFrames(&frameRecebido);
				if(frameRespostaVetorizado != NULL)
					destroy_obj((void **)&frameRespostaVetorizado, tamanhoRespostaVetorizada);

//				taskEXIT_CRITICAL();
			}
			else
				PRINTF(" -PROCESS_TASK: Sem dados na fila UART!\r\n");

		}
    	vTaskDelay(pdMS_TO_TICKS(50));
    }
    vTaskSuspend(NULL); // Deleta task
}


/*******************************************************************************
 * ERRO Alocação de memória
 ******************************************************************************/


void vApplicationIdleHook(void)
{

}


void vApplicationMallocFailedHook(void)
{
    // Aqui você pode logar, sinalizar erro, reiniciar o sistema, etc.
    PRINTF(" -APP_MALLOC_FAILED_HOOK: ERRO - Falha na alocação de memória!\r\n");

    // Opcional: entrar em loop ou resetar
    for (;;)
    {
        // Pode piscar LED, enviar log, etc.
        asm("NOP");
    }
}


void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    // Aqui você pode logar, reiniciar, ou entrar em modo seguro
    printf("Stack overflow detectado na task: %s\n", pcTaskName);
    while (1); // ou resetar o sistema
}



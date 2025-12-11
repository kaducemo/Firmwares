/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <stdbool.h>



/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef void (*pFunction)(void);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define BOOT_ADDR   ((uint32_t)0x08000000)  // início da aplicação (Setor 4)

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void jump_to_application(void)
{
    __disable_irq();

    // 1) Parar e limpar SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    // 2) Limpar pendências de exceções
    SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;

    // 3) Desabilitar e limpar NVIC
    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    // 4) Limpar status de Faults
    SCB->CFSR = SCB->CFSR;
    SCB->HFSR = SCB->HFSR;
    SCB->DFSR = SCB->DFSR;
    SCB->SHCSR = 0;

    // 5) Limpar pendências EXTI (evita “flag latente”)
    EXTI->PR = 0x00FFFFFFU; // write-1-to-clear

    // ==============================================================
    // ALTERAÇÕES SUGERIDAS A PARTIR DAQUI
    // ==============================================================

    // 6.a) Resetar e liberar o SYSCFG (Importante para o MUX do EXTI)
    __HAL_RCC_SYSCFG_FORCE_RESET();
    __HAL_RCC_SYSCFG_RELEASE_RESET();

    // 6.b) Desabilitar CLOCKS de todos os GPIOs e periféricos do bootloader
    // Isso força a aplicação a reconfigurar tudo do zero.
    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();
    __HAL_RCC_GPIOD_CLK_DISABLE();
    __HAL_RCC_GPIOE_CLK_DISABLE();
    __HAL_RCC_GPIOF_CLK_DISABLE();
    __HAL_RCC_GPIOG_CLK_DISABLE();
    __HAL_RCC_GPIOH_CLK_DISABLE();
    // Adicione outros periféricos que o BOOTLOADER usou (UART, SPI, etc.) se necessário
    // Exemplo: __HAL_RCC_USART2_CLK_DISABLE();

    // 6.c) Deinit HAL/periféricos (função que você já tinha)
    HAL_DeInit();

    // ==============================================================
    // FIM DAS ALTERAÇÕES
    // ==============================================================


    // 7) Validar imagem da App
    uint32_t app_msp   = *(__IO uint32_t*)BOOT_ADDR;
    uint32_t app_reset = *(__IO uint32_t*)(BOOT_ADDR + 4);
    if ((app_msp & 0x2FF00000U) != 0x20000000U || (app_reset & 1U) == 0U) {
        // Validação falhou, resetar o sistema
        NVIC_SystemReset();
    }

    // 8) Garantir Thread Mode usando MSP (não PSP)
    __set_CONTROL(0);  // SPSEL=0, nPRIV=0
    __DSB(); __ISB();

    // 9) Zerar BASEPRI (se bootloader usou seções críticas)
    __set_BASEPRI(0);



    // 10) VTOR da app (Você já tinha isso corretamente)
    SCB->VTOR = BOOT_ADDR;    // alinhado a 0x200
    __DSB(); __ISB(); // Barreiras de memória para garantir a ordem

    // 11) MSP da app
    __set_MSP(app_msp);
    __DSB(); __ISB(); // Barreiras de memória



    // 12) Saltar para Reset_Handler da app (bit0=1 já vem correto)
    ((pFunction)app_reset)();

    // Se a aplicação retornar (o que não deveria), resetar
    NVIC_SystemReset();
}

bool flag_jump = false;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == shield_SwS1_Pin)
	{
		HAL_GPIO_TogglePin(shield_LedD2_GPIO_Port, shield_LedD2_Pin);
	}
	else if(GPIO_Pin == shield_SwS2_Pin)
	{
		HAL_GPIO_TogglePin(shield_LedD3_GPIO_Port, shield_LedD3_Pin);
	}
	else if(GPIO_Pin == shield_SwS3_Pin)
	{
		flag_jump = true;
	}
}


//static void dump_exti_nvic(void)
//{
//    uint32_t vtor     = SCB->VTOR;
//    uint32_t primask  = __get_PRIMASK();
//    uint32_t basepri  = __get_BASEPRI();
//    uint32_t aircr    = SCB->AIRCR; // prioridade grouping
//    uint32_t iser0    = NVIC->ISER[0]; // EXTI0..4 ficam no ISER[0] em F4
//    uint32_t imr      = EXTI->IMR;
//    uint32_t emr      = EXTI->EMR;
//    uint32_t rtsr     = EXTI->RTSR;
//    uint32_t ftsr     = EXTI->FTSR;
//    uint32_t pr       = EXTI->PR;
//#if defined(SYSCFG) && defined(SYSCFG_EXTICR1) && defined(SYSCFG_EXTICR2)
//    uint32_t exticr1  = SYSCFG->EXTICR[0]; // EXTI0..3
//    uint32_t exticr2  = SYSCFG->EXTICR[1]; // EXTI4..7
//#endif
//
//    // Troque por sua forma de log (semihosting/ITM/USART/LED patterns)
//    printf("VTOR=0x%08lX, PRIMASK=%lu, BASEPRI=%lu, AIRCR=0x%08lX\r\n",
//           vtor, primask, basepri, aircr);
//    printf("NVIC.ISER0=0x%08lX\r\n", iser0);
//    printf("EXTI: IMR=0x%08lX EMR=0x%08lX RTSR=0x%08lX FTSR=0x%08lX PR=0x%08lX\r\n",
//           imr, emr, rtsr, ftsr, pr);
//#if defined(SYSCFG) && defined(SYSCFG_EXTICR1) && defined(SYSCFG_EXTICR2)
//    printf("SYSCFG EXTICR1=0x%08lX EXTICR2=0x%08lX\r\n", exticr1, exticr2);
//#endif
//}


extern void EXTI0_IRQHandler(void);
extern void EXTI1_IRQHandler(void);
extern void EXTI4_IRQHandler(void);


//static void checar_vetor(void)
//{
//    uint32_t *vt = (uint32_t *)SCB->VTOR;         // já vimos que é 0x08010000
//    uint32_t vt_exti0 = vt[16 + EXTI0_IRQn];      // entrada do vetor para EXTI0
//    uint32_t vt_exti1 = vt[16 + EXTI1_IRQn];      // EXTI1
//    uint32_t vt_exti4 = vt[16 + EXTI4_IRQn];      // EXTI4
//
//    uintptr_t fn_exti0 = (uintptr_t)EXTI0_IRQHandler;
//    uintptr_t fn_exti1 = (uintptr_t)EXTI1_IRQHandler;
//    uintptr_t fn_exti4 = (uintptr_t)EXTI4_IRQHandler;
//
//    // Coloque breakpoints aqui e compare no debugger:
//    // - vt_extiX deve ser IGUAL a fn_extiX (ou diferir apenas no bit0 por thumb).
//    // - Se vt_extiX == 0 ou não bate com fn_extiX, a tabela está apontando para Default_Handler ou outro lugar.
//    (void)vt_exti0; (void)vt_exti1; (void)vt_exti4;
//    (void)fn_exti0; (void)fn_exti1; (void)fn_exti4;
//}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

int main(void)
{
//	uint32_t basepri_val = __get_BASEPRI();
//	uint32_t aircr_val = SCB->AIRCR;

  HAL_Init();

  // Herdado do bootloader: garanta ambiente previsível
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
//  __set_BASEPRI(0);
//  __HAL_RCC_SYSCFG_CLK_ENABLE();

  // (Opcional e recomendado) resetar blocos que o bootloader possa ter tocado
//  __HAL_RCC_SYSCFG_FORCE_RESET();  __HAL_RCC_SYSCFG_RELEASE_RESET();
//  __HAL_RCC_GPIOA_FORCE_RESET();   __HAL_RCC_GPIOA_RELEASE_RESET();
//  __HAL_RCC_GPIOB_FORCE_RESET();   __HAL_RCC_GPIOB_RELEASE_RESET();

  // Clocks do sistema
  SystemClock_Config();

  // Reabilite clocks de GPIO novamente (após o reset acima)
//  __HAL_RCC_GPIOA_CLK_ENABLE();
//  __HAL_RCC_GPIOB_CLK_ENABLE();

  // GPIO/EXTI (CubeMX)
  MX_GPIO_Init();

  // --- FORÇAR mapeamento SYSCFG->EXTICR para suas linhas ---
  // Ajuste os nibbles ao PORT real de cada chave:
  // 0 = PORTA, 1 = PORTB, 2 = PORTC, ...
  // (Ex.: S1=PA0, S2=PA1, S3=PA4)
//  SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~(0xF << 0)) | (0x0 << 0); // EXTI0 <- PA
//  SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~(0xF << 4)) | (0x0 << 4); // EXTI1 <- PA
//  SYSCFG->EXTICR[1] = (SYSCFG->EXTICR[1] & ~(0xF << 0)) | (0x0 << 0); // EXTI4 <- PA
  // Se S3 for PB4, use: (0x1 << 0) em EXTICR[1]

  // Limpeza de pendências e garantia de máscara/trigger
//  EXTI->PR   = (1U << 0) | (1U << 1) | (1U << 4);
//  EXTI->IMR |= (1U << 0) | (1U << 1) | (1U << 4);
//  EXTI->FTSR |= (1U << 0) | (1U << 1) | (1U << 4);  // IT_FALLING

  // Reabilitar interrupções globais (PRIMASK=0)
  __enable_irq();

//  uint32_t primask = __get_PRIMASK();
//  uint32_t faultmask = __get_FAULTMASK();
//  uint32_t basepri = __get_BASEPRI();


//  checar_vetor();

  // (Opcional) teste de disparo por software — deve chamar HAL_GPIO_EXTI_Callback()
//   EXTI->SWIER |= (1U << 0);
//   EXTI->SWIER |= (1U << 1);
//   EXTI->SWIER |= (1U << 4);

  	int i = 5;
	while (i)
	{

	 HAL_GPIO_TogglePin(shield_LedD4_GPIO_Port, shield_LedD4_Pin);
	 HAL_Delay(200);
	 HAL_GPIO_TogglePin(shield_LedD4_GPIO_Port, shield_LedD4_Pin);
	 HAL_Delay(200);
	 i--;
	  /* USER CODE END WHILE */

	  /* USER CODE BEGIN 3 */
	}

  while (1)
  {
	  if(flag_jump)
	  		  jump_to_application();
  }
}




//int main(void)
//{
//
//
//	int32_t ctrl = __get_CONTROL();
//	uint32_t msp  = __get_MSP();
//	uint32_t psp  = __get_PSP();
//
//  HAL_Init();
//
//  /* Ajuste de ambiente herdado do bootloader */
//  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
//  __set_BASEPRI(0);
//  __HAL_RCC_SYSCFG_CLK_ENABLE();
//
//  /* Clock do sistema */
//  SystemClock_Config();
//
//  /* GPIO/EXTI */
//  MX_GPIO_Init();
//
//
//  /* Limpeza de pendências EXTI antes de começar (linhas usadas) */
//  EXTI->PR = (1U << 0) | (1U << 1) | (1U << 4);
//
//  __enable_irq();
//
//  dump_exti_nvic();
//
//  HAL_GPIO_WritePin(shield_LedD4_GPIO_Port, shield_LedD4_Pin, 0);
//
//  while (1) {
//  }
//}

//int main(void)
//{
//  /* USER CODE BEGIN 1 */
//
//  /* USER CODE END 1 */
//
//  /* MCU Configuration--------------------------------------------------------*/
//
//  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
//  HAL_Init();
//
//  /* USER CODE BEGIN Init */
//
//  /* USER CODE END Init */
//
//  /* Configure the system clock */
//  SystemClock_Config();
//
//  /* USER CODE BEGIN SysInit */
//
//  /* USER CODE END SysInit */
//
//  /* Initialize all configured peripherals */
//  MX_GPIO_Init();
//  /* USER CODE BEGIN 2 */
//
//  /* USER CODE END 2 */
//
//  /* Infinite loop */
//  /* USER CODE BEGIN WHILE */
//  HAL_GPIO_WritePin(shield_LedD4_GPIO_Port, shield_LedD4_Pin, 0);
//  while (1)
//  {
//    /* USER CODE END WHILE */
//
//    /* USER CODE BEGIN 3 */
//  }
//  /* USER CODE END 3 */
//}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, shield_LedD2_Pin|shield_LedD3_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(shield_LedD4_GPIO_Port, shield_LedD4_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : shield_SwS1_Pin shield_SwS2_Pin */
  GPIO_InitStruct.Pin = shield_SwS1_Pin|shield_SwS2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : shield_LedD2_Pin shield_LedD3_Pin */
  GPIO_InitStruct.Pin = shield_LedD2_Pin|shield_LedD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : shield_SwS3_Pin */
  GPIO_InitStruct.Pin = shield_SwS3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(shield_SwS3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : shield_LedD4_Pin */
  GPIO_InitStruct.Pin = shield_LedD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(shield_LedD4_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

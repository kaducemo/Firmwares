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
#include "stm32f4xx_hal.h"
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

#define APP_ADDR   ((uint32_t)0x08010000)  // início da aplicação (Setor 4)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

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
    uint32_t app_msp   = *(__IO uint32_t*)APP_ADDR;
    uint32_t app_reset = *(__IO uint32_t*)(APP_ADDR + 4);
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
    SCB->VTOR = APP_ADDR;    // alinhado a 0x200
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
//		jump_to_application(APP_ADDR);
		flag_jump = true;
	}
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
  __enable_irq();       // <<< habilita PRIMASK=0

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  int i = 5;
  while (i)
  {

	 HAL_GPIO_TogglePin(shield_LedD2_GPIO_Port, shield_LedD2_Pin);
	 HAL_Delay(200);
	 HAL_GPIO_TogglePin(shield_LedD2_GPIO_Port, shield_LedD2_Pin);
	 HAL_Delay(200);
	 i--;

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }


  while(1)
  {
	  if(flag_jump)
		  jump_to_application();

  }
  /* USER CODE END 3 */
}

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(shield_LedD2_GPIO_Port, shield_LedD2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : shield_SwS1_Pin */
  GPIO_InitStruct.Pin = shield_SwS1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(shield_SwS1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : shield_LedD2_Pin */
  GPIO_InitStruct.Pin = shield_LedD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(shield_LedD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

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

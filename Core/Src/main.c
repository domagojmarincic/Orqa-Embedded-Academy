/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdint.h>
#include "spif.h"
#include "spif_tests.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
spif_handle_t spif;
static uint8_t tx_buf_dbg[16];
static uint8_t rx_buf_dbg[16];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
int _write(int file, char *ptr, int len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* printf() goes out over USART2 -> ST-Link Virtual COM Port, see _write() below */
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("\r\nORQA Embedded Academy - Task 2: SPI NOR flash driver\r\n");

  /* TODO Step 1: enable SPI2 and the chip select pin in CubeMX (see Docs/), regenerate,
   *              then uncomment the two blocks below. MX_SPI2_Init() is generated above.
   *
   */
  if (!spif_init(&spif, &hspi2, SPI2_CS_GPIO_Port, SPI2_CS_Pin))
  {
      printf("spif_init() failed\r\n");
      Error_Handler();
  }

  /* PRIVREMENO - debug test_bad_parameters */
  uint32_t dbg_sector_cnt = spif.block_cnt * (SPIF_BLOCK_SIZE / SPIF_SECTOR_SIZE);
  uint32_t dbg_page_cnt   = spif.block_cnt * (SPIF_BLOCK_SIZE / SPIF_PAGE_SIZE);

  printf("1: %d\r\n", spif_write_page(&spif, dbg_page_cnt, tx_buf_dbg, 16, 0));
  printf("2: %d\r\n", spif_read_page(&spif, dbg_page_cnt, rx_buf_dbg, 16, 0));
  printf("3: %d\r\n", spif_erase_sector(&spif, dbg_sector_cnt));
  printf("4: %d\r\n", spif_erase_block(&spif, spif.block_cnt));
  printf("5: %d\r\n", spif_write_page(&spif, 0, tx_buf_dbg, 16, SPIF_PAGE_SIZE));
  printf("6: %d\r\n", spif_read_sector(&spif, 0, rx_buf_dbg, 16, SPIF_SECTOR_SIZE));
  printf("7: %d\r\n", spif_read_block(&spif, 0, rx_buf_dbg, 16, SPIF_BLOCK_SIZE));
  printf("8: %d\r\n", spif_read_address(&spif, spif.total_size - 4, rx_buf_dbg, 16));
  printf("9: %d\r\n", spif_write_address(&spif, 0, NULL, 16));
  printf("10: %d\r\n", spif_read_address(&spif, 0, rx_buf_dbg, 0));
  printf("11: %d\r\n", spif_erase_chip(NULL));
  /* KRAJ PRIVREMENOG */

  if (spif_test_run(&hspi2, SPI2_CS_GPIO_Port, SPI2_CS_Pin))
  {
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
  }

  if (!spif_init(&spif, &hspi2, SPI2_CS_GPIO_Port, SPI2_CS_Pin))
  {
    printf("spif_init() failed\r\n");
    Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  /*
    HAL_GPIO_TogglePin(test_led_GPIO_Port, test_led_Pin);
	HAL_Delay(500);
*/
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* Overrides the weak _write() in syscalls.c, called by printf() and friends */
int _write(int file, char *ptr, int len)
{
  (void)file;

  if (HAL_UART_Transmit(&huart2, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY) != HAL_OK)
  {
    return -1;
  }
  return len;
}

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
#ifdef USE_FULL_ASSERT
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

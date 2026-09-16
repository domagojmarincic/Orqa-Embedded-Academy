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
#include "app_fatfs.h"
#include "spi.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdint.h>
#include "spif.h"
#include "spif_tests.h"

#include "usbd_core.h"
#include "usb_host.h"
#include "usbh_cdc.h"

#include "app_fatfs.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
extern USBH_HandleTypeDef hUsbHostFS;
extern USBD_HandleTypeDef hUsbDeviceFS;
volatile uint8_t current_mode = 1;

extern HCD_HandleTypeDef hhcd_USB_DRD_FS;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
spif_handle_t spif;

static uint8_t cdc_sent = 0;
static uint32_t cdc_ready_tick = 0;
extern ApplicationTypeDef Appli_state;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
int _write(int file, char *ptr, int len);
void switch_to_device(void);
void switch_to_host(void);
void MX_USB_HOST_Process(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void switch_to_device(void)
{
  if(current_mode == 1)
  {
    return;
  }

  USBH_DeInit(&hUsbHostFS);
  HAL_HCD_DeInit(&hhcd_USB_DRD_FS);
  HAL_GPIO_WritePin(VBUS_EN_GPIO_Port, VBUS_EN_Pin, GPIO_PIN_RESET);

  current_mode = 1;
  HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
  MX_USB_Device_Init();
}

void switch_to_host(void)
{
  if(current_mode == 2)
  {
    return;
  }

  USBD_DeInit(&hUsbDeviceFS);
  HAL_GPIO_WritePin(VBUS_EN_GPIO_Port, VBUS_EN_Pin, GPIO_PIN_SET);

  current_mode = 2;
  HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
  MX_USB_Host_Init();
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

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART2_UART_Init();
  MX_USB_Device_Init();
  if (MX_FATFS_Init() != APP_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN 2 */
  /* printf() goes out over USART2 -> ST-Link Virtual COM Port, see _write() below */

  setvbuf(stdout, NULL, _IONBF, 0);
  printf("\r\nORQA Embedded Academy - Dual Role USB (CDC Host)\r\n");

  if (!spif_init(&spif, &hspi2, SPI2_CS_GPIO_Port, SPI2_CS_Pin))
  {
      printf("spif_init() failed\r\n");
      Error_Handler();
  }

  FRESULT fres = f_mount(&USERFatFs, USERPath, 1);
  if (fres != FR_OK)
  {
      printf("f_mount failed: %d\r\n", fres);
  }
  else
  {
      printf("f_mount OK!\r\n");

      FATFS *pfs;
      DWORD free_clusters;
      if (f_getfree(USERPath, &free_clusters, &pfs) == FR_OK)
      {
          uint32_t total_sectors = (pfs->n_fatent - 2) * pfs->csize;
          uint32_t free_sectors = free_clusters * pfs->csize;
          printf("Total: %lu KB, Free: %lu KB\r\n",
                 (unsigned long)(total_sectors / 2), (unsigned long)(free_sectors / 2));
      }

      FIL testFile;
      FRESULT fres_open = f_open(&testFile, "test.txt", FA_READ);
      if (fres_open != FR_OK)
      {
          printf("f_open failed: %d\r\n", fres_open);
      }
      else
      {
          char buffer[128];
          UINT bytesRead;
          FRESULT fres_read = f_read(&testFile, buffer, sizeof(buffer) - 1, &bytesRead);
          if (fres_read == FR_OK)
          {
              buffer[bytesRead] = '\0';
              printf("Procitano %u bajtova: %s\r\n", bytesRead, buffer);
          }
          else
          {
              printf("f_read failed: %d\r\n", fres_read);
          }
          f_close(&testFile);
      }
  }

  HAL_GPIO_WritePin(VBUS_EN_GPIO_Port, VBUS_EN_Pin, GPIO_PIN_RESET);
  current_mode = 1;
  HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  if (HAL_GPIO_ReadPin(SW_1_GPIO_Port, SW_1_Pin) == GPIO_PIN_RESET)
	  {
	    HAL_Delay(200);
	    switch_to_device();
	  }
	  else if (HAL_GPIO_ReadPin(SW_2_GPIO_Port, SW_2_Pin) == GPIO_PIN_RESET)
	  {
	    HAL_Delay(200);
	    switch_to_host();
	  }

	  if (current_mode == 2)
	  {
		  MX_USB_HOST_Process();

		  if (Appli_state == APPLICATION_READY && !cdc_sent)
		  {
			  if (cdc_ready_tick == 0)
			  {
				  cdc_ready_tick = HAL_GetTick();
			  }
			  else if (HAL_GetTick() - cdc_ready_tick >= 500)
			  {
				  uint8_t msg[] = "Hello from CDC Host\r\n";
				  USBH_CDC_Transmit(&hUsbHostFS, msg, sizeof(msg) - 1);
				  cdc_sent = 1;
			  }
		  }
	  }
	  if (Appli_state != APPLICATION_READY)
	  {
	    cdc_sent = 0;
	    cdc_ready_tick = 0;
	  }
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
  RCC_CRSInitTypeDef pInit = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the SYSCFG APB clock
  */
  __HAL_RCC_CRS_CLK_ENABLE();

  /** Configures CRS
  */
  pInit.Prescaler = RCC_CRS_SYNC_DIV1;
  pInit.Source = RCC_CRS_SYNC_SOURCE_USB;
  pInit.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  pInit.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000,1000);
  pInit.ErrorLimitValue = 34;
  pInit.HSI48CalibrationValue = 32;

  HAL_RCCEx_CRSConfig(&pInit);
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

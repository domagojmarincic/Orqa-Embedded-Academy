#include "App.h"
#include "main.h"
#include "mode_switch.h"
#include "protocol_host.h"
#include "app_fatfs.h"
#include "spif.h"
#include "spi.h"
#include "usb_host.h"
#include <stdio.h>

spif_handle_t spif;
static uint8_t transfer_started = 0;

extern ApplicationTypeDef Appli_state;

void App_Init(void)
{
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
    }

    HAL_GPIO_WritePin(VBUS_EN_GPIO_Port, VBUS_EN_Pin, GPIO_PIN_RESET);
    current_mode = DEVICE_MODE;
    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
}

void App_CheckButtons(void)
{
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
}

void App_Task(void)
{
    if (current_mode != HOST_MODE)
    {
        return;
    }

    MX_USB_Host_Process();

    static uint32_t ready_tick = 0;

    if (Appli_state != APPLICATION_READY)
    {
        transfer_started = 0;
        ready_tick = 0;
        return;
    }

    if (!transfer_started)
    {
        if (ready_tick == 0)
        {
            ready_tick = HAL_GetTick();
        }
        else if (HAL_GetTick() - ready_tick >= 500)
        {
            ProtocolHost_Init();
            ProtocolHost_StartTransfer();
            transfer_started = 1;
        }
    }
    else
    {
        ProtocolHost_Process();
    }
}

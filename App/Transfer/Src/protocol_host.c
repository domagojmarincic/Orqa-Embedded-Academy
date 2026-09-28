#include "protocol_host.h"
#include "usbh_cdc.h"
#include "usb_host.h"
#include "app_fatfs.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

extern USBH_HandleTypeDef hUsbHostFS;
extern ApplicationTypeDef Appli_state;

#define FIRMWARE_FILENAME   "snake_academy.bin"

static FIL file;
static uint8_t buffer[1 + HOST_PACKET_SIZE + 2];
static uint32_t buffer_len;
static volatile bool file_finished;
static volatile bool transfer_abort = false;

static uint8_t rx_buf[4];
static uint8_t cmd_buf[1];
static volatile bool response_ready = false;

osSemaphoreId_t xDataReadySem;
osSemaphoreId_t xBufferFreeSem;

static uint16_t CalculateChecksum(const uint8_t *data, uint32_t length);
static bool SendAndWaitAck(uint8_t *data, uint32_t len);

void ProtocolHost_RTOS_Init(void)
{
    xDataReadySem = osSemaphoreNew(1, 0, NULL);
    xBufferFreeSem = osSemaphoreNew(1, 0, NULL);
}

void USBH_CDC_ReceiveCallback(USBH_HandleTypeDef *phost)
{
    response_ready = true;
}

static bool SendAndWaitAck(uint8_t *data, uint32_t len)
{
    for (int retry = 0; retry < 5; retry++)
    {
    	if (Appli_state != APPLICATION_READY)
		{
			return false;
		}

        response_ready = false;
        USBH_CDC_Receive(&hUsbHostFS, rx_buf, 1);
        USBH_CDC_Transmit(&hUsbHostFS, data, len);

        uint32_t start = HAL_GetTick();
        while (!response_ready)
        {
            if (Appli_state != APPLICATION_READY || HAL_GetTick() - start > 2000)
            {
                return false;
            }
            osDelay(1);
        }

        if (rx_buf[0] == HOST_CMD_ACK)
        {
            return true;
        }
    }
    return false;
}

bool ProtocolHost_StartTransfer(void)
{
	FRESULT mres = f_mount(&USERFatFs, USERPath, 1);
	if (mres != FR_OK)
	{
	    printf("f_mount GRESKA: %d\r\n", mres);
	    return false;
	}

	FRESULT res = f_open(&file, FIRMWARE_FILENAME, FA_READ);
	if (res != FR_OK)
	{
		printf("f_open GRESKA: %d\r\n", res);
		return false;
	}
	file_finished = false;
	transfer_abort = false;
	return true;
}

void vReaderTask(void *pv)
{
    for(;;)
    {
        osSemaphoreAcquire(xBufferFreeSem, osWaitForever);
        if (transfer_abort)
        {
            f_close(&file);
            vTaskDelete(NULL);
        }

        UINT bytes_read = 0;
        f_read(&file, &buffer[1], HOST_PACKET_SIZE, &bytes_read);

        if (bytes_read == 0)
        {
            file_finished = true;
            f_close(&file);
        }
        else
        {
            if (bytes_read < HOST_PACKET_SIZE)
            {
                memset(&buffer[1 + bytes_read], 0xFF, HOST_PACKET_SIZE - bytes_read);
            }
            uint16_t crc = CalculateChecksum(&buffer[1], HOST_PACKET_SIZE);
            buffer[1 + HOST_PACKET_SIZE] = (uint8_t)(crc & 0xFF);
            buffer[1 + HOST_PACKET_SIZE + 1] = (uint8_t)(crc >> 8);
            buffer[0] = HOST_CMD_DATA_BLOCK_WITH_CRC;
            buffer_len = 1 + HOST_PACKET_SIZE + 2;
        }

        osSemaphoreRelease(xDataReadySem);

        if (file_finished)
        {
            vTaskDelete(NULL);
        }
    }
}

void vSenderTask(void *pv)
{
    cmd_buf[0] = HOST_CMD_SET_PACKET_SIZE;
    bool ok = SendAndWaitAck(cmd_buf, 1);
    if (ok)
    {
        cmd_buf[0] = HOST_CMD_START_TRANSFER;
        ok = SendAndWaitAck(cmd_buf, 1);
    }
    if (!ok)
    {
        printf("Handshake neuspjesan\r\n");
        f_close(&file);
        vTaskDelete(NULL);
    }

    osSemaphoreRelease(xBufferFreeSem);

    for(;;)
    {
        osSemaphoreAcquire(xDataReadySem, osWaitForever);

        if (file_finished)
        {
            cmd_buf[0] = HOST_CMD_END_TRANSFER;
            USBH_CDC_Transmit(&hUsbHostFS, cmd_buf, 1);
            printf("END_TRANSFER poslan\r\n");
            vTaskDelete(NULL);
        }

        if (!SendAndWaitAck(buffer, buffer_len))
        {
            printf("Transfer prekinut: nema ACK-a\r\n");
            transfer_abort = true;
            osSemaphoreRelease(xBufferFreeSem);
            vTaskDelete(NULL);
        }
        osSemaphoreRelease(xBufferFreeSem);
    }
}

static uint16_t CalculateChecksum(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}

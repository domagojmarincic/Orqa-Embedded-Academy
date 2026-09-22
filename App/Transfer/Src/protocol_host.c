#include "protocol_host.h"
#include "usbh_cdc.h"
#include "usb_host.h"
#include "app_fatfs.h"
#include <string.h>
#include <stdio.h>

extern USBH_HandleTypeDef hUsbHostFS;

#define FIRMWARE_FILENAME   "snake_academy.bin"

typedef enum {
    H_IDLE = 0,
    H_SEND_SET_PACKET_SIZE,
    H_SEND_START_TRANSFER,
    H_READ_CHUNK,
    H_SEND_DATA_BLOCK,
    H_WAIT_ACK,
    H_SEND_END_TRANSFER,
    H_DONE,
    H_ERROR,
} HostState_t;

static HostState_t state = H_IDLE;
static HostState_t next_state_after_ack;

static FIL file;
static uint8_t send_buf[1 + HOST_PACKET_SIZE + 2];
static uint8_t rx_buf[4];
static volatile bool response_ready = false;

/* --- Forward deklaracije privatnih funkcija --- */
static void SendSetPacketSize(void);
static void SendStartTransfer(void);
static void ReadChunk(void);
static void SendDataBlock(void);
static void WaitForAck(void);
static void SendEndTransfer(void);
static uint16_t CalculateChecksum(const uint8_t *data, uint32_t length);

/* --- Javne funkcije --- */

void ProtocolHost_Init(void)
{
    state = H_IDLE;
    response_ready = false;
}

void ProtocolHost_StartTransfer(void)
{
    printf("ProtocolHost_StartTransfer pozvana\r\n");   /* DEBUG */

    FRESULT res = f_open(&file, FIRMWARE_FILENAME, FA_READ);
    if (res != FR_OK)
    {
        printf("f_open GRESKA: %d (fajl: %s)\r\n", res, FIRMWARE_FILENAME);   /* DODANO */
        state = H_ERROR;
        return;
    }

    printf("Otvoren %s, pocinjem prijenos\r\n", FIRMWARE_FILENAME);
    state = H_SEND_SET_PACKET_SIZE;
}

void ProtocolHost_Process(void)
{
    switch (state)
    {
    case H_IDLE:
        break;

    case H_SEND_SET_PACKET_SIZE:
        SendSetPacketSize();
        break;

    case H_SEND_START_TRANSFER:
        SendStartTransfer();
        break;

    case H_READ_CHUNK:
        ReadChunk();
        break;

    case H_SEND_DATA_BLOCK:
        SendDataBlock();
        break;

    case H_WAIT_ACK:
        WaitForAck();
        break;

    case H_SEND_END_TRANSFER:
        SendEndTransfer();
        break;

    case H_DONE:
    case H_ERROR:
        break;
    }
}

bool ProtocolHost_IsDone(void)
{
    return (state == H_DONE);
}

/* USBH_CDC middleware poziva OVO automatski kad stigne odgovor */
void USBH_CDC_ReceiveCallback(USBH_HandleTypeDef *phost)
{
    response_ready = true;
}

/* --- Privatne funkcije, jedna po korak --- */

static void SendSetPacketSize(void)
{
    printf("SendSetPacketSize pozvana\r\n");

    send_buf[0] = HOST_CMD_SET_PACKET_SIZE;
    response_ready = false;
    USBH_CDC_Receive(&hUsbHostFS, rx_buf, 1);
    USBH_CDC_Transmit(&hUsbHostFS, send_buf, 1);

    next_state_after_ack = H_SEND_START_TRANSFER;
    state = H_WAIT_ACK;
}

static void SendStartTransfer(void)
{
    printf("SendStartTransfer pozvana\r\n");

    send_buf[0] = HOST_CMD_START_TRANSFER;
    response_ready = false;
    USBH_CDC_Receive(&hUsbHostFS, rx_buf, 1);
    USBH_CDC_Transmit(&hUsbHostFS, send_buf, 1);

    next_state_after_ack = H_READ_CHUNK;
    state = H_WAIT_ACK;
}

static void ReadChunk(void)
{
    printf("Read Chunk\r\n");

    UINT bytes_read = 0;
    FRESULT res = f_read(&file, &send_buf[1], HOST_PACKET_SIZE, &bytes_read);

    if (res != FR_OK)
    {
        printf("Greska pri citanju fajla\r\n");
        state = H_ERROR;
        return;
    }

    if (bytes_read == 0)
    {
        f_close(&file);
        state = H_SEND_END_TRANSFER;
        return;
    }

    if (bytes_read < HOST_PACKET_SIZE)
    {
        memset(&send_buf[1 + bytes_read], 0xFF, HOST_PACKET_SIZE - bytes_read);
    }

    uint16_t crc = CalculateChecksum(&send_buf[1], HOST_PACKET_SIZE);
    send_buf[1 + HOST_PACKET_SIZE] = (uint8_t)(crc & 0xFF);
    send_buf[1 + HOST_PACKET_SIZE + 1] = (uint8_t)(crc >> 8);
    send_buf[0] = HOST_CMD_DATA_BLOCK_WITH_CRC;

    state = H_SEND_DATA_BLOCK;
}

static void SendDataBlock(void)
{
    printf("SendDataBlock pozvana\r\n");

    response_ready = false;
    USBH_CDC_Receive(&hUsbHostFS, rx_buf, 1);
    USBH_CDC_Transmit(&hUsbHostFS, send_buf, 1 + HOST_PACKET_SIZE + 2);

    next_state_after_ack = H_READ_CHUNK;
    state = H_WAIT_ACK;
}

static void WaitForAck(void)
{
    if (!response_ready)
    {
        return;
    }

    printf("WaitForAck - odgovor stigao: 0x%02X\r\n", rx_buf[0]);

    response_ready = false;

    if (rx_buf[0] == HOST_CMD_ACK)
    {
        state = next_state_after_ack;
    }
    else
    {
        state = H_SEND_DATA_BLOCK;
    }
}

static void SendEndTransfer(void)
{
    send_buf[0] = HOST_CMD_END_TRANSFER;
    USBH_CDC_Transmit(&hUsbHostFS, send_buf, 1);
    printf("END_TRANSFER poslan\r\n");
    state = H_DONE;
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

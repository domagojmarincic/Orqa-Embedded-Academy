#include "mode_switch.h"
#include "main.h"
#include "usb_device.h"
#include "usbh_core.h"
#include "usbd_core.h"
#include "usb_host.h"

extern USBH_HandleTypeDef hUsbHostFS;
extern USBD_HandleTypeDef hUsbDeviceFS;
extern HCD_HandleTypeDef hhcd_USB_DRD_FS;

volatile uint8_t current_mode = DEVICE_MODE;

void switch_to_device(void)
{
    if (current_mode == DEVICE_MODE)
    {
        return;
    }

    USBH_DeInit(&hUsbHostFS);
    HAL_HCD_DeInit(&hhcd_USB_DRD_FS);
    HAL_GPIO_WritePin(VBUS_EN_GPIO_Port, VBUS_EN_Pin, GPIO_PIN_RESET);

    current_mode = DEVICE_MODE;
    HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
    MX_USB_Device_Init();
}

void switch_to_host(void)
{
    if (current_mode == HOST_MODE)
    {
        return;
    }

    USBD_DeInit(&hUsbDeviceFS);
    HAL_GPIO_WritePin(VBUS_EN_GPIO_Port, VBUS_EN_Pin, GPIO_PIN_SET);

    current_mode = HOST_MODE;
    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
    MX_USB_Host_Init();
}

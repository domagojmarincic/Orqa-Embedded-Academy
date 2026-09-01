/**
 * @file spif_commander.c
 * @author ORQA Embedded Academy
 * @brief Look at spif_commander.h
 *
 * @note Every function is a stub. This is the layer that actually touches the bus,
 *       so this is where the logic analyzer earns its keep.
 */

/*------------------------------------------------------ INCLUDES -----------------------------------------------------*/
#include "spif_commander.h"

/*----------------------------------------------- PRIVATE MACROS / DEFINES --------------------------------------------*/

/*--------------------------------------------- PRIVATE VARIABLES (STATIC) --------------------------------------------*/

/*------------------------------------------- PUBLIC FUNCTION IMPLEMENTATIONS -----------------------------------------*/

void spif_commander_cs(spif_handle_t *handle, bool select)
{
    if(select == true)
    {
      handle->gpio->BSRR = (uint32_t)(handle->pin) << 16;
    }
    else
    {
      handle->gpio->BSRR = (uint32_t)(handle->pin);
    }
    /* TODO */
}

bool spif_commander_transmit(spif_handle_t *handle, const uint8_t *tx, uint32_t size, uint32_t timeout)
{
	//sve pokazivače treba provjeriti
    HAL_StatusTypeDef status = HAL_SPI_Transmit(handle->hspi, (uint8_t *)tx, (uint16_t)size, timeout);
    return (status == HAL_OK);
}

bool spif_commander_receive(spif_handle_t *handle, uint8_t *rx, uint32_t size, uint32_t timeout)
{
	HAL_StatusTypeDef status = HAL_SPI_Receive(handle->hspi, rx, (uint16_t)size, timeout);
	return (status == HAL_OK);
}

bool spif_commander_command(spif_handle_t *handle, spif_cmd_t cmd)
{
	uint8_t cmd_byte = (uint8_t)cmd;
	bool ret_val = false;
	spif_commander_cs(handle, true);
	ret_val = spif_commander_transmit(handle, &cmd_byte, 1, SPIF_TIMEOUT_CMD);
	spif_commander_cs(handle, false);

	return ret_val;
}

bool spif_commander_command_address(spif_handle_t *handle, spif_cmd_t cmd_3addr, spif_cmd_t cmd_4addr,
                                    uint32_t address)
{
    (void)handle;
    (void)cmd_3addr;
    (void)cmd_4addr;
    (void)address;
    return false; /* TODO */
}

uint8_t spif_commander_read_status(spif_handle_t *handle)
{
	uint8_t cmd_byte = (uint8_t)SPIF_CMD_READ_STATUS1;
	uint8_t status = 0xFF;

	spif_commander_cs(handle, true);
	spif_commander_transmit(handle, &cmd_byte, 1, SPIF_TIMEOUT_CMD);
	spif_commander_receive(handle, &status, 1, SPIF_TIMEOUT_CMD);
	spif_commander_cs(handle, false);

	return status;
}

bool spif_commander_wait_for_writing(spif_handle_t *handle, uint32_t timeout)
{
	uint32_t start_tick = HAL_GetTick();

	    while (spif_commander_read_status(handle) & SPIF_STATUS1_BUSY)
	    {
	        if ((HAL_GetTick() - start_tick) >= timeout)
	        {
	            return false;
	        }
	    }

	    return true;
}

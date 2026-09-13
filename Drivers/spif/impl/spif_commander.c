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

#define SPIF_BSRR_RESET_OFFSET   16U

/*--------------------------------------------- PRIVATE VARIABLES (STATIC) --------------------------------------------*/

/*------------------------------------------- PUBLIC FUNCTION IMPLEMENTATIONS -----------------------------------------*/

void spif_commander_cs(spif_handle_t *handle, bool select)
{
    if(select == true)
    {
      handle->gpio->BSRR = (uint32_t)(handle->pin) << SPIF_BSRR_RESET_OFFSET;
    }
    else
    {
      handle->gpio->BSRR = (uint32_t)(handle->pin);
    }
}

bool spif_commander_transmit(spif_handle_t *handle, const uint8_t *tx, uint32_t size, uint32_t timeout)
{
	if(handle == NULL || tx == NULL)
	{
	  return false;
	}
	else if(size > UINT16_MAX)
	{
		return false;
	}
    HAL_StatusTypeDef status = HAL_SPI_Transmit(handle->hspi, (uint8_t *)tx, (uint16_t)size, timeout);
    return (status == HAL_OK);
}

bool spif_commander_receive(spif_handle_t *handle, uint8_t *rx, uint32_t size, uint32_t timeout)
{
	if(handle == NULL || rx == NULL)
	{
	  return false;
	}
	HAL_StatusTypeDef status = HAL_SPI_Receive(handle->hspi, rx, (uint16_t)size, timeout);
	return (status == HAL_OK);
}

bool spif_commander_command(spif_handle_t *handle, spif_cmd_t cmd)
{
	if(handle == NULL)
	{
	  return false;
	}
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
	if(handle == NULL)
	{
	  return false;
	}

	bool use_4addr = (address > SPIF_MAX_3ADDR_ADDRESS);

	uint8_t cmd_byte = use_4addr ? (uint8_t)cmd_4addr : (uint8_t)cmd_3addr;

	if(!spif_commander_transmit(handle, &cmd_byte, 1, SPIF_TIMEOUT_CMD))
	{
	  return false;
	}

	uint8_t addr_bytes[4];
	uint32_t addr_size;

	if(use_4addr)
	{
	  addr_bytes[0] = (uint8_t)(address >> 24);
	  addr_bytes[1] = (uint8_t)(address >> 16);
	  addr_bytes[2] = (uint8_t)(address >> 8);
	  addr_bytes[3] = (uint8_t)(address);
	  addr_size = 4;
	}
	else
	{
	  addr_bytes[0] = (uint8_t)(address >> 16);
	  addr_bytes[1] = (uint8_t)(address >> 8);
	  addr_bytes[2] = (uint8_t)(address);
	  addr_size = 3;
	}

	return spif_commander_transmit(handle, addr_bytes, addr_size, SPIF_TIMEOUT_CMD);
}

uint8_t spif_commander_read_status(spif_handle_t *handle)
{
	if(handle == NULL)
	{
	  return 0xFF;
	}

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
	if(handle == NULL)
	{
	  return false;
	}

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

/**
 * @file spif.c
 * @author ORQA Embedded Academy
 * @brief Look at spif.h
 *
 * @note Every function is a stub, so the whole suite starts red. Build the public API on
 *       top of spif_commander and spif_utils - add private static helpers here if you
 *       need them, but keep bus access in the commander and pure logic in the utils.
 *
 * @note Every public call takes the guard in the handle before touching the bus and
 *       releases it on every exit path, including the failing ones. SPIF_USE_RTOS is 0 for
 *       now, so only the bare-metal branch is built and tested this week - keep the take
 *       and release in one place each so Week 4 can switch the mutex on without a rewrite.
 */

/*------------------------------------------------------ INCLUDES -----------------------------------------------------*/
#include "spif.h"
#include "spif_commander.h"
#include "spif_utils.h"

/*----------------------------------------------- PRIVATE MACROS / DEFINES --------------------------------------------*/

/*--------------------------------------------- PRIVATE VARIABLES (STATIC) --------------------------------------------*/

/*------------------------------------------- PUBLIC FUNCTION IMPLEMENTATIONS -----------------------------------------*/

bool spif_init(spif_handle_t *handle, SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio, uint16_t pin)
{
	uint8_t JEDEC[SPIF_JEDEC_ID_SIZE] = {0};
	uint8_t cmd_byte = (uint8_t)SPIF_CMD_JEDEC_ID;
	bool ret_val = false;

	if(handle == NULL || hspi == NULL || gpio == NULL)
	{
	  return false;
	}
	else
	{
	  handle->hspi = hspi;
	  handle->gpio = gpio;
	  handle->pin = pin;

	  spif_commander_cs(handle, true);
	  ret_val = spif_commander_transmit(handle, &cmd_byte, 1, SPIF_TIMEOUT_CMD);
	  if(ret_val)
	  {
		ret_val = spif_commander_receive(handle, JEDEC, SPIF_JEDEC_ID_SIZE, SPIF_TIMEOUT_CMD);
	  }
	  spif_commander_cs(handle, false);

	  if(!ret_val)
	  {
		return false;
	  }

	  handle->manufactor = (JEDEC[0] == SPIF_MANUFACTOR_WINBOND) ? SPIF_MANUFACTOR_WINBOND : SPIF_MANUFACTOR_ERROR;

	  if (handle->manufactor == SPIF_MANUFACTOR_ERROR)
	  {
	    return false;
	  }

	  handle->mem_type = JEDEC[1];
	  handle->capacity = JEDEC[2];

	  if(!spif_utils_capacity_to_block_count(handle->capacity, &handle->block_cnt))
	  {
	    return false;
	  }

	  handle->total_size = handle->block_cnt * SPIF_BLOCK_SIZE;
	  handle->sector_cnt = handle->total_size / SPIF_SECTOR_SIZE;
	  handle->page_cnt = handle->total_size / SPIF_PAGE_SIZE;

	  handle->inited = true;

	}
	return true;
}

bool spif_erase_chip(spif_handle_t *handle)
{
    if(!spif_utils_is_ready(handle))
    {
      return false;
    }
    else if(!spif_commander_command(handle, SPIF_CMD_WRITE_ENABLE))
    {
      return false;
    }
    else if(!spif_commander_command(handle, SPIF_CMD_CHIP_ERASE))
    {
      return false;
    }
    else if(!spif_commander_wait_for_writing(handle, SPIF_TIMEOUT_CHIP_ERASE))
    {
      return false;
    }
    return true;
}

bool spif_erase_sector(spif_handle_t *handle, uint32_t sector)
{
	bool ret_val;
	uint32_t address = sector * SPIF_SECTOR_SIZE;

	if(!spif_utils_is_ready(handle))
	{
	  return false;
	}
	else if(!spif_utils_check_range(handle, address, SPIF_SECTOR_SIZE))
	{
	  return false;
	}
	else if(!spif_commander_command(handle, SPIF_CMD_WRITE_ENABLE))
	{
	  return false;
	}

	spif_commander_cs(handle, true);
	ret_val = spif_commander_command_address(handle, SPIF_CMD_SECTOR_ERASE_3ADDR, SPIF_CMD_SECTOR_ERASE_4ADDR, address);
	spif_commander_cs(handle, false);

	if(!ret_val)
	{
	  return false;
	}
	else if(!spif_commander_wait_for_writing(handle, SPIF_TIMEOUT_SECTOR_ERASE))
	{
	  return false;
	}
	return true;
}

bool spif_erase_block(spif_handle_t *handle, uint32_t block)
{
	bool ret_val = false;
	uint32_t address = block * SPIF_BLOCK_SIZE;

	if(!spif_utils_is_ready(handle))
	{
	  return false;
	}
	else if(!spif_utils_check_range(handle, address, SPIF_BLOCK_SIZE))
	{
	  return false;
	}
	else if(!spif_commander_command(handle, SPIF_CMD_WRITE_ENABLE))
	{
	  return false;
	}

	spif_commander_cs(handle, true);
	ret_val = spif_commander_command_address(handle, SPIF_CMD_BLOCK_ERASE_3ADDR, SPIF_CMD_BLOCK_ERASE_4ADDR, address);
	spif_commander_cs(handle, false);

	if(!ret_val)
	{
	  return false;
	}
	else if(!spif_commander_wait_for_writing(handle, SPIF_TIMEOUT_BLOCK_ERASE))
	{
	  return false;
	}
	return true;
}

bool spif_write_address(spif_handle_t *handle, uint32_t address, const uint8_t *data, uint32_t size)
{
	if(data == NULL)
	{
	  return false;
	}

	if(!spif_utils_check_range(handle, address, size))
	{
	  return false;
	}

	uint32_t written = 0;

	while(written < size)
	{
	  uint32_t current      = address + written;
	  uint32_t page_index   = current / SPIF_PAGE_SIZE;
	  uint32_t page_offset  = current % SPIF_PAGE_SIZE;
	  uint32_t chunk        = size - written;

	  if(page_offset + chunk > SPIF_PAGE_SIZE)
	  {
	    chunk = SPIF_PAGE_SIZE - page_offset;
	  }

	  if(!spif_write_page(handle, page_index, data + written, chunk, page_offset))
	  {
	    return false;
	  }

	  written += chunk;
	}

	return true;
}

bool spif_write_page(spif_handle_t *handle, uint32_t page, const uint8_t *data, uint32_t size, uint32_t offset)
{
	bool ret_val;

	if(!spif_utils_is_ready(handle))
	{
	  return false;
	}
	else if(data == NULL)
	{
	  return false;
	}
	else if(!spif_utils_clamp_region(SPIF_PAGE_SIZE, offset, &size))
	{
	  return false;
	}

	uint32_t address = (page * SPIF_PAGE_SIZE) + offset;

	if(!spif_utils_check_range(handle, address, size))
	{
	  return false;
	}
	else if(!spif_commander_command(handle, SPIF_CMD_WRITE_ENABLE))
	{
	  return false;
	}

	spif_commander_cs(handle, true);
	ret_val = spif_commander_command_address(handle, SPIF_CMD_PAGE_PROG_3ADDR, SPIF_CMD_PAGE_PROG_4ADDR, address);
	if(ret_val)
	{
	  ret_val = spif_commander_transmit(handle, data, size, SPIF_TIMEOUT_CMD);
	}
	spif_commander_cs(handle, false);

	if(!ret_val)
	{
	  return false;
	}
	else if(!spif_commander_wait_for_writing(handle, SPIF_TIMEOUT_PAGE_PROG))
	{
	  return false;
	}

	return true;
}

bool spif_write_sector(spif_handle_t *handle, uint32_t sector, const uint8_t *data, uint32_t size, uint32_t offset)
{
	if(!spif_utils_clamp_region(SPIF_SECTOR_SIZE, offset, &size))
	{
	  return false;
	}
	return spif_write_address(handle, (sector * SPIF_SECTOR_SIZE) + offset, data, size);
}

bool spif_write_block(spif_handle_t *handle, uint32_t block, const uint8_t *data, uint32_t size, uint32_t offset)
{
	if(!spif_utils_clamp_region(SPIF_BLOCK_SIZE, offset, &size))
	{
	  return false;
	}
	return spif_write_address(handle, (block * SPIF_BLOCK_SIZE) + offset, data, size);
}

bool spif_read_address(spif_handle_t *handle, uint32_t address, uint8_t *data, uint32_t size)
{
	bool ret_val;

	if(data == NULL)
	{
	  return false;
	}

	if(!spif_utils_check_range(handle, address, size))
	{
	  return false;
	}

	spif_commander_cs(handle, true);
	ret_val = spif_commander_command_address(handle, SPIF_CMD_READ_DATA_3ADDR, SPIF_CMD_READ_DATA_4ADDR, address);
	if(ret_val)
	{
	  ret_val = spif_commander_receive(handle, data, size, SPIF_TIMEOUT_CMD);
	}
	spif_commander_cs(handle, false);

	return ret_val;
}

bool spif_read_page(spif_handle_t *handle, uint32_t page, uint8_t *data, uint32_t size, uint32_t offset)
{
	if(!spif_utils_clamp_region(SPIF_PAGE_SIZE, offset, &size))
	{
	  return false;
	}

	return spif_read_address(handle, (page * SPIF_PAGE_SIZE) + offset, data, size);
}

bool spif_read_sector(spif_handle_t *handle, uint32_t sector, uint8_t *data, uint32_t size, uint32_t offset)
{
    if(!spif_utils_clamp_region(SPIF_SECTOR_SIZE, offset, &size))
    {
      return false;
    }

    return spif_read_address(handle, (sector * SPIF_SECTOR_SIZE) + offset, data, size);
}

bool spif_read_block(spif_handle_t *handle, uint32_t block, uint8_t *data, uint32_t size, uint32_t offset)
{
	if(!spif_utils_clamp_region(SPIF_BLOCK_SIZE, offset, &size))
	{
	  return false;
	}

	return spif_read_address(handle, (block * SPIF_BLOCK_SIZE) + offset, data, size);
}

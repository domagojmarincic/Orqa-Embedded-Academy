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
	uint8_t JEDEC[SPIF_JEDEC_ID_SIZE];

	if(!spif_utils_is_ready(handle))
	{
	  return false;
	}
	else
	{
	  handle->hspi = hspi;
	  handle->gpio = gpio;
	  handle->pin = pin;
	  if(!spif_commander_command(handle, SPIF_CMD_JEDEC_ID))
	  {
        return false;
	  }
	  else if(!spif_commander_receive(handle, JEDEC, SPIF_JEDEC_ID_SIZE, SPIF_TIMEOUT_CMD))
	  {
        return false;
	  }
	  else
	  {
	    handle->manufactor = (JEDEC[0] == SPIF_MANUFACTOR_WINBOND) ? SPIF_MANUFACTOR_WINBOND : SPIF_MANUFACTOR_ERROR;

	    if (handle->manufactor == SPIF_MANUFACTOR_ERROR)
	    {
	      return false;
	    }

	    handle->mem_type = JEDEC[1];
	    handle->capacity = JEDEC[2];

	    handle->total_size = (1UL << handle->capacity);
	    handle->block_cnt = handle->total_size / SPIF_BLOCK_SIZE;
	    handle->sector_cnt = handle->total_size / SPIF_SECTOR_SIZE;
	    handle->page_cnt = handle->total_size / SPIF_PAGE_SIZE;
	  }
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
	if(!spif_utils_is_ready(handle))
	{
	  return false;
	}
	else if(!spif_utils_check_range(handle, sector, SPIF_SECTOR_SIZE))
	{
	  return false;
	}
	else if(!spif_commander_command(handle, SPIF_CMD_WRITE_ENABLE))
	{
	  return false;
	}
	else if(!spif_commander_command_address(handle, SPIF_CMD_SECTOR_ERASE_3ADDR, SPIF_CMD_SECTOR_ERASE_4ADDR, sector))
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
	if(!spif_utils_is_ready(handle))
	{
	  return false;
	}
	else if(!spif_utils_check_range(handle, block, SPIF_BLOCK_SIZE))
	{
	  return false;
	}
	else if(!spif_commander_command(handle, SPIF_CMD_WRITE_ENABLE))
	{
	  return false;
	}
	else if(!spif_commander_command_address(handle, SPIF_CMD_BLOCK_ERASE_3ADDR, SPIF_CMD_BLOCK_ERASE_4ADDR, block))
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
	  uint32_t page_start   = (current / SPIF_PAGE_SIZE) * SPIF_PAGE_SIZE;
	  uint32_t page_offset  = current - page_start;
	  uint32_t chunk        = size - written;

	  if(page_offset + chunk > SPIF_PAGE_SIZE)
	  {
	    chunk = SPIF_PAGE_SIZE - page_offset;
	  }

	  if(!spif_write_page(handle, page_start, data + written, chunk, page_offset))
	  {
	    return false;
	  }

	  written += chunk;
	}

	return true;
}

bool spif_write_page(spif_handle_t *handle, uint32_t page, const uint8_t *data, uint32_t size, uint32_t offset)
{
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

	uint32_t address = page + offset;

	if(!spif_utils_check_range(handle, address, size))
	{
	  return false;
	}
	else if(!spif_commander_command(handle, SPIF_CMD_WRITE_ENABLE))
	{
	  return false;
	}

	bool ret_val;
	ret_val = spif_commander_command_address(handle, SPIF_CMD_PAGE_PROGRAM_3ADDR, SPIF_CMD_PAGE_PROGRAM_4ADDR, address);

	if(ret_val)
	{
	  ret_val = spif_commander_transmit(handle, data, size, SPIF_TIMEOUT_CMD);
	}

	if(!ret_val)
	{
	  return false;
	}

	if(!spif_commander_wait_for_writing(handle, SPIF_TIMEOUT_PAGE_PROGRAM))
	{
	  return false;
	}

	return true;
}

bool spif_write_sector(spif_handle_t *handle, uint32_t sector, const uint8_t *data, uint32_t size, uint32_t offset)
{
	if(data == NULL)
	{
	  return false;
	}

	if(!spif_utils_clamp_region(SPIF_SECTOR_SIZE, offset, &size))
	{
	  return false;
	}

	uint32_t written = 0;

	while(written < size)
	{
	  uint32_t address     = sector + offset + written;
	  uint32_t page_start  = (address / SPIF_PAGE_SIZE) * SPIF_PAGE_SIZE;
	  uint32_t page_offset = address - page_start;
      uint32_t remaining   = size - written;
	  uint32_t chunk       = remaining;

	  if(page_offset + chunk > SPIF_PAGE_SIZE)
	  {
        chunk = SPIF_PAGE_SIZE - page_offset;
	  }

	  if(!spif_write_page(handle, page_start, data + written, chunk, page_offset))
	  {
        return false;
	  }

	  written += chunk;
	}

	return true;
}

bool spif_write_block(spif_handle_t *handle, uint32_t block, const uint8_t *data, uint32_t size, uint32_t offset)
{
	if(data == NULL)
	{
	  return false;
	}

	if(!spif_utils_clamp_region(SPIF_BLOCK_SIZE, offset, &size))
	{
	  return false;
	}

	uint32_t written = 0;

	while(written < size)
	{
	  uint32_t address     = block + offset + written;
	  uint32_t page_start  = (address / SPIF_PAGE_SIZE) * SPIF_PAGE_SIZE;
	  uint32_t page_offset = address - page_start;
	  uint32_t remaining   = size - written;
	  uint32_t chunk       = remaining;

	  if(page_offset + chunk > SPIF_PAGE_SIZE)
	  {
		chunk = SPIF_PAGE_SIZE - page_offset;
	  }

	  if(!spif_write_page(handle, page_start, data + written, chunk, page_offset))
	  {
		return false;
	  }

	  written += chunk;
	}

	return true;
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

	return spif_read_address(handle, page + offset, data, size);
}

bool spif_read_sector(spif_handle_t *handle, uint32_t sector, uint8_t *data, uint32_t size, uint32_t offset)
{
    if(!spif_utils_clamp_region(SPIF_SECTOR_SIZE, offset, &size))
    {
      return false;
    }

    return spif_read_address(handle, sector + offset, data, size);
}

bool spif_read_block(spif_handle_t *handle, uint32_t block, uint8_t *data, uint32_t size, uint32_t offset)
{
	if(!spif_utils_clamp_region(SPIF_BLOCK_SIZE, offset, &size))
	{
	  return false;
	}

	return spif_read_address(handle, block + offset, data, size);
}

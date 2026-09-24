/**
 * @file spif_utils.c
 * @author ORQA Embedded Academy
 * @brief Look at spif_utils.h
 *
 * @note Every function is a stub. These four need no hardware, so they are the
 *       cheapest tests to turn green - start here.
 */

/*------------------------------------------------------ INCLUDES -----------------------------------------------------*/
#include "spif_utils.h"

/*----------------------------------------------- PRIVATE MACROS / DEFINES --------------------------------------------*/

#define SPIF_CAPACITY_CODE_MIN   0x11U
#define SPIF_CAPACITY_CODE_MAX   0x1AU

/*--------------------------------------------- PRIVATE VARIABLES (STATIC) --------------------------------------------*/

/*------------------------------------------- PUBLIC FUNCTION IMPLEMENTATIONS -----------------------------------------*/

bool spif_utils_is_ready(const spif_handle_t *handle)
{
    if(handle == NULL || handle->inited != true)
    {
      return false;
    }
    return true;
}

bool spif_utils_check_range(const spif_handle_t *handle, uint32_t address, uint32_t size)
{
	if(!spif_utils_is_ready(handle))
	{
	  return false;
	}
	if(size == 0)
	{
	  return false;
	}
	if(address >= handle->total_size )
	{
	  return false;
	}
	if(size > handle->total_size - address)
	{
	  return false;
	}
    return true;
}

bool spif_utils_clamp_region(uint32_t region_size, uint32_t offset, uint32_t *size)
{
	if(size == NULL)
	{
	  return false;
	}
	if(offset >= region_size)
	{
	  return false;
	}
	if(*size > region_size - offset)
	{
	  *size = region_size - offset;
	}
	return true;
}

bool spif_utils_capacity_to_block_count(uint8_t capacity, uint32_t *block_count)
{
	if(block_count == NULL)
	{
	  return false;
	}
	if((capacity >= SPIF_CAPACITY_CODE_MIN) && (capacity <= SPIF_CAPACITY_CODE_MAX))
	{
	  *block_count = (1u << (capacity - 0x10));
	  return true;
	}
	if(capacity == 0x20)
	{
	  *block_count = 1024;
	  return true;
	}

	return false;
}

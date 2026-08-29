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

/*--------------------------------------------- PRIVATE VARIABLES (STATIC) --------------------------------------------*/

/*------------------------------------------- PUBLIC FUNCTION IMPLEMENTATIONS -----------------------------------------*/

bool spif_utils_is_ready(const spif_handle_t *handle)
{
    (void)handle;
    return false; /* TODO */
}

bool spif_utils_check_range(const spif_handle_t *handle, uint32_t address, uint32_t size)
{
    (void)handle;
    (void)address;
    (void)size;
    return false; /* TODO */
}

bool spif_utils_clamp_region(uint32_t region_size, uint32_t offset, uint32_t *size)
{
    (void)region_size;
    (void)offset;
    (void)size;
    return false; /* TODO */
}

bool spif_utils_capacity_to_block_count(uint8_t capacity, uint32_t *block_count)
{
    (void)capacity;
    (void)block_count;
    return false; /* TODO */
}

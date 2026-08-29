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
    (void)handle;
    (void)hspi;
    (void)gpio;
    (void)pin;
    return false; /* TODO */
}

bool spif_erase_chip(spif_handle_t *handle)
{
    (void)handle;
    return false; /* TODO */
}

bool spif_erase_sector(spif_handle_t *handle, uint32_t sector)
{
    (void)handle;
    (void)sector;
    return false; /* TODO */
}

bool spif_erase_block(spif_handle_t *handle, uint32_t block)
{
    (void)handle;
    (void)block;
    return false; /* TODO */
}

bool spif_write_address(spif_handle_t *handle, uint32_t address, const uint8_t *data, uint32_t size)
{
    (void)handle;
    (void)address;
    (void)data;
    (void)size;
    return false; /* TODO */
}

bool spif_write_page(spif_handle_t *handle, uint32_t page, const uint8_t *data, uint32_t size, uint32_t offset)
{
    (void)handle;
    (void)page;
    (void)data;
    (void)size;
    (void)offset;
    return false; /* TODO */
}

bool spif_write_sector(spif_handle_t *handle, uint32_t sector, const uint8_t *data, uint32_t size, uint32_t offset)
{
    (void)handle;
    (void)sector;
    (void)data;
    (void)size;
    (void)offset;
    return false; /* TODO */
}

bool spif_write_block(spif_handle_t *handle, uint32_t block, const uint8_t *data, uint32_t size, uint32_t offset)
{
    (void)handle;
    (void)block;
    (void)data;
    (void)size;
    (void)offset;
    return false; /* TODO */
}

bool spif_read_address(spif_handle_t *handle, uint32_t address, uint8_t *data, uint32_t size)
{
    (void)handle;
    (void)address;
    (void)data;
    (void)size;
    return false; /* TODO */
}

bool spif_read_page(spif_handle_t *handle, uint32_t page, uint8_t *data, uint32_t size, uint32_t offset)
{
    (void)handle;
    (void)page;
    (void)data;
    (void)size;
    (void)offset;
    return false; /* TODO */
}

bool spif_read_sector(spif_handle_t *handle, uint32_t sector, uint8_t *data, uint32_t size, uint32_t offset)
{
    (void)handle;
    (void)sector;
    (void)data;
    (void)size;
    (void)offset;
    return false; /* TODO */
}

bool spif_read_block(spif_handle_t *handle, uint32_t block, uint8_t *data, uint32_t size, uint32_t offset)
{
    (void)handle;
    (void)block;
    (void)data;
    (void)size;
    (void)offset;
    return false; /* TODO */
}

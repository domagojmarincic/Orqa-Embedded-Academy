/**
 * @file spif.h
 * @author ORQA Embedded Academy
 * @brief Public interface of the SPI NOR-Flash driver (blocking HAL).
 * @version 0.1
 *
 * @note This is the contract for Task 2. Do not change anything in this file - the
 *       provided test suite is written against it.
 */

#ifndef SPIF_H
#define SPIF_H

/*------------------------------------------------------ INCLUDES -----------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Set to 1 when building with FreeRTOS / CMSIS-RTOS v2. When enabled, public API calls are
 * protected by a mutex and busy-waits yield to the scheduler instead of spinning.
 * Leave it at 0 for Task 2 - it is switched on in Week 4, and the driver must be ready for it. */
#define SPIF_USE_RTOS               0
#if SPIF_USE_RTOS == 1
#include "cmsis_os2.h"
#endif

/*-------------------------------------------------- MACROS / DEFINES -------------------------------------------------*/
#define SPIF_PAGE_SIZE              0x100
#define SPIF_SECTOR_SIZE            0x1000
#define SPIF_BLOCK_SIZE             0x10000

#define SPIF_PAGE_TO_ADDRESS(page)      ((uint32_t)(page)    * SPIF_PAGE_SIZE)
#define SPIF_SECTOR_TO_ADDRESS(sector)  ((uint32_t)(sector)  * SPIF_SECTOR_SIZE)
#define SPIF_BLOCK_TO_ADDRESS(block)    ((uint32_t)(block)   * SPIF_BLOCK_SIZE)
#define SPIF_ADDRESS_TO_PAGE(address)   ((uint32_t)(address) / SPIF_PAGE_SIZE)
#define SPIF_ADDRESS_TO_SECTOR(address) ((uint32_t)(address) / SPIF_SECTOR_SIZE)
#define SPIF_ADDRESS_TO_BLOCK(address)  ((uint32_t)(address) / SPIF_BLOCK_SIZE)

/*-------------------------------------------------- EXTERNAL VARIABLES -----------------------------------------------*/

/*---------------------------------------------- TYPEDEFS / ENUMS / STRUCTS -------------------------------------------*/

/** @brief Manufacturer byte of the JEDEC ID. */
typedef enum
{
    SPIF_MANUFACTOR_ERROR   = 0x00,
    SPIF_MANUFACTOR_WINBOND = 0xEF,

} spif_manufactor_t;

/** @brief Driver handle: bus, chip select and the geometry discovered at init. */
typedef struct
{
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *gpio;
    uint16_t           pin;
    spif_manufactor_t  manufactor;
    uint8_t            mem_type;
    uint8_t            capacity;   /* raw JEDEC capacity byte */
    bool               inited;
    uint32_t           total_size; /* in bytes */
    uint32_t           page_cnt;
    uint32_t           sector_cnt;
    uint32_t           block_cnt;
#if SPIF_USE_RTOS == 1
    osMutexId_t        mutex;      /* created by spif_init, protects all bus access */
#else
    volatile uint8_t   lock;       /* simple spin-lock for bare-metal re-entrancy guard */
#endif

    /* Add your own fields below if your implementation needs them. */

} spif_handle_t;

/*--------------------------------------------- PUBLIC FUNCTION DECLARATIONS ------------------------------------------*/

/**
 * @brief Initialize the driver, read the JEDEC ID and fill in the handle.
 * @note  When SPIF_USE_RTOS is 1 this also creates the mutex, so call it from a task, not before the scheduler starts.
 */
bool spif_init(spif_handle_t *handle, SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio, uint16_t pin);

/** @brief Erase the entire chip. */
bool spif_erase_chip(spif_handle_t *handle);

/** @brief Erase one 4 KB sector. */
bool spif_erase_sector(spif_handle_t *handle, uint32_t sector);

/** @brief Erase one 64 KB block. */
bool spif_erase_block(spif_handle_t *handle, uint32_t block);

/** @brief Write data at an arbitrary byte address, crossing page boundaries as needed. */
bool spif_write_address(spif_handle_t *handle, uint32_t address, const uint8_t *data, uint32_t size);

/** @brief Write data into a page at the given offset, clamped to the end of the page. */
bool spif_write_page(spif_handle_t *handle, uint32_t page, const uint8_t *data, uint32_t size, uint32_t offset);

/** @brief Write data into a sector at the given offset, clamped to the end of the sector. */
bool spif_write_sector(spif_handle_t *handle, uint32_t sector, const uint8_t *data, uint32_t size, uint32_t offset);

/** @brief Write data into a block at the given offset, clamped to the end of the block. */
bool spif_write_block(spif_handle_t *handle, uint32_t block, const uint8_t *data, uint32_t size, uint32_t offset);

/** @brief Read data from an arbitrary byte address. */
bool spif_read_address(spif_handle_t *handle, uint32_t address, uint8_t *data, uint32_t size);

/** @brief Read data from a page at the given offset, clamped to the end of the page. */
bool spif_read_page(spif_handle_t *handle, uint32_t page, uint8_t *data, uint32_t size, uint32_t offset);

/** @brief Read data from a sector at the given offset, clamped to the end of the sector. */
bool spif_read_sector(spif_handle_t *handle, uint32_t sector, uint8_t *data, uint32_t size, uint32_t offset);

/** @brief Read data from a block at the given offset, clamped to the end of the block. */
bool spif_read_block(spif_handle_t *handle, uint32_t block, uint8_t *data, uint32_t size, uint32_t offset);

#ifdef __cplusplus
}
#endif
#endif /* SPIF_H */

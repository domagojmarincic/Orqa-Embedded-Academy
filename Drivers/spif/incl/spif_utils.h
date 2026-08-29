/**
 * @file spif_utils.h
 * @author ORQA Embedded Academy
 * @brief Pure helper functions for the SPI NOR-Flash driver, no hardware access.
 * @version 0.1
 */

#ifndef SPIF_UTILS_H
#define SPIF_UTILS_H

/*------------------------------------------------------ INCLUDES -----------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "spif.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------- MACROS / DEFINES -------------------------------------------------*/

/*-------------------------------------------------- EXTERNAL VARIABLES -----------------------------------------------*/

/*---------------------------------------------- TYPEDEFS / ENUMS / STRUCTS -------------------------------------------*/

/*--------------------------------------------- PUBLIC FUNCTION DECLARATIONS ------------------------------------------*/

/** @brief Check that the handle exists and has been initialized. */
bool spif_utils_is_ready(const spif_handle_t *handle);

/** @brief Check that address + size fits inside the chip. */
bool spif_utils_check_range(const spif_handle_t *handle, uint32_t address, uint32_t size);

/** @brief Clamp a region access to the end of its page / sector / block, shortening size in place. */
bool spif_utils_clamp_region(uint32_t region_size, uint32_t offset, uint32_t *size);

/** @brief Decode the raw JEDEC capacity byte into a 64 kByte block count. */
bool spif_utils_capacity_to_block_count(uint8_t capacity, uint32_t *block_count);

#ifdef __cplusplus
}
#endif
#endif /* SPIF_UTILS_H */

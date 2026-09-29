/**
 * @file spif_commander.h
 * @author ORQA Embedded Academy
 * @brief Generic SPI command layer: chip select, raw transfers and addressed commands.
 * @version 0.1
 */

#ifndef SPIF_COMMANDER_H
#define SPIF_COMMANDER_H

/*------------------------------------------------------ INCLUDES -----------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "spif.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------- MACROS / DEFINES -------------------------------------------------*/
#define SPIF_DUMMY_BYTE                 0xA5

#define SPIF_STATUS1_BUSY               (1 << 0)
#define SPIF_STATUS1_WEL                (1 << 1)

#define SPIF_TIMEOUT_CMD                100  /* ms, short command transfers */
#define SPIF_TIMEOUT_DATA               2000 /* ms, per data chunk          */
#define SPIF_TIMEOUT_PAGE_PROG          100  /* ms, page program            */
#define SPIF_TIMEOUT_SECTOR_ERASE       1000 /* ms                          */
#define SPIF_TIMEOUT_BLOCK_ERASE        3000 /* ms                          */
#define SPIF_TIMEOUT_CHIP_ERASE         1000000UL

/*  highest byte address reachable with a 3-byte address (16 MB - 1) */
#define SPIF_MAX_3ADDR_ADDRESS 0xFFFFFFUL
/* Length of the JEDEC ID answer: manufacturer, memory type, capacity */
#define SPIF_JEDEC_ID_SIZE              3

/*-------------------------------------------------- EXTERNAL VARIABLES -----------------------------------------------*/

/*---------------------------------------------- TYPEDEFS / ENUMS / STRUCTS -------------------------------------------*/

/** @brief Opcodes used by the driver, three and four byte addressing variants where they differ. */
typedef enum
{
    SPIF_CMD_JEDEC_ID           = 0x9F,
    SPIF_CMD_WRITE_ENABLE       = 0x06,
    SPIF_CMD_WRITE_DISABLE      = 0x04,
    SPIF_CMD_READ_STATUS1       = 0x05,
    SPIF_CMD_PAGE_PROG_3ADDR    = 0x02,
    SPIF_CMD_PAGE_PROG_4ADDR    = 0x12,
    SPIF_CMD_READ_DATA_3ADDR    = 0x03,
    SPIF_CMD_READ_DATA_4ADDR    = 0x13,
    SPIF_CMD_SECTOR_ERASE_3ADDR = 0x20,
    SPIF_CMD_SECTOR_ERASE_4ADDR = 0x21,
    SPIF_CMD_BLOCK_ERASE_3ADDR  = 0xD8,
    SPIF_CMD_BLOCK_ERASE_4ADDR  = 0xDC,
    SPIF_CMD_CHIP_ERASE         = 0xC7,

} spif_cmd_t;

/*--------------------------------------------- PUBLIC FUNCTION DECLARATIONS ------------------------------------------*/

/** @brief Assert (select = true) or release the chip select line. */
void spif_commander_cs(spif_handle_t *handle, bool select);

/** @brief Send raw bytes with the chip select handled by the caller. */
bool spif_commander_transmit(spif_handle_t *handle, const uint8_t *tx, uint32_t size, uint32_t timeout);

/** @brief Receive raw bytes with the chip select handled by the caller. */
bool spif_commander_receive(spif_handle_t *handle, uint8_t *rx, uint32_t size, uint32_t timeout);

/** @brief Send a single byte command inside its own chip select section. */
bool spif_commander_command(spif_handle_t *handle, spif_cmd_t cmd);

/** @brief Send a command plus address, with the chip select already asserted by the caller. */
bool spif_commander_command_address(spif_handle_t *handle, spif_cmd_t cmd_3addr, spif_cmd_t cmd_4addr,
                                    uint32_t address);

/** @brief Read status register 1. */
uint8_t spif_commander_read_status(spif_handle_t *handle);

/** @brief Poll the busy bit until it clears or the timeout expires. */
bool spif_commander_wait_for_writing(spif_handle_t *handle, uint32_t timeout);

#ifdef __cplusplus
}
#endif
#endif /* SPIF_COMMANDER_H */

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
    (void)handle;
    (void)select;
    /* TODO */
}

bool spif_commander_transmit(spif_handle_t *handle, const uint8_t *tx, uint32_t size, uint32_t timeout)
{
    (void)handle;
    (void)tx;
    (void)size;
    (void)timeout;
    return false; /* TODO */
}

bool spif_commander_receive(spif_handle_t *handle, uint8_t *rx, uint32_t size, uint32_t timeout)
{
    (void)handle;
    (void)rx;
    (void)size;
    (void)timeout;
    return false; /* TODO */
}

bool spif_commander_command(spif_handle_t *handle, spif_cmd_t cmd)
{
    (void)handle;
    (void)cmd;
    return false; /* TODO */
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
    (void)handle;
    return 0; /* TODO */
}

bool spif_commander_wait_for_writing(spif_handle_t *handle, uint32_t timeout)
{
    (void)handle;
    (void)timeout;
    return false; /* TODO */
}

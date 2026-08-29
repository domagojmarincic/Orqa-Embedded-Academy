/**
 * @file spif_tests.h
 * @author ORQA Embedded Academy
 * @brief Self test for the spif driver: spif, spif_commander and spif_utils in one run.
 * @version 0.1
 *
 * @note The test never uses the driver to check the driver. Everything is verified against a
 *       small reference implementation inside spif_tests.c that talks to the chip directly
 *       through the HAL, so a driver that is wrong the same way in both directions still fails.
 *
 * @warning This test is destructive, it erases and rewrites the block selected by SPIF_TEST_BLOCK.
 *          If SPIF_TEST_ERASE_CHIP is set to 1 the whole chip is erased.
 *          Results are printed with printf(), so an _write() override that pushes characters
 *          out of your UART must be in place before calling it.
 */

#ifndef SPIF_TESTS_H
#define SPIF_TESTS_H

/*------------------------------------------------------ INCLUDES -----------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "spif.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------- MACROS / DEFINES -------------------------------------------------*/

/* Block used for all read / write tests */
#define SPIF_TEST_BLOCK             0

/* Set to 1 to also run the (slow, whole chip) spif_erase_chip() test */
#define SPIF_TEST_ERASE_CHIP        0

/*-------------------------------------------------- EXTERNAL VARIABLES -----------------------------------------------*/

/*---------------------------------------------- TYPEDEFS / ENUMS / STRUCTS -------------------------------------------*/

/*--------------------------------------------- PUBLIC FUNCTION DECLARATIONS ------------------------------------------*/

/**
 * @brief  Run all tests: spif_utils (offline), spif_commander (on the wire) and the spif public API.
 * @retval true when every test passed
 */
bool spif_test_run(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio, uint16_t pin);

#ifdef __cplusplus
}
#endif
#endif /* SPIF_TESTS_H */

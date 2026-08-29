/**
 * @file spif_tests.c
 * @author ORQA Embedded Academy
 * @brief Look at spif_tests.h
 */

/*------------------------------------------------------ INCLUDES -----------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include "spif_tests.h"
#include "spif_commander.h"
#include "spif_utils.h"



/*----------------------------------------------- PRIVATE MACROS / DEFINES --------------------------------------------*/

#define TEST_BUF_SIZE           512

/* Reference geometry and opcodes, taken from the datasheet and kept apart from the driver
 * headers on purpose: the checking side must not reuse the code under test. */
#define REF_PAGE_SIZE           0x100u
#define REF_SECTOR_SIZE         0x1000u
#define REF_BLOCK_SIZE          0x10000u

#define REF_CMD_JEDEC_ID        0x9F
#define REF_CMD_WRITE_ENABLE    0x06
#define REF_CMD_WRITE_DISABLE   0x04
#define REF_CMD_READ_STATUS1    0x05
#define REF_CMD_PAGE_PROG_3A    0x02
#define REF_CMD_PAGE_PROG_4A    0x12
#define REF_CMD_READ_DATA_3A    0x03
#define REF_CMD_READ_DATA_4A    0x13
#define REF_CMD_SECTOR_ERASE_3A 0x20
#define REF_CMD_SECTOR_ERASE_4A 0x21

#define REF_STATUS_BUSY         0x01
#define REF_STATUS_WEL          0x02

#define REF_DUMMY_BYTE          0xA5
#define REF_TIMEOUT             1000  /* ms, commands and data                       */
#define REF_TIMEOUT_ERASE       3000  /* ms, sector erase                            */
#define REF_POWER_UP_DELAY      10    /* ms                                          */
#define REF_4ADDR_BLOCK_CNT     512   /* from here on the chip needs 4 address bytes  */

/* Layout inside SPIF_TEST_BLOCK, one sector per test so they cannot overlap. Inside a
 * sector the low half belongs to the driver and the high half to the reference, so both
 * directions can be tested without stepping on each other. */
#define TEST_SECTOR_FOR_ADDRESS 0
#define TEST_SECTOR_FOR_PAGE    1
#define TEST_SECTOR_FOR_SECTOR  2
#define TEST_SECTOR_FOR_BLOCK   3
#define TEST_SECTOR_FOR_CLAMP   4
#define TEST_SECTOR_FOR_PAGING  5
#define TEST_SECTOR_FOR_SEAM    6 /* the seam test also writes into sector 7 */
#define TEST_REF_HALF           0x800

#define TEST_BLOCK_ADDRESS      ((uint32_t)SPIF_TEST_BLOCK * REF_BLOCK_SIZE)
#define TEST_AREA_OFFSET(idx)   ((uint32_t)(idx) * REF_SECTOR_SIZE)
#define TEST_AREA_ADDRESS(idx)  (TEST_BLOCK_ADDRESS + TEST_AREA_OFFSET(idx))

/* Fake chip size used by the offline spif_utils tests */
#define TEST_FAKE_TOTAL_SIZE    0x100000

/*-------------------------------------------------- PRIVATE TYPEDEFS ------------------------------------------------*/

/*---------------------------------------------------- CONSTANT DATA --------------------------------------------------*/

/*--------------------------------------------- PRIVATE VARIABLES (STATIC) --------------------------------------------*/

static spif_handle_t spif;
static uint8_t tx_buf[TEST_BUF_SIZE];
static uint8_t rx_buf[TEST_BUF_SIZE];
static uint32_t test_passed;
static uint32_t test_failed;

/* State of the reference implementation, filled in by ref_setup() */
static SPI_HandleTypeDef *ref_hspi;
static GPIO_TypeDef      *ref_gpio;
static uint16_t           ref_pin;
static uint8_t            ref_id[3];
static uint32_t           ref_block_cnt;
static uint32_t           ref_total_size;
static bool               ref_addr4;

/*-------------------------------------- PRIVATE FUNCTION DECLARATIONS (PROTOTYPES) -----------------------------------*/

/* Reference implementation: talks to the chip through the HAL, never through the driver */
static void ref_cs(bool select);
static bool ref_transmit(const uint8_t *tx, uint16_t size);
static bool ref_receive(uint8_t *rx, uint16_t size);
static bool ref_command(uint8_t cmd);
static bool ref_command_address(uint8_t cmd_3addr, uint8_t cmd_4addr, uint32_t address);
static uint8_t ref_read_status(void);
static bool ref_wait_ready(uint32_t timeout);
static bool ref_capacity_to_block_count(uint8_t capacity, uint32_t *block_count);
static bool ref_setup(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio, uint16_t pin);
static bool ref_read(uint32_t address, uint8_t *data, uint16_t size);
static bool ref_write(uint32_t address, const uint8_t *data, uint16_t size);
static bool ref_start_sector_erase(uint32_t address);

static void report(const char *name, bool result);
static void fill_pattern(uint32_t size, uint8_t seed);
static bool compare_pattern(uint32_t size, uint8_t seed);
static bool is_blank(uint32_t address, uint32_t size);
static uint32_t test_sector_number(uint32_t index_in_block);

static bool test_utils_is_ready(void);
static bool test_utils_check_range(void);
static bool test_utils_clamp_region(void);
static bool test_utils_capacity_to_block_count(void);

static bool test_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio, uint16_t pin);

static bool test_commander_command(void);
static bool test_commander_read_status(void);
static bool test_commander_wait_for_writing(void);

static bool test_null_and_uninitialized(void);
static bool test_erase_block(void);
static bool test_write_read_address(void);
static bool test_write_read_page(void);
static bool test_write_read_sector(void);
static bool test_write_read_block(void);
static bool test_erase_sector(void);
static bool test_write_clamping(void);
static bool test_write_multiple_pages(void);
static bool test_read_across_sectors(void);
static bool test_bad_parameters(void);
#if SPIF_TEST_ERASE_CHIP == 1
static bool test_erase_chip(void);
#endif

/*--------------------------------------------------- EVENT HANDLERS --------------------------------------------------*/

/*------------------------------------------- PUBLIC FUNCTION IMPLEMENTATIONS -----------------------------------------*/

bool spif_test_run(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio, uint16_t pin)
{
    test_passed = 0;
    test_failed = 0;
    printf("\r\n--- SPIF TEST START ---\r\n");

    /* spif_utils: no hardware needed, so run them first */
    report("spif_utils_is_ready", test_utils_is_ready());
    report("spif_utils_check_range", test_utils_check_range());
    report("spif_utils_clamp_region", test_utils_clamp_region());
    report("spif_utils_capacity_to_block_count", test_utils_capacity_to_block_count());

    /* Every hardware test is judged by the reference, so the reference has to work first */
    if (!ref_setup(hspi, gpio, pin))
    {
        report("reference flash access", false);
        printf("--- SPIF TEST ABORTED ---\r\n");
        return false;
    }
    report("reference flash access", true);

    if (!test_init(hspi, gpio, pin))
    {
        report("spif_init", false);
        printf("--- SPIF TEST ABORTED ---\r\n");
        return false;
    }
    report("spif_init", true);

    /* spif_commander: generic primitives */
    report("spif_commander_command (write enable / disable)", test_commander_command());
    report("spif_commander_read_status", test_commander_read_status());
    report("spif_commander_wait_for_writing", test_commander_wait_for_writing());

    /* spif: public API, destructive from here on */
    report("null / uninitialized handle", test_null_and_uninitialized());
    report("spif_erase_block + blank check", test_erase_block());
    report("spif_write_address / spif_read_address", test_write_read_address());
    report("spif_write_page / spif_read_page", test_write_read_page());
    report("spif_write_sector / spif_read_sector", test_write_read_sector());
    report("spif_write_block / spif_read_block", test_write_read_block());
    report("spif_erase_sector", test_erase_sector());
    report("size clamping at region ends", test_write_clamping());
    report("write across several pages", test_write_multiple_pages());
    report("read across a sector boundary", test_read_across_sectors());
    report("parameter checks", test_bad_parameters());
#if SPIF_TEST_ERASE_CHIP == 1
    report("spif_erase_chip", test_erase_chip());
#else
    printf("[ SKIP ] spif_erase_chip (set SPIF_TEST_ERASE_CHIP to 1)\r\n");
#endif

    printf("--- SPIF TEST DONE: %lu passed, %lu failed ---\r\n\r\n",
           (unsigned long)test_passed, (unsigned long)test_failed);
    return (test_failed == 0);
}

/*------------------------------------------ PRIVATE FUNCTION IMPLEMENTATIONS -----------------------------------------*/

/*----------------------------------------------- REFERENCE IMPLEMENTATION --------------------------------------------*/
/* A minimal, independent SPI NOR access path used only to check the driver. It shares no
 * code with spif / spif_commander / spif_utils, so a driver that is wrong the same way in
 * both directions still fails the test. */

static void ref_cs(bool select)
{
    HAL_GPIO_WritePin(ref_gpio, ref_pin, select ? GPIO_PIN_RESET : GPIO_PIN_SET);
    for (volatile int i = 0; i < 10; i++);
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool ref_transmit(const uint8_t *tx, uint16_t size)
{
    return (HAL_SPI_Transmit(ref_hspi, (uint8_t *)tx, size, REF_TIMEOUT) == HAL_OK);
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool ref_receive(uint8_t *rx, uint16_t size)
{
    return (HAL_SPI_Receive(ref_hspi, rx, size, REF_TIMEOUT) == HAL_OK);
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool ref_command(uint8_t cmd)
{
    bool ret_val;

    ref_cs(true);
    ret_val = ref_transmit(&cmd, 1);
    ref_cs(false);
    return ret_val;
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* Chip select must already be asserted by the caller */
static bool ref_command_address(uint8_t cmd_3addr, uint8_t cmd_4addr, uint32_t address)
{
    uint8_t tx[5];
    uint16_t len;

    if (ref_addr4)
    {
        tx[0] = cmd_4addr;
        tx[1] = (uint8_t)(address >> 24);
        tx[2] = (uint8_t)(address >> 16);
        tx[3] = (uint8_t)(address >> 8);
        tx[4] = (uint8_t)(address);
        len = 5;
    }
    else
    {
        tx[0] = cmd_3addr;
        tx[1] = (uint8_t)(address >> 16);
        tx[2] = (uint8_t)(address >> 8);
        tx[3] = (uint8_t)(address);
        len = 4;
    }
    return ref_transmit(tx, len);
}

/*---------------------------------------------------------------------------------------------------------------------*/

static uint8_t ref_read_status(void)
{
    uint8_t tx[2] = {REF_CMD_READ_STATUS1, REF_DUMMY_BYTE};
    uint8_t rx[2] = {0, 0};

    ref_cs(true);
    HAL_SPI_TransmitReceive(ref_hspi, tx, rx, 2, REF_TIMEOUT);
    ref_cs(false);
    return rx[1];
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool ref_wait_ready(uint32_t timeout)
{
    uint32_t start_time = HAL_GetTick();

    while ((ref_read_status() & REF_STATUS_BUSY) != 0)
    {
        if (HAL_GetTick() - start_time >= timeout)
        {
            return false;
        }
        HAL_Delay(1);
    }
    return true;
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool ref_capacity_to_block_count(uint8_t capacity, uint32_t *block_count)
{
    if (block_count == NULL)
    {
        return false;
    }
    if ((capacity >= 0x11) && (capacity <= 0x1A))
    {
        *block_count = 1u << (capacity - 0x10);
        return true;
    }
    if (capacity == 0x20)
    {
        *block_count = 1024;
        return true;
    }
    return false;
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool ref_setup(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio, uint16_t pin)
{
    uint8_t tx[4] = {REF_CMD_JEDEC_ID, REF_DUMMY_BYTE, REF_DUMMY_BYTE, REF_DUMMY_BYTE};
    uint8_t rx[4] = {0};

    if ((hspi == NULL) || (gpio == NULL))
    {
        return false;
    }
    ref_hspi = hspi;
    ref_gpio = gpio;
    ref_pin = pin;
    ref_addr4 = false;

    ref_cs(false);
    HAL_Delay(REF_POWER_UP_DELAY);
    ref_command(REF_CMD_WRITE_DISABLE);

    ref_cs(true);
    if (HAL_SPI_TransmitReceive(hspi, tx, rx, sizeof(tx), REF_TIMEOUT) != HAL_OK)
    {
        ref_cs(false);
        printf("         reference JEDEC ID transfer failed\r\n");
        return false;
    }
    ref_cs(false);

    ref_id[0] = rx[1];
    ref_id[1] = rx[2];
    ref_id[2] = rx[3];
    if ((ref_id[0] == 0x00) || (ref_id[0] == 0xFF))
    {
        printf("         no chip answered, check wiring and CS pin\r\n");
        return false;
    }
    if (!ref_capacity_to_block_count(ref_id[2], &ref_block_cnt))
    {
        printf("         unsupported capacity byte 0x%02X\r\n", ref_id[2]);
        return false;
    }
    ref_total_size = ref_block_cnt * REF_BLOCK_SIZE;
    ref_addr4 = (ref_block_cnt >= REF_4ADDR_BLOCK_CNT);

    printf("         ID 0x%02X 0x%02X 0x%02X, %lu blocks, %lu bytes, %d address bytes\r\n",
           ref_id[0], ref_id[1], ref_id[2], (unsigned long)ref_block_cnt,
           (unsigned long)ref_total_size, ref_addr4 ? 4 : 3);

    if ((uint32_t)SPIF_TEST_BLOCK >= ref_block_cnt)
    {
        printf("         SPIF_TEST_BLOCK is outside the chip\r\n");
        return false;
    }
    return true;
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool ref_read(uint32_t address, uint8_t *data, uint16_t size)
{
    bool ret_val;

    ref_cs(true);
    ret_val = ref_command_address(REF_CMD_READ_DATA_3A, REF_CMD_READ_DATA_4A, address);
    if (ret_val)
    {
        ret_val = ref_receive(data, size);
    }
    ref_cs(false);
    return ret_val;
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* Programs across page boundaries, one page program command per page */
static bool ref_write(uint32_t address, const uint8_t *data, uint16_t size)
{
    while (size > 0)
    {
        uint16_t chunk = (uint16_t)(REF_PAGE_SIZE - (address % REF_PAGE_SIZE));
        bool ret_val;

        if (chunk > size)
        {
            chunk = size;
        }
        if (!ref_command(REF_CMD_WRITE_ENABLE))
        {
            return false;
        }
        ref_cs(true);
        ret_val = ref_command_address(REF_CMD_PAGE_PROG_3A, REF_CMD_PAGE_PROG_4A, address);
        if (ret_val)
        {
            ret_val = ref_transmit(data, chunk);
        }
        ref_cs(false);
        if ((!ret_val) || (!ref_wait_ready(REF_TIMEOUT)))
        {
            return false;
        }
        address += chunk;
        data += chunk;
        size = (uint16_t)(size - chunk);
    }
    return true;
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* Starts a real sector erase and returns immediately, the caller does the waiting */
static bool ref_start_sector_erase(uint32_t address)
{
    bool ret_val;

    if (!ref_command(REF_CMD_WRITE_ENABLE))
    {
        return false;
    }
    ref_cs(true);
    ret_val = ref_command_address(REF_CMD_SECTOR_ERASE_3A, REF_CMD_SECTOR_ERASE_4A, address);
    ref_cs(false);
    return ret_val;
}

/*--------------------------------------------------- TEST HELPERS ----------------------------------------------------*/

static void report(const char *name, bool result)
{
    if (result)
    {
        test_passed++;
        printf("[ PASS ] %s\r\n", name);
    }
    else
    {
        test_failed++;
        printf("[ FAIL ] %s\r\n", name);
    }
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* Fill tx_buf with a repeatable pattern */
static void fill_pattern(uint32_t size, uint8_t seed)
{
    for (uint32_t i = 0; i < size; i++)
    {
        tx_buf[i] = (uint8_t)(seed + i);
    }
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool compare_pattern(uint32_t size, uint8_t seed)
{
    for (uint32_t i = 0; i < size; i++)
    {
        if (rx_buf[i] != (uint8_t)(seed + i))
        {
            printf("         mismatch at %lu: read 0x%02X expected 0x%02X\r\n",
                   (unsigned long)i, rx_buf[i], (uint8_t)(seed + i));
            return false;
        }
    }
    return true;
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* Check that an area reads back as 0xFF, always through the reference */
static bool is_blank(uint32_t address, uint32_t size)
{
    while (size > 0)
    {
        uint16_t chunk = (size > TEST_BUF_SIZE) ? TEST_BUF_SIZE : (uint16_t)size;
        if (!ref_read(address, rx_buf, chunk))
        {
            printf("         reference read failed at 0x%08lX\r\n", (unsigned long)address);
            return false;
        }
        for (uint32_t i = 0; i < chunk; i++)
        {
            if (rx_buf[i] != 0xFF)
            {
                printf("         not blank at 0x%08lX: 0x%02X\r\n",
                       (unsigned long)(address + i), rx_buf[i]);
                return false;
            }
        }
        address += chunk;
        size -= chunk;
    }
    return true;
}

/*---------------------------------------------------------------------------------------------------------------------*/

static uint32_t test_sector_number(uint32_t index_in_block)
{
    return ((uint32_t)SPIF_TEST_BLOCK * (REF_BLOCK_SIZE / REF_SECTOR_SIZE)) + index_in_block;
}

/*----------------------------------------------------- TEST CASES ----------------------------------------------------*/

static bool test_utils_is_ready(void)
{
    spif_handle_t fake;
    bool ok = true;

    memset(&fake, 0, sizeof(fake));
    ok &= (spif_utils_is_ready(NULL) == false);
    ok &= (spif_utils_is_ready(&fake) == false); /* not inited */
    fake.inited = true;
    ok &= (spif_utils_is_ready(&fake) == true);
    return ok;
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool test_utils_check_range(void)
{
    spif_handle_t fake;
    bool ok = true;

    memset(&fake, 0, sizeof(fake));
    fake.inited = true;
    fake.total_size = TEST_FAKE_TOTAL_SIZE;

    ok &= (spif_utils_check_range(NULL, 0, 16) == false);              /* null handle          */
    fake.inited = false;
    ok &= (spif_utils_check_range(&fake, 0, 16) == false);             /* not inited           */
    fake.inited = true;
    ok &= (spif_utils_check_range(&fake, 0, 0) == false);              /* zero size            */
    ok &= (spif_utils_check_range(&fake, TEST_FAKE_TOTAL_SIZE, 1) == false);      /* address past end     */
    ok &= (spif_utils_check_range(&fake, TEST_FAKE_TOTAL_SIZE - 4, 8) == false);  /* runs past the end    */
    ok &= (spif_utils_check_range(&fake, 0, TEST_FAKE_TOTAL_SIZE) == true);       /* exactly the whole chip */
    ok &= (spif_utils_check_range(&fake, TEST_FAKE_TOTAL_SIZE - 4, 4) == true);   /* last bytes           */
    return ok;
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool test_utils_clamp_region(void)
{
    uint32_t size;
    bool ok = true;

    size = 16;
    ok &= (spif_utils_clamp_region(REF_PAGE_SIZE, 0, &size) == true);
    ok &= (size == 16); /* fits, untouched */

    size = REF_PAGE_SIZE;
    ok &= (spif_utils_clamp_region(REF_PAGE_SIZE, 0x10, &size) == true);
    ok &= (size == (REF_PAGE_SIZE - 0x10)); /* clamped to the end of the page */

    size = 16;
    ok &= (spif_utils_clamp_region(REF_PAGE_SIZE, REF_PAGE_SIZE, &size) == false); /* offset outside */

    size = 16;
    ok &= (spif_utils_clamp_region(REF_SECTOR_SIZE, REF_SECTOR_SIZE - 1, &size) == true);
    ok &= (size == 1); /* only one byte left */

    ok &= (spif_utils_clamp_region(REF_PAGE_SIZE, 0, NULL) == false); /* null size pointer */
    return ok;
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool test_utils_capacity_to_block_count(void)
{
    uint32_t blocks = 0;
    bool ok = true;

    ok &= (spif_utils_capacity_to_block_count(0x11, &blocks) == true) && (blocks == 2);     /* 1 Mbit   */
    ok &= (spif_utils_capacity_to_block_count(0x18, &blocks) == true) && (blocks == 256);   /* 128 Mbit */
    ok &= (spif_utils_capacity_to_block_count(0x1A, &blocks) == true) && (blocks == 1024);  /* 512 Mbit */
    ok &= (spif_utils_capacity_to_block_count(0x20, &blocks) == true) && (blocks == 1024);  /* alt code */
    ok &= (spif_utils_capacity_to_block_count(0x10, &blocks) == false); /* below the range */
    ok &= (spif_utils_capacity_to_block_count(0x1B, &blocks) == false); /* above the range */
    ok &= (spif_utils_capacity_to_block_count(0x18, NULL) == false);    /* null output     */
    return ok;
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* The handle must match what the reference found on the bus, not just be self consistent */
static bool test_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio, uint16_t pin)
{
    bool ok = true;

    if (!spif_init(&spif, hspi, gpio, pin))
    {
        printf("         spif_init() failed, check wiring and CS pin\r\n");
        return false;
    }
    ok &= ((uint8_t)spif.manufactor == ref_id[0]);
    ok &= (spif.mem_type == ref_id[1]);
    ok &= (spif.capacity == ref_id[2]);
    ok &= (spif.block_cnt == ref_block_cnt);
    ok &= (spif.sector_cnt == (ref_block_cnt * (REF_BLOCK_SIZE / REF_SECTOR_SIZE)));
    ok &= (spif.page_cnt == (ref_block_cnt * (REF_BLOCK_SIZE / REF_PAGE_SIZE)));
    ok &= (spif.total_size == ref_total_size);

    if (!ok)
    {
        printf("         handle disagrees with the reference: ID 0x%02X 0x%02X 0x%02X, "
               "%lu blocks, %lu sectors, %lu pages, %lu bytes\r\n",
               (uint8_t)spif.manufactor, spif.mem_type, spif.capacity,
               (unsigned long)spif.block_cnt, (unsigned long)spif.sector_cnt,
               (unsigned long)spif.page_cnt, (unsigned long)spif.total_size);
    }
    return ok;
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* Write enable must set the WEL bit, write disable must clear it, both read by the reference */
static bool test_commander_command(void)
{
    bool ok = true;

    if (!spif_commander_command(&spif, SPIF_CMD_WRITE_ENABLE))
    {
        return false;
    }
    ok &= ((ref_read_status() & REF_STATUS_WEL) != 0);

    if (!spif_commander_command(&spif, SPIF_CMD_WRITE_DISABLE))
    {
        return false;
    }
    ok &= ((ref_read_status() & REF_STATUS_WEL) == 0);
    return ok;
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* The driver must return the same status byte the reference reads, and an idle chip is not busy */
static bool test_commander_read_status(void)
{
    uint8_t expected = ref_read_status();
    uint8_t actual = spif_commander_read_status(&spif);

    if (actual != expected)
    {
        printf("         status 0x%02X, reference reads 0x%02X\r\n", actual, expected);
        return false;
    }
    return ((expected & REF_STATUS_BUSY) == 0);
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* The reference starts a real erase, the driver has to wait it out */
static bool test_commander_wait_for_writing(void)
{
    if (!ref_start_sector_erase(TEST_AREA_ADDRESS(TEST_SECTOR_FOR_ADDRESS)))
    {
        return false;
    }
    if (!spif_commander_wait_for_writing(&spif, REF_TIMEOUT_ERASE))
    {
        printf("         wait timed out during a sector erase\r\n");
        return false;
    }
    /* the reference has to agree that the chip is idle again */
    return ((ref_read_status() & REF_STATUS_BUSY) == 0);
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* Nothing may reach the bus through a missing or uninitialized handle */
static bool test_null_and_uninitialized(void)
{
    spif_handle_t fake;
    bool ok = true;

    memset(&fake, 0, sizeof(fake));

    /* spif_init has to reject missing arguments */
    ok &= (spif_init(NULL, ref_hspi, ref_gpio, ref_pin) == false);
    ok &= (spif_init(&fake, NULL, ref_gpio, ref_pin) == false);
    ok &= (spif_init(&fake, ref_hspi, NULL, ref_pin) == false);

    /* every entry point has to refuse a handle that was never initialized */
    ok &= (spif_read_address(&fake, 0, rx_buf, 16) == false);
    ok &= (spif_write_address(&fake, 0, tx_buf, 16) == false);
    ok &= (spif_read_page(&fake, 0, rx_buf, 16, 0) == false);
    ok &= (spif_write_page(&fake, 0, tx_buf, 16, 0) == false);
    ok &= (spif_read_sector(&fake, 0, rx_buf, 16, 0) == false);
    ok &= (spif_write_sector(&fake, 0, tx_buf, 16, 0) == false);
    ok &= (spif_read_block(&fake, 0, rx_buf, 16, 0) == false);
    ok &= (spif_write_block(&fake, 0, tx_buf, 16, 0) == false);
    ok &= (spif_erase_sector(&fake, 0) == false);
    ok &= (spif_erase_block(&fake, 0) == false);
    ok &= (spif_erase_chip(&fake) == false);

    /* and the same calls with no handle at all */
    ok &= (spif_read_address(NULL, 0, rx_buf, 16) == false);
    ok &= (spif_write_address(NULL, 0, tx_buf, 16) == false);
    ok &= (spif_erase_sector(NULL, 0) == false);
    ok &= (spif_erase_block(NULL, 0) == false);
    return ok;
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool test_erase_block(void)
{
    if (!spif_erase_block(&spif, SPIF_TEST_BLOCK))
    {
        return false;
    }
    return is_blank(TEST_BLOCK_ADDRESS, REF_BLOCK_SIZE);
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* 200 bytes across a page boundary, which exercises the paging loop in both directions */
static bool test_write_read_address(void)
{
    uint32_t base = TEST_AREA_ADDRESS(TEST_SECTOR_FOR_ADDRESS);

    /* the driver writes, the reference reads back */
    fill_pattern(200, 0x10);
    if (!spif_write_address(&spif, base + 0xF0, tx_buf, 200))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!ref_read(base + 0xF0, rx_buf, 200))
    {
        return false;
    }
    if (!compare_pattern(200, 0x10))
    {
        return false;
    }

    /* the reference writes, the driver reads back */
    fill_pattern(200, 0x50);
    if (!ref_write(base + TEST_REF_HALF + 0xF0, tx_buf, 200))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!spif_read_address(&spif, base + TEST_REF_HALF + 0xF0, rx_buf, 200))
    {
        return false;
    }
    return compare_pattern(200, 0x50);
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool test_write_read_page(void)
{
    uint32_t base = TEST_AREA_ADDRESS(TEST_SECTOR_FOR_PAGE);
    uint32_t page = base / REF_PAGE_SIZE;

    /* the driver writes at offset 0x10, the reference reads back */
    fill_pattern(32, 0x20);
    if (!spif_write_page(&spif, page, tx_buf, 32, 0x10))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!ref_read(base + 0x10, rx_buf, 32))
    {
        return false;
    }
    if (!compare_pattern(32, 0x20))
    {
        return false;
    }

    /* the reference writes at offset 0x80, the driver reads back */
    fill_pattern(32, 0x60);
    if (!ref_write(base + 0x80, tx_buf, 32))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!spif_read_page(&spif, page, rx_buf, 32, 0x80))
    {
        return false;
    }
    if (!compare_pattern(32, 0x60))
    {
        return false;
    }
    /* nothing may have leaked into the gap between the two areas */
    return is_blank(base + 0x30, 0x50);
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool test_write_read_sector(void)
{
    uint32_t base = TEST_AREA_ADDRESS(TEST_SECTOR_FOR_SECTOR);
    uint32_t sector = test_sector_number(TEST_SECTOR_FOR_SECTOR);

    /* the driver writes at offset 0x100, the reference reads back */
    fill_pattern(64, 0x30);
    if (!spif_write_sector(&spif, sector, tx_buf, 64, 0x100))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!ref_read(base + 0x100, rx_buf, 64))
    {
        return false;
    }
    if (!compare_pattern(64, 0x30))
    {
        return false;
    }

    /* the reference writes in the upper half, the driver reads back */
    fill_pattern(64, 0x70);
    if (!ref_write(base + TEST_REF_HALF, tx_buf, 64))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!spif_read_sector(&spif, sector, rx_buf, 64, TEST_REF_HALF))
    {
        return false;
    }
    return compare_pattern(64, 0x70);
}

/*---------------------------------------------------------------------------------------------------------------------*/

static bool test_write_read_block(void)
{
    uint32_t base = TEST_AREA_ADDRESS(TEST_SECTOR_FOR_BLOCK);
    uint32_t area_offset = TEST_AREA_OFFSET(TEST_SECTOR_FOR_BLOCK);

    /* the driver writes, the reference reads back */
    fill_pattern(128, 0x40);
    if (!spif_write_block(&spif, SPIF_TEST_BLOCK, tx_buf, 128, area_offset + 0x40))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!ref_read(base + 0x40, rx_buf, 128))
    {
        return false;
    }
    if (!compare_pattern(128, 0x40))
    {
        return false;
    }

    /* the reference writes in the upper half, the driver reads back */
    fill_pattern(128, 0x80);
    if (!ref_write(base + TEST_REF_HALF, tx_buf, 128))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!spif_read_block(&spif, SPIF_TEST_BLOCK, rx_buf, 128, area_offset + TEST_REF_HALF))
    {
        return false;
    }
    return compare_pattern(128, 0x80);
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* Erases only the first test sector, the others must keep their data */
static bool test_erase_sector(void)
{
    uint32_t sector = test_sector_number(TEST_SECTOR_FOR_ADDRESS);

    if (!spif_erase_sector(&spif, sector))
    {
        return false;
    }
    if (!is_blank(TEST_AREA_ADDRESS(TEST_SECTOR_FOR_ADDRESS), REF_SECTOR_SIZE))
    {
        return false;
    }
    /* the sector test data must be untouched */
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!ref_read(TEST_AREA_ADDRESS(TEST_SECTOR_FOR_SECTOR) + 0x100, rx_buf, 64))
    {
        return false;
    }
    return compare_pattern(64, 0x30);
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* An access that would run past its page / sector must be shortened, not rejected or wrapped */
static bool test_write_clamping(void)
{
    uint32_t base = TEST_AREA_ADDRESS(TEST_SECTOR_FOR_CLAMP);
    uint32_t page = base / REF_PAGE_SIZE;
    uint32_t sector = test_sector_number(TEST_SECTOR_FOR_CLAMP);

    /* 64 bytes written at page offset 0xF0: only the last 16 bytes of the page may be used */
    fill_pattern(64, 0x90);
    if (!spif_write_page(&spif, page, tx_buf, 64, REF_PAGE_SIZE - 16))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!ref_read(base + REF_PAGE_SIZE - 16, rx_buf, 16))
    {
        return false;
    }
    if (!compare_pattern(16, 0x90))
    {
        return false;
    }
    if (!is_blank(base + REF_PAGE_SIZE, 16)) /* the next page must be untouched */
    {
        return false;
    }

    /* the same at the end of a sector */
    fill_pattern(64, 0xA0);
    if (!spif_write_sector(&spif, sector, tx_buf, 64, REF_SECTOR_SIZE - 16))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!ref_read(base + REF_SECTOR_SIZE - 16, rx_buf, 16))
    {
        return false;
    }
    if (!compare_pattern(16, 0xA0))
    {
        return false;
    }
    if (!is_blank(base + REF_SECTOR_SIZE, 16)) /* the next sector must be untouched */
    {
        return false;
    }

    /* reads clamp as well, and must not write past the clamped length in the buffer */
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!spif_read_page(&spif, page, rx_buf, 64, REF_PAGE_SIZE - 16))
    {
        return false;
    }
    if (!compare_pattern(16, 0x90))
    {
        return false;
    }
    if (rx_buf[16] != 0x00)
    {
        printf("         read past the end of the page: 0x%02X\r\n", rx_buf[16]);
        return false;
    }
    return true;
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* 512 bytes starting mid page, so the write has to be split over three page programs */
static bool test_write_multiple_pages(void)
{
    uint32_t address = TEST_AREA_ADDRESS(TEST_SECTOR_FOR_PAGING) + 0x80;

    fill_pattern(TEST_BUF_SIZE, 0xB0);
    if (!spif_write_address(&spif, address, tx_buf, TEST_BUF_SIZE))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!ref_read(address, rx_buf, TEST_BUF_SIZE))
    {
        return false;
    }
    if (!compare_pattern(TEST_BUF_SIZE, 0xB0))
    {
        return false;
    }
    /* and the driver has to read the whole span back in one call */
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!spif_read_address(&spif, address, rx_buf, TEST_BUF_SIZE))
    {
        return false;
    }
    return compare_pattern(TEST_BUF_SIZE, 0xB0);
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* A read keeps streaming while CS is low, so it may cross page and sector boundaries */
static bool test_read_across_sectors(void)
{
    uint32_t seam = TEST_AREA_ADDRESS(TEST_SECTOR_FOR_SEAM) + REF_SECTOR_SIZE;

    /* the reference puts 64 bytes on each side of the sector boundary */
    fill_pattern(128, 0xC0);
    if (!ref_write(seam - 64, tx_buf, 128))
    {
        return false;
    }
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!spif_read_address(&spif, seam - 64, rx_buf, 128))
    {
        return false;
    }
    return compare_pattern(128, 0xC0);
}

/*---------------------------------------------------------------------------------------------------------------------*/

/* Bounds come from the reference geometry, so a driver with a wrong idea of the chip fails here */
static bool test_bad_parameters(void)
{
    uint32_t sector_cnt = ref_block_cnt * (REF_BLOCK_SIZE / REF_SECTOR_SIZE);
    uint32_t page_cnt = ref_block_cnt * (REF_BLOCK_SIZE / REF_PAGE_SIZE);
    bool ok = true;

    /* page / sector / block out of range */
    ok &= (spif_write_page(&spif, page_cnt, tx_buf, 16, 0) == false);
    ok &= (spif_read_page(&spif, page_cnt, rx_buf, 16, 0) == false);
    ok &= (spif_erase_sector(&spif, sector_cnt) == false);
    ok &= (spif_erase_block(&spif, ref_block_cnt) == false);

    /* offset outside the region */
    ok &= (spif_write_page(&spif, 0, tx_buf, 16, REF_PAGE_SIZE) == false);
    ok &= (spif_read_sector(&spif, 0, rx_buf, 16, REF_SECTOR_SIZE) == false);
    ok &= (spif_read_block(&spif, 0, rx_buf, 16, REF_BLOCK_SIZE) == false);

    /* address range, null pointer, zero size, null handle */
    ok &= (spif_read_address(&spif, ref_total_size - 4, rx_buf, 16) == false);
    ok &= (spif_write_address(&spif, 0, NULL, 16) == false);
    ok &= (spif_read_address(&spif, 0, rx_buf, 0) == false);
    ok &= (spif_erase_chip(NULL) == false);

    return ok;
}

/*---------------------------------------------------------------------------------------------------------------------*/

#if SPIF_TEST_ERASE_CHIP == 1
static bool test_erase_chip(void)
{
    printf("         erasing the whole chip, this can take a while\r\n");
    if (!spif_erase_chip(&spif))
    {
        return false;
    }
    /* spot check: first and last sector */
    if (!is_blank(0, REF_SECTOR_SIZE))
    {
        return false;
    }
    return is_blank(ref_total_size - REF_SECTOR_SIZE, REF_SECTOR_SIZE);
}
#endif

/**
 * Custom U8G COM handler for EasyThreed K2 Plus
 * Uses hardware SPI2 (PB13=SCK, PB14=MISO, PB15=MOSI)
 * with GPIO chip select, DC, and RESET pins.
 *
 * This replaces both the SW SPI and generic HW SPI handlers
 * to ensure SPI2 is used correctly on the MKS Robin Lite board.
 */

#ifdef __STM32F1__

#include "../../../inc/MarlinConfig.h"

#if HAS_MARLINUI_U8GLIB && !defined(FORCE_SOFT_SPI)

#include <U8glib-HAL.h>
#include <SPI.h>

// Use a dedicated SPI instance for SPI2
static SPIClass spi2(2);  // SPI2
static bool spi2_initialized = false;

uint8_t u8g_com_stm32duino_hw_spi_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr) {
  switch (msg) {
    case U8G_COM_MSG_INIT:
      // Set CS, A0, RESET as outputs
      u8g_SetPIOutput(u8g, U8G_PI_CS);
      u8g_SetPIOutput(u8g, U8G_PI_A0);
      u8g_SetPIOutput(u8g, U8G_PI_RESET);

      // CS HIGH (deselect)
      u8g_SetPILevel(u8g, U8G_PI_CS, 1);

      // Initialize SPI2 explicitly
      if (!spi2_initialized) {
        spi2.setModule(2);
        spi2.begin();
        spi2.setBitOrder(MSBFIRST);
        spi2.setDataMode(SPI_MODE0);
        spi2.setClockDivider(SPI_CLOCK_DIV4);  // 36MHz/4 = 9MHz
        spi2_initialized = true;
      }
      break;

    case U8G_COM_MSG_STOP:
      break;

    case U8G_COM_MSG_RESET:
      u8g_SetPILevel(u8g, U8G_PI_RESET, arg_val);
      break;

    case U8G_COM_MSG_ADDRESS:
      // DC pin: 0 = command, 1 = data
      u8g_SetPILevel(u8g, U8G_PI_A0, arg_val);
      break;

    case U8G_COM_MSG_CHIP_SELECT:
      // arg_val: 0 = deselect (CS HIGH), 1 = select (CS LOW)
      u8g_SetPILevel(u8g, U8G_PI_CS, arg_val ? 0 : 1);
      break;

    case U8G_COM_MSG_WRITE_BYTE:
      spi2.transfer(arg_val);
      break;

    case U8G_COM_MSG_WRITE_SEQ: {
      uint8_t *ptr = (uint8_t *)arg_ptr;
      while (arg_val > 0) {
        spi2.transfer(*ptr++);
        arg_val--;
      }
      break;
    }

    case U8G_COM_MSG_WRITE_SEQ_P: {
      uint8_t *ptr = (uint8_t *)arg_ptr;
      while (arg_val > 0) {
        spi2.transfer(u8g_pgm_read(ptr));
        ptr++;
        arg_val--;
      }
      break;
    }
  }
  return 1;
}

#endif // HAS_MARLINUI_U8GLIB && !FORCE_SOFT_SPI
#endif // __STM32F1__

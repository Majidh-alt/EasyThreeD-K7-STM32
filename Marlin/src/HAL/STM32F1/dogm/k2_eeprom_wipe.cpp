/**
 * One-time I2C EEPROM wipe for EasyThreed K2 Plus
 * Writes 0xFF to all 2048 bytes of the AT24C16 EEPROM
 * so stock firmware can boot fresh.
 *
 * AT24C16: I2C addresses 0x50-0x57 (8 pages of 256 bytes each)
 * Page write: max 16 bytes per write, then 5ms delay
 */

#ifdef __STM32F1__

#include "../../inc/MarlinConfig.h"

#if ENABLED(I2C_EEPROM)

#include <Wire.h>

void k2_eeprom_wipe(void) {
  Wire.begin();

  uint8_t ff_buf[16];
  for (uint8_t i = 0; i < 16; i++) ff_buf[i] = 0xFF;

  // AT24C16: 2048 bytes total
  // Device addresses 0x50-0x57, each handles 256 bytes
  // Write in 16-byte pages
  for (uint8_t dev = 0; dev < 8; dev++) {
    uint8_t i2c_addr = 0x50 + dev;
    for (uint16_t addr = 0; addr < 256; addr += 16) {
      Wire.beginTransmission(i2c_addr);
      Wire.write((uint8_t)addr);  // Memory address within this device page
      for (uint8_t i = 0; i < 16; i++) {
        Wire.write((uint8_t)0xFF);
      }
      Wire.endTransmission();

      // AT24C16 needs 5ms write cycle time
      volatile uint32_t count = 36000;  // ~5ms at 72MHz
      while (count--) __asm__ volatile ("nop");
    }
  }
}

#endif // I2C_EEPROM
#endif // __STM32F1__

/**
 * K2 Plus OLED Test — COMBINED (tries both pin mappings)
 *
 * This test tries OPTION A first, waits 5 seconds, then tries OPTION B.
 * Also prints status over Serial (115200 baud) so you can see which
 * test is running via USB serial monitor.
 *
 * OPTION A (Schematic-based):
 *   CS=PC3, DC=PC0, SCK=PB13, MOSI=PB15, RST=none, SW bit-bang SPI
 *
 * OPTION B (Binary analysis):
 *   CS=PC3, DC=PC1, SCK=PB13, MOSI=PB15, RST=PA11, HW SPI2
 *
 * If you see the checkerboard/white screen during the FIRST 5 seconds,
 * OPTION A is correct. If it appears AFTER the pause, OPTION B is correct.
 *
 * Replace the existing k2_oled_test.cpp in:
 *   Marlin/src/HAL/STM32F1/dogm/k2_oled_test.cpp
 */

#include "../../inc/MarlinConfig.h"
#include <SPI.h>

// ==== Pin definitions ====
// Shared
#define OLED_CS_PIN    PC3
#define OLED_SCK_PIN   PB13
#define OLED_MOSI_PIN  PB15

// Option A specific
#define OLED_DC_A      PC0   // Schematic: LCD_RS

// Option B specific
#define OLED_DC_B      PC1   // Binary analysis: DC/A0
#define OLED_RST_B     PA11  // Binary analysis: RESET

// ==== Software SPI byte send ====
static inline void pin_high(int pin) { digitalWrite(pin, HIGH); }
static inline void pin_low(int pin)  { digitalWrite(pin, LOW); }

static void sw_spi_byte(uint8_t b) {
  for (int8_t i = 7; i >= 0; i--) {
    pin_low(OLED_SCK_PIN);
    if (b & (1 << i)) pin_high(OLED_MOSI_PIN);
    else               pin_low(OLED_MOSI_PIN);
    pin_high(OLED_SCK_PIN);
  }
}

// ==== SSD1306 init sequence (from stock firmware) ====
static void ssd1306_init_sw(int dc_pin) {
  // Helper lambdas for SW SPI
  auto cmd = [&](uint8_t c) {
    pin_low(dc_pin);
    pin_low(OLED_CS_PIN);
    sw_spi_byte(c);
    pin_high(OLED_CS_PIN);
  };

  cmd(0xAE);                     // Display OFF
  cmd(0xD5); cmd(0x80);          // Clock div
  cmd(0xA8); cmd(0x3F);          // Mux 64
  cmd(0xD3); cmd(0x00);          // Offset 0
  cmd(0x40);                     // Start line 0
  cmd(0x8D); cmd(0x14);          // Charge pump ON
  cmd(0x20); cmd(0x00);          // Horiz addr mode
  cmd(0xA1);                     // Seg remap
  cmd(0xC8);                     // COM scan dec
  cmd(0xDA); cmd(0x12);          // COM pins
  cmd(0x81); cmd(0xCF);          // Contrast
  cmd(0xD9); cmd(0xF1);          // Pre-charge
  cmd(0xDB); cmd(0x40);          // VCOMH
  cmd(0x2E);                     // Stop scroll
  cmd(0xA4);                     // RAM display
  cmd(0xA6);                     // Normal
  cmd(0xAF);                     // Display ON
}

static void draw_checkerboard_sw(int dc_pin) {
  // Set address window
  auto cmd = [&](uint8_t c) {
    pin_low(dc_pin);
    pin_low(OLED_CS_PIN);
    sw_spi_byte(c);
    pin_high(OLED_CS_PIN);
  };

  cmd(0x21); cmd(0); cmd(127);   // Col 0-127
  cmd(0x22); cmd(0); cmd(7);     // Page 0-7

  pin_high(dc_pin);
  pin_low(OLED_CS_PIN);
  for (uint8_t page = 0; page < 8; page++)
    for (uint8_t col = 0; col < 128; col++)
      sw_spi_byte(((col / 8) + page) & 1 ? 0xFF : 0x00);
  pin_high(OLED_CS_PIN);
}

static void draw_white_sw(int dc_pin) {
  auto cmd = [&](uint8_t c) {
    pin_low(dc_pin);
    pin_low(OLED_CS_PIN);
    sw_spi_byte(c);
    pin_high(OLED_CS_PIN);
  };

  cmd(0x21); cmd(0); cmd(127);
  cmd(0x22); cmd(0); cmd(7);

  pin_high(dc_pin);
  pin_low(OLED_CS_PIN);
  for (uint16_t i = 0; i < 1024; i++) sw_spi_byte(0xFF);
  pin_high(OLED_CS_PIN);
}

static void display_off_sw(int dc_pin) {
  pin_low(dc_pin);
  pin_low(OLED_CS_PIN);
  sw_spi_byte(0xAE);
  pin_high(OLED_CS_PIN);
}

// ==== HW SPI2 version ====
static SPIClass spi2(2);

static void ssd1306_init_hw(int dc_pin) {
  auto cmd = [&](uint8_t c) {
    pin_low(dc_pin);
    pin_low(OLED_CS_PIN);
    spi2.transfer(c);
    pin_high(OLED_CS_PIN);
  };

  cmd(0xAE);
  cmd(0xD5); cmd(0x80);
  cmd(0xA8); cmd(0x3F);
  cmd(0xD3); cmd(0x00);
  cmd(0x40);
  cmd(0x8D); cmd(0x14);
  cmd(0x20); cmd(0x00);
  cmd(0xA1);
  cmd(0xC8);
  cmd(0xDA); cmd(0x12);
  cmd(0x81); cmd(0xCF);
  cmd(0xD9); cmd(0xF1);
  cmd(0xDB); cmd(0x40);
  cmd(0x2E);
  cmd(0xA4);
  cmd(0xA6);
  cmd(0xAF);
}

static void draw_checkerboard_hw(int dc_pin) {
  auto cmd = [&](uint8_t c) {
    pin_low(dc_pin);
    pin_low(OLED_CS_PIN);
    spi2.transfer(c);
    pin_high(OLED_CS_PIN);
  };

  cmd(0x21); cmd(0); cmd(127);
  cmd(0x22); cmd(0); cmd(7);

  pin_high(dc_pin);
  pin_low(OLED_CS_PIN);
  for (uint8_t page = 0; page < 8; page++)
    for (uint8_t col = 0; col < 128; col++)
      spi2.transfer(((col / 8) + page) & 1 ? 0xFF : 0x00);
  pin_high(OLED_CS_PIN);
}

static void draw_white_hw(int dc_pin) {
  auto cmd = [&](uint8_t c) {
    pin_low(dc_pin);
    pin_low(OLED_CS_PIN);
    spi2.transfer(c);
    pin_high(OLED_CS_PIN);
  };

  cmd(0x21); cmd(0); cmd(127);
  cmd(0x22); cmd(0); cmd(7);

  pin_high(dc_pin);
  pin_low(OLED_CS_PIN);
  for (uint16_t i = 0; i < 1024; i++) spi2.transfer(0xFF);
  pin_high(OLED_CS_PIN);
}

static void display_off_hw(int dc_pin) {
  pin_low(dc_pin);
  pin_low(OLED_CS_PIN);
  spi2.transfer(0xAE);
  pin_high(OLED_CS_PIN);
}


// ============================================================
// MAIN TEST
// ============================================================
extern "C" void k2_oled_test(void) {

  SERIAL_ECHOLNPGM("=== K2 OLED COMBINED TEST ===");
  SERIAL_ECHOLNPGM("If display works during A test -> Option A is correct");
  SERIAL_ECHOLNPGM("If display works during B test -> Option B is correct");

  // ============================================================
  // OPTION A: Schematic pins, SW SPI, DC=PC0, no reset
  // ============================================================
  SERIAL_ECHOLNPGM("");
  SERIAL_ECHOLNPGM(">>> OPTION A: CS=PC3, DC=PC0, SW-SPI PB13/PB15, no RST");
  SERIAL_ECHOLNPGM(">>> Starting in 1 second...");
  delay(1000);

  // Configure pins
  pinMode(OLED_CS_PIN,   OUTPUT);
  pinMode(OLED_DC_A,     OUTPUT);
  pinMode(OLED_SCK_PIN,  OUTPUT);
  pinMode(OLED_MOSI_PIN, OUTPUT);
  pin_high(OLED_CS_PIN);
  pin_high(OLED_SCK_PIN);
  delay(100);

  SERIAL_ECHOLNPGM(">>> A: Sending SSD1306 init...");
  ssd1306_init_sw(OLED_DC_A);

  SERIAL_ECHOLNPGM(">>> A: Drawing checkerboard...");
  draw_checkerboard_sw(OLED_DC_A);

  SERIAL_ECHOLNPGM(">>> A: Checkerboard displayed. Waiting 5 sec...");
  delay(5000);

  SERIAL_ECHOLNPGM(">>> A: Drawing full white...");
  draw_white_sw(OLED_DC_A);

  SERIAL_ECHOLNPGM(">>> A: White displayed. Waiting 5 sec...");
  delay(5000);

  // Turn off display before switching
  display_off_sw(OLED_DC_A);
  SERIAL_ECHOLNPGM(">>> A: Display OFF. Switching to Option B...");
  delay(2000);

  // ============================================================
  // OPTION B: Binary analysis pins, HW SPI2, DC=PC1, RST=PA11
  // ============================================================
  SERIAL_ECHOLNPGM("");
  SERIAL_ECHOLNPGM(">>> OPTION B: CS=PC3, DC=PC1, HW-SPI2, RST=PA11");
  SERIAL_ECHOLNPGM(">>> Starting in 1 second...");
  delay(1000);

  // Configure pins
  pinMode(OLED_DC_B,    OUTPUT);
  pinMode(OLED_RST_B,   OUTPUT);
  pin_high(OLED_CS_PIN);

  // Hardware reset
  SERIAL_ECHOLNPGM(">>> B: Hardware reset via PA11...");
  pin_high(OLED_RST_B);
  delay(10);
  pin_low(OLED_RST_B);
  delay(50);
  pin_high(OLED_RST_B);
  delay(100);

  // Init HW SPI2
  spi2.begin();
  spi2.setBitOrder(MSBFIRST);
  spi2.setDataMode(SPI_MODE0);
  spi2.setClockDivider(SPI_CLOCK_DIV4);

  SERIAL_ECHOLNPGM(">>> B: Sending SSD1306 init...");
  ssd1306_init_hw(OLED_DC_B);

  SERIAL_ECHOLNPGM(">>> B: Drawing checkerboard...");
  draw_checkerboard_hw(OLED_DC_B);

  SERIAL_ECHOLNPGM(">>> B: Checkerboard displayed. Waiting 5 sec...");
  delay(5000);

  SERIAL_ECHOLNPGM(">>> B: Drawing full white...");
  draw_white_hw(OLED_DC_B);

  SERIAL_ECHOLNPGM(">>> B: White displayed. Waiting 5 sec...");
  delay(5000);

  // ============================================================
  // OPTION C: Hybrid — SW SPI with DC=PC1, RST=PA11
  // (in case HW SPI2 init is the problem, not the pins)
  // ============================================================
  display_off_hw(OLED_DC_B);
  SERIAL_ECHOLNPGM("");
  SERIAL_ECHOLNPGM(">>> OPTION C: CS=PC3, DC=PC1, SW-SPI PB13/PB15, RST=PA11");
  delay(1000);

  // Reconfigure PB13/PB15 as GPIO (undo SPI2 AF)
  spi2.end();
  pinMode(OLED_SCK_PIN,  OUTPUT);
  pinMode(OLED_MOSI_PIN, OUTPUT);
  pin_high(OLED_SCK_PIN);

  // Reset again
  pin_high(OLED_RST_B);
  delay(10);
  pin_low(OLED_RST_B);
  delay(50);
  pin_high(OLED_RST_B);
  delay(100);

  SERIAL_ECHOLNPGM(">>> C: Sending SSD1306 init...");
  ssd1306_init_sw(OLED_DC_B);   // Same init, but with DC=PC1

  SERIAL_ECHOLNPGM(">>> C: Drawing checkerboard...");
  draw_checkerboard_sw(OLED_DC_B);

  SERIAL_ECHOLNPGM(">>> C: Checkerboard displayed. Waiting 5 sec...");
  delay(5000);

  SERIAL_ECHOLNPGM(">>> C: Drawing full white...");
  draw_white_sw(OLED_DC_B);

  SERIAL_ECHOLNPGM(">>> C: White displayed. Waiting 30 sec...");
  delay(30000);

  SERIAL_ECHOLNPGM("=== TEST COMPLETE ===");
  SERIAL_ECHOLNPGM("If you saw the pattern during A -> Use PC0 for DC");
  SERIAL_ECHOLNPGM("If you saw the pattern during B -> Use PC1+PA11+HW-SPI2");
  SERIAL_ECHOLNPGM("If you saw the pattern during C -> Use PC1+PA11+SW-SPI");
  SERIAL_ECHOLNPGM("If nothing appeared -> Pin mapping is still wrong");
}

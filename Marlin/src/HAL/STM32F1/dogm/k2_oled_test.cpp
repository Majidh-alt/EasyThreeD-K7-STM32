/**
 * K2 Plus — GPIO PIN SCANNER v3
 * - Waits 10 seconds for USB serial to initialize
 * - Each pin gets a unique beep count (1 beep, 2 beeps, 3 beeps...)
 * - Long pause between pins so you can count
 * - Then tries OLED init combos
 *
 * Replace k2_oled_test.cpp with this file.
 */

#include "../../inc/MarlinConfig.h"

static inline void pin_high(int pin) { digitalWrite(pin, HIGH); }
static inline void pin_low(int pin)  { digitalWrite(pin, LOW); }

struct PinInfo {
  int pin;
  const char* name;
};

static const PinInfo candidates[] = {
  { PA8,  "PA8"  },   // 1 beep
  { PA11, "PA11" },   // 2 beeps
  { PA12, "PA12" },   // 3 beeps
  { PB3,  "PB3"  },   // 4 beeps
  { PB4,  "PB4"  },   // 5 beeps
  { PB5,  "PB5"  },   // 6 beeps
  { PB13, "PB13" },   // 7 beeps
  { PB15, "PB15" },   // 8 beeps
  { PC0,  "PC0"  },   // 9 beeps
  { PC1,  "PC1"  },   // 10 beeps
  { PC2,  "PC2"  },   // 11 beeps
  { PC3,  "PC3"  },   // 12 beeps
  { PC7,  "PC7"  },   // 13 beeps
  { PD2,  "PD2"  },   // 14 beeps
};

static const int NUM_CANDIDATES = sizeof(candidates) / sizeof(candidates[0]);

// SW SPI helpers
static int current_sck, current_mosi;

static void sw_spi_byte(uint8_t b) {
  for (int8_t i = 7; i >= 0; i--) {
    pin_low(current_sck);
    if (b & (1 << i)) pin_high(current_mosi);
    else               pin_low(current_mosi);
    pin_high(current_sck);
  }
}

static void oled_cmd_sw(int cs, int dc, uint8_t c) {
  pin_low(dc);
  pin_low(cs);
  sw_spi_byte(c);
  pin_high(cs);
}

static void try_oled(int cs, int dc, int sck, int mosi, int rst, int num) {
  current_sck = sck;
  current_mosi = mosi;

  MYSERIAL1.print("A");
  MYSERIAL1.print(num);
  MYSERIAL1.println(" start");

  pinMode(cs,   OUTPUT);
  pinMode(dc,   OUTPUT);
  pinMode(sck,  OUTPUT);
  pinMode(mosi, OUTPUT);
  pin_high(cs);
  pin_high(sck);

  if (rst >= 0) {
    pinMode(rst, OUTPUT);
    pin_high(rst); delay(10);
    pin_low(rst);  delay(50);
    pin_high(rst); delay(100);
  } else {
    delay(100);
  }

  // SSD1306 init
  oled_cmd_sw(cs, dc, 0xAE);
  oled_cmd_sw(cs, dc, 0xD5); oled_cmd_sw(cs, dc, 0x80);
  oled_cmd_sw(cs, dc, 0xA8); oled_cmd_sw(cs, dc, 0x3F);
  oled_cmd_sw(cs, dc, 0xD3); oled_cmd_sw(cs, dc, 0x00);
  oled_cmd_sw(cs, dc, 0x40);
  oled_cmd_sw(cs, dc, 0x8D); oled_cmd_sw(cs, dc, 0x14);
  oled_cmd_sw(cs, dc, 0x20); oled_cmd_sw(cs, dc, 0x00);
  oled_cmd_sw(cs, dc, 0xA1);
  oled_cmd_sw(cs, dc, 0xC8);
  oled_cmd_sw(cs, dc, 0xDA); oled_cmd_sw(cs, dc, 0x12);
  oled_cmd_sw(cs, dc, 0x81); oled_cmd_sw(cs, dc, 0xCF);
  oled_cmd_sw(cs, dc, 0xD9); oled_cmd_sw(cs, dc, 0xF1);
  oled_cmd_sw(cs, dc, 0xDB); oled_cmd_sw(cs, dc, 0x40);
  oled_cmd_sw(cs, dc, 0x2E);
  oled_cmd_sw(cs, dc, 0xA4);
  oled_cmd_sw(cs, dc, 0xA6);
  oled_cmd_sw(cs, dc, 0xAF);

  // Checkerboard
  oled_cmd_sw(cs, dc, 0x21); oled_cmd_sw(cs, dc, 0); oled_cmd_sw(cs, dc, 127);
  oled_cmd_sw(cs, dc, 0x22); oled_cmd_sw(cs, dc, 0); oled_cmd_sw(cs, dc, 7);

  pin_high(dc);
  pin_low(cs);
  for (uint8_t page = 0; page < 8; page++)
    for (uint8_t col = 0; col < 128; col++)
      sw_spi_byte(((col / 8) + page) & 1 ? 0xFF : 0x00);
  pin_high(cs);

  // Full white after 3 sec
  delay(3000);
  oled_cmd_sw(cs, dc, 0x21); oled_cmd_sw(cs, dc, 0); oled_cmd_sw(cs, dc, 127);
  oled_cmd_sw(cs, dc, 0x22); oled_cmd_sw(cs, dc, 0); oled_cmd_sw(cs, dc, 7);
  pin_high(dc);
  pin_low(cs);
  for (uint16_t i = 0; i < 1024; i++) sw_spi_byte(0xFF);
  pin_high(cs);

  delay(3000);
  oled_cmd_sw(cs, dc, 0xAE);
  delay(500);

  MYSERIAL1.print("A");
  MYSERIAL1.print(num);
  MYSERIAL1.println(" done");
}

extern "C" void k2_oled_test(void) {

  // ======================================================
  // Wait 10 seconds for USB serial to fully initialize
  // ======================================================
  delay(10000);

  MYSERIAL1.begin(115200);
  delay(1000);

  MYSERIAL1.println("==========================");
  MYSERIAL1.println("K2 GPIO SCANNER v3");
  MYSERIAL1.println("==========================");

  // ======================================================
  // PHASE 1: Toggle each pin with unique beep count
  // Pin 1 = 1 pulse, Pin 2 = 2 pulses, etc.
  // Each pulse: 200ms ON, 200ms OFF
  // 3 second pause between pins
  // ======================================================
  MYSERIAL1.println("PHASE 1: PIN TOGGLE");
  MYSERIAL1.println("Count the beeps!");
  delay(2000);

  for (int i = 0; i < NUM_CANDIDATES; i++) {
    int p = candidates[i].pin;
    int beep_count = i + 1;

    MYSERIAL1.print("Pin ");
    MYSERIAL1.print(beep_count);
    MYSERIAL1.print(": ");
    MYSERIAL1.println(candidates[i].name);

    pinMode(p, OUTPUT);

    // Send beep_count pulses
    for (int b = 0; b < beep_count; b++) {
      pin_high(p);
      delay(200);
      pin_low(p);
      delay(200);
    }

    // 3 second silence before next pin
    delay(3000);
  }

  MYSERIAL1.println("PHASE 1 DONE");
  MYSERIAL1.println("==========================");
  delay(3000);

  // ======================================================
  // PHASE 2: Try OLED init with different pin combos
  // Watch display for checkerboard/white pattern
  // ======================================================
  MYSERIAL1.println("PHASE 2: OLED ATTEMPTS");
  delay(2000);

  // A1: Binary analysis original
  try_oled(PC3, PC1, PB13, PB15, PA11, 1);

  // A2: Binary analysis no reset
  try_oled(PC3, PC1, PB13, PB15, -1, 2);

  // A3: Schematic DC=PC0
  try_oled(PC3, PC0, PB13, PB15, -1, 3);

  // A4: Row-reversal: CS=PD2, DC=PC3, SCK=PB5, MOSI=PC1
  try_oled(PD2, PC3, PB5, PC1, -1, 4);

  // A5: Row-reversal + reset
  try_oled(PD2, PC3, PB5, PC1, PA11, 5);

  // A6: Swapped CS/DC from binary
  try_oled(PC1, PC3, PB13, PB15, PA11, 6);

  // A7: CS=PC0, DC=PC3
  try_oled(PC0, PC3, PB13, PB15, PA11, 7);

  // A8: Row-reversal alt SCK/MOSI
  try_oled(PD2, PC3, PB3, PB4, -1, 8);

  // A9: CS=PC7
  try_oled(PC7, PC3, PB5, PC1, -1, 9);

  // A10: CS=PD2, DC=PC0
  try_oled(PD2, PC0, PB13, PB15, PA11, 10);

  // A11: CS=PA8
  try_oled(PA8, PC3, PB5, PC1, -1, 11);

  // A12: SCK=PC0, MOSI=PC1, CS=PD2, DC=PC3
  try_oled(PD2, PC3, PC0, PC1, -1, 12);

  MYSERIAL1.println("==========================");
  MYSERIAL1.println("ALL TESTS COMPLETE");
  MYSERIAL1.println("==========================");

  while(1) delay(1000);
}

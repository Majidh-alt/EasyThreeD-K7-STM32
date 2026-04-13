/**
 * K2 Plus — GPIO PIN SCANNER v4
 *
 * FIXES:
 * - Immediately sets PD2 (buzzer) LOW to stop beeping
 * - Waits for USB serial properly
 * - Unique beep count per pin
 * - Then tries OLED combos
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

// Pins to test - EXCLUDES PD2 (buzzer) from toggle test
// to avoid constant beeping during scan
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
};

static const int NUM_CANDIDATES = sizeof(candidates) / sizeof(candidates[0]);

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

  delay(3000);

  // Full white
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

// Use PD2 buzzer to beep N times (as feedback since we control it)
static void buzzer_beep(int count) {
  for (int i = 0; i < count; i++) {
    pin_high(PD2);
    delay(100);
    pin_low(PD2);
    delay(100);
  }
}

extern "C" void k2_oled_test(void) {

  // =============================================
  // IMMEDIATELY silence the buzzer
  // =============================================
  pinMode(PD2, OUTPUT);
  pin_low(PD2);

  // Also set all EXP1 signal pins to safe defaults
  pinMode(PC0, OUTPUT); pin_low(PC0);
  pinMode(PC1, OUTPUT); pin_low(PC1);
  pinMode(PC2, OUTPUT); pin_low(PC2);
  pinMode(PC3, OUTPUT); pin_low(PC3);

  // =============================================
  // Wait for USB serial to initialize
  // =============================================
  delay(5000);

  // Signal we're alive: 3 short beeps
  buzzer_beep(3);

  delay(5000);

  MYSERIAL1.println("==========================");
  MYSERIAL1.println("K2 GPIO SCANNER v4");
  MYSERIAL1.println("==========================");
  MYSERIAL1.println("Buzzer silenced OK");
  MYSERIAL1.println("Starting Phase 1...");

  // Signal Phase 1 start: 1 long beep
  pin_high(PD2); delay(500); pin_low(PD2); delay(1000);

  // =============================================
  // PHASE 1: Toggle each pin with unique count
  // Listen for buzzer — if a pin triggers sound
  // on the display board, count the beeps
  // =============================================
  for (int i = 0; i < NUM_CANDIDATES; i++) {
    int p = candidates[i].pin;
    int beep_count = i + 1;

    MYSERIAL1.print("Pin ");
    MYSERIAL1.print(beep_count);
    MYSERIAL1.print("/13: ");
    MYSERIAL1.println(candidates[i].name);

    pinMode(p, OUTPUT);

    // Send beep_count short pulses
    for (int b = 0; b < beep_count; b++) {
      pin_high(p);
      delay(150);
      pin_low(p);
      delay(150);
    }

    pin_low(p);

    // 3 second silence
    delay(3000);
  }

  MYSERIAL1.println("PHASE 1 DONE");

  // Signal Phase 2: 2 long beeps via PD2
  pin_high(PD2); delay(500); pin_low(PD2); delay(300);
  pin_high(PD2); delay(500); pin_low(PD2); delay(2000);

  // =============================================
  // PHASE 2: OLED init attempts
  // Row-reversal mapping theory:
  //   Odd pins:  mobo 1<->13, 3<->11, 5<->9, 7=7
  //   Even pins: mobo 2<->14, 4<->12, 6<->10, 8=8
  //
  // So OLED signals map to:
  //   Display CS (pin1)   -> Mobo pin13 (IO1)
  //   Display DC (pin13)  -> Mobo pin1  (PC3)
  //   Display SCLK(pin7)  -> Mobo pin7  (PC1)
  //   Display MOSI(pin11) -> Mobo pin3  (PC2)
  //   Display GND (pin3)  -> Mobo pin11 (GND) ✓
  //   Display VCC (pin4)  -> Mobo pin12 (3V3) ✓
  //   Display VCC (pin2)  -> Mobo pin14 (PD2/buzzer)
  //
  // If IO1 is not MCU-controllable, CS may be
  // active-low via pull-down, or we find the MCU pin.
  // =============================================
  MYSERIAL1.println("PHASE 2: OLED ATTEMPTS");
  delay(1000);

  // A1: Binary analysis (original assumption)
  try_oled(PC3, PC1, PB13, PB15, PA11, 1);

  // A2: Row-reversal: DC=PC3, SCLK=PC1, MOSI=PC2, CS=PC0 (guess)
  try_oled(PC0, PC3, PC1, PC2, -1, 2);

  // A3: Row-reversal: DC=PC3, SCLK=PC1, MOSI=PC2, CS=PA11
  try_oled(PA11, PC3, PC1, PC2, -1, 3);

  // A4: Row-reversal: DC=PC3, SCLK=PC1, MOSI=PC2, CS=PA8
  try_oled(PA8, PC3, PC1, PC2, -1, 4);

  // A5: Row-reversal: DC=PC3, SCLK=PC1, MOSI=PC2, CS=PC7
  try_oled(PC7, PC3, PC1, PC2, -1, 5);

  // A6: Row-reversal with CS tied low (always selected)
  // Just ground CS and see if DC/SCK/MOSI alone work
  // We'll use a pin set permanently LOW as CS
  pinMode(PA12, OUTPUT); pin_low(PA12);
  try_oled(PA12, PC3, PC1, PC2, -1, 6);

  // A7: Schematic direct: CS=PC3, DC=PC0, SCK=PB13, MOSI=PB15
  try_oled(PC3, PC0, PB13, PB15, -1, 7);

  // A8: Schematic: CS=PC1, DC=PC0, SCK=PB13, MOSI=PB15
  try_oled(PC1, PC0, PB13, PB15, -1, 8);

  // A9: DC=PC3, SCK=PC0, MOSI=PC1, CS=PC2
  try_oled(PC2, PC3, PC0, PC1, -1, 9);

  // A10: Binary + swap CS/DC: CS=PC1, DC=PC3, SCK=PB13, MOSI=PB15
  try_oled(PC1, PC3, PB13, PB15, PA11, 10);

  // A11: CS=PD2, DC=PC3, SCK=PC1, MOSI=PC2
  // (PD2 reaches display board — maybe it's actually CS not buzzer?)
  try_oled(PD2, PC3, PC1, PC2, -1, 11);

  // A12: CS=PD2, DC=PC3, SCK=PC1, MOSI=PC2, RST=PA11
  try_oled(PD2, PC3, PC1, PC2, PA11, 12);

  MYSERIAL1.println("==========================");
  MYSERIAL1.println("ALL TESTS COMPLETE");
  MYSERIAL1.println("==========================");

  // 5 beeps to signal completion
  delay(1000);
  buzzer_beep(5);

  while(1) delay(1000);
}

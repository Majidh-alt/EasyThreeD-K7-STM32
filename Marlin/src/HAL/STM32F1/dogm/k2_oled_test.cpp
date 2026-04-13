/**
 * K2 Plus — GPIO PIN SCANNER (FIXED BUILD)
 * Replace k2_oled_test.cpp with this file.
 */

#include "../../inc/MarlinConfig.h"
#include <SPI.h>

static inline void pin_high(int pin) { digitalWrite(pin, HIGH); }
static inline void pin_low(int pin)  { digitalWrite(pin, LOW); }

struct PinInfo {
  int pin;
  const char* name;
};

static const PinInfo candidates[] = {
  { PA8,  "PA8" },
  { PA11, "PA11" },
  { PA12, "PA12" },
  { PB3,  "PB3" },
  { PB4,  "PB4" },
  { PB5,  "PB5" },
  { PB6,  "PB6" },
  { PB7,  "PB7" },
  { PB8,  "PB8" },
  { PB13, "PB13" },
  { PB15, "PB15" },
  { PC0,  "PC0" },
  { PC1,  "PC1" },
  { PC2,  "PC2" },
  { PC3,  "PC3" },
  { PC7,  "PC7" },
  { PD2,  "PD2" },
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

static void try_oled_init(int cs, int dc, int sck, int mosi, int rst,
                          const char* cs_name, const char* dc_name,
                          const char* sck_name, const char* mosi_name,
                          const char* rst_name, int attempt_num) {
  current_sck = sck;
  current_mosi = mosi;

  MYSERIAL1.print(">>> Attempt A");
  MYSERIAL1.print(attempt_num);
  MYSERIAL1.print(": CS=");
  MYSERIAL1.print(cs_name);
  MYSERIAL1.print(" DC=");
  MYSERIAL1.print(dc_name);
  MYSERIAL1.print(" SCK=");
  MYSERIAL1.print(sck_name);
  MYSERIAL1.print(" MOSI=");
  MYSERIAL1.print(mosi_name);
  MYSERIAL1.print(" RST=");
  MYSERIAL1.println(rst_name);

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

  oled_cmd_sw(cs, dc, 0x21); oled_cmd_sw(cs, dc, 0); oled_cmd_sw(cs, dc, 127);
  oled_cmd_sw(cs, dc, 0x22); oled_cmd_sw(cs, dc, 0); oled_cmd_sw(cs, dc, 7);

  pin_high(dc);
  pin_low(cs);
  for (uint8_t page = 0; page < 8; page++)
    for (uint8_t col = 0; col < 128; col++)
      sw_spi_byte(((col / 8) + page) & 1 ? 0xFF : 0x00);
  pin_high(cs);

  MYSERIAL1.println(">>> Pattern sent. Waiting 4 sec...");
  delay(4000);

  oled_cmd_sw(cs, dc, 0xAE);
  delay(500);
}

extern "C" void k2_oled_test(void) {

  MYSERIAL1.println("========================================");
  MYSERIAL1.println("=== K2 PLUS GPIO PIN SCANNER ===");
  MYSERIAL1.println("========================================");
  delay(2000);

  // PHASE 1: Toggle each pin
  for (int i = 0; i < NUM_CANDIDATES; i++) {
    int p = candidates[i].pin;

    MYSERIAL1.print("Toggling: ");
    MYSERIAL1.println(candidates[i].name);

    pinMode(p, OUTPUT);
    unsigned long start = millis();
    while (millis() - start < 3000) {
      pin_high(p);
      delay(50);
      pin_low(p);
      delay(50);
    }
    pin_low(p);

    MYSERIAL1.print("  Done: ");
    MYSERIAL1.println(candidates[i].name);
    delay(2000);
  }

  MYSERIAL1.println("=== PHASE 1 COMPLETE ===");
  delay(3000);

  // PHASE 2: Try OLED init combos
  MYSERIAL1.println("=== PHASE 2: OLED INIT ATTEMPTS ===");
  delay(2000);

  try_oled_init(PC3, PC1, PB13, PB15, PA11,  "PC3","PC1","PB13","PB15","PA11", 1);
  try_oled_init(PC3, PC1, PB13, PB15, -1,    "PC3","PC1","PB13","PB15","none", 2);
  try_oled_init(PC3, PC0, PB13, PB15, -1,    "PC3","PC0","PB13","PB15","none", 3);
  try_oled_init(PD2, PC3, PB5,  PC1,  -1,    "PD2","PC3","PB5","PC1","none",   4);
  try_oled_init(PD2, PC3, PB5,  PC1,  PA11,  "PD2","PC3","PB5","PC1","PA11",   5);
  try_oled_init(PC1, PC3, PB13, PB15, PA11,  "PC1","PC3","PB13","PB15","PA11", 6);
  try_oled_init(PC0, PC3, PB13, PB15, PA11,  "PC0","PC3","PB13","PB15","PA11", 7);
  try_oled_init(PD2, PC3, PB3,  PB4,  -1,    "PD2","PC3","PB3","PB4","none",   8);
  try_oled_init(PC7, PC3, PB5,  PC1,  -1,    "PC7","PC3","PB5","PC1","none",   9);
  try_oled_init(PD2, PC0, PB13, PB15, PA11,  "PD2","PC0","PB13","PB15","PA11", 10);
  try_oled_init(PA8, PC3, PB5,  PC1,  -1,    "PA8","PC3","PB5","PC1","none",   11);
  try_oled_init(PD2, PC3, PC0,  PC1,  -1,    "PD2","PC3","PC0","PC1","none",   12);

  MYSERIAL1.println("=== ALL TESTS COMPLETE ===");

  while(1) delay(1000);
}

/**
 * K2 Plus — GPIO PIN SCANNER
 *
 * Toggles each candidate GPIO pin one at a time.
 * Connect USB and open serial monitor at 115200 baud.
 * Watch the display AND listen for the buzzer.
 *
 * For each pin, the code:
 *   1. Prints which pin it's toggling
 *   2. Toggles the pin HIGH/LOW rapidly for 3 seconds
 *   3. Pauses 2 seconds before next pin
 *
 * YOU note down:
 *   - Which pin makes the buzzer sound
 *   - Which pin (if any) causes display flicker
 *
 * After the scan, it tries a full SSD1306 init + checkerboard
 * on every possible pin combination (CS/DC pairs).
 *
 * Replace k2_oled_test.cpp with this file.
 * Called from MarlinCore.cpp setup()
 */

#include "../../inc/MarlinConfig.h"

static inline void pin_high(int pin) { digitalWrite(pin, HIGH); }
static inline void pin_low(int pin)  { digitalWrite(pin, LOW); }

// All candidate pins on EXP1 + nearby
struct PinInfo {
  int pin;
  const char* name;
};

static const PinInfo candidates[] = {
  { PA8,  "PA8  (FAN/IO?)" },
  { PA11, "PA11 (NRST?)"   },
  { PA12, "PA12"            },
  { PA15, "PA15 (SD_CS)"   },
  { PB0,  "PB0  (Z_STEP)"  },
  { PB1,  "PB1  (Z_DIR?)"  },
  { PB3,  "PB3  (BTN_ENC)" },
  { PB4,  "PB4  (BTN_EN2)" },
  { PB5,  "PB5  (BTN_EN1)" },
  { PB6,  "PB6  (I2C_SCL)" },
  { PB7,  "PB7  (I2C_SDA)" },
  { PB8,  "PB8  (Filament)"},
  { PB13, "PB13 (SPI2_SCK)"},
  { PB15, "PB15 (SPI2_MOSI)"},
  { PC0,  "PC0  (LCD_RS)"  },
  { PC1,  "PC1  (LCD_CS)"  },
  { PC2,  "PC2"             },
  { PC3,  "PC3"             },
  { PC4,  "PC4  (E0_STEP)" },
  { PC5,  "PC5  (Z_DIR)"   },
  { PC6,  "PC6  (X_STEP)"  },
  { PC7,  "PC7  (IO1?)"    },
  { PC8,  "PC8  (HOTBED)"  },
  { PC9,  "PC9  (HEATER1)" },
  { PC10, "PC10 (SD_DET?)" },
  { PC11, "PC11 (SD_MISO)" },
  { PC12, "PC12 (X-)"      },
  { PC13, "PC13 (Z-)"      },
  { PD2,  "PD2  (BEERPER)" },
};

static const int NUM_CANDIDATES = sizeof(candidates) / sizeof(candidates[0]);

// ---- SW SPI helpers ----
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

static void try_oled_init(int cs, int dc, int sck, int mosi, int rst, const char* label) {
  current_sck = sck;
  current_mosi = mosi;

  SERIAL_ECHOPGM(">>> Trying: ");
  SERIAL_ECHOLNPGM(label);

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

  // Draw checkerboard
  oled_cmd_sw(cs, dc, 0x21); oled_cmd_sw(cs, dc, 0); oled_cmd_sw(cs, dc, 127);
  oled_cmd_sw(cs, dc, 0x22); oled_cmd_sw(cs, dc, 0); oled_cmd_sw(cs, dc, 7);

  pin_high(dc);
  pin_low(cs);
  for (uint8_t page = 0; page < 8; page++)
    for (uint8_t col = 0; col < 128; col++)
      sw_spi_byte(((col / 8) + page) & 1 ? 0xFF : 0x00);
  pin_high(cs);

  SERIAL_ECHOLNPGM(">>> Pattern sent. Waiting 4 sec...");
  delay(4000);

  // Turn off
  oled_cmd_sw(cs, dc, 0xAE);
  delay(500);
}

// ============================================================
extern "C" void k2_oled_test(void) {

  // ============================================================
  // PHASE 1: Toggle each pin individually
  // ============================================================
  SERIAL_ECHOLNPGM("========================================");
  SERIAL_ECHOLNPGM("=== K2 PLUS GPIO PIN SCANNER ===");
  SERIAL_ECHOLNPGM("Watch display + listen for buzzer");
  SERIAL_ECHOLNPGM("========================================");
  delay(2000);

  for (int i = 0; i < NUM_CANDIDATES; i++) {
    int p = candidates[i].pin;
    const char* name = candidates[i].name;

    SERIAL_ECHOPGM("Toggling: ");
    SERIAL_ECHOLNPGM(name);

    pinMode(p, OUTPUT);

    // Toggle rapidly for 3 seconds
    unsigned long start = millis();
    while (millis() - start < 3000) {
      pin_high(p);
      delay(50);
      pin_low(p);
      delay(50);
    }

    // Leave pin low
    pin_low(p);

    SERIAL_ECHOPGM("  Done: ");
    SERIAL_ECHOLNPGM(name);
    SERIAL_ECHOLNPGM("  --- pause 2s ---");
    delay(2000);
  }

  SERIAL_ECHOLNPGM("");
  SERIAL_ECHOLNPGM("========================================");
  SERIAL_ECHOLNPGM("=== PHASE 1 COMPLETE ===");
  SERIAL_ECHOLNPGM("Note which pins caused buzzer/flicker");
  SERIAL_ECHOLNPGM("========================================");
  delay(3000);

  // ============================================================
  // PHASE 2: Try OLED init with most likely pin combos
  // Based on row-reversal cable theory:
  //   CS=PD2, DC=PC3, SCK=PB5, MOSI=NRST? (can't use NRST)
  // Based on binary analysis:
  //   CS=PC3, DC=PC1, SCK=PB13, MOSI=PB15
  // Other combos mixing the two
  // ============================================================
  SERIAL_ECHOLNPGM("");
  SERIAL_ECHOLNPGM("========================================");
  SERIAL_ECHOLNPGM("=== PHASE 2: OLED INIT ATTEMPTS ===");
  SERIAL_ECHOLNPGM("========================================");
  delay(2000);

  // Attempt 1: Binary analysis original
  try_oled_init(PC3, PC1, PB13, PB15, PA11,
    "A1: CS=PC3 DC=PC1 SCK=PB13 MOSI=PB15 RST=PA11");

  // Attempt 2: Binary analysis, no reset
  try_oled_init(PC3, PC1, PB13, PB15, -1,
    "A2: CS=PC3 DC=PC1 SCK=PB13 MOSI=PB15 RST=none");

  // Attempt 3: Schematic-based
  try_oled_init(PC3, PC0, PB13, PB15, -1,
    "A3: CS=PC3 DC=PC0 SCK=PB13 MOSI=PB15 RST=none");

  // Attempt 4: Row-reversal theory (CS=PD2, DC=PC3)
  try_oled_init(PD2, PC3, PB5, PC1, -1,
    "A4: CS=PD2 DC=PC3 SCK=PB5 MOSI=PC1 RST=none");

  // Attempt 5: Row-reversal with PA11 as reset
  try_oled_init(PD2, PC3, PB5, PC1, PA11,
    "A5: CS=PD2 DC=PC3 SCK=PB5 MOSI=PC1 RST=PA11");

  // Attempt 6: Maybe CS and DC are swapped from binary
  try_oled_init(PC1, PC3, PB13, PB15, PA11,
    "A6: CS=PC1 DC=PC3 SCK=PB13 MOSI=PB15 RST=PA11");

  // Attempt 7: PC0 as CS, PC3 as DC
  try_oled_init(PC0, PC3, PB13, PB15, PA11,
    "A7: CS=PC0 DC=PC3 SCK=PB13 MOSI=PB15 RST=PA11");

  // Attempt 8: Full row-reversal with PB3/PB4 as SPI
  try_oled_init(PD2, PC3, PB3, PB4, -1,
    "A8: CS=PD2 DC=PC3 SCK=PB3 MOSI=PB4 RST=none");

  // Attempt 9: IO1 might be PC7
  try_oled_init(PC7, PC3, PB5, PC1, -1,
    "A9: CS=PC7 DC=PC3 SCK=PB5 MOSI=PC1 RST=none");

  // Attempt 10: Using PD2 for CS, PC0 for DC
  try_oled_init(PD2, PC0, PB13, PB15, PA11,
    "A10: CS=PD2 DC=PC0 SCK=PB13 MOSI=PB15 RST=PA11");

  // Attempt 11: PA8 might be IO1 for CS
  try_oled_init(PA8, PC3, PB5, PC1, -1,
    "A11: CS=PA8 DC=PC3 SCK=PB5 MOSI=PC1 RST=none");

  // Attempt 12: PC0/PC1 as SPI data, PC3 as DC, PD2 as CS
  try_oled_init(PD2, PC3, PC0, PC1, -1,
    "A12: CS=PD2 DC=PC3 SCK=PC0 MOSI=PC1 RST=none");

  SERIAL_ECHOLNPGM("");
  SERIAL_ECHOLNPGM("========================================");
  SERIAL_ECHOLNPGM("=== ALL TESTS COMPLETE ===");
  SERIAL_ECHOLNPGM("Report which attempt showed a pattern!");
  SERIAL_ECHOLNPGM("========================================");

  // Hang
  while(1) delay(1000);
}

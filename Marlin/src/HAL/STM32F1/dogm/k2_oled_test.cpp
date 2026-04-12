/**
 * Raw SPI2 OLED test for EasyThreed K2 Plus
 * Bypasses U8G entirely — directly initializes SPI2 and sends
 * SSD1306 commands to test the pin mapping.
 * 
 * Call k2_oled_test() early in setup() to test.
 * If the OLED lights up with a pattern, pins are correct.
 */

#ifdef __STM32F1__

#include "../../inc/MarlinConfig.h"

#if HAS_MARLINUI_U8GLIB

#include <libmaple/spi.h>
#include <libmaple/gpio.h>
#include <libmaple/rcc.h>

// Pin definitions — testing ALL possible CS pins
// DC = PC1 (confirmed from stock firmware)
// RESET = PA11 (confirmed from stock firmware)
// CS candidates: PC2 (original) or PC3 (from binary analysis)

#define OLED_DC_PORT   GPIOC
#define OLED_DC_BIT    1

#define OLED_RST_PORT  GPIOA
#define OLED_RST_BIT   11

// Try PC3 as CS (from stock firmware analysis)
#define OLED_CS_PORT   GPIOC
#define OLED_CS_BIT    3

static void oled_pin_high(gpio_dev *port, uint8_t bit) {
  port->regs->BSRR = (1U << bit);
}

static void oled_pin_low(gpio_dev *port, uint8_t bit) {
  port->regs->BRR = (1U << bit);
}

static void oled_delay_ms(uint32_t ms) {
  volatile uint32_t count;
  while (ms--) {
    count = 7200;  // ~1ms at 72MHz
    while (count--) __asm__ volatile ("nop");
  }
}

static void oled_spi2_init(void) {
  // Enable clocks
  rcc_clk_enable(RCC_SPI2);
  rcc_clk_enable(RCC_GPIOB);
  rcc_clk_enable(RCC_GPIOC);
  rcc_clk_enable(RCC_GPIOA);

  // Configure CS, DC, RESET as GPIO output push-pull 50MHz
  gpio_set_mode(OLED_CS_PORT, OLED_CS_BIT, GPIO_OUTPUT_PP);
  gpio_set_mode(OLED_DC_PORT, OLED_DC_BIT, GPIO_OUTPUT_PP);
  gpio_set_mode(OLED_RST_PORT, OLED_RST_BIT, GPIO_OUTPUT_PP);

  // CS HIGH (deselect)
  oled_pin_high(OLED_CS_PORT, OLED_CS_BIT);
  // DC LOW (command mode)
  oled_pin_low(OLED_DC_PORT, OLED_DC_BIT);

  // Configure SPI2 pins: PB13=SCK(AF-PP), PB14=MISO(input), PB15=MOSI(AF-PP)
  gpio_set_mode(GPIOB, 13, GPIO_AF_OUTPUT_PP);  // SCK
  gpio_set_mode(GPIOB, 15, GPIO_AF_OUTPUT_PP);  // MOSI

  // Configure SPI2: Master, 8-bit, MSB first, CPOL=0, CPHA=0, software NSS
  SPI2->regs->CR1 = 0;  // Clear
  SPI2->regs->CR1 = SPI_CR1_MSTR    // Master mode
                   | SPI_CR1_SSM     // Software slave management
                   | SPI_CR1_SSI     // Internal slave select HIGH
                   | SPI_CR1_BR_DIV_16  // 36MHz/16 = 2.25MHz (safe speed)
                   ;
  SPI2->regs->CR2 = 0;
  SPI2->regs->CR1 |= SPI_CR1_SPE;   // Enable SPI
}

static void oled_spi2_send(uint8_t byte) {
  SPI2->regs->DR = byte;
  while (!(SPI2->regs->SR & SPI_SR_TXE)) ;  // Wait for TX empty
  while (SPI2->regs->SR & SPI_SR_BSY) ;      // Wait for not busy
}

static void oled_cmd(uint8_t cmd) {
  oled_pin_low(OLED_DC_PORT, OLED_DC_BIT);    // Command mode
  oled_pin_low(OLED_CS_PORT, OLED_CS_BIT);    // Select
  oled_spi2_send(cmd);
  oled_pin_high(OLED_CS_PORT, OLED_CS_BIT);   // Deselect
}

static void oled_data(uint8_t data) {
  oled_pin_high(OLED_DC_PORT, OLED_DC_BIT);   // Data mode
  oled_pin_low(OLED_CS_PORT, OLED_CS_BIT);    // Select
  oled_spi2_send(data);
  oled_pin_high(OLED_CS_PORT, OLED_CS_BIT);   // Deselect
}

void k2_oled_test(void) {
  oled_spi2_init();

  // Hardware reset
  oled_pin_high(OLED_RST_PORT, OLED_RST_BIT);
  oled_delay_ms(10);
  oled_pin_low(OLED_RST_PORT, OLED_RST_BIT);
  oled_delay_ms(50);
  oled_pin_high(OLED_RST_PORT, OLED_RST_BIT);
  oled_delay_ms(50);

  // SSD1306 init sequence (exact match to stock firmware)
  oled_cmd(0xAE);        // Display OFF
  oled_cmd(0xD5); oled_cmd(0x80);  // Clock div
  oled_cmd(0xA8); oled_cmd(0x3F);  // MUX ratio = 63 (128x64)
  oled_cmd(0xD3); oled_cmd(0x00);  // Display offset = 0
  oled_cmd(0x40);        // Start line = 0
  oled_cmd(0x8D); oled_cmd(0x14);  // Charge pump ON
  oled_cmd(0x20); oled_cmd(0x00);  // Horizontal addressing mode
  oled_cmd(0xA1);        // Segment remap
  oled_cmd(0xC8);        // COM scan descending
  oled_cmd(0xDA); oled_cmd(0x12);  // COM pins config
  oled_cmd(0x81); oled_cmd(0xCF);  // Contrast
  oled_cmd(0xD9); oled_cmd(0xF1);  // Precharge
  oled_cmd(0xDB); oled_cmd(0x40);  // VCOMH deselect
  oled_cmd(0x2E);        // Deactivate scroll
  oled_cmd(0xA4);        // Display from RAM
  oled_cmd(0xA6);        // Normal (not inverted)
  oled_cmd(0xAF);        // Display ON

  oled_delay_ms(100);

  // Fill screen with a checkerboard pattern to prove it works
  for (uint8_t page = 0; page < 8; page++) {
    oled_cmd(0xB0 + page);  // Set page
    oled_cmd(0x00);          // Lower column = 0
    oled_cmd(0x10);          // Upper column = 0

    oled_pin_high(OLED_DC_PORT, OLED_DC_BIT);  // Data mode
    oled_pin_low(OLED_CS_PORT, OLED_CS_BIT);   // Select
    for (uint8_t col = 0; col < 128; col++) {
      oled_spi2_send((page & 1) ? 0x55 : 0xAA);  // Checkerboard
    }
    oled_pin_high(OLED_CS_PORT, OLED_CS_BIT);  // Deselect
  }

  // Leave display on for 3 seconds so you can see it
  oled_delay_ms(3000);
}

#endif // HAS_MARLINUI_U8GLIB
#endif // __STM32F1__

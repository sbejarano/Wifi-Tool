// ===== User_Setup.h for ESP32-3248S035C (ST7796 + GT911) =====
// From official schematic in https://github.com/ardnew/ESP32-3248S035

#define USER_SETUP_LOADED 1

// --- Display driver ---
#define ST7796_DRIVER
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// --- SPI pins (HSPI) ---
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1     // RESET tied to ESP32 EN pin
#define TFT_BL   27     // Backlight

#define USE_HSPI_PORT

// --- SPI Frequency ---
#define SPI_FREQUENCY       65000000
#define SPI_READ_FREQUENCY  20000000

// --- Touch GT911 ----
#define TOUCH_DRIVER 0x0911
#define TOUCH_SDA    33
#define TOUCH_SCL    32
#define TOUCH_IRQ    21
#define TOUCH_RST    25
#define I2C_TOUCH_FREQUENCY 400000
#define I2C_TOUCH_PORT      1
#define I2C_TOUCH_ADDRESS   0x5D

// --- Fonts ---
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT

// TFT_eSPI 自定义引脚配置 - ESP32 库仑计
// 屏幕: 1.8寸 TFT ST7735 (160x128) SPI接口
// 接口: CN8 (7针, 软件SPI)

#define USER_SETUP_LOADED 1

// 驱动芯片
#define ST7735_DRIVER

// 屏幕分辨率
#define TFT_WIDTH  128
#define TFT_HEIGHT 160

// 颜色顺序 (根据实际显示效果调整，如果颜色反了取消下面注释)
// #define TFT_RGB_ORDER TFT_BGR

// ESP32 引脚映射 (基于网络表)
#define TFT_MOSI 13   // CN8.4 → U2.16 → GPIO13 (SPI Data)
#define TFT_SCLK 15   // CN8.3 → U2.23 → GPIO15 (SPI Clock)
#define TFT_CS   -1   // 7针模块CS接地常开
#define TFT_DC   26   // CN8.6 → U2.11 → GPIO26 (Data/Command)
#define TFT_RST  25   // CN8.5 → U2.10 → GPIO25 (Reset)
#define TFT_BL   12   // CN8.7 → U2.14 → GPIO12 (Backlight)

// SPI 频率
#define SPI_FREQUENCY       20000000   // 20MHz
#define SPI_READ_FREQUENCY  10000000
#define SPI_TOUCH_FREQUENCY  2500000

// 禁用触摸 (本项目无触摸)
#define TOUCH_CS -1

// 加载字体
#define LOAD_GLCD   // 基础小字体 (Font 1)
#define LOAD_FONT2  // 小字体 (Font 2)
#define LOAD_FONT4  // 中等字体 (Font 4)
#define LOAD_GFXFF  // FreeFonts

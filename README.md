# ESP32 Coulomb Meter - Remote Switch - Power Meter
# ESP32 库仑计 - 远程开关 - 功率计

An ESP32-based high-precision coulomb meter with remote switch control and power monitoring capabilities.
基于 ESP32 的高精度库仑计，支持远程开关控制和功率监测。

## Specifications / 规格参数

| Parameter / 参数 | Value / 值 |
|-----------|-------|
| Input Voltage / 输入电压 | 12 ~ 96.0V |
| Output Voltage / 输出电压 | 12 ~ 96.0V |
| Output Current / 输出电流 | 0 ~ 80.0A (depends on shunt resistor / 取决于采样电阻) |
| Remote Switches / 远程开关 | 5 channels / 5路 |
| Serial Interface / 串口读取 | 1 channel / 1路 |
| Temperature Monitor / 温度监控 | 1 channel / 1路 (NTC) |
| Shunt Resistor Config / 采样电阻配置 | 1mΩ=80A, 2mΩ=40A, 4mΩ=20A |

## Hardware Design / 硬件设计

- **MCU / 主控**: ESP32
- **Display / 屏幕**: 1.8" TFT SPI (ST7789)
- **Current Sensor / 电流检测**: INA226 (I2C)
- **Shunt Resistor / 采样电阻**: Default 1mΩ (max 80A), configurable to 2mΩ (40A) or 4mΩ (20A) / 默认1mΩ (最大80A), 可改为2mΩ(40A)或4mΩ(20A)
- **Power Supply / 电源**: MP9486A buck converter + AMS1117-3.3 regulator / 降压 + 稳压
- **Temperature / 温度检测**: NTC thermistor / NTC热敏电阻
- **Fan Control / 风扇控制**: PWM (GPIO16)
- **Output / 输出**: 5-channel relay / 5路继电器

## Features / 功能特性

- ✅ Real-time voltage / current / power measurement (INA226) / 实时电压/电流/功率测量
- ✅ Battery percentage display / 电池电量百分比显示
- ✅ Cumulative AH / KWh metering / 累计AH/KWh计量
- ✅ 5-channel remote switch control (Blinker App) / 5路远程开关控制
- ✅ NTC temperature monitoring / NTC温度监控
- ✅ Fan PWM control / 风扇PWM控制
- ✅ OTA firmware update / OTA固件升级
- ✅ Data persistence (Preferences) / 数据掉电保存
- ✅ Local button operation / 本地按键操作
- ✅ 1.8" TFT color display / 1.8寸TFT彩屏显示
- ✅ Power curve chart / 功率曲线图表

## Pin Connections / 引脚连接

### I2C - INA226
| Signal / 信号 | ESP32 Pin / 引脚 |
|--------|-----------|
| SDA | GPIO21 |
| SCL | GPIO22 |

### TFT Display / 屏幕 (1.8" SPI)
| Signal / 信号 | ESP32 Pin / 引脚 |
|--------|-----------|
| CS | GPIO5 |
| DC | GPIO23 |
| RST | GPIO17 |
| SCLK | GPIO18 |
| MOSI | GPIO19 |
| BLK | GPIO4 |

### Buttons / 按键
| Button / 按键 | ESP32 Pin / 引脚 |
|--------|-----------|
| MENU | GPIO36 |
| LEFT | GPIO0 |
| RIGHT | GPIO35 |
| EN | GPIO34 |

### Relay Outputs / 继电器输出 (5 channels / 5路)
| Relay / 继电器 | ESP32 Pin / 引脚 |
|-------|-----------|
| Relay 1 | GPIO15 |
| Relay 2 | GPIO13 |
| Relay 3 | GPIO25 |
| Relay 4 | GPIO26 |
| Relay 5 | GPIO14 |

### Others / 其他
| Function / 功能 | ESP32 Pin / 引脚 |
|----------|-----------|
| Fan PWM / 风扇PWM | GPIO16 |
| NTC Temp / NTC温度 | GPIO39 |

## Shunt Resistor Configuration / 采样电阻配置

Modify the macro definition in the code according to the actual shunt resistor value:
根据实际焊接的采样电阻值，修改代码中的宏定义:

```cpp
// 1mΩ → max 80A / 最大80A (default / 默认)
#define SHUNT_RESISTOR  0.001
#define MAX_CURRENT     80.0

// 2mΩ → max 40A / 最大40A
// #define SHUNT_RESISTOR  0.002
// #define MAX_CURRENT     40.0

// 4mΩ → max 20A / 最大20A
// #define SHUNT_RESISTOR  0.004
// #define MAX_CURRENT     20.0
```

## Blinker App Configuration / 点灯科技配置

### One-click Network Configuration (Recommended) / 一键配网模式（推荐）

1. Install **Blinker** App on your phone / 手机安装 **点灯科技Blinker** App
2. Open App → Tap **+** → **Independent Device** → **Network Access** → Select **ESP32** / 打开App → 点击右上角 **+** → **独立设备** → **网络接入** → 选择 **ESP32**
3. Copy the generated **Auth Key** and replace it in the code / 复制生成的 **Auth Key**，替换代码中的 `auth[]`:
   ```cpp
   char auth[] = "Your Auth Key Here / 这里填你的密钥";
   ```
4. Compile and upload firmware to ESP32 / 编译上传固件到ESP32
5. **First time setup / 首次配网**: ESP32 will start a hotspot named `Blinker_xxx` / ESP32启动后会发出名为 `Blinker_xxx` 的热点
6. Connect your phone to this hotspot → Open Blinker App → Network config page will appear automatically / 手机连接该热点 → 打开Blinker App → 自动弹出配网页面
7. Enter your WiFi SSID and password → Configuration successful / 输入你家WiFi的名称和密码 → 配网成功
8. ESP32 will automatically connect to WiFi afterwards / 以后ESP32会自动连接WiFi，无需重复配网

> **Change WiFi / 更换WiFi**: Press and hold **EN button (GPIO34)** for 3 seconds, the screen will show "WiFi Config Mode". ESP32 will clear saved WiFi info and restart the hotspot. Connect your phone to the hotspot and reconfigure WiFi in Blinker App. / 长按 **EN按键（GPIO34）** 3秒，屏幕显示"WiFi配网模式"，ESP32会清除已保存的WiFi信息并重新发出热点，然后用手机连接该热点，在Blinker App中重新配置WiFi即可。

## Required Libraries / 依赖库

- Blinker (点灯科技)
- TFT_eSPI (screen driver / 屏幕驱动)
- INA226 (current sensor / 电流检测)
- Preferences (data storage / 数据存储)
- Wire (I2C communication / I2C通信)

## Project Structure / 项目结构

```
├── ESP32_库仑计_完整代码/
│   ├── ESP32_CoulombMeter.ino   # Main firmware / 主固件
│   └── README.md                # Chinese documentation / 中文文档
├── SCH_ESP32_库仑计_2026-05-30.pdf  # Schematic / 原理图
├── Netlist_ESP32_库仑计_2026-05-30.* # Netlist files / 网络表文件
├── 固件_KLJ_ST7789_F1.01.bin        # Precompiled firmware / 预编译固件
└── 点灯配置.txt                      # Blinker config guide / 点灯配置说明
```

## License / 许可证

Open source hardware project. / 开源硬件项目。

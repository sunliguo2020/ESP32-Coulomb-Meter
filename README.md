# ESP32 Coulomb Meter - Remote Switch - Power Meter
# ESP32 库仑计 - 远程开关 - 功率计

An ESP32-based high-precision coulomb meter with remote switch control, BMS communication, and power monitoring capabilities.
基于 ESP32 的高精度库仑计，支持远程开关控制、BMS通信和功率监测。

## Specifications / 规格参数

| Parameter / 参数 | Value / 值 |
|-----------|-------|
| Input Voltage / 输入电压 | 12 ~ 96.0V |
| Output Voltage / 输出电压 | 12 ~ 96.0V |
| Output Current / 输出电流 | 0 ~ 80.0A (depends on shunt resistor / 取决于采样电阻) |
| Remote Switches / 远程开关 | 5 channels / 5路 |
| BMS Serial / BMS串口 | 1 channel / 1路 (UART1: RX=GPIO16, TX=GPIO17) |
| Temperature Monitor / 温度监控 | 1 channel / 1路 (NTC, GPIO39) |
| Shunt Resistor Config / 采样电阻配置 | 1mΩ=80A, 2mΩ=40A, 4mΩ=20A |

## Hardware Design / 硬件设计

Based on netlist `Netlist_ESP32_库仑计_2026-06-08.enet`:
基于网络表 `Netlist_ESP32_库仑计_2026-06-08.enet`:

- **MCU / 主控**: ESP32 (U2)
- **Display / 屏幕**: 1.8" TFT SPI (ST7789) via CN8 connector / 通过CN8座子连接
- **Current Sensor / 电流检测**: INA226 (U70) MSOP-10, I2C interface / I2C接口
- **Shunt Resistor / 采样电阻**: 2×2512 in parallel, default ~2mΩ (max 77.5A) / 2个2512并联，默认约2mΩ
- **Power Supply / 电源**: MP9486A (U3) HV buck converter + AMS1117-3.3 (U5) regulator / 高压降压 + 3.3V稳压
- **Temperature / 温度检测**: NTC thermistor (GPIO39) with 10kΩ pull-up / NTC热敏电阻+10kΩ上拉
- **BMS Serial / BMS串口**: UART1 (GPIO16 RX, GPIO17 TX) via H1 connector, connect to BMS protection board / 通过H1座子接电池保护板
- **Output / 输出**: 5-channel relay / 5路继电器 (GPIO14/27/4/33/32)
- **Buttons / 按键**: MENU(GPIO0), LEFT(GPIO18), RIGHT(GPIO19)

## Features / 功能特性

- ✅ Real-time voltage / current / power measurement (INA226, 0.1% accuracy) / 实时电压/电流/功率测量
- ✅ Battery percentage display (LFP/NCM/Lead-acid) / 电池电量百分比显示 (铁锂/三元/铅酸)
- ✅ Cumulative AH / KWh metering / 累计AH/KWh计量
- ✅ 5-channel remote switch control (Blinker App) / 5路远程开关控制
- ✅ NTC temperature monitoring / NTC温度监控
- ✅ BMS serial communication (UART1, connect to battery protection board) / BMS串口通信
- ✅ OTA firmware update / OTA固件升级
- ✅ Data persistence (Preferences) / 数据掉电保存
- ✅ Local button operation / 本地按键操作
- ✅ 1.8" TFT color display (160x128) / 1.8寸TFT彩屏显示
- ✅ Power curve chart / 功率曲线图表
- ✅ Voltage/current calibration via serial AT commands / 电压/电流校准

## Pin Connections / 引脚连接 (Netlist_ESP32_库仑计_2026-06-08.enet)

### I2C - INA226
| Signal / 信号 | ESP32 Pin / 引脚 | Netlist / 网络表 |
|--------|-----------|-----------|
| SDA | GPIO21 | U2.33 → SDA → U70.4 |
| SCL | GPIO22 | U2.36 → SCL → U70.5 |

### TFT Display / 屏幕 (1.8" SPI) - CN8
| Signal / 信号 | CN8 Pin | ESP32 Pin / 引脚 |
|--------|---------|-----------|
| GND | CN8.1 | - |
| VCC | CN8.2 | 3.3V |
| SCK | CN8.3 | GPIO15 |
| SDA(MOSI) | CN8.4 | GPIO13 |
| RES | CN8.5 | GPIO25 |
| DC | CN8.6 | GPIO26 |
| BLK | CN8.7 | GPIO12 |

> Note: CS pin is tied to GND (compatible with 7-pin SPI modules / 7针模块CS接地常开)

### Buttons / 按键
| Button / 按键 | ESP32 Pin / 引脚 |
|--------|-----------|
| MENU | GPIO0 (with 4.7kΩ pull-up to 3.3V) |
| LEFT | GPIO18 |
| RIGHT | GPIO19 |

### Relay Outputs / 继电器输出 (5 channels / 5路)
| Relay / 继电器 | PCB Mark / 丝印 | ESP32 Pin / 引脚 |
|-------|---------|-----------|
| Relay 1 | RELAY1 | GPIO14 |
| Relay 2 | RELAY2 | GPIO27 |
| Relay 3 | K1 | GPIO4 |
| Relay 4 | K2 | GPIO33 |
| Relay 5 | K3 | GPIO32 |

### BMS Serial / BMS串口 - H1 (connect to battery protection board / 接电池保护板)
| Signal / 信号 | H1 Pin | ESP32 Pin / 引脚 |
|----------|--------|-----------|
| 3.3V | H1.1 | - (output / 输出) |
| RX (receive / 接收) | H1.2 | GPIO16 (U2.27 → RXD1) |
| TX (transmit / 发送) | H1.3 | GPIO17 (U2.28 → TXD1) |
| GND | H1.4 | - |

> Note: Direct TTL connection to BMS protection board serial port. If the protection board uses RS485, an external converter is required. / 直接TTL连接电池保护板串口，若保护板为RS485需外加转接板。

### Others / 其他
| Function / 功能 | ESP32 Pin / 引脚 | Notes / 备注 |
|----------|-----------|-------|
| NTC Temp / NTC温度 | GPIO39 | U2.5, 10kΩ pull-up to 3.3V |
| ADC Battery Voltage / 电池电压 | GPIO34 | U2.6, through voltage divider / 通过分压电阻 |
| Unused / 未使用 | GPIO5 | U2.29 |
| Unused / 未使用 | GPIO23 | U2.37 |
| Unused / 未使用 | GPIO2 | U2.24 |
| Unused / 未使用 | GPIO35 | U2.7 |
| Unused / 未使用 | GPIO36 | U2.4 |

## Serial AT Commands / 串口AT指令

Send via USB serial port (115200 baud):
通过USB串口(115200波特率)发送：

| Command / 命令 | Function / 功能 |
|------|------|
| `AT+HELP` | Show help / 显示帮助 |
| `AT+RST` | Restart device / 重启设备 |
| `AT+REG <activation code>` | Product activation / 产品激活 |
| `AT+WIFI.connect <SSID> <password>` | Connect WiFi / 连接WiFi |
| `AT+INAV <actual voltage>` | Set voltage calibration / 设置电压校准 |
| `AT+INAC <actual current>` | Set current calibration / 设置电流校准 |
| `AT+RESTORE` | Factory reset (preserves activation) / 恢复出厂设置 |

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

> **Change WiFi / 更换WiFi**: Use serial command `AT+WIFI.connect <SSID> <password>` / 使用串口命令 `AT+WIFI.connect WiFi名称 WiFi密码`

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
├── pcb/
│   ├── SCH_ESP32_库仑计_2026-05-18.pdf  # Schematic v1 / 原理图v1
│   ├── SCH_ESP32_库仑计_2026-05-30.pdf  # Schematic v2 / 原理图v2
│   ├── Netlist_ESP32_库仑计_2026-06-08.* # Netlist files / 网络表文件
│   └── BOM_ESP32_库仑计_2026-05-18.xlsx  # BOM / 物料清单
├── ST7789小屏幕/                      # Display module info / 屏幕模块资料
├── images/                           # Product images / 产品图片
├── 固件_KLJ_ST7789_F1.01.bin        # Precompiled firmware / 预编译固件
├── 点灯配置.txt                      # Blinker config guide / 点灯配置说明
└── ESP32库仑计.3mf                   # 3D model / 3D模型
```

## License / 许可证

Open source hardware project. / 开源硬件项目。
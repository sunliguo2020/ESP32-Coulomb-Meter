# ESP32 Coulomb Meter - Remote Switch - Power Meter

An ESP32-based high-precision coulomb meter with remote switch control and power monitoring capabilities.

## Specifications

| Parameter | Value |
|-----------|-------|
| Input Voltage | 12 ~ 96.0V |
| Output Voltage | 12 ~ 96.0V |
| Output Current | 0 ~ 80.0A (depends on shunt resistor) |
| Remote Switches | 5 channels |
| Serial Interface | 1 channel |
| Temperature Monitor | 1 channel (NTC) |
| Shunt Resistor Config | 1mΩ=80A, 2mΩ=40A, 4mΩ=20A |

## Hardware Design

- **MCU**: ESP32
- **Display**: 1.8" TFT SPI (ST7789)
- **Current Sensor**: INA226 (I2C)
- **Shunt Resistor**: Default 1mΩ (max 80A), configurable to 2mΩ (40A) or 4mΩ (20A)
- **Power Supply**: MP9486A buck converter + AMS1117-3.3 regulator
- **Temperature**: NTC thermistor
- **Fan Control**: PWM (GPIO16)
- **Output**: 5-channel relay

## Features

- ✅ Real-time voltage / current / power measurement (INA226)
- ✅ Battery percentage display
- ✅ Cumulative AH / KWh metering
- ✅ 5-channel remote switch control (Blinker App)
- ✅ NTC temperature monitoring
- ✅ Fan PWM control
- ✅ OTA firmware update
- ✅ Data persistence (Preferences)
- ✅ Local button operation
- ✅ 1.8" TFT color display
- ✅ Power curve chart

## Pin Connections

### I2C - INA226
| Signal | ESP32 Pin |
|--------|-----------|
| SDA | GPIO21 |
| SCL | GPIO22 |

### TFT Display (1.8" SPI)
| Signal | ESP32 Pin |
|--------|-----------|
| CS | GPIO5 |
| DC | GPIO23 |
| RST | GPIO17 |
| SCLK | GPIO18 |
| MOSI | GPIO19 |
| BLK | GPIO4 |

### Buttons
| Button | ESP32 Pin |
|--------|-----------|
| MENU | GPIO36 |
| LEFT | GPIO0 |
| RIGHT | GPIO35 |
| EN | GPIO34 |

### Relay Outputs (5 channels)
| Relay | ESP32 Pin |
|-------|-----------|
| Relay 1 | GPIO15 |
| Relay 2 | GPIO13 |
| Relay 3 | GPIO25 |
| Relay 4 | GPIO26 |
| Relay 5 | GPIO14 |

### Others
| Function | ESP32 Pin |
|----------|-----------|
| Fan PWM | GPIO16 |
| NTC Temp | GPIO39 |

## Shunt Resistor Configuration

Modify the macro definition in the code according to the actual shunt resistor value:

```cpp
// 1mΩ → max 80A (default)
#define SHUNT_RESISTOR  0.001
#define MAX_CURRENT     80.0

// 2mΩ → max 40A
// #define SHUNT_RESISTOR  0.002
// #define MAX_CURRENT     40.0

// 4mΩ → max 20A
// #define SHUNT_RESISTOR  0.004
// #define MAX_CURRENT     20.0
```

## Blinker App Configuration

### One-click Network Configuration (Recommended)

1. Install **Blinker** App on your phone
2. Open App → Tap **+** → **Independent Device** → **Network Access** → Select **ESP32**
3. Copy the generated **Auth Key** and replace it in the code:
   ```cpp
   char auth[] = "Your Auth Key Here";
   ```
4. Compile and upload firmware to ESP32
5. **First time setup**: ESP32 will start a hotspot named `Blinker_xxx`
6. Connect your phone to this hotspot → Open Blinker App → Network config page will appear automatically
7. Enter your WiFi SSID and password → Configuration successful
8. ESP32 will automatically connect to WiFi afterwards

> **Change WiFi**: Press and hold **EN button (GPIO34)** for 3 seconds, the screen will show "WiFi Config Mode". ESP32 will clear saved WiFi info and restart the hotspot. Connect your phone to the hotspot and reconfigure WiFi in Blinker App.

## Required Libraries

- Blinker
- TFT_eSPI
- INA226
- Preferences
- Wire

## Project Structure

```
├── ESP32_库仑计_完整代码/
│   ├── ESP32_CoulombMeter.ino   # Main firmware
│   └── README.md                # Chinese documentation
├── SCH_ESP32_库仑计_2026-05-30.pdf  # Schematic
├── Netlist_ESP32_库仑计_2026-05-30.* # Netlist files
└── 固件_KLJ_ST7789_F1.01.bin        # Precompiled firmware
```

## License

Open source hardware project.

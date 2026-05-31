/**
 * ESP32 库仑计 - 远程开关 - 功率计
 * 基于点灯科技(Blinker)平台
 * 
 * 项目规格参数:
 * - 输入电压: 12 ~ 96.0V
 * - 输出电压: 12 ~ 96.0V
 * - 输出电流: 0 ~ 80.0A (取决于采样电阻)
 * - 远程开关: 5路
 * - 串口读取: 1路
 * - 温度监控: 1路
 * - 采样电阻配置: 1mΩ=80A, 2mΩ=40A, 4mΩ=20A
 * 
 * 硬件设计 (基于网络表 Netlist_ESP32_库仑计_2026-05-30.tel):
 * - 主控: ESP32 (U2)
 * - 屏幕: TFT 1.8寸 (CN4) SPI接口
 * - 电流检测: INA226 (U70) MSOP-10封装, I2C接口
 * - 采样电阻: 默认1mΩ (最大80A), 可改为2mΩ(40A)或4mΩ(20A)
 * - 运放: U71-U74 电流信号调理
 * - 电源: MP9486A (U3) 降压 + AMS1117-3.3 (U5) 稳压
 * - NTC温度检测 (H16)
 * - 风扇PWM控制 (GPIO16)
 * - 5路继电器输出
 */

#include <Blinker.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <INA226.h>
#include <Preferences.h>

// ==================== 引脚定义 (基于网络表) ====================
// I2C - INA226
#define I2C_SDA 21  // U2.33 → SDA → U70.4
#define I2C_SCL 22  // U2.36 → SCL → U70.5

// TFT 屏幕 (1.8寸 SPI) - CN4
// CN4引脚: 1=VDD, 2=SCL, 3=SDA, 4=RST, 5=DC, 6=CS, 7=BLK
#define TFT_CS   5   // U2.29 → GPIO5
#define TFT_DC   23  // U2.37 → GPIO23
#define TFT_RST  17  // 默认
#define TFT_SCLK 18  // U2.30 → GPIO18
#define TFT_MOSI 19  // U2.31 → GPIO19
#define TFT_BLK  4   // U2.26 → GPIO4 (WK_PWM)

// 按键 (网络表: EN, LEFT, MENU, RIGHT)
#define KEY_MENU   36  // U2.4  → GPIO36 → MENU
#define KEY_LEFT   0   // U2.25 → GPIO0  → LEFT (也连MENU按键)
#define KEY_RIGHT  35  // U2.7  → GPIO35 → RIGHT
#define KEY_EN     34  // U2.6  → GPIO34 → EN

// 继电器输出 (网络表: CN8) - 项目说明为5路
// CN8引脚: 1=GND, 2=VCC, 3=SCK, 4=SDA, 5=RES, 6=DC, 7=BLK
#define RELAY1_PIN  15  // U2.23 → GPIO15 → CN8.3
#define RELAY2_PIN  13  // U2.16 → GPIO13 → CN8.4
#define RELAY3_PIN  25  // U2.10 → GPIO25 → CN8.5
#define RELAY4_PIN  26  // U2.11 → GPIO26 → CN8.6
#define RELAY5_PIN  14  // U2.13 → GPIO14
// RELAY6_PIN 12  // 保留未启用 (项目说明为5路)

// 风扇控制
#define FAN_PWM     16  // U2.27 → GPIO16

// NTC温度检测
#define NTC_PIN     39  // U2.5  → GPIO39 → C4.2/R6.2

// 其他GPIO
#define GPIO_27     27  // U2.12 → GPIO27
#define GPIO_32     32  // U2.8  → GPIO32 (K3)
#define GPIO_33     33  // U2.9  → GPIO33 (K2)

// ==================== 采样电阻配置 ====================
// 项目说明:
//   1mΩ → 最大80A (默认)
//   2mΩ → 最大40A
//   4mΩ → 最大20A
// 根据实际焊接的采样电阻修改此值
#define SHUNT_RESISTOR  0.001  // 采样电阻值 (Ω): 0.001=1mΩ, 0.002=2mΩ, 0.004=4mΩ
#define MAX_CURRENT     80.0   // 最大电流 (A): 80A@1mΩ, 40A@2mΩ, 20A@4mΩ

// ==================== INA226 引脚定义 (MSOP-10) ====================
// 基于网络表 U70:
// U70.1  → GND
// U70.2  → GND
// U70.3  → NC (未连接)
// U70.4  → SDA
// U70.5  → SCL
// U70.6  → 3.3V
// U70.7  → GND
// U70.8  → $2N62237 (连接 C63.2, R52.1, R54.2) - VBUS电压测量
// U70.9  → GND
// U70.10 → IN- (连接 C115.2, R57.2, R58.2, U72.1-4) - 采样电阻负端

// ==================== 全局对象 ====================
TFT_eSPI tft = TFT_eSPI();
INA226 ina226(0x40);  // INA226 I2C地址 (A0=GND时为0x40)
Preferences preferences;

// ==================== 点灯科技密钥 ====================
// 一键配网模式：只需填写auth密钥，WiFi信息通过App下发
// 首次使用时，ESP32会发出热点，在App中连接并配置WiFi
char auth[] = "Your_Blinker_Auth_Key";
// char ssid[] = "Your_WiFi_SSID";      // 硬编码模式（已注释）
// char pswd[] = "Your_WiFi_Password";  // 硬编码模式（已注释）

// ==================== 全局变量 ====================
// 测量数据
float busVoltage = 0.0;     // 总线电压 (V)
float shuntVoltage = 0.0;   // 采样电阻电压 (mV)
float current = 0.0;        // 电流 (A)
float power = 0.0;          // 功率 (W)
float batteryVoltage = 0.0; // 电池电压 (V)
float batteryPercent = 0;   // 电池电量 (%)
float temperature = 25.0;   // 温度 (℃)

// 累计数据
float accumulatedAH = 0.0;
float accumulatedKWh = 0.0;
float dailyCost = 0.0;

// 电池配置
int batteryCells = 4;
int batteryType = 0;        // 0=铁锂, 1=三元, 2=铅酸
float fullVoltage = 14.6;
float batteryCapacity = 100;

// 开关状态 (5路)
bool relayState[5] = {false, false, false, false, false};
bool fanState = false;
int fanSpeed = 0;

// 系统状态
bool wifiConnected = false;
unsigned long lastMeasureTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastSaveTime = 0;
unsigned long lastBlinkerTime = 0;

// ==================== 点灯科技组件 ====================
BlinkerNumber NUM_POWER("num-9y2");
BlinkerNumber NUM_KWH("num-rgu");
BlinkerNumber NUM_VOLT("num-m2k");
BlinkerNumber NUM_BATTERY("num-ahb");
BlinkerNumber NUM_DAH("num-dah");
BlinkerNumber NUM_DWH("num-dwh");
BlinkerNumber NUM_CURRENT("num-5up");

BlinkerChart CHART_POWER("cha-9ay");

BlinkerButton BTN_UPDATE("btn-update");
BlinkerButton BTN_OUT1("btn-out1");
BlinkerButton BTN_OUT2("btn-out2");
BlinkerButton BTN_OUT3("btn-out3");
BlinkerButton BTN_OUT4("btn-out4");
BlinkerButton BTN_OUT5("btn-out5");

BlinkerSlider SLIDER_CELLS("ran-bn");

BlinkerButton BTN_BT0("btn-bt0");
BlinkerButton BTN_BT1("btn-bt1");
BlinkerButton BTN_BT2("btn-bt2");

BlinkerNumber NUM_BVM("num-bvm");
BlinkerNumber NUM_BAH("num-bah");

// ==================== 函数声明 ====================
void setupHardware();
void setupDisplay();
void setupBlinker();
void readSensors();
void readINA226();
void readBatteryVoltage();
void readTemperature();
void calculateBatteryPercent();
void updateDisplay();
void drawMainPage();
void drawBatteryIcon(int x, int y, int w, int h, int percent);
void drawRelayIcon(int x, int y, bool state);
void drawWifiIcon(int x, int y, bool connected);
void updateBlinkerData();
void saveData();
void loadData();
void handleButtons();
void setRelay(int index, bool state);

void btnUpdateCallback(const String &state);
void btnOut1Callback(const String &state);
void btnOut2Callback(const String &state);
void btnOut3Callback(const String &state);
void btnOut4Callback(const String &state);
void btnOut5Callback(const String &state);
void sliderCellsCallback(int32_t value);
void btnBt0Callback(const String &state);
void btnBt1Callback(const String &state);
void btnBt2Callback(const String &state);

// ==================== 设置函数 ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n==================================");
  Serial.println("  ESP32 库仑计 v1.0");
  Serial.println("  项目规格: 12~96V / 0~80A / 5路开关");
  Serial.println("==================================\n");
  
  setupHardware();
  setupDisplay();
  setupBlinker();
  loadData();
  
  Serial.println("\n✅ 系统启动完成！");
}

void setupHardware() {
  // 初始化I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // 初始化INA226
  if (!ina226.begin()) {
    Serial.println("❌ INA226 初始化失败！请检查I2C连接");
    Serial.println("   I2C引脚: SDA=GPIO21, SCL=GPIO22");
    Serial.println("   INA226地址: 0x40 (A0=GND)");
  } else {
    Serial.println("✅ INA226 初始化成功");
    // 采样电阻配置 (根据项目说明)
    // 1mΩ → 最大80A, 2mΩ → 最大40A, 4mΩ → 最大20A
    Serial.printf("   采样电阻: %.0fmΩ, 最大电流: %.0fA\n", 
                  SHUNT_RESISTOR * 1000, MAX_CURRENT);
    ina226.setMaxCurrentShunt(MAX_CURRENT, SHUNT_RESISTOR);
    ina226.setAveraging(INA226_AVG_128);
    ina226.setConversionTime(INA226_CONV_TIME_1100US);
    ina226.setMode(INA226_MODE_CONTINUOUS);
  }
  
  // 初始化继电器 (5路)
  int relayPins[] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, 
                     RELAY4_PIN, RELAY5_PIN};
  for (int i = 0; i < 5; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }
  
  // 风扇PWM
  pinMode(FAN_PWM, OUTPUT);
  ledcSetup(0, 25000, 8);
  ledcAttachPin(FAN_PWM, 0);
  ledcWrite(0, 0);
  
  // 按键输入
  pinMode(KEY_MENU, INPUT_PULLUP);
  pinMode(KEY_LEFT, INPUT_PULLUP);
  pinMode(KEY_RIGHT, INPUT_PULLUP);
  pinMode(KEY_EN, INPUT_PULLUP);
  
  // ADC配置
  analogReadResolution(12);
  
  Serial.println("✅ 硬件初始化完成");
}

void setupDisplay() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  // 启动画面 - 图形化
  tft.fillScreen(TFT_BLACK);
  
  // 绘制边框
  tft.drawRect(0, 0, 160, 128, TFT_NAVY);
  
  // 标题
  tft.fillRect(1, 1, 158, 14, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(1);
  tft.drawString("ESP32 库仑计", 5, 3);
  
  // 版本号
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("v1.0", 130, 3);
  
  // 中间图标 - 电池符号
  drawBatteryIcon(55, 35, 50, 25, 75);
  
  // 文字
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("初始化中...", 50, 70);
  
  // 进度条动画
  for (int i = 0; i <= 100; i += 5) {
    tft.fillRect(20, 90, 120, 8, TFT_BLACK);
    tft.drawRect(20, 90, 120, 8, TFT_WHITE);
    tft.fillRect(21, 91, map(i, 0, 100, 0, 118), 6, TFT_GREEN);
    delay(30);
  }
  
  // 开启背光
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);
}

void setupBlinker() {
  // 一键配网模式：ESP32启动后自动进入配网状态
  // 手机连接ESP32发出的热点 → 打开Blinker App → 自动弹出配网页面
  Blinker.begin(auth);
  
  BTN_UPDATE.attach(btnUpdateCallback);
  BTN_OUT1.attach(btnOut1Callback);
  BTN_OUT2.attach(btnOut2Callback);
  BTN_OUT3.attach(btnOut3Callback);
  BTN_OUT4.attach(btnOut4Callback);
  BTN_OUT5.attach(btnOut5Callback);
  
  SLIDER_CELLS.attach(sliderCellsCallback);
  
  BTN_BT0.attach(btnBt0Callback);
  BTN_BT1.attach(btnBt1Callback);
  BTN_BT2.attach(btnBt2Callback);
  
  Blinker.attachData([](const String &data) {
    Blinker.vibrate();
  });
  
  Serial.println("✅ 点灯科技初始化完成");
}

// ==================== 主循环 ====================
void loop() {
  Blinker.run();
  
  unsigned long now = millis();
  
  if (now - lastMeasureTime >= 100) {
    readSensors();
    lastMeasureTime = now;
  }
  
  if (now - lastDisplayTime >= 200) {
    updateDisplay();
    lastDisplayTime = now;
  }
  
  if (now - lastBlinkerTime >= 1000) {
    updateBlinkerData();
    lastBlinkerTime = now;
  }
  
  if (now - lastSaveTime >= 60000) {
    saveData();
    lastSaveTime = now;
  }
  
  handleButtons();
}

// ==================== 传感器读取 ====================
void readSensors() {
  static unsigned long lastIntegrate = 0;
  unsigned long now = millis();
  
  readINA226();
  readBatteryVoltage();
  readTemperature();
  calculateBatteryPercent();
  
  // 每秒积分一次
  if (now - lastIntegrate >= 1000) {
    float deltaTime = 1.0 / 3600.0;
    accumulatedAH += current * deltaTime;
    accumulatedKWh += (busVoltage * current) * deltaTime / 1000.0;
    lastIntegrate = now;
    dailyCost = accumulatedKWh * 0.5;
  }
}

void readINA226() {
  busVoltage = ina226.getBusVoltage();
  shuntVoltage = ina226.getShuntVoltage_mV();
  current = ina226.getCurrent();
  power = busVoltage * current;
  
  if (isnan(busVoltage) || busVoltage < 0) busVoltage = 0;
  if (isnan(current) || current < 0) current = 0;
  if (isnan(power) || power < 0) power = 0;
}

void readBatteryVoltage() {
  // GPIO34 (EN按键) 也用作ADC读取电池电压
  int adcValue = analogRead(KEY_EN);
  // 分压比需根据实际电路调整
  batteryVoltage = (adcValue / 4095.0) * 3.3 * 5.0;
}

void readTemperature() {
  // NTC温度检测 GPIO39
  int ntcValue = analogRead(NTC_PIN);
  float resistance = (4095.0 / max(ntcValue, 1) - 1.0) * 10000;
  float steinhart = log(resistance / 10000.0) / 3988.0 + 1.0 / 298.15;
  steinhart = 1.0 / steinhart - 273.15;
  temperature = steinhart;
}

void calculateBatteryPercent() {
  float minVoltage, maxVoltage;
  
  switch (batteryType) {
    case 0: // 铁锂
      minVoltage = 2.5 * batteryCells;
      maxVoltage = 3.65 * batteryCells;
      break;
    case 1: // 三元
      minVoltage = 3.0 * batteryCells;
      maxVoltage = 4.2 * batteryCells;
      break;
    case 2: // 铅酸
      minVoltage = 1.75 * batteryCells;
      maxVoltage = 2.45 * batteryCells;
      break;
    default:
      minVoltage = 3.0 * batteryCells;
      maxVoltage = 4.2 * batteryCells;
  }
  
  if (fullVoltage > 0) maxVoltage = fullVoltage;
  
  if (batteryVoltage >= maxVoltage) batteryPercent = 100;
  else if (batteryVoltage <= minVoltage) batteryPercent = 0;
  else batteryPercent = (batteryVoltage - minVoltage) / (maxVoltage - minVoltage) * 100;
  
  batteryPercent = constrain(batteryPercent, 0, 100);
}

// ==================== 图形化屏幕显示 ====================
void updateDisplay() {
  tft.fillScreen(TFT_BLACK);
  
  // 顶部状态栏
  tft.fillRect(0, 0, 160, 14, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(1);
  tft.drawString("ESP32 库仑计", 3, 2);
  
  // WiFi图标
  drawWifiIcon(130, 1, wifiConnected);
  
  // 分隔线
  tft.drawFastHLine(0, 14, 160, TFT_DARKGREY);
  
  // ===== 第一行: 电压 + 电流 (大字体) =====
  // 电压
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("V", 3, 18);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(String(busVoltage, 2), 15, 16);
  
  // 电流
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("A", 85, 18);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(String(current, 2), 97, 16);
  
  // ===== 第二行: 功率 (大号突出显示) =====
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("W", 3, 38);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawString(String(power, 1), 15, 34);
  
  // ===== 第三行: 电池电量进度条 =====
  int barY = 60;
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("BAT", 3, barY);
  
  // 电池图标
  drawBatteryIcon(35, barY - 2, 85, 14, batteryPercent);
  
  // 电量百分比
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(String(batteryPercent, 0) + "%", 125, barY);
  
  // ===== 第四行: 累计数据 =====
  int row4Y = 78;
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("AH", 3, row4Y);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(String(accumulatedAH, 2), 25, row4Y);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("KWh", 75, row4Y);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(String(accumulatedKWh, 3), 105, row4Y);
  
  // ===== 第五行: 温度 + 继电器状态 =====
  int row5Y = 94;
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("TEMP", 3, row5Y);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(temperature, 1) + "C", 40, row5Y);
  
  // 继电器状态图标 (5路)
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("OUT", 90, row5Y);
  for (int i = 0; i < 5; i++) {
    drawRelayIcon(118 + i * 8, row5Y, relayState[i]);
  }
  
  // ===== 第六行: 电池类型 + 串数 =====
  int row6Y = 110;
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("CELL", 3, row6Y);
  String typeStr[] = {"LFP", "NCM", "Pb"};  // 铁锂/三元/铅酸
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(typeStr[batteryType] + " " + String(batteryCells) + "S", 40, row6Y);
  
  // 满电电压
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("FV", 90, row6Y);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(fullVoltage, 1) + "V", 110, row6Y);
}

// ==================== 图形化辅助函数 ====================

// 绘制电池图标
void drawBatteryIcon(int x, int y, int w, int h, int percent) {
  // 电池外壳
  tft.drawRect(x, y, w, h, TFT_WHITE);
  // 电池正极凸起
  tft.fillRect(x + w, y + h / 4, 3, h / 2, TFT_WHITE);
  
  // 电量填充
  int fillW = map(percent, 0, 100, 0, w - 2);
  uint16_t color;
  if (percent > 60) color = TFT_GREEN;
  else if (percent > 20) color = TFT_YELLOW;
  else color = TFT_RED;
  
  if (fillW > 0) {
    tft.fillRect(x + 1, y + 1, fillW, h - 2, color);
  }
}

// 绘制WiFi图标
void drawWifiIcon(int x, int y, bool connected) {
  uint16_t color = connected ? TFT_GREEN : TFT_RED;
  // 简单WiFi符号: 三个弧线
  if (connected) {
    tft.drawCircle(x + 6, y + 8, 2, color);  // 中心点
    tft.drawArc(x + 6, y + 8, 5, 3, 180, 360, color, TFT_BLACK);  // 内弧
    tft.drawArc(x + 6, y + 8, 8, 6, 180, 360, color, TFT_BLACK);  // 外弧
  } else {
    tft.drawString("X", x + 2, y + 2);
  }
}

// 绘制继电器状态指示灯
void drawRelayIcon(int x, int y, bool state) {
  if (state) {
    tft.fillCircle(x + 3, y + 4, 3, TFT_GREEN);
  } else {
    tft.drawCircle(x + 3, y + 4, 3, TFT_DARKGREY);
  }
}

// ==================== 点灯数据更新 ====================
void updateBlinkerData() {
  NUM_POWER.print(power, 1);
  NUM_KWH.print(accumulatedKWh, 3);
  NUM_VOLT.print(busVoltage, 2);
  NUM_BATTERY.print(batteryPercent, 0);
  NUM_DAH.print(accumulatedAH, 2);
  NUM_DWH.print(dailyCost, 2);
  NUM_CURRENT.print(current, 3);
  
  NUM_BVM.print(fullVoltage, 1);
  NUM_BAH.print(batteryCapacity, 0);
  
  static int chartCounter = 0;
  chartCounter++;
  if (chartCounter >= 5) {
    CHART_POWER.point(power, busVoltage, current);
    chartCounter = 0;
  }
}

// ==================== 按钮回调 ====================
void btnUpdateCallback(const String &state) {
  if (state == BLINKER_CMD_BUTTON_PRESSED) {
    Serial.println("进入OTA固件升级模式");
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("OTA升级中...", 20, 60);
    Blinker.print("即将进入固件升级模式...");
    delay(1000);
    Blinker.ota();
  }
}

void btnOut1Callback(const String &state) { setRelay(0, !relayState[0]); }
void btnOut2Callback(const String &state) { setRelay(1, !relayState[1]); }
void btnOut3Callback(const String &state) { setRelay(2, !relayState[2]); }
void btnOut4Callback(const String &state) { setRelay(3, !relayState[3]); }
void btnOut5Callback(const String &state) { setRelay(4, !relayState[4]); }

void setRelay(int index, bool state) {
  if (index < 0 || index > 4) return;
  
  relayState[index] = state;
  int pins[] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, 
                RELAY4_PIN, RELAY5_PIN};
  digitalWrite(pins[index], state ? HIGH : LOW);
  
  BlinkerButton *btns[] = {&BTN_OUT1, &BTN_OUT2, &BTN_OUT3, 
                           &BTN_OUT4, &BTN_OUT5};
  btns[index]->print(state ? "on" : "off");
  
  Serial.printf("继电器%d: %s\n", index + 1, state ? "ON" : "OFF");
}

void sliderCellsCallback(int32_t value) {
  batteryCells = value;
  calculateBatteryPercent();
  Serial.printf("电池串数: %d\n", batteryCells);
}

void btnBt0Callback(const String &state) {
  batteryType = 0;
  BTN_BT0.print("on"); BTN_BT1.print("off"); BTN_BT2.print("off");
  calculateBatteryPercent();
  Serial.println("电池类型: 铁锂");
}

void btnBt1Callback(const String &state) {
  batteryType = 1;
  BTN_BT0.print("off"); BTN_BT1.print("on"); BTN_BT2.print("off");
  calculateBatteryPercent();
  Serial.println("电池类型: 三元");
}

void btnBt2Callback(const String &state) {
  batteryType = 2;
  BTN_BT0.print("off"); BTN_BT1.print("off"); BTN_BT2.print("on");
  calculateBatteryPercent();
  Serial.println("电池类型: 铅酸");
}

// ==================== 本地按键处理 ====================
void handleButtons() {
  static unsigned long lastBtnCheck = 0;
  static bool lastMenu = HIGH, lastLeft = HIGH, lastRight = HIGH, lastEn = HIGH;
  static unsigned long enPressStart = 0;
  static bool enLongPressHandled = false;
  
  if (millis() - lastBtnCheck < 50) return;
  lastBtnCheck = millis();
  
  bool menu = digitalRead(KEY_MENU);
  bool left = digitalRead(KEY_LEFT);
  bool right = digitalRead(KEY_RIGHT);
  bool en = digitalRead(KEY_EN);
  
  if (lastMenu == HIGH && menu == LOW) Serial.println("MENU按键");
  if (lastLeft == HIGH && left == LOW) Serial.println("LEFT按键");
  if (lastRight == HIGH && right == LOW) Serial.println("RIGHT按键");
  
  // EN按键：短按打印信息，长按3秒进入WiFi配网模式
  if (lastEn == HIGH && en == LOW) {
    // 按键按下
    enPressStart = millis();
    enLongPressHandled = false;
    Serial.println("EN按键按下（长按3秒进入WiFi配网）");
  }
  
  if (lastEn == LOW && en == LOW) {
    // 按键持续按住，检测长按
    if (!enLongPressHandled && (millis() - enPressStart >= 3000)) {
      enLongPressHandled = true;
      Serial.println("🔁 长按EN按键触发：进入WiFi配网模式...");
      
      // 屏幕提示
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextSize(1);
      tft.drawString("WiFi 配网模式", 25, 40);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("请用手机连接", 30, 60);
      tft.drawString("Blinker热点", 35, 75);
      tft.drawString("打开App配置WiFi", 20, 90);
      
      // 清除已保存的WiFi信息，重新进入配网模式
      Blinker.reset();
      
      // 重新初始化Blinker进入配网模式
      delay(500);
      Blinker.begin(auth);
    }
  }
  
  if (lastEn == LOW && en == HIGH) {
    // 按键释放
    if (!enLongPressHandled) {
      Serial.println("EN按键短按");
    }
  }
  
  lastMenu = menu; lastLeft = left; lastRight = right; lastEn = en;
}

// ==================== 数据保存与加载 ====================
void saveData() {
  preferences.begin("coulomb", false);
  
  preferences.putFloat("dailyAH", accumulatedAH);
  preferences.putFloat("dailyKWh", accumulatedKWh);
  preferences.putInt("battCells", batteryCells);
  preferences.putInt("battType", batteryType);
  preferences.putFloat("fullVolt", fullVoltage);
  preferences.putFloat("battCap", batteryCapacity);
  
  for (int i = 0; i < 5; i++) {
    char key[8]; sprintf(key, "relay%d", i);
    preferences.putBool(key, relayState[i]);
  }
  
  preferences.putBool("fanState", fanState);
  preferences.putInt("fanSpeed", fanSpeed);
  
  preferences.end();
  Serial.println("✅ 数据已保存");
}

void loadData() {
  preferences.begin("coulomb", true);
  
  accumulatedAH = preferences.getFloat("dailyAH", 0.0);
  accumulatedKWh = preferences.getFloat("dailyKWh", 0.0);
  batteryCells = preferences.getInt("battCells", 4);
  batteryType = preferences.getInt("battType", 0);
  fullVoltage = preferences.getFloat("fullVolt", 14.6);
  batteryCapacity = preferences.getFloat("battCap", 100);
  fanState = preferences.getBool("fanState", false);
  fanSpeed = preferences.getInt("fanSpeed", 0);
  
  for (int i = 0; i < 5; i++) {
    char key[8]; sprintf(key, "relay%d", i);
    relayState[i] = preferences.getBool(key, false);
    setRelay(i, relayState[i]);
  }
  
  preferences.end();
  Serial.println("✅ 数据已加载");
  
  BTN_OUT1.print(relayState[0] ? "on" : "off");
  BTN_OUT2.print(relayState[1] ? "on" : "off");
  BTN_OUT3.print(relayState[2] ? "on" : "off");
  BTN_OUT4.print(relayState[3] ? "on" : "off");
  BTN_OUT5.print(relayState[4] ? "on" : "off");
  
  if (batteryType == 0) BTN_BT0.print("on");
  else if (batteryType == 1) BTN_BT1.print("on");
  else BTN_BT2.print("on");
}

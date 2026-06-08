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
 * 硬件设计 (基于网络表 Netlist_ESP32_库仑计_2026-06-08.enet):
 * - 主控: ESP32 (U2)
 * - 屏幕: TFT 1.8寸 (CN8) SPI接口 (软件SPI, 7针)
 * - 电流检测: INA226 (U70) MSOP-10封装, I2C接口
 * - 采样电阻: 默认1mΩ (最大80A), 可改为2mΩ(40A)或4mΩ(20A)
 * - 电源: MP9486A (U3) 降压 + AMS1117-3.3 (U5) 稳压
 * - NTC温度检测 (GPIO39)
 * - BMS串口 (UART1: RX=GPIO16, TX=GPIO17) 接电池保护板
 * - 5路继电器输出 (GPIO14/27/4/33/32)
 * - 按键: MENU(GPIO0), LEFT(GPIO18), RIGHT(GPIO19)
 * 
 * 【常用串口指令】
 * ✔重启设备：AT+RST
 * ✔产品激活：AT+REG 激活码
 * ✔通过命令快速连接WIFI：AT+WIFI.connect WIFI名称 WIFI密码
 * ✔设置输出电压校验：AT+INAV 实际电压
 * ✔设置输出电流校验：AT+INAC 实际电流
 * ✔恢复出厂设置：AT+RESTORE
 *   默认网页管理员：admin
 *   默认热点管理页面：http://192.168.0.1
 *   默认网页与热点密码：12345678
 *   注意：激活状态不受影响
 */

#include <Blinker.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <INA226.h>
#include <Preferences.h>

// ==================== 引脚定义 (基于网络表 Netlist_ESP32_库仑计_2026-06-08.enet) ====================
// I2C - INA226
#define I2C_SDA 21  // U2.33 → SDA → U70.4
#define I2C_SCL 22  // U2.36 → SCL → U70.5

// TFT 屏幕 (1.8寸 SPI) - CN8（实际PCB上的屏幕接口）
// CN8引脚: 1=GND, 2=VCC, 3=SCK(GPIO15), 4=SDA(GPIO13), 5=RES(GPIO25), 6=DC(GPIO26), 7=BLK(GPIO12)
#define TFT_CS   -1  // 7针模块CS接地常开，无需控制
#define TFT_DC   26  // CN8.6 → U2.11 → GPIO26
#define TFT_RST  25  // CN8.5 → U2.10 → GPIO25
#define TFT_SCLK 15  // CN8.3 → U2.23 → GPIO15
#define TFT_MOSI 13  // CN8.4 → U2.16 → GPIO13
#define TFT_BLK  12  // CN8.7 → U2.14 → GPIO12

// 按键 (基于网络表)
#define KEY_MENU   0   // MENU按键 → U2.25 → GPIO0
#define KEY_LEFT   18  // LEFT按键 → U2.30 → GPIO18
#define KEY_RIGHT  19  // RIGHT按键 → U2.31 → GPIO19

// 继电器输出
#define RELAY1_PIN  14  // U2.13 → GPIO14
#define RELAY2_PIN  27  // U2.12 → GPIO27
#define RELAY3_PIN  4   // U2.26 → GPIO4 (K1)
#define RELAY4_PIN  33  // U2.9  → GPIO33 (K2)
#define RELAY5_PIN  32  // U2.8  → GPIO32 (K3)

// BMS串口 (接电池保护板) - UART1
#define BMS_RX      16  // U2.27 → RXD1 → H1.2 (接保护板TX)
#define BMS_TX      17  // U2.28 → TXD1 → H1.3 (接保护板RX)

// NTC温度检测
#define NTC_PIN     39  // U2.5  → GPIO39

// ADC - 电池电压检测
#define ADC_BATTERY 34  // U2.6 → GPIO34

// 串口命令缓冲区
#define SERIAL_BUF_SIZE  256

// ==================== 采样电阻配置 ====================
#define SHUNT_RESISTOR  0.001
#define MAX_CURRENT     80.0

// ==================== 全局对象 ====================
TFT_eSPI tft = TFT_eSPI();
INA226 ina226(0x40);
Preferences preferences;

// ==================== 点灯科技密钥 ====================
char auth[] = "Your_Blinker_Auth_Key";

// ==================== 全局变量 ====================
float busVoltage = 0.0;
float shuntVoltage = 0.0;
float current = 0.0;
float power = 0.0;
float batteryVoltage = 0.0;
float batteryPercent = 0;
float temperature = 25.0;

float accumulatedAH = 0.0;
float accumulatedKWh = 0.0;
float dailyCost = 0.0;

int batteryCells = 4;
int batteryType = 0;        // 0=铁锂, 1=三元, 2=铅酸
float fullVoltage = 14.6;
float batteryCapacity = 100;

bool relayState[5] = {false, false, false, false, false};

bool wifiConnected = false;
bool activated = false;
unsigned long lastMeasureTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastSaveTime = 0;
unsigned long lastBlinkerTime = 0;

float voltageCalibOffset = 0.0;
float currentCalibOffset = 0.0;

char serialBuf[SERIAL_BUF_SIZE];
int serialIdx = 0;

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
void processSerialCommand();
void handleATRestore();
void handleATRst();
void handleATReg(const char *param);
void handleATWifiConnect(const char *param);
void handleATInav(const char *param);
void handleATInac(const char *param);
void printHelp();
void saveCalibration();
void loadCalibration();

// ==================== 设置函数 ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n==================================");
  Serial.println("  ESP32 库仑计 v1.0");
  Serial.println("  项目规格: 12~96V / 0~80A / 5路开关");
  Serial.println("  输入AT+HELP查看串口指令帮助");
  Serial.println("==================================\n");
  
  setupHardware();
  setupDisplay();
  loadCalibration();
  setupBlinker();
  loadData();
  
  Serial.println("\n✅ 系统启动完成！");
}

void setupHardware() {
  Wire.begin(I2C_SDA, I2C_SCL);
  
  if (!ina226.begin()) {
    Serial.println("❌ INA226 初始化失败！请检查I2C连接");
  } else {
    Serial.println("✅ INA226 初始化成功");
    Serial.printf("   采样电阻: %.0fmΩ, 最大电流: %.0fA\n", SHUNT_RESISTOR * 1000, MAX_CURRENT);
    ina226.setMaxCurrentShunt(MAX_CURRENT, SHUNT_RESISTOR);
    ina226.setAveraging(INA226_AVG_128);
    ina226.setConversionTime(INA226_CONV_TIME_1100US);
    ina226.setMode(INA226_MODE_CONTINUOUS);
  }
  
  int relayPins[] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN, RELAY5_PIN};
  for (int i = 0; i < 5; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }
  
  // 初始化BMS串口 (UART1, 9600波特率, 接电池保护板)
  Serial1.begin(9600, SERIAL_8N1, BMS_RX, BMS_TX);
  Serial.println("✅ BMS串口(UART1)初始化完成 (RX=GPIO16, TX=GPIO17)");

  pinMode(KEY_MENU, INPUT_PULLUP);
  pinMode(KEY_LEFT, INPUT_PULLUP);
  pinMode(KEY_RIGHT, INPUT_PULLUP);
  
  analogReadResolution(12);
  Serial.println("✅ 硬件初始化完成");
}

void setupDisplay() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, 160, 128, TFT_NAVY);
  tft.fillRect(1, 1, 158, 14, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(1);
  tft.drawString("ESP32 库仑计", 5, 3);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("v1.0", 130, 3);
  drawBatteryIcon(55, 35, 50, 25, 75);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("初始化中...", 50, 70);
  for (int i = 0; i <= 100; i += 5) {
    tft.fillRect(20, 90, 120, 8, TFT_BLACK);
    tft.drawRect(20, 90, 120, 8, TFT_WHITE);
    tft.fillRect(21, 91, map(i, 0, 100, 0, 118), 6, TFT_GREEN);
    delay(30);
  }
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);
}

void setupBlinker() {
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
  Blinker.attachData([](const String &data) { Blinker.vibrate(); });
  Serial.println("✅ 点灯科技初始化完成");
}

// ==================== 主循环 ====================
void loop() {
  Blinker.run();
  processSerialCommand();
  
  unsigned long now = millis();
  if (now - lastMeasureTime >= 100) { readSensors(); lastMeasureTime = now; }
  if (now - lastDisplayTime >= 200) { updateDisplay(); lastDisplayTime = now; }
  if (now - lastBlinkerTime >= 1000) { updateBlinkerData(); lastBlinkerTime = now; }
  if (now - lastSaveTime >= 60000) { saveData(); lastSaveTime = now; }
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
  if (now - lastIntegrate >= 1000) {
    float dt = 1.0 / 3600.0;
    accumulatedAH += current * dt;
    accumulatedKWh += (busVoltage * current) * dt / 1000.0;
    lastIntegrate = now;
    dailyCost = accumulatedKWh * 0.5;
  }
}

void readINA226() {
  busVoltage = ina226.getBusVoltage() + voltageCalibOffset;
  shuntVoltage = ina226.getShuntVoltage_mV();
  current = ina226.getCurrent() + currentCalibOffset;
  power = busVoltage * current;
  if (isnan(busVoltage) || busVoltage < 0) busVoltage = 0;
  if (isnan(current) || current < 0) current = 0;
  if (isnan(power) || power < 0) power = 0;
}

void readBatteryVoltage() {
  int adcValue = analogRead(ADC_BATTERY);
  batteryVoltage = (adcValue / 4095.0) * 3.3 * 5.0;
}

void readTemperature() {
  int ntcValue = analogRead(NTC_PIN);
  float resistance = (4095.0 / max(ntcValue, 1) - 1.0) * 10000;
  float steinhart = log(resistance / 10000.0) / 3988.0 + 1.0 / 298.15;
  steinhart = 1.0 / steinhart - 273.15;
  temperature = steinhart;
}

void calculateBatteryPercent() {
  float minVoltage, maxVoltage;
  switch (batteryType) {
    case 0: minVoltage = 2.5 * batteryCells; maxVoltage = 3.65 * batteryCells; break;
    case 1: minVoltage = 3.0 * batteryCells; maxVoltage = 4.2 * batteryCells; break;
    case 2: minVoltage = 1.75 * batteryCells; maxVoltage = 2.45 * batteryCells; break;
    default: minVoltage = 3.0 * batteryCells; maxVoltage = 4.2 * batteryCells;
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
  tft.fillRect(0, 0, 160, 14, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY); tft.setTextSize(1);
  tft.drawString("ESP32 库仑计", 3, 2);
  drawWifiIcon(130, 1, wifiConnected);
  tft.drawFastHLine(0, 14, 160, TFT_DARKGREY);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(1);
  tft.drawString("V", 3, 18);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setTextSize(2);
  tft.drawString(String(busVoltage, 2), 15, 16);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(1);
  tft.drawString("A", 85, 18);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setTextSize(2);
  tft.drawString(String(current, 2), 97, 16);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(1);
  tft.drawString("W", 3, 38);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(3);
  tft.drawString(String(power, 1), 15, 34);
  
  int barY = 60;
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(1);
  tft.drawString("BAT", 3, barY);
  drawBatteryIcon(35, barY - 2, 85, 14, batteryPercent);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(batteryPercent, 0) + "%", 125, barY);
  
  int row4Y = 78;
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(1);
  tft.drawString("AH", 3, row4Y);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(String(accumulatedAH, 2), 25, row4Y);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("KWh", 75, row4Y);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(String(accumulatedKWh, 3), 105, row4Y);
  
  int row5Y = 94;
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(1);
  tft.drawString("TEMP", 3, row5Y);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(temperature, 1) + "C", 40, row5Y);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("OUT", 90, row5Y);
  for (int i = 0; i < 5; i++) drawRelayIcon(118 + i * 8, row5Y, relayState[i]);
  
  int row6Y = 110;
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("CELL", 3, row6Y);
  String typeStr[] = {"LFP", "NCM", "Pb"};
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(typeStr[batteryType] + " " + String(batteryCells) + "S", 40, row6Y);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("FV", 90, row6Y);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(fullVoltage, 1) + "V", 110, row6Y);
}

// ==================== 图形化辅助函数 ====================
void drawBatteryIcon(int x, int y, int w, int h, int percent) {
  tft.drawRect(x, y, w, h, TFT_WHITE);
  tft.fillRect(x + w, y + h / 4, 3, h / 2, TFT_WHITE);
  int fillW = map(percent, 0, 100, 0, w - 2);
  uint16_t color = (percent > 60) ? TFT_GREEN : ((percent > 20) ? TFT_YELLOW : TFT_RED);
  if (fillW > 0) tft.fillRect(x + 1, y + 1, fillW, h - 2, color);
}

void drawWifiIcon(int x, int y, bool connected) {
  uint16_t color = connected ? TFT_GREEN : TFT_RED;
  if (connected) {
    tft.drawCircle(x + 6, y + 8, 2, color);
    tft.drawArc(x + 6, y + 8, 5, 3, 180, 360, color, TFT_BLACK);
    tft.drawArc(x + 6, y + 8, 8, 6, 180, 360, color, TFT_BLACK);
  } else {
    tft.drawString("X", x + 2, y + 2);
  }
}

void drawRelayIcon(int x, int y, bool state) {
  if (state) tft.fillCircle(x + 3, y + 4, 3, TFT_GREEN);
  else tft.drawCircle(x + 3, y + 4, 3, TFT_DARKGREY);
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
  if (chartCounter >= 5) { CHART_POWER.point(power, busVoltage, current); chartCounter = 0; }
}

// ==================== 按钮回调 ====================
void btnUpdateCallback(const String &state) {
  if (state == BLINKER_CMD_BUTTON_PRESSED) {
    Serial.println("进入OTA固件升级模式");
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW); tft.drawString("OTA升级中...", 20, 60);
    Blinker.print("即将进入固件升级模式...");
    delay(1000); Blinker.ota();
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
  int pins[] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN, RELAY5_PIN};
  digitalWrite(pins[index], state ? HIGH : LOW);
  BlinkerButton *btns[] = {&BTN_OUT1, &BTN_OUT2, &BTN_OUT3, &BTN_OUT4, &BTN_OUT5};
  btns[index]->print(state ? "on" : "off");
  Serial.printf("继电器%d: %s\n", index + 1, state ? "ON" : "OFF");
}

void sliderCellsCallback(int32_t value) { batteryCells = value; calculateBatteryPercent(); Serial.printf("电池串数: %d\n", batteryCells); }
void btnBt0Callback(const String &state) { batteryType = 0; BTN_BT0.print("on"); BTN_BT1.print("off"); BTN_BT2.print("off"); calculateBatteryPercent(); Serial.println("电池类型: 铁锂"); }
void btnBt1Callback(const String &state) { batteryType = 1; BTN_BT0.print("off"); BTN_BT1.print("on"); BTN_BT2.print("off"); calculateBatteryPercent(); Serial.println("电池类型: 三元"); }
void btnBt2Callback(const String &state) { batteryType = 2; BTN_BT0.print("off"); BTN_BT1.print("off"); BTN_BT2.print("on"); calculateBatteryPercent(); Serial.println("电池类型: 铅酸"); }

// ==================== 本地按键处理 ====================
void handleButtons() {
  static unsigned long lastBtnCheck = 0;
  static bool lastMenu = HIGH, lastLeft = HIGH, lastRight = HIGH;
  if (millis() - lastBtnCheck < 50) return;
  lastBtnCheck = millis();
  bool menu = digitalRead(KEY_MENU);
  bool left = digitalRead(KEY_LEFT);
  bool right = digitalRead(KEY_RIGHT);
  if (lastMenu == HIGH && menu == LOW) Serial.println("MENU按键");
  if (lastLeft == HIGH && left == LOW) Serial.println("LEFT按键");
  if (lastRight == HIGH && right == LOW) Serial.println("RIGHT按键");
  lastMenu = menu; lastLeft = left; lastRight = right;
}

// ==================== 串口AT命令处理 ====================
void processSerialCommand() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialIdx > 0) {
        serialBuf[serialIdx] = '\0';
        String cmd = String(serialBuf);
        cmd.trim();
        Serial.print("\n>>> ");
        Serial.println(cmd);
        
        if (cmd == "AT+HELP") {
          printHelp();
        } else if (cmd == "AT+RST") {
          handleATRst();
        } else if (cmd.startsWith("AT+REG ")) {
          handleATReg(cmd.substring(7).c_str());
        } else if (cmd.startsWith("AT+WIFI.connect ")) {
          handleATWifiConnect(cmd.substring(16).c_str());
        } else if (cmd.startsWith("AT+INAV ")) {
          handleATInav(cmd.substring(8).c_str());
        } else if (cmd.startsWith("AT+INAC ")) {
          handleATInac(cmd.substring(8).c_str());
        } else if (cmd == "AT+RESTORE") {
          handleATRestore();
        } else if (cmd.length() > 0) {
          Serial.println("❌ 未知命令，输入 AT+HELP 查看帮助");
        }
        serialIdx = 0;
      }
    } else if (serialIdx < SERIAL_BUF_SIZE - 1) {
      serialBuf[serialIdx++] = c;
    }
  }
}

void printHelp() {
  Serial.println("\n=================== AT 指令帮助 ===================");
  Serial.println("AT+HELP            - 显示此帮助");
  Serial.println("AT+RST             - 重启设备");
  Serial.println("AT+REG 激活码      - 产品激活");
  Serial.println("AT+WIFI.connect    - 连接WiFi: AT+WIFI.connect SSID PASSWORD");
  Serial.println("AT+INAV 实际电压   - 设置电压校准 (输入万用表测量值)");
  Serial.println("AT+INAC 实际电流   - 设置电流校准 (输入万用表测量值)");
  Serial.println("AT+RESTORE         - 恢复出厂设置 (清除数据，不含激活状态)");
  Serial.println("====================================================\n");
  Serial.printf("当前状态: 激活=%s, WiFi=%s\n", activated ? "已激活" : "未激活", wifiConnected ? "已连接" : "未连接");
  Serial.printf("电压校准偏移: %.3fV, 电流校准偏移: %.3fA\n", voltageCalibOffset, currentCalibOffset);
  Serial.printf("当前读数: %.2fV, %.2fA, %.1fW\n", busVoltage, current, power);
}

void handleATRst() {
  Serial.println("♻️ 设备即将重启...");
  delay(100);
  ESP.restart();
}

void handleATReg(const char *param) {
  String key = String(param);
  key.trim();
  if (key.length() == 0) { Serial.println("❌ 激活码不能为空"); return; }
  preferences.begin("coulomb", false);
  preferences.putBool("activated", true);
  preferences.putString("actKey", key);
  preferences.end();
  activated = true;
  Serial.println("✅ 产品激活成功！");
}

void handleATWifiConnect(const char *param) {
  String params = String(param);
  params.trim();
  int spaceIdx = params.indexOf(' ');
  if (spaceIdx <= 0) { Serial.println("❌ 用法: AT+WIFI.connect WIFI名称 WIFI密码"); return; }
  String ssid = params.substring(0, spaceIdx);
  String password = params.substring(spaceIdx + 1);
  Serial.printf("📶 正在连接WiFi: %s ...\n", ssid.c_str());
  preferences.begin("coulomb", false);
  preferences.putString("wifiSSID", ssid);
  preferences.putString("wifiPass", password);
  preferences.end();
  WiFi.begin(ssid.c_str(), password.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) { delay(500); Serial.print("."); attempts++; }
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\n✅ WiFi连接成功！IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n❌ WiFi连接失败，请检查SSID和密码");
  }
}

void handleATInav(const char *param) {
  float actualVoltage = String(param).toFloat();
  if (actualVoltage <= 0) { Serial.println("❌ 无效电压值"); return; }
  voltageCalibOffset = actualVoltage - ina226.getBusVoltage();
  saveCalibration();
  Serial.printf("✅ 电压校准完成！偏移: %.3fV, 校准后: %.3fV\n", voltageCalibOffset, ina226.getBusVoltage() + voltageCalibOffset);
}

void handleATInac(const char *param) {
  float actualCurrent = String(param).toFloat();
  if (actualCurrent < 0) { Serial.println("❌ 无效电流值"); return; }
  currentCalibOffset = actualCurrent - ina226.getCurrent();
  saveCalibration();
  Serial.printf("✅ 电流校准完成！偏移: %.3fA, 校准后: %.3fA\n", currentCalibOffset, ina226.getCurrent() + currentCalibOffset);
}

void handleATRestore() {
  Serial.println("⚠️ 正在恢复出厂设置...");
  preferences.begin("coulomb", false);
  preferences.clear();
  if (activated) preferences.putBool("activated", true);
  preferences.end();
  accumulatedAH = 0; accumulatedKWh = 0; dailyCost = 0;
  batteryCells = 4; batteryType = 0; fullVoltage = 14.6; batteryCapacity = 100;
  voltageCalibOffset = 0; currentCalibOffset = 0;
  for (int i = 0; i < 5; i++) { relayState[i] = false; setRelay(i, false); }
  Serial.println("✅ 恢复出厂设置完成！");
  Serial.println("   默认网页管理员: admin");
  Serial.println("   默认热点管理页面: http://192.168.0.1");
  Serial.println("   默认网页与热点密码: 12345678");
  Serial.println("   注意: 激活状态不受影响");
}

// ==================== 校准值存储 ====================
void saveCalibration() {
  preferences.begin("coulomb", false);
  preferences.putFloat("voltCalib", voltageCalibOffset);
  preferences.putFloat("curCalib", currentCalibOffset);
  preferences.end();
  Serial.println("✅ 校准值已保存");
}

void loadCalibration() {
  preferences.begin("coulomb", true);
  activated = preferences.getBool("activated", false);
  voltageCalibOffset = preferences.getFloat("voltCalib", 0.0);
  currentCalibOffset = preferences.getFloat("curCalib", 0.0);
  preferences.end();
  if (activated) Serial.println("✅ 产品已激活");
  if (voltageCalibOffset != 0.0 || currentCalibOffset != 0.0)
    Serial.printf("📐 校准值: 电压偏移=%.3fV, 电流偏移=%.3fA\n", voltageCalibOffset, currentCalibOffset);
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
  for (int i = 0; i < 5; i++) { char key[8]; sprintf(key, "relay%d", i); preferences.putBool(key, relayState[i]); }
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
  String savedSSID = preferences.getString("wifiSSID", "");
  String savedPass = preferences.getString("wifiPass", "");
  for (int i = 0; i < 5; i++) { char key[8]; sprintf(key, "relay%d", i); relayState[i] = preferences.getBool(key, false); setRelay(i, relayState[i]); }
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
  
  if (savedSSID.length() > 0) {
    Serial.printf("📶 正在自动连接WiFi: %s ...\n", savedSSID.c_str());
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) { delay(500); Serial.print("."); attempts++; }
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.printf("\n✅ WiFi自动连接成功！IP: %s\n", WiFi.localIP().toString().c_str());
    } else { Serial.println("\n❌ WiFi自动连接失败"); }
  }
}
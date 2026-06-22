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
 * ✔在线升级：AT+UPDATE
 * ✔产品激活：AT+REG 激活码
 * ✔连接WiFi：AT+WIFI SSID 密码
 * ✔输入电压校验：AT+INAV 实际的输入电压值
 * ✔输入电流校验：AT+INAC 实际的输入电流值
 * ✔输出电压校验：AT+ONAV 实际的输出电压值
 * ✔输出电流校验：AT+ONAC 实际的输出电流值
 * ✔点灯科技密钥查询：AT+DD
 * ✔修改点灯科技密钥：AT+DD ID
 * ✔恢复出厂设置：AT+RESTORE
 *   默认网页管理员：admin
 *   默认热点管理页面：http://192.168.4.1
 *   默认网页与热点密码：12345678
 *   注意：激活状态不受影响
 */

#define BLINKER_WIFI
#include <Blinker.h>
#include "User_Setup.h"    // 优先加载项目目录下的TFT_eSPI配置
#include <TFT_eSPI.h>
#include <Wire.h>
#include <INA226.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <time.h>

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
// EN 按键直接连接到 ESP32 模块 EN 脚，不作为普通 GPIO 输入读取
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

// ==================== 电压分压网络 ====================
// INA226 VBUS引脚通过R52(39kΩ)+R54(20kΩ)分压:
// V_VBUS / V_IN = R54 / (R52+R54) = 20k / (39k+20k) ≈ 0.3390
// 代码中需乘以分压比倒数以还原实际输入电压
#define VBUS_DIVIDER_RATIO  ((39.0 + 20.0) / 20.0)  // ≈ 2.95

// 串口命令缓冲区
#define SERIAL_BUF_SIZE  256

// ==================== 采样电阻配置 ====================
#define SHUNT_RESISTOR  0.001
#define MAX_CURRENT     80.0

// ==================== 全局对象 ====================
TFT_eSPI tft = TFT_eSPI();   // 引脚配置在 User_Setup.h 中
INA226 ina226(0x44);  // 实际I2C地址（A1=VS+, A0=GND）
Preferences preferences;
WebServer server(80);
DNSServer dnsServer;
bool configMode = false;  // 是否处于配网模式

// ==================== 点灯科技密钥 ====================
char auth[64] = ""; // 默认空，使用 AT+DD ID 设置或从保存数据加载

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
bool timeSynced = false;
unsigned long lastTimeSync = 0;
bool activated = false;
unsigned long lastMeasureTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastSaveTime = 0;
unsigned long lastBlinkerTime = 0;

bool showConfigScreen = false;

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
// BlinkerChart 在当前 Blinker 库中不可用，暂时移除图表功能
// 可后续通过 Blinker 自定义数据接口实现
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
void startConfigMode();
void handleConfigRoot();
void handleConfigSave();
void handleConfigNotFound();
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
void syncNetworkTime();
String getTimeString();
void updateWifiState();
void drawConfigScreen();
String getDeviceId();
String getSavedWifiSSID();
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
void handleATUpdate();
void handleATInav(const char *param);
void handleATInac(const char *param);
void handleATOnav(const char *param);
void handleATOnac(const char *param);
void handleATDD(const char *param);
void handleATWifiStatus();
void handleATWifiSaved();
void printHelp();
void saveCalibration();
void loadCalibration();
void saveBlinkerAuth();
void loadBlinkerAuth();

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
  loadBlinkerAuth();
  
  // 检查是否有保存的WiFi，如果没有则进入配网模式
  preferences.begin("coulomb", true);
  String savedSSID = preferences.getString("wifiSSID", "");
  preferences.end();
  
  if (savedSSID.length() == 0) {
    Serial.println("📶 未找到WiFi配置，启动配网模式...");
    startConfigMode();
  } else {
    setupBlinker();
    loadData();
  }
  
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
    ina226.setAverage(INA226_128_SAMPLES);
    ina226.setBusVoltageConversionTime(INA226_1100_us);
    ina226.setShuntVoltageConversionTime(INA226_1100_us);
    ina226.setModeShuntBusContinuous();
  }
  
  int relayPins[] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN, RELAY5_PIN};
  for (int i = 0; i < 5; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }
  
  // 初始化BMS串口 (UART1, 9600波特率, 接电池保护板)
  Serial1.begin(9600, SERIAL_8N1, BMS_RX, BMS_TX);
  Serial.println("✅ BMS串口(UART1)初始化完成 (RX=GPIO16, TX=GPIO17)");

  pinMode(KEY_MENU, INPUT_PULLUP);   // MENU 按键，使用内部上拉
  pinMode(KEY_LEFT, INPUT_PULLUP);   // LEFT 按键，使用内部上拉
  pinMode(KEY_RIGHT, INPUT_PULLUP);  // RIGHT 按键，使用内部上拉
  // EN 按键连接到 ESP32 模块 EN 脚，不能当作普通 GPIO 读取
  
  analogReadResolution(12);
  Serial.println("✅ 硬件初始化完成");
}

void setupDisplay() {
  tft.init();
  // 1.33寸屏幕建议竖屏显示，旋转180度（根据实际安装调整）
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, 240, 240, TFT_NAVY);
  tft.fillRect(1, 1, 238, 22, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(1);
  tft.drawString("ESP32 库仑计", 5, 4);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("v1.0", 200, 4);
  drawBatteryIcon(80, 60, 80, 35, 75);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("初始化中...", 80, 120);
  for (int i = 0; i <= 100; i += 5) {
    tft.fillRect(40, 160, 160, 12, TFT_BLACK);
    tft.drawRect(40, 160, 160, 12, TFT_WHITE);
    tft.fillRect(41, 161, map(i, 0, 100, 0, 158), 10, TFT_GREEN);
    delay(30);
  }
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);
}

void setupBlinker() {
  // 读取已保存的WiFi凭据，Blinker需要3个参数初始化
  preferences.begin("coulomb", true);
  String savedSSID = preferences.getString("wifiSSID", "");
  String savedPass = preferences.getString("wifiPass", "");
  preferences.end();

  if (savedSSID.length() > 0) {
    Blinker.begin(auth, savedSSID.c_str(), savedPass.c_str());
    Serial.printf("📶 Blinker 初始化: SSID=%s\n", savedSSID.c_str());
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      timeSynced = false;
      lastTimeSync = 0;
    }
  } else {
    // 没有WiFi配置时，传入空字符串（不会成功连接，但避免编译错误）
    Blinker.begin(auth, "", "");
    Serial.println("⚠️ 无WiFi配置，Blinker未连接");
  }

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

// ==================== Web配网模式 ====================
void startConfigMode() {
  configMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-库仑计", "12345678");
  Serial.printf("📶 配网热点已启动: ESP32-库仑计 (密码: 12345678)\n");
  Serial.printf("   配网页面: http://%s\n", WiFi.softAPIP().toString().c_str());

  // DNS劫持：所有域名都指向AP IP（Captive Portal）
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", handleConfigRoot);
  server.on("/save", HTTP_POST, handleConfigSave);
  server.onNotFound(handleConfigNotFound);
  server.begin();

  // 屏幕提示
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_NAVY); tft.setTextSize(1);
  tft.fillRect(0, 0, 240, 22, TFT_NAVY);
  tft.drawString("ESP32 库仑计 - 配网模式", 3, 4);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(2);
  tft.drawString("WiFi配置", 70, 35);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setTextSize(1);
  tft.drawString("1.连接热点: ESP32-库仑计", 10, 70);
  tft.drawString("  密码: 12345678", 10, 85);
  tft.drawString("2.浏览器访问:", 10, 110);
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(2);
  tft.drawString("192.168.4.1", 40, 130);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setTextSize(1);
  tft.drawString("3.输入WiFi名称和密码", 10, 160);
  tft.drawString("4.点击连接", 10, 175);
}

void handleConfigRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 库仑计 - WiFi配置</title>
<style>
  body{font-family:Arial,sans-serif;max-width:400px;margin:0 auto;padding:20px;background:#1a1a2e;color:#fff}
  h1{color:#00d4ff;text-align:center;font-size:22px}
  .card{background:#16213e;border-radius:10px;padding:20px;margin:15px 0}
  input{width:100%;padding:10px;margin:8px 0;border:1px solid #0f3460;border-radius:5px;
    background:#0a0a23;color:#fff;font-size:16px;box-sizing:border-box}
  button{width:100%;padding:12px;background:#00d4ff;color:#000;border:none;border-radius:5px;
    font-size:18px;font-weight:bold;cursor:pointer;margin-top:10px}
  button:hover{background:#00b4d8}
  .info{text-align:center;color:#aaa;font-size:13px;margin-top:10px}
</style>
</head><body>
<h1>ESP32 库仑计</h1>
<div class="card">
  <form action="/save" method="POST">
    <label>WiFi 名称</label>
    <input type="text" name="ssid" placeholder="输入WiFi名称(仅支持2.4G)">
    <label>WiFi 密码</label>
    <input type="password" name="pass" placeholder="输入WiFi密码">
    <button type="submit">连 接</button>
  </form>
  <p class="info">仅支持2.4GHz WiFi</p>
</div>
</body></html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleConfigSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  
  if (ssid.length() == 0) {
    server.send(400, "text/html", "<html><body><h3>WiFi名称不能为空！</h3><a href='/'>返回</a></body></html>");
    return;
  }
  
  // 保存WiFi配置
  preferences.begin("coulomb", false);
  preferences.putString("wifiSSID", ssid);
  preferences.putString("wifiPass", pass);
  preferences.end();
  
  String html = R"rawliteral(
<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>配置成功</title>
<style>body{font-family:Arial;text-align:center;padding:40px;background:#1a1a2e;color:#fff}
  h2{color:#00ff88}.info{color:#aaa;margin-top:20px}</style>
</head><body>
<h2>WiFi配置成功！</h2>
<p>设备正在连接 WiFi ...</p>
<p class="info">设备将自动重启</p>
</body></html>
)rawliteral";
  server.send(200, "text/html", html);
  
  Serial.printf("📶 收到WiFi配置: %s\n", ssid.c_str());
  delay(1000);
  
  // 重启以应用新配置
  Serial.println("🔄 正在重启...");
  ESP.restart();
}

void handleConfigNotFound() {
  // Captive Portal：所有未知请求都重定向到配网页面
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
  server.send(302, "text/plain", "");
}

// ==================== 主循环 ====================
/**
 * @brief Arduino 主循环，负责按时间调度执行各项任务
 * 
 * 本函数采用非阻塞方式，通过 millis() 判断各任务是否到达执行间隔。
 * 支持配置模式（WiFi AP + DNS + Web 服务器）与正常运行模式，
 * 并定期执行传感器读取、显示刷新、物联网数据上报、数据存储及按键处理。
 */
void loop() {
  // ----- 配置模式处理 -----
  // 若当前处于配网模式（configMode = true），则仅处理 DNS 和 Web 服务器请求，
  // 不执行其他任务，直到用户完成配置并重启。
  if (configMode) {
    dnsServer.processNextRequest();  // 处理 DNS 解析请求（用于 captive portal）
    server.handleClient();           // 处理 HTTP 客户端请求（配置页面）
    return;                          // 直接返回，跳过后续所有任务
  }
  // ----- 正常运行模式 -----
  // 处理 Blinker 物联网框架的后台任务（如保持 MQTT 连接、处理云端指令）
  Blinker.run();
  // 处理通过串口发送的命令（如调试指令、参数修改）
  processSerialCommand();

  updateWifiState();
  if (wifiConnected && !timeSynced && millis() - lastTimeSync >= 10000) {
    syncNetworkTime();
    lastTimeSync = millis();
  }

  // 获取当前系统运行时间（毫秒）
  unsigned long now = millis();
 // 每 100ms 读取一次传感器（INA226 等），更新全局变量
  if (now - lastMeasureTime >= 100) {
    readSensors();          // 调用传感器读取函数（内部可能包含 readINA226()）
    lastMeasureTime = now;  // 更新上次执行时间
  }

  // 每 500ms 更新一次显示（OLED/LCD 屏幕刷新）
  if (now - lastDisplayTime >= 500) {
    updateDisplay();        // 将当前数据渲染到显示屏
    lastDisplayTime = now;
  }

  // 每 1000ms（1秒）向 Blinker 云端上报一次数据（电压、电流、功率等）
  if (now - lastBlinkerTime >= 1000) {
    updateBlinkerData();    // 将传感器数据发送至 Blinker 平台
    lastBlinkerTime = now;
  }

  // 每 60000ms（1分钟）将当前数据保存到 EEPROM 或 SD 卡（掉电保存）
  if (now - lastSaveTime >= 60000) {
    saveData();             // 存储关键参数或统计数据
    lastSaveTime = now;
  }

  // 实时处理按键事件（短按、长按等），用于本地交互（如切换显示界面、校准）
  handleButtons();
}

// ==================== 传感器读取 ====================
/**
 * @brief 读取所有传感器数据，并每秒积分累加电量（Ah）和电能（kWh）
 * 
 * 本函数依次调用各传感器读取函数，更新全局变量（电压、电流、功率、温度、电量百分比等）。
 * 同时以 1 秒为周期，对电流和功率进行时间积分，分别累加为安时（Ah）和千瓦时（kWh），
 * 并据此计算预估电费（按每度电 0.5 元）。
 * 
 * 外部依赖（全局变量）：
 * - busVoltage, current, power（由 readINA226 更新）
 * - accumulatedAH：累计安时（Ah）
 * - accumulatedKWh：累计千瓦时（kWh）
 * - dailyCost：累计电费（元）
 */
void readSensors() {
  // 静态变量记录上次积分时间，仅在函数内保持，不丢失
  static unsigned long lastIntegrate = 0;
  unsigned long now = millis();

  // 读取 INA226 电压、电流、功率数据（含校准和异常处理）
  readINA226();

  // 读取电池电压（分压电阻测量，用于电量百分比计算）
  readBatteryVoltage();

  // 读取温度（板载或外部温度传感器）
  readTemperature();

  // 根据电池电压计算剩余电量百分比（查表或线性映射）
  calculateBatteryPercent();

  // 每秒执行一次电量与电能积分（时间步长固定为 1 秒）
  if (now - lastIntegrate >= 1000) {
    // 时间步长转换为小时（1 秒 = 1/3600 小时），用于积分至 Ah 和 kWh
    float dt = 1.0 / 3600.0;

    // 累计安时（Ah）：电流（A）× 时间（h），累加至总安时
    accumulatedAH += current * dt;

    // 累计千瓦时（kWh）：功率（W）× 时间（h）/ 1000，累加至总千瓦时
    // 此处 power = busVoltage * current，单位为 W
    accumulatedKWh += (busVoltage * current) * dt / 1000.0;

    // 更新上次积分时间戳
    lastIntegrate = now;

    // 按每度电 0.5 元计算当前累计电费（仅供参考，可在外部配置此单价）
    dailyCost = accumulatedKWh * 0.5;
  }
}
/**
 * @brief 读取 INA226 电压、电流、功率数据，并进行校准与异常值处理
 * 
 * 本函数从 INA226 模块获取总线电压、分流电压、电流和功率，
 * 并根据外部定义的校准参数进行修正。最后将所有数值限制为非负值，
 * 并处理无效读数（NaN）。
 * 
 * 外部依赖（需在全局或类中定义）：
 * - ina226：INA226 库对象，提供 getBusVoltage()、getShuntVoltage_mV()、getCurrent()
 * - VBUS_DIVIDER_RATIO：总线电压分压比（若未使用分压电阻则为 1.0）
 * - voltageCalibOffset：电压校准偏移量（单位：V）
 * - currentCalibOffset：电流校准偏移量（单位：mA 或 A，与 getCurrent() 单位一致）
 */
void readINA226() {
  // 读取原始总线电压（V），乘以分压比还原实际电压，再叠加校准偏移量
  busVoltage = ina226.getBusVoltage() * VBUS_DIVIDER_RATIO + voltageCalibOffset;
  // 读取分流电压（mV），此处仅存储，未参与后续计算（可选用于诊断）
  shuntVoltage = ina226.getShuntVoltage_mV();
  // 读取原始电流（单位与库配置一致），叠加校准偏移量
  current = ina226.getCurrent() + currentCalibOffset;
  // 计算功率 = 实际总线电压 × 实际电流（单位需匹配，若电压为 V，电流为 A，则功率为 W）
  power = busVoltage * current;
   // 异常值保护：若总线电压为 NaN 或负值，则强制归零（避免后续计算错误）
  if (isnan(busVoltage) || busVoltage < 0) busVoltage = 0;
  // 若电流为 NaN 或负值，强制归零
  if (isnan(current) || current < 0) current = 0;
  // 若功率为 NaN 或负值（可能因负电流或电压引起），强制归零
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
  if (showConfigScreen) {
    drawConfigScreen();
    return;
  }

  static bool firstRun = true;
  static bool lastWifiState = false;
  static bool lastRelayState[5] = {false, false, false, false, false};

  if (firstRun) {
    tft.fillScreen(TFT_BLACK);
    firstRun = false;
    // 静态标签只画一次
    tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(2);
    tft.drawString("V", 5, 32);
    tft.drawString("A", 5, 77);
    tft.drawString("W", 5, 122);
    tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(1);
    tft.drawString("AH", 5, 168);
    tft.drawString("KWh", 125, 168);
    tft.drawString("TEMP", 5, 195);
    tft.drawString("OUT", 130, 195);
    tft.drawString("CELL", 5, 220);
    tft.drawString("FV", 130, 220);
    tft.drawFastHLine(0, 23, 240, TFT_DARKGREY);
  }

  // 标题栏 - 仅在首次或 WiFi 状态变化时刷新
  String timeString = getTimeString();
  if (firstRun || wifiConnected != lastWifiState) {
    tft.fillRect(0, 0, 240, 22, TFT_NAVY);
    tft.setTextColor(TFT_WHITE, TFT_NAVY); tft.setTextSize(1);
    tft.drawString("ESP32 库仑计", 3, 4);
    drawWifiIcon(210, 2, wifiConnected);
    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    tft.drawString(timeString, 90, 4);
    lastWifiState = wifiConnected;
  } else {
    // 更新时间字符串区域
    tft.fillRect(90, 4, 110, 14, TFT_NAVY);
    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    tft.drawString(timeString, 90, 4);
  }

  // 左侧三行大字体：电压 / 电流 / 功率
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setTextSize(4);
  tft.drawString(String(busVoltage, 2) + "   ", 30, 30);
  tft.drawString(String(current, 2) + "   ", 30, 75);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(5);
  tft.drawString(String(power, 1) + "   ", 35, 118);

  // 右侧大电池图标（调小一点给左侧让空间）
  drawBatteryIconBig(175, 28, 60, 130, batteryPercent);

  // AH / KWh
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(String(accumulatedAH, 2) + "  ", 35, 155);
  tft.drawString(String(accumulatedKWh, 3) + "  ", 160, 155);

  // 温度 / 继电器
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(temperature, 1) + "C  ", 55, 190);

  for (int i = 0; i < 5; i++) {
    if (firstRun || relayState[i] != lastRelayState[i]) {
      drawRelayIcon(165 + i * 14, 190, relayState[i]);
      lastRelayState[i] = relayState[i];
    }
  }

  // 电池类型
  String typeStr[] = {"LFP", "NCM", "Pb"};
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(typeStr[batteryType] + " " + String(batteryCells) + "S  ", 45, 220);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(fullVoltage, 1) + "V  ", 155, 220);
}

// ==================== 图形化辅助函数 ====================
void drawBatteryIcon(int x, int y, int w, int h, int percent) {
  // 清除旧图标区域
  tft.fillRect(x, y, w + 4, h, TFT_BLACK);
  tft.drawRect(x, y, w, h, TFT_WHITE);
  tft.fillRect(x + w, y + h / 4, 3, h / 2, TFT_WHITE);
  int fillW = map(percent, 0, 100, 0, w - 2);
  uint16_t color = (percent > 60) ? TFT_GREEN : ((percent > 20) ? TFT_YELLOW : TFT_RED);
  if (fillW > 0) tft.fillRect(x + 1, y + 1, fillW, h - 2, color);
}

// 右侧大电池图标（竖向）
void drawBatteryIconBig(int x, int y, int w, int h, int percent) {
  // 清除旧图标区域
  tft.fillRect(x - 2, y - 5, w + 4, h + 10, TFT_BLACK);
  // 电池顶部凸起（正极）
  int capW = w / 2;
  int capH = 4;
  tft.fillRect(x + (w - capW) / 2, y - capH, capW, capH, TFT_WHITE);
  // 电池外框
  tft.drawRect(x, y, w, h, TFT_WHITE);
  // 竖向填充（从底部向上）
  int fillH = map(percent, 0, 100, 0, h - 4);
  uint16_t color = (percent > 60) ? TFT_GREEN : ((percent > 20) ? TFT_YELLOW : TFT_RED);
  if (fillH > 0) {
    tft.fillRect(x + 2, y + h - fillH - 2, w - 4, fillH, color);
  }
  // 中央百分比文字
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  String pct = String(percent, 0) + "%";
  int tw = tft.textWidth(pct);
  int th = 16;
  tft.drawString(pct, x + (w - tw) / 2, y + (h - th) / 2);
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
  NUM_POWER.print(power);
  NUM_KWH.print(accumulatedKWh);
  NUM_VOLT.print(busVoltage);
  NUM_BATTERY.print(batteryPercent);
  NUM_DAH.print(accumulatedAH);
  NUM_DWH.print(dailyCost);
  NUM_CURRENT.print(current);
  NUM_BVM.print(fullVoltage);
  NUM_BAH.print(batteryCapacity);
  static int chartCounter = 0;
  chartCounter++;
  // 图表功能暂未实现
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

  if (lastMenu == HIGH && menu == LOW) {
    if (!showConfigScreen) {
      showConfigScreen = true;
      Serial.println("进入配置界面");
    } else {
      showConfigScreen = false;
      Serial.println("退出配置界面");
    }
  }

  if (showConfigScreen && ((lastLeft == HIGH && left == LOW) || (lastRight == HIGH && right == LOW))) {
    showConfigScreen = false;
    Serial.println("退出配置界面");
  }

  if (!showConfigScreen) {
    if (lastLeft == HIGH && left == LOW) Serial.println("LEFT按键");
    if (lastRight == HIGH && right == LOW) Serial.println("RIGHT按键");
  }

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
        } else if (cmd == "AT+UPDATE") {
          handleATUpdate();
        } else if (cmd.startsWith("AT+REG ")) {
          handleATReg(cmd.substring(7).c_str());
        } else if (cmd.startsWith("AT+WIFI ")) {
          handleATWifiConnect(cmd.substring(8).c_str());
        } else if (cmd.startsWith("AT+WIFI.connect ")) {
          handleATWifiConnect(cmd.substring(16).c_str());
        } else if (cmd.startsWith("AT+INAV ")) {
          handleATInav(cmd.substring(8).c_str());
        } else if (cmd.startsWith("AT+INAC ")) {
          handleATInac(cmd.substring(8).c_str());
        } else if (cmd.startsWith("AT+ONAV ")) {
          handleATOnav(cmd.substring(8).c_str());
        } else if (cmd.startsWith("AT+ONAC ")) {
          handleATOnac(cmd.substring(8).c_str());
        } else if (cmd.startsWith("AT+DD")) {
          handleATDD(cmd.length() > 5 ? cmd.substring(5).c_str() : "");
        } else if (cmd == "AT+RESTORE") {
          handleATRestore();
        } else if (cmd == "AT+WIFI.STATUS") {
          handleATWifiStatus();
        } else if (cmd == "AT+WIFI.SAVED") {
          handleATWifiSaved();
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
  Serial.println("AT+RST             - 硬件重启设备");
  Serial.println("AT+UPDATE          - 在线升级固件");
  Serial.println("AT+REG 激活码      - 产品激活");
  Serial.println("AT+WIFI SSID 密码   - 连接WiFi");
  Serial.println("AT+WIFI.STATUS     - 查看WiFi连接状态详情");
  Serial.println("AT+WIFI.SAVED      - 查看已保存的WiFi名称和密码");
  Serial.println("AT+INAV 电压值     - 输入电压校验");
  Serial.println("AT+INAC 电流值     - 输入电流校验");
  Serial.println("AT+ONAV 电压值     - 输出电压校验");
  Serial.println("AT+ONAC 电流值     - 输出电流校准");
  Serial.println("AT+DD              - 显示/修改点灯科技密钥");
  Serial.println("AT+RESTORE         - 恢复出厂设置 (清除数据，不含激活状态)");
  Serial.println("====================================================\n");
  Serial.printf("当前状态: 激活=%s, WiFi=%s\n", activated ? "已激活" : "未激活", WiFi.status() == WL_CONNECTED ? "已连接" : "未连接");
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
    timeSynced = false;
    lastTimeSync = millis();
    Serial.printf("\n✅ WiFi连接成功！IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n❌ WiFi连接失败，请检查SSID和密码");
  }
}

void handleATInav(const char *param) {
  float actualVoltage = String(param).toFloat();
  if (actualVoltage <= 0) { Serial.println("❌ 无效电压值"); return; }
  voltageCalibOffset = actualVoltage - (ina226.getBusVoltage() * VBUS_DIVIDER_RATIO);
  saveCalibration();
  Serial.printf("✅ 电压校准完成！偏移: %.3fV, 校准后: %.3fV\n", voltageCalibOffset, ina226.getBusVoltage() * VBUS_DIVIDER_RATIO + voltageCalibOffset);
}

void handleATInac(const char *param) {
  float actualCurrent = String(param).toFloat();
  if (actualCurrent < 0) { Serial.println("❌ 无效电流值"); return; }
  currentCalibOffset = actualCurrent - ina226.getCurrent();
  saveCalibration();
  Serial.printf("✅ 输入电流校准完成！偏移: %.3fA, 校准后: %.3fA\n", currentCalibOffset, ina226.getCurrent() + currentCalibOffset);
}

void handleATOnav(const char *param) {
  float actualVoltage = String(param).toFloat();
  if (actualVoltage <= 0) { Serial.println("❌ 无效电压值"); return; }
  voltageCalibOffset = actualVoltage - (ina226.getBusVoltage() * VBUS_DIVIDER_RATIO);
  saveCalibration();
  Serial.printf("✅ 输出电压校准完成！偏移: %.3fV, 校准后: %.3fV\n", voltageCalibOffset, ina226.getBusVoltage() * VBUS_DIVIDER_RATIO + voltageCalibOffset);
}

void handleATOnac(const char *param) {
  float actualCurrent = String(param).toFloat();
  if (actualCurrent < 0) { Serial.println("❌ 无效电流值"); return; }
  currentCalibOffset = actualCurrent - ina226.getCurrent();
  saveCalibration();
  Serial.printf("✅ 输出电流校准完成！偏移: %.3fA, 校准后: %.3fA\n", currentCalibOffset, ina226.getCurrent() + currentCalibOffset);
}

void handleATUpdate() {
  Serial.println("🔄 在线升级启动中...");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2);
  tft.drawString("OTA升级中...", 20, 60);
  Blinker.print("即将进入固件升级模式...");
  delay(1000);
  Blinker.ota();
}

void handleATDD(const char *param) {
  String id = String(param);
  id.trim();
  if (id.length() == 0) {
    Serial.printf("🔑 当前点灯科技密钥: %s\n", auth);
  } else {
    id.toCharArray(auth, sizeof(auth));
    saveBlinkerAuth();
    Serial.printf("✅ 点灯科技密钥已更新: %s\n", auth);
  }
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
  Serial.println("   默认热点管理页面: http://192.168.4.1");
  Serial.println("   默认网页与热点密码: 12345678");
  Serial.println("   注意: 激活状态不受影响");
}

void handleATWifiStatus() {
  Serial.println("\n================== WiFi 连接状态 ==================");
  wl_status_t status = WiFi.status();
  
  Serial.print("连接状态: ");
  switch (status) {
    case WL_CONNECTED:    Serial.println("✅ 已连接"); break;
    case WL_NO_SSID_AVAIL: Serial.println("❌ 找不到该WiFi"); break;
    case WL_CONNECT_FAILED: Serial.println("❌ 连接失败 (密码错误)"); break;
    case WL_IDLE_STATUS:  Serial.println("⏳ 正在连接..."); break;
    case WL_DISCONNECTED: Serial.println("⛔ 未连接"); break;
    default:              Serial.printf("未知状态(%d)\n", status);
  }
  
  if (status == WL_CONNECTED) {
    Serial.printf("   SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("   IP地址: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("   子网掩码: %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("   网关: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("   DNS服务器: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("   MAC地址: %s\n", WiFi.macAddress().c_str());
    Serial.printf("   信号强度(RSSI): %d dBm\n", WiFi.RSSI());
    if (WiFi.RSSI() >= -50)      Serial.println("   信号质量: ★★★★★ (极好)");
    else if (WiFi.RSSI() >= -60) Serial.println("   信号质量: ★★★★☆ (良好)");
    else if (WiFi.RSSI() >= -70) Serial.println("   信号质量: ★★★☆☆ (一般)");
    else if (WiFi.RSSI() >= -80) Serial.println("   信号质量: ★★☆☆☆ (较差)");
    else                         Serial.println("   信号质量: ★☆☆☆☆ (极差)");
    Serial.printf("   连接通道: %d\n", WiFi.channel());
    Serial.printf("   BSSID: %s\n", WiFi.BSSIDstr().c_str());
  } else if (configMode) {
    Serial.printf("   热点SSID: ESP32-库仑计\n");
    Serial.printf("   热点IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("   MAC地址: %s\n", WiFi.softAPmacAddress().c_str());
    Serial.printf("   已连接客户端数: %d\n", WiFi.softAPgetStationNum());
  }
  
  // 显示WiFi模式
  wifi_mode_t mode = WiFi.getMode();
  Serial.print("WiFi模式: ");
  switch (mode) {
    case WIFI_MODE_NULL:   Serial.println("无"); break;
    case WIFI_MODE_STA:    Serial.println("Station(客户端)"); break;
    case WIFI_MODE_AP:     Serial.println("AP(热点模式)"); break;
    case WIFI_MODE_APSTA:  Serial.println("AP+Station(混合模式)"); break;
  }
  
  Serial.println("==================================================\n");
}

void handleATWifiSaved() {
  Serial.println("\n================== 已保存的WiFi信息 ==================");
  preferences.begin("coulomb", true);
  String savedSSID = preferences.getString("wifiSSID", "");
  String savedPass = preferences.getString("wifiPass", "");
  preferences.end();
  
  if (savedSSID.length() > 0) {
    Serial.printf("   WiFi名称: %s\n", savedSSID.c_str());
    Serial.printf("   WiFi密码: %s\n", savedPass.c_str());
    Serial.println("\n   提示: 输入 AT+WIFI.connect SSID PASSWORD 可修改WiFi配置");
    Serial.println("   提示: 输入 AT+RESTORE 可清除WiFi配置并进入配网模式");
  } else {
    Serial.println("   ⚠️ 未保存任何WiFi配置");
    Serial.println("   可通过以下方式配置WiFi:");
    Serial.println("   1. 串口输入: AT+WIFI.connect WIFI名称 WIFI密码");
    Serial.println("   2. 配网模式: 重启设备进入热点配置模式");
  }
  Serial.println("======================================================\n");
}

// ==================== 校准值存储 ====================
void saveCalibration() {
  preferences.begin("coulomb", false);
  preferences.putFloat("voltCalib", voltageCalibOffset);
  preferences.putFloat("curCalib", currentCalibOffset);
  preferences.end();
  Serial.println("✅ 校准值已保存");
}

void syncNetworkTime() {
  if (!wifiConnected) {
    Serial.println("⚠️ 未连接 WiFi，无法同步时间");
    return;
  }

  configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org", "time1.google.com");
  Serial.println("🔄 开始同步网络时间...");

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 5000)) {
    char timeString[64];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("✅ 时间同步成功: %s\n", timeString);
    timeSynced = true;
  } else {
    Serial.println("❌ 时间同步失败，稍后重试");
  }
}

void updateWifiState() {
  bool connectedNow = (WiFi.status() == WL_CONNECTED);
  if (connectedNow && !wifiConnected) {
    wifiConnected = true;
    timeSynced = false;
    lastTimeSync = 0;
    Serial.println("📶 WiFi 已连接，准备同步时间");
  } else if (!connectedNow && wifiConnected) {
    wifiConnected = false;
    timeSynced = false;
    Serial.println("⚠️ WiFi 已断开");
  }
}

String getTimeString() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 1000)) {
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
    return String(buf);
  }
  return "--:--:--";
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

void saveBlinkerAuth() {
  preferences.begin("coulomb", false);
  preferences.putString("blinkerAuth", auth);
  preferences.end();
}

void loadBlinkerAuth() {
  preferences.begin("coulomb", true);
  String savedAuth = preferences.getString("blinkerAuth", "");
  preferences.end();
  if (savedAuth.length() > 0) {
    savedAuth.toCharArray(auth, sizeof(auth));
    Serial.println("✅ 已加载保存的点灯科技密钥");
  } else {
    Serial.println("⚠️ 未设置点灯科技密钥，请使用 AT+DD ID 设置");
  }
}

String getDeviceId() {
  return WiFi.macAddress();
}

String getSavedWifiSSID() {
  preferences.begin("coulomb", true);
  String savedSSID = preferences.getString("wifiSSID", "未配置");
  preferences.end();
  return savedSSID;
}

void drawConfigScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("设备信息", 60, 10);
  tft.drawFastHLine(10, 34, 220, TFT_WHITE);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("设备号:", 10, 50);
  tft.drawString(getDeviceId(), 90, 50);

  tft.drawString("Blinker:", 10, 70);
  tft.drawString(auth[0] ? String(auth) : "未配置", 90, 70);

  tft.drawString("WiFi:", 10, 90);
  tft.drawString(getSavedWifiSSID(), 90, 90);

  tft.drawString("LAN IP:", 10, 110);
  tft.drawString(WiFi.localIP().toString(), 90, 110);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("MENU: 返回", 10, 140);
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
  for (int i = 0; i < 5; i++) { char key[8]; sprintf(key, "relay%d", i); relayState[i] = preferences.getBool(key, false); setRelay(i, relayState[i]); }
  preferences.end();
  Serial.println("✅ 数据已加载");
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    timeSynced = false;
    lastTimeSync = 0;
  }

  BTN_OUT1.print(relayState[0] ? "on" : "off");
  BTN_OUT2.print(relayState[1] ? "on" : "off");
  BTN_OUT3.print(relayState[2] ? "on" : "off");
  BTN_OUT4.print(relayState[3] ? "on" : "off");
  BTN_OUT5.print(relayState[4] ? "on" : "off");
  if (batteryType == 0) BTN_BT0.print("on");
  else if (batteryType == 1) BTN_BT1.print("on");
  else BTN_BT2.print("on");
  
  // WiFi 由 Blinker 管理，这里只检查连接状态
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("📶 WiFi 已连接！IP: %s\n", WiFi.localIP().toString().c_str());
  }
}
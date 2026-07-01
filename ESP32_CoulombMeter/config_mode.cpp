#include "config_mode.h"
#include <TFT_eSPI.h>
#include <Preferences.h>

namespace {
WebServer *gServer = nullptr;
DNSServer *gDnsServer = nullptr;
TFT_eSPI *gDisplay = nullptr;
Preferences *gPreferences = nullptr;

void showConfigGuideScreen() {
  if (!gDisplay) return;
  gDisplay->fillScreen(TFT_BLACK);
  gDisplay->setTextColor(TFT_WHITE, TFT_NAVY);
  gDisplay->setTextSize(1);
  gDisplay->fillRect(0, 0, 240, 22, TFT_NAVY);
  gDisplay->drawString("ESP32 库仑计 - 配网模式", 3, 4);
  gDisplay->setTextColor(TFT_YELLOW, TFT_BLACK);
  gDisplay->setTextSize(2);
  gDisplay->drawString("WiFi配置", 70, 35);
  gDisplay->setTextColor(TFT_WHITE, TFT_BLACK);
  gDisplay->setTextSize(1);
  gDisplay->drawString("1.连接热点: ESP32-库仑计", 10, 70);
  gDisplay->drawString("  密码: 12345678", 10, 85);
  gDisplay->drawString("2.浏览器访问:", 10, 110);
  gDisplay->setTextColor(TFT_CYAN, TFT_BLACK);
  gDisplay->setTextSize(2);
  gDisplay->drawString("192.168.4.1", 40, 130);
  gDisplay->setTextColor(TFT_WHITE, TFT_BLACK);
  gDisplay->setTextSize(1);
  gDisplay->drawString("3.输入WiFi名称和密码", 10, 160);
  gDisplay->drawString("4.点击连接", 10, 175);
}

void showConfigStatusScreen(const String &title, const String &message) {
  if (!gDisplay) return;
  gDisplay->fillScreen(TFT_BLACK);
  gDisplay->setTextColor(TFT_WHITE, TFT_NAVY);
  gDisplay->setTextSize(1);
  gDisplay->fillRect(0, 0, 240, 22, TFT_NAVY);
  gDisplay->drawString("ESP32 库仑计", 3, 4);
  gDisplay->setTextColor(TFT_YELLOW, TFT_BLACK);
  gDisplay->setTextSize(2);
  gDisplay->drawString(title, 30, 40);
  gDisplay->setTextColor(TFT_CYAN, TFT_BLACK);
  gDisplay->setTextSize(1);
  gDisplay->drawString(message, 15, 90);
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
  gServer->send(200, "text/html", html);
}

void handleConfigSave() {
  String ssid = gServer->arg("ssid");
  String pass = gServer->arg("pass");

  if (ssid.length() == 0) {
    gServer->send(400, "text/html", "<html><body><h3>WiFi名称不能为空！</h3><a href='/'>返回</a></body></html>");
    return;
  }

  gPreferences->begin("coulomb", false);
  gPreferences->putString("wifiSSID", ssid);
  gPreferences->putString("wifiPass", pass);
  gPreferences->end();

  showConfigStatusScreen("配置成功", "正在保存配置并重启...");

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
  gServer->send(200, "text/html", html);

  Serial.printf("📶 收到WiFi配置: %s\n", ssid.c_str());
  delay(1000);
  Serial.println("🔄 正在重启...");
  ESP.restart();
}

void handleConfigNotFound() {
  gServer->sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
  gServer->send(302, "text/plain", "");
}
}  // namespace

void setConfigModeRefs(WebServer *webServer, DNSServer *dns, TFT_eSPI *display) {
  gServer = webServer;
  gDnsServer = dns;
  gDisplay = display;
}

void setConfigModePreferences(Preferences *prefs) {
  gPreferences = prefs;
}

void startConfigMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-库仑计", "12345678");
  Serial.printf("📶 配网热点已启动: ESP32-库仑计 (密码: 12345678)\n");
  Serial.printf("   配网页面: http://%s\n", WiFi.softAPIP().toString().c_str());

  gDnsServer->start(53, "*", WiFi.softAPIP());
  gServer->on("/", handleConfigRoot);
  gServer->on("/save", HTTP_POST, handleConfigSave);
  gServer->onNotFound(handleConfigNotFound);
  gServer->begin();

  showConfigGuideScreen();
}

void handleConfigModeLoop() {
  gDnsServer->processNextRequest();
  gServer->handleClient();
}

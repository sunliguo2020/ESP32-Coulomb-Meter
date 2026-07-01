#ifndef CONFIG_MODE_H
#define CONFIG_MODE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

void startConfigMode();
void handleConfigModeLoop();
void setConfigModeRefs(WebServer *webServer, DNSServer *dns, TFT_eSPI *display);
void setConfigModePreferences(Preferences *prefs);

#endif

#include "i2c_scanner.h"

void scanI2CDevices() {
  Serial.println("🔎 正在扫描 I2C 总线...");
  uint8_t candidateAddrs[] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F};
  bool foundIna226Like = false;

  for (uint8_t i = 0; i < sizeof(candidateAddrs) / sizeof(candidateAddrs[0]); i++) {
    Wire.beginTransmission(candidateAddrs[i]);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("   发现 I2C 设备: 0x%02X\n", candidateAddrs[i]);
      foundIna226Like = true;
      if (candidateAddrs[i] == 0x44) {
        Serial.println("   0x44 为默认 INA226 地址");
      }
    }
  }

  if (!foundIna226Like) {
    Serial.println("   未发现 0x40~0x4F 范围内的 INA226 兼容地址");
  }
}

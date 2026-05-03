#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include <LittleFS.h>

class StorageManager {
public:
    static void init();
    static bool hasWifiConfig();
    static bool hasImage();
    static String getSSID();
    static String getPassword();
    static void saveWifi(const String& ssid, const String& password);
    static void clearWifi();
    static void clearImage();
    static uint8_t* readImageToPSRAM(size_t* outLen);
    static bool shouldResetToDefaults();
    static void setResetToDefaults();

private:
    static Preferences wifiPrefs;
    static const char* NAMESPACE_WIFI;
};
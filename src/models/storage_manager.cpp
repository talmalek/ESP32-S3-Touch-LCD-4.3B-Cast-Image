#include "storage_manager.h"

Preferences StorageManager::wifiPrefs;
const char* StorageManager::NAMESPACE_WIFI = "wifi_conf";

void StorageManager::init() {
    wifiPrefs.begin(NAMESPACE_WIFI, false);
    
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
    }
}

bool StorageManager::shouldResetToDefaults() {
    return wifiPrefs.getBool("reset", false);
}

void StorageManager::setResetToDefaults() {
    wifiPrefs.putBool("reset", true);
    Serial.println("Set factory reset flag");
}

bool StorageManager::hasWifiConfig() {
    if (shouldResetToDefaults()) {
        Serial.println("Factory reset flag set - forcing AP mode");
        return false;
    }
    String ssid = wifiPrefs.getString("ssid", "");
    return ssid.length() > 0;
}

bool StorageManager::hasImage() {
    return LittleFS.exists("/image.bin");
}

String StorageManager::getSSID() {
    return wifiPrefs.getString("ssid", "");
}

String StorageManager::getPassword() {
    return wifiPrefs.getString("pass", "");
}

void StorageManager::saveWifi(const String& ssid, const String& password) {
    wifiPrefs.putBool("reset", false);
    wifiPrefs.putString("ssid", ssid);
    wifiPrefs.putString("pass", password);
    Serial.println("WiFi saved: " + ssid);
}

void StorageManager::clearWifi() {
    Serial.println("=== FACTORY RESET: Clearing WiFi ===");
    wifiPrefs.remove("ssid");
    wifiPrefs.remove("pass");
    wifiPrefs.putBool("reset", true);
    Serial.println("Factory reset flag set to true");
}

void StorageManager::clearImage() {
    if (LittleFS.exists("/image.bin")) {
        LittleFS.remove("/image.bin");
    }
}

uint8_t* StorageManager::readImageToPSRAM(size_t* outLen) {
    // Remount to get a fresh view of the filesystem after an upload from another task
    LittleFS.end();
    if (!LittleFS.begin(true)) {
        return nullptr;
    }
    
    if (!LittleFS.exists("/image.bin")) {
        return nullptr;
    }
    
    File file = LittleFS.open("/image.bin", FILE_READ);
    if (!file) {
        return nullptr;
    }
    
    size_t size = file.size();
    if (outLen) *outLen = size;
    
    uint8_t* buffer = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (!buffer) {
        file.close();
        return nullptr;
    }
    
    file.read(buffer, size);
    file.close();
    
    return buffer;
}

bool StorageManager::getClockEnabled() {
    return wifiPrefs.getBool("clock_on", false);
}

void StorageManager::saveClockEnabled(bool enabled) {
    wifiPrefs.putBool("clock_on", enabled);
}
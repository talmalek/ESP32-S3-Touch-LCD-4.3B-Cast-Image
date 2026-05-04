#include "esp_heap_caps.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_display_panel.hpp>
#include <esp_system.h>

#include "lvgl_v8_port.h"
#include <lvgl.h>

#include "storage_manager.h"
#include "splash.h"
#include "frame_ui.h"
#include "web_server.h"

#include "esp_panel_board_custom_conf.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static bool wifi_connected = false;
Board *board = nullptr;

LCD* getLcd() {
    if (board) return board->getLCD();
    return nullptr;
}

void setup() {
    // Note: RST button reset will restart everything fresh
    
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Picture Frame ===\n");
    Serial.println("1. Storage...");
    StorageManager::init(); // Initialize LittleFS early
    
    Serial.println("2. Board...");
    board = new Board();
    board->init();
    board->begin();
    
    // Force backlight OFF as soon as possible after begin
    if (board->getBacklight()) {
        board->getBacklight()->off();
    }
    
    Serial.println("3. LVGL...");
    lvgl_port_init(board->getLCD(), board->getTouch());
    
    Serial.println("4. Splash...");
    lvgl_port_lock(-1);
    SplashScreen::create();
    lv_refr_now(nullptr); // Ensure first frame is rendered
    lvgl_port_unlock();
    
    auto backlight = board->getBacklight();
    if (backlight) {
        backlight->on();
        Serial.println("Backlight ON (Splash Ready)");
    }
    
    lvgl_port_lock(-1);
    SplashScreen::update_subtitle("Connecting to WiFi...");
    lvgl_port_unlock();
    
    Serial.println("5. WiFi...");
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
    
    // Check for saved WiFi - require manual reset to clear
    bool hasStoredWiFi = StorageManager::hasWifiConfig();
    Serial.println("Stored WiFi: " + String(hasStoredWiFi ? "YES" : "NO"));
    
    if (hasStoredWiFi) {
        String ssid = StorageManager::getSSID();
        String pass = StorageManager::getPassword();
        if (ssid.length() > 0) {
            Serial.println("Trying: " + ssid);
            WiFi.disconnect(true);
            delay(100);
            WiFi.begin(ssid.c_str(), pass.c_str());
            for (int i = 0; i < 25; i++) {
                delay(200);
                if (WiFi.status() == WL_CONNECTED) {
                    wifi_connected = true;
                    break;
                }
            }
        }
    }
    
    if (!wifi_connected) {
        Serial.println("6. WiFiManager...");
        lvgl_port_lock(-1);
        SplashScreen::update_subtitle("Configure WiFi Settings");
        lvgl_port_unlock();
        
        // Force reset WiFi and WiFiManager storage
        WiFi.disconnect(true);
        delay(200);
        WiFi.mode(WIFI_OFF);
        delay(100);
        WiFi.mode(WIFI_STA);
        
        WiFiManager wm;
        wm.resetSettings();
        delay(100);
        
        // This will block until a connection is made or it times out
        if (!wm.autoConnect("PictureFrame", "12345678")) {
            Serial.println("Failed to connect or timeout");
            // Restart and try again - will show splash
            ESP.restart();
        }
        
        // If we get here, we are connected!
        wifi_connected = true;
        // Save the new config since WiFiManager succeeded
        StorageManager::saveWifi(WiFi.SSID(), WiFi.psk());
    }
    
    Serial.println("WiFi OK: " + WiFi.localIP().toString());
    
    delay(500);
    
    Serial.println("6. Web server...");
    ImageWebServer::start();
    
    Serial.println("7. UI...");
    delay(100);
    yield();
    
    lvgl_port_lock(-1);
    delay(100);
    yield();
    
    FrameUI::create();
    FrameUI::setServerIP(WiFi.localIP().toString().c_str());
    SplashScreen::destroy();
    delay(100);
    yield();
    lvgl_port_unlock();

    
    Serial.println("=== Ready ===");
}

void loop() {
    ImageWebServer::loop();
    FrameUI::loop();
}
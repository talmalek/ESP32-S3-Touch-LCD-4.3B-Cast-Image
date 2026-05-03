#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>
#include <LittleFS.h>
#include "../src/lvgl_v8_port.h"

static inline void ensureDisplayReady() {
    for (int i = 0; i < 10; i++) {
        yield();
        delay(5);
    }
    if (lvgl_port_lock(-1)) {
        lv_refr_now(nullptr);
        lvgl_port_unlock();
    }
    delay(5);
    yield();
}

static inline void ensureFileReady() {
    for (int i = 0; i < 20; i++) {
        delay(20);
        yield();
    }
    
    if (LittleFS.exists("/image.bin")) {
        File f = LittleFS.open("/image.bin", FILE_READ);
        if (f) {
            size_t size = f.size();
            f.close();
            Serial.printf("File ready: %d bytes\n", size);
            delay(50);
            yield();
        }
    }
}
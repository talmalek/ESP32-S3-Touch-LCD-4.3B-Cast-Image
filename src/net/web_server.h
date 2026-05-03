#pragma once

#include <WiFi.h>
#include <WebServer.h>

class ImageWebServer {
public:
    static void start();
    static void stop();
    static void loop();
    static bool isRunning();
    static const char* getServerIP();
    static uint16_t getServerPort();
};
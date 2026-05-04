#pragma once

#include <lvgl.h>
#include <ESP.h>

class FrameUI {
public:
    static void create();
    static void showImage();
    static void clearImage();
    static bool hasImage();
    static void showWaitingScreen();
    static void showUploading();
    static void setServerIP(const char* ip);
    static void loadStoredImage();
    static void showSettingsMenu();
    static void hideSettingsMenu();
    static bool isSettingsMenuVisible();
    static void onClearImage();
    static void onRestoreDefaults();
    static void loop();
    static void queueImageLoad();
    
    // Clock
    static void toggleClock();
    static bool isClockEnabled();

private:
    static bool clock_enabled;
};
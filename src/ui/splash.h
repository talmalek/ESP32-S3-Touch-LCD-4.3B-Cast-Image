#pragma once

#include <lvgl.h>

class SplashScreen {
public:
    static void create();
    static void update_subtitle(const char* text);
    static void destroy();
    static bool isVisible();
};
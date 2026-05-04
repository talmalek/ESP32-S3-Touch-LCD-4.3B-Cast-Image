#pragma once
#include <lvgl.h>

class FlipClock {
public:
    static void create(lv_obj_t* parent);
    static void update_time();
};

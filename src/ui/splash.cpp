#include "splash.h"
#include "display_utils.h"

static lv_obj_t* splash_container = nullptr;
static lv_obj_t* status_label = nullptr;

void SplashScreen::create() {
    ensureDisplayReady();
    
    splash_container = lv_obj_create(NULL);
    lv_obj_set_size(splash_container, 800, 480);
    lv_obj_set_pos(splash_container, 0, 0);
    lv_obj_set_style_bg_color(splash_container, lv_color_hex(0x001122), 0);
    lv_obj_set_style_pad_all(splash_container, 0, 0);

    lv_obj_t* title = lv_label_create(splash_container);
    lv_label_set_text(title, "Digital Picture Frame");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4ECDC4), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 80);

    status_label = lv_label_create(splash_container);
    lv_label_set_text(status_label, "Waiting for WiFi...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_24, 0);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t* hint = lv_label_create(splash_container);
    lv_label_set_text(hint, "Network: PictureFrame");
    lv_obj_set_style_text_color(hint, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_20, 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t* pass = lv_label_create(splash_container);
    lv_label_set_text(pass, "Password: 12345678");
    lv_obj_set_style_text_color(pass, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font(pass, &lv_font_montserrat_20, 0);
    lv_obj_align(pass, LV_ALIGN_CENTER, 0, 55);

    lv_obj_t* footer = lv_label_create(splash_container);
    lv_label_set_text(footer, "Connect to WiFi, then open browser to");
    lv_obj_set_style_text_color(footer, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_18, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -60);

    lv_obj_t* footer2 = lv_label_create(splash_container);
    lv_label_set_text(footer2, "upload your image");
    lv_obj_set_style_text_color(footer2, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(footer2, &lv_font_montserrat_18, 0);
    lv_obj_align(footer2, LV_ALIGN_BOTTOM_MID, 0, -30);

    ensureDisplayReady();
    lv_scr_load(splash_container);
    ensureDisplayReady();
}

void SplashScreen::update_subtitle(const char* text) {
    ensureDisplayReady();
    if (status_label) {
        lv_label_set_text(status_label, text);
    }
    ensureDisplayReady();
}

void SplashScreen::destroy() {
    if (splash_container) {
        lv_obj_del(splash_container);
        splash_container = nullptr;
        status_label = nullptr;
    }
}

bool SplashScreen::isVisible() {
    return splash_container != nullptr;
}
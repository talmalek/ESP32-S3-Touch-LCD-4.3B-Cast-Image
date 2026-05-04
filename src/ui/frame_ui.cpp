#include "frame_ui.h"
#include "storage_manager.h"
#include "../include/display_utils.h"
#include "../lvgl_v8_port.h"
#include <ESP.h>
#include <lvgl.h>

extern esp_panel::drivers::LCD* getLcd();

static lv_obj_t* main_cont = nullptr;
static lv_obj_t* info_label = nullptr;
static lv_obj_t* ip_label = nullptr;
static lv_obj_t* settings_menu = nullptr;
static lv_obj_t* bg_img = nullptr;
static bool settings_open = false;
static char server_ip[16] = "";
static uint8_t* img_buffer = nullptr;
static bool imageLoadQueued = false;

static lv_img_dsc_t img_dsc = {
    .header = {
        .cf = LV_IMG_CF_TRUE_COLOR,
        .always_zero = 0,
        .reserved = 0,
        .w = 800,
        .h = 480,
    },
    .data_size = 800 * 480 * 2,
.data = nullptr,
    };

void FrameUI::create() {
    ensureDisplayReady();
    ensureFileReady();
    
    main_cont = lv_obj_create(NULL);
    lv_obj_set_size(main_cont, 800, 480);
    lv_obj_set_pos(main_cont, 0, 0);
    lv_obj_set_style_bg_color(main_cont, lv_color_hex(0x0F0F1A), 0);

    info_label = lv_label_create(main_cont);
    lv_label_set_text(info_label, "Picture Frame");
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(info_label, lv_color_hex(0x4ECDC4), 0);
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_24, 0);

    ip_label = lv_label_create(main_cont);
    lv_label_set_text(ip_label, "Open browser to this IP");
    lv_obj_set_pos(ip_label, 50, 80);
    lv_obj_set_style_text_color(ip_label, lv_color_hex(0x888890), 0);
    lv_obj_set_style_text_font(ip_label, LV_FONT_DEFAULT, 0);

    bg_img = lv_img_create(main_cont);
    lv_obj_set_pos(bg_img, 0, 0);
    lv_obj_set_size(bg_img, 800, 480);
    lv_obj_add_flag(bg_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(main_cont, LV_OBJ_FLAG_CLICKABLE);

    // Settings Menu - Premium Look
    settings_menu = lv_obj_create(main_cont);
    lv_obj_set_size(settings_menu, 460, 400);
    lv_obj_align(settings_menu, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(settings_menu, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_bg_opa(settings_menu, 245, 0);
    lv_obj_set_style_radius(settings_menu, 32, 0);
    lv_obj_set_style_border_color(settings_menu, lv_color_hex(0x4ecdc4), 0);
    lv_obj_set_style_border_width(settings_menu, 2, 0);
    lv_obj_set_style_pad_all(settings_menu, 0, 0);
    lv_obj_add_flag(settings_menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(settings_menu, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* menu_title = lv_label_create(settings_menu);
    lv_label_set_text(menu_title, "Settings");
    lv_obj_set_style_text_color(menu_title, lv_color_hex(0x4ecdc4), 0);
    lv_obj_set_style_text_font(menu_title, &lv_font_montserrat_32, 0);
    lv_obj_align(menu_title, LV_ALIGN_TOP_MID, 0, 25);

    // Flex container for buttons
    lv_obj_t* btn_cont = lv_obj_create(settings_menu);
    lv_obj_set_size(btn_cont, 400, 300);
    lv_obj_align(btn_cont, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_opa(btn_cont, 0, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(btn_cont, 20, 0);

    // 1. Clear Image Button
    lv_obj_t* clear_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(clear_btn, 320, 60);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0x4ecdc4), 0);
    lv_obj_set_style_radius(clear_btn, 16, 0);
    lv_obj_add_event_cb(clear_btn, [](lv_event_t* e) {
        FrameUI::clearImage();
        FrameUI::hideSettingsMenu();
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* clear_lbl = lv_label_create(clear_btn);
    lv_label_set_text(clear_lbl, "Clear Image");
    lv_obj_set_style_text_color(clear_lbl, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_text_font(clear_lbl, &lv_font_montserrat_24, 0);
    lv_obj_align(clear_lbl, LV_ALIGN_CENTER, 0, 0);

    // 2. Restore Defaults Button
    lv_obj_t* reset_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(reset_btn, 320, 60);
    lv_obj_set_style_bg_color(reset_btn, lv_color_hex(0xff6b6b), 0);
    lv_obj_set_style_radius(reset_btn, 16, 0);
    lv_obj_add_event_cb(reset_btn, [](lv_event_t* e) {
        FrameUI::onRestoreDefaults();
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* reset_lbl = lv_label_create(reset_btn);
    lv_label_set_text(reset_lbl, "Factory Reset");
    lv_obj_set_style_text_color(reset_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(reset_lbl, &lv_font_montserrat_24, 0);
    lv_obj_align(reset_lbl, LV_ALIGN_CENTER, 0, 0);

    // 3. Close Button
    lv_obj_t* close_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(close_btn, 320, 60);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x3d3d5c), 0);
    lv_obj_set_style_radius(close_btn, 16, 0);
    lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
        FrameUI::hideSettingsMenu();
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, "Back to Frame");
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(close_lbl, LV_ALIGN_CENTER, 0, 0);

    // Toggle settings event on screen click
    lv_event_cb_t toggle_settings = [](lv_event_t* e) {
        lv_obj_t * target = lv_event_get_target(e);
        if (target != main_cont && target != bg_img) return;
        
        if (settings_open) {
            FrameUI::hideSettingsMenu();
        } else {
            FrameUI::showSettingsMenu();
            lv_obj_move_foreground(settings_menu);
        }
    };
    lv_obj_add_event_cb(main_cont, toggle_settings, LV_EVENT_CLICKED, nullptr);

    lv_scr_load(main_cont);
    
    loadStoredImage();
}

void FrameUI::showUploading() {
    lvgl_port_lock(-1);
    if (bg_img) lv_obj_add_flag(bg_img, LV_OBJ_FLAG_HIDDEN);
    if (settings_menu) lv_obj_add_flag(settings_menu, LV_OBJ_FLAG_HIDDEN);
    settings_open = false;
    
    if (info_label) lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    if (ip_label) lv_obj_add_flag(ip_label, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_invalidate(lv_scr_act());
    lvgl_port_unlock();
}

void FrameUI::loadStoredImage() {
    ensureDisplayReady();
    ensureFileReady();
    
    if (img_buffer) {
        heap_caps_free(img_buffer);
        img_buffer = nullptr;
    }
    
    size_t len = 0;
    img_buffer = StorageManager::readImageToPSRAM(&len);
    
    // Images are always 800px wide (fixed by web client crop tool)
    uint32_t width = 800;
    uint32_t height = len / (width * 2); // RGB565 = 2 bytes per pixel
    
    if (img_buffer && height > 0) {
        img_dsc.data = img_buffer;
        img_dsc.header.w = width;
        img_dsc.header.h = height;
        img_dsc.data_size = len;
        
        lv_img_cache_invalidate_src(&img_dsc);
        
        lv_obj_add_flag(bg_img, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(bg_img, &img_dsc);
        lv_obj_set_size(bg_img, width, height);
        lv_obj_set_pos(bg_img, 0, 0);
        lv_obj_clear_flag(bg_img, LV_OBJ_FLAG_HIDDEN);
        
        lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ip_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (img_buffer) heap_caps_free(img_buffer);
        img_buffer = nullptr;
        lv_obj_add_flag(bg_img, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(info_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ip_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void FrameUI::showImage() {
    loadStoredImage();
}

void FrameUI::clearImage() {
    StorageManager::clearImage();
    if (img_buffer) {
        heap_caps_free(img_buffer);
        img_buffer = nullptr;
    }
    lv_obj_add_flag(bg_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ip_label, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(info_label, "Image Cleared");
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_24, 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);
}

bool FrameUI::hasImage() {
    return StorageManager::hasImage();
}

void FrameUI::showWaitingScreen() {
    lv_label_set_text(info_label, "No Image");
}

void FrameUI::setServerIP(const char* ip) {
    strncpy(server_ip, ip, sizeof(server_ip) - 1);
    server_ip[sizeof(server_ip) - 1] = '\0';
    
    char buf[128];
    snprintf(buf, sizeof(buf), "http://%s", ip);
    lv_label_set_text(ip_label, buf);
}

void FrameUI::showSettingsMenu() {
    lv_obj_clear_flag(settings_menu, LV_OBJ_FLAG_HIDDEN);
    settings_open = true;
}

void FrameUI::hideSettingsMenu() {
    lv_obj_add_flag(settings_menu, LV_OBJ_FLAG_HIDDEN);
    settings_open = false;
}

bool FrameUI::isSettingsMenuVisible() { return settings_open; }

void FrameUI::onClearImage() {
    FrameUI::clearImage();
}

void FrameUI::onRestoreDefaults() {
    hideSettingsMenu();
    StorageManager::clearWifi();
    StorageManager::clearImage();
    delay(500);
    ESP.restart();
}

void FrameUI::queueImageLoad() {
    imageLoadQueued = true;
}

void FrameUI::loop() {
    if (!imageLoadQueued) return;
    imageLoadQueued = false;
    
    lvgl_port_lock(-1);
    loadStoredImage();
    lvgl_port_unlock();
}
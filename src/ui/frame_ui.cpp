#include "frame_ui.h"
#include "storage_manager.h"
#include "flip_clock.h"
#include "../include/display_utils.h"
#include "../lvgl_v8_port.h"
#include <ESP.h>
#include <lvgl.h>

extern esp_panel::drivers::LCD* getLcd();
extern esp_panel::board::Board *board;

static lv_obj_t* main_cont = nullptr;
static lv_obj_t* no_image_cont = nullptr;
static lv_obj_t* info_label = nullptr;
static lv_obj_t* ip_label = nullptr;
static lv_obj_t* settings_menu = nullptr;
static lv_obj_t* settings_overlay = nullptr;
static lv_obj_t* clock_cont = nullptr;
static lv_obj_t* clock_btn = nullptr;
static lv_obj_t* clock_toggle_lbl = nullptr;
static lv_obj_t* bg_img = nullptr;
static bool settings_open = false;
static char server_ip[16] = "";
static uint8_t* img_buffer = nullptr;
static bool imageLoadQueued = false;
static uint32_t imageLoadQueuedTime = 0;

bool FrameUI::clock_enabled = false;

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
    clock_enabled = StorageManager::getClockEnabled();
    
    ensureDisplayReady();
    ensureFileReady();
    
    main_cont = lv_obj_create(NULL);
    lv_obj_set_size(main_cont, 800, 480);
    lv_obj_set_pos(main_cont, 0, 0);
    lv_obj_set_style_bg_color(main_cont, lv_color_hex(0x0F0F1A), 0);

    no_image_cont = lv_obj_create(main_cont);
    lv_obj_set_size(no_image_cont, 800, 480);
    lv_obj_set_pos(no_image_cont, 0, 0);
    lv_obj_set_style_bg_opa(no_image_cont, 0, 0);
    lv_obj_set_style_border_width(no_image_cont, 0, 0);
    lv_obj_clear_flag(no_image_cont, LV_OBJ_FLAG_SCROLLABLE);

    info_label = lv_label_create(no_image_cont);
    lv_label_set_text(info_label, "No Image Loaded");
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_color(info_label, lv_color_hex(0x4ECDC4), 0);
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_32, 0);

    ip_label = lv_label_create(no_image_cont);
    lv_label_set_text(ip_label, "Connecting to WiFi...");
    lv_obj_align(ip_label, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_text_color(ip_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ip_label, &lv_font_montserrat_24, 0);

    lv_obj_t* hint_lbl = lv_label_create(no_image_cont);
    lv_label_set_text(hint_lbl, "Open the URL above in your browser to upload an image");
    lv_obj_align(hint_lbl, LV_ALIGN_CENTER, 0, 75);
    lv_obj_set_style_text_color(hint_lbl, lv_color_hex(0x888890), 0);
    lv_obj_set_style_text_font(hint_lbl, &lv_font_montserrat_18, 0);

    bg_img = lv_img_create(main_cont);
    lv_obj_set_pos(bg_img, 0, 0);
    lv_obj_set_size(bg_img, 800, 480);
    lv_obj_add_flag(bg_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(main_cont, LV_OBJ_FLAG_CLICKABLE);

    // Clock Container (Overlay)
    clock_cont = lv_obj_create(main_cont);
    lv_obj_set_size(clock_cont, 800, 480);
    lv_obj_set_pos(clock_cont, 0, 0);
    lv_obj_set_style_bg_opa(clock_cont, 0, 0);
    lv_obj_set_style_border_width(clock_cont, 0, 0);
    if (clock_enabled) {
        lv_obj_clear_flag(clock_cont, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(clock_cont, LV_OBJ_FLAG_HIDDEN);
    }
    FlipClock::create(clock_cont);

    // Settings Overlay (Transparent layer to catch clicks outside menu)
    settings_overlay = lv_obj_create(main_cont);
    lv_obj_set_size(settings_overlay, 800, 480);
    lv_obj_set_pos(settings_overlay, 0, 0);
    lv_obj_set_style_bg_opa(settings_overlay, 100, 0); // Slight dim
    lv_obj_set_style_bg_color(settings_overlay, lv_color_black(), 0);
    lv_obj_add_flag(settings_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(settings_overlay, [](lv_event_t* e) {
        FrameUI::hideSettingsMenu();
    }, LV_EVENT_CLICKED, nullptr);

    // Settings Menu - Premium Look
    settings_menu = lv_obj_create(main_cont);
    lv_obj_set_size(settings_menu, 460, 440);
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

    // Close button (X) for settings
    lv_obj_t* x_btn = lv_btn_create(settings_menu);
    lv_obj_set_size(x_btn, 40, 40);
    lv_obj_align(x_btn, LV_ALIGN_TOP_RIGHT, -15, 15);
    lv_obj_set_style_bg_opa(x_btn, 0, 0);
    lv_obj_set_style_shadow_opa(x_btn, 0, 0);
    lv_obj_add_event_cb(x_btn, [](lv_event_t* e) {
        FrameUI::hideSettingsMenu();
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* x_lbl = lv_label_create(x_btn);
    lv_label_set_text(x_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(x_lbl, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(x_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(x_lbl);

    // Flex container for buttons
    lv_obj_t* btn_cont = lv_obj_create(settings_menu);
    lv_obj_set_size(btn_cont, 400, 340);
    lv_obj_align(btn_cont, LV_ALIGN_TOP_MID, 0, 75);
    lv_obj_set_style_bg_opa(btn_cont, 0, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    lv_obj_set_style_pad_all(btn_cont, 0, 0); // Clear default padding
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(btn_cont, 15, 0); // Tighter spacing to fit all

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

    // 3. Clock Toggle Button
    clock_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(clock_btn, 320, 60);
    lv_obj_set_style_bg_color(clock_btn, clock_enabled ? lv_color_hex(0x4ecdc4) : lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(clock_btn, 16, 0);
    lv_obj_add_event_cb(clock_btn, [](lv_event_t* e) {
        FrameUI::toggleClock();
    }, LV_EVENT_CLICKED, nullptr);
    
    clock_toggle_lbl = lv_label_create(clock_btn);
    lv_label_set_text(clock_toggle_lbl, clock_enabled ? "24H Clock: ON" : "24H Clock: OFF");
    lv_obj_set_style_text_color(clock_toggle_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(clock_toggle_lbl, &lv_font_montserrat_24, 0);
    lv_obj_align(clock_toggle_lbl, LV_ALIGN_CENTER, 0, 0);

    // 4. Sync Display Button
    lv_obj_t* sync_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(sync_btn, 320, 60);
    lv_obj_set_style_bg_color(sync_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(sync_btn, 2, 0);
    lv_obj_set_style_border_color(sync_btn, lv_color_hex(0x4ecdc4), 0);
    lv_obj_set_style_radius(sync_btn, 16, 0);
    lv_obj_add_event_cb(sync_btn, [](lv_event_t* e) {
        FrameUI::syncDisplay();
        FrameUI::hideSettingsMenu();
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* sync_lbl = lv_label_create(sync_btn);
    lv_label_set_text(sync_lbl, "Sync & Fix Display");
    lv_obj_set_style_text_color(sync_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(sync_lbl, &lv_font_montserrat_24, 0);
    lv_obj_align(sync_lbl, LV_ALIGN_CENTER, 0, 0);

    // Toggle settings event on screen click
    lv_event_cb_t toggle_settings = [](lv_event_t* e) {
        if (settings_open) return; // Handled by overlay when open
        
        // If we are here, it means something was clicked that is NOT the settings menu
        // and we are not in 'settings_open' state. Just open it.
        FrameUI::showSettingsMenu();
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
    
    if (no_image_cont) lv_obj_add_flag(no_image_cont, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_invalidate(lv_scr_act());
    lvgl_port_unlock();
}

void FrameUI::loadStoredImage() {
    // 1. Prepare system for heavy load
    // Ensure screen is logically black/hidden before we start
    lvgl_port_lock(-1);
    if (bg_img) lv_obj_add_flag(bg_img, LV_OBJ_FLAG_HIDDEN);
    if (no_image_cont) lv_obj_add_flag(no_image_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(lv_scr_act());
    lvgl_port_unlock();

    lvgl_port_stop(); // Stop LVGL task to prevent PSRAM bus contention
    
    // Ensure backlight is OFF during the 'sausage making'
    esp_panel::drivers::LCD* lcd = getLcd();
    if (board && board->getBacklight()) {
        board->getBacklight()->off();
    }
    
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
        
        if (no_image_cont) lv_obj_add_flag(no_image_cont, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (img_buffer) heap_caps_free(img_buffer);
        img_buffer = nullptr;
        lv_obj_add_flag(bg_img, LV_OBJ_FLAG_HIDDEN);
        if (no_image_cont) lv_obj_clear_flag(no_image_cont, LV_OBJ_FLAG_HIDDEN);
    }

    // 3. Resume and reveal only when ready
    lvgl_port_resume(); 
    
    // Force a single full refresh while still dark
    lvgl_port_lock(-1);
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(nullptr); 
    lvgl_port_unlock();

    // Brief delay to allow DMA to push the first 'clean' frame
    delay(100);

    // Finally turn on the lights
    if (board && board->getBacklight()) {
        board->getBacklight()->on();
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
    if (no_image_cont) lv_obj_clear_flag(no_image_cont, LV_OBJ_FLAG_HIDDEN);
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
    lvgl_port_lock(-1);
    if (settings_menu) {
        lv_obj_clear_flag(settings_menu, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(settings_menu);
    }
    if (settings_overlay) {
        lv_obj_clear_flag(settings_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(settings_overlay);
        lv_obj_move_foreground(settings_menu);
    }
    settings_open = true;
    lvgl_port_unlock();
}

void FrameUI::hideSettingsMenu() {
    lvgl_port_lock(-1);
    if (settings_menu) lv_obj_add_flag(settings_menu, LV_OBJ_FLAG_HIDDEN);
    if (settings_overlay) lv_obj_add_flag(settings_overlay, LV_OBJ_FLAG_HIDDEN);
    settings_open = false;
    lvgl_port_unlock();
}

bool FrameUI::isSettingsMenuVisible() { return settings_open; }

void FrameUI::syncDisplay() {
    lvgl_port_stop();
    delay(100);
    lvgl_port_resume();
    
    lvgl_port_lock(-1);
    lv_obj_invalidate(lv_scr_act());
    lvgl_port_unlock();
}

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
    imageLoadQueuedTime = millis();
}

void FrameUI::toggleClock() {
    clock_enabled = !clock_enabled;
    StorageManager::saveClockEnabled(clock_enabled);
    
    if (clock_toggle_lbl && clock_btn) {
        lv_label_set_text(clock_toggle_lbl, clock_enabled ? "24H Clock: ON" : "24H Clock: OFF");
        // Color on when active, black when off
        lv_obj_set_style_bg_color(clock_btn, clock_enabled ? lv_color_hex(0x4ecdc4) : lv_color_hex(0x000000), 0);
    }
    if (clock_cont) {
        if (clock_enabled) {
            lv_obj_clear_flag(clock_cont, LV_OBJ_FLAG_HIDDEN);
            FlipClock::update_time();
        } else {
            lv_obj_add_flag(clock_cont, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

bool FrameUI::isClockEnabled() {
    return clock_enabled;
}

void FrameUI::loop() {
    if (clock_enabled) {
        static uint32_t last_tick = 0;
        if (lv_tick_elaps(last_tick) > 1000) {
            last_tick = lv_tick_get();
            lvgl_port_lock(-1);
            FlipClock::update_time();
            lvgl_port_unlock();
        }
    }

    if (!imageLoadQueued) return;
    
    // Wait for system to settle after heavy flash write
    if (millis() - imageLoadQueuedTime < 1500) return; // Increased to 1.5s
    
    imageLoadQueued = false;
    
    // loadStoredImage now handles its own locking/stopping
    loadStoredImage();
}
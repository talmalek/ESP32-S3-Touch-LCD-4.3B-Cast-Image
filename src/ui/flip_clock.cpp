#include "flip_clock.h"
#include <Arduino.h>
#include <cstdio>
#include <time.h>

// GIF digit transition assets (0→1, 1→2, ... 9→0)
LV_IMG_DECLARE(gif_01);
LV_IMG_DECLARE(gif_12);
LV_IMG_DECLARE(gif_23);
LV_IMG_DECLARE(gif_34);
LV_IMG_DECLARE(gif_45);
LV_IMG_DECLARE(gif_56);
LV_IMG_DECLARE(gif_67);
LV_IMG_DECLARE(gif_78);
LV_IMG_DECLARE(gif_89);
LV_IMG_DECLARE(gif_90);

// 6 digit GIF objects for HH:MM:SS
static lv_obj_t *img1, *img2, *img3, *img4, *img5, *img6;
static lv_obj_t *date_label = nullptr;

static const void *trans_anim_buf[] = {
    &gif_01, &gif_12, &gif_23, &gif_34, &gif_45,
    &gif_56, &gif_67, &gif_78, &gif_89, &gif_90
};

#define set_anim_src(x)  ((const void *)trans_anim_buf[(x)])
#define buf_limit(idx)   ((idx) % 10)

#define timer_h(v, n, obj)                                                \
  do {                                                                    \
    if (v != n / 10) {                                                    \
      v = buf_limit(n / 10);                                              \
      if (v != 0)                                                         \
        lv_gif_set_src(obj, (const void *)trans_anim_buf[v - 1]);         \
      else                                                                \
        lv_gif_set_src(obj, (const void *)trans_anim_buf[buf_limit(9)]);  \
    }                                                                     \
  } while (0);

#define timer_l(v, n, obj)                                                \
  do {                                                                    \
    if (v != n % 10) {                                                    \
      v = buf_limit(n % 10);                                              \
      if (v != 0)                                                         \
        lv_gif_set_src(obj, (const void *)trans_anim_buf[v - 1]);         \
      else                                                                \
        lv_gif_set_src(obj, (const void *)trans_anim_buf[buf_limit(9)]);  \
    }                                                                     \
  } while (0);

static void set_flip_time_anim(int hour, int minute, int second) {
    static int sec_h = -1, sec_l = -1, min_h = -1, min_l = -1, hou_h = -1, hou_l = -1;

    timer_l(sec_l, second, img6);
    timer_h(sec_h, second, img5);
    timer_l(min_l, minute, img4);
    timer_h(min_h, minute, img3);
    timer_l(hou_l, hour,   img2);
    timer_h(hou_h, hour,   img1);
}

void FlipClock::create(lv_obj_t* parent) {
    // Transparent background
    lv_obj_set_style_bg_opa(parent, 0, 0);
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_IGNORE_LAYOUT);

    // Full-size container
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    // Clock container — centered
    lv_obj_t *cont_clk = lv_obj_create(cont);
    lv_obj_remove_style_all(cont_clk);
    lv_obj_set_size(cont_clk, LV_PCT(100), 200);
    lv_obj_add_flag(cont_clk, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(cont_clk, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(cont_clk, LV_ALIGN_CENTER, 0, -20);

    // Create 6 GIF objects
    img1 = lv_gif_create(cont_clk);
    img2 = lv_gif_create(cont_clk);
    img3 = lv_gif_create(cont_clk);
    img4 = lv_gif_create(cont_clk);
    img5 = lv_gif_create(cont_clk);
    img6 = lv_gif_create(cont_clk);

    lv_obj_clear_flag(img1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(img2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(img3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(img4, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(img5, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(img6, LV_OBJ_FLAG_CLICKABLE);

    // Initialize all to "0" (9→0 transition frozen)
    lv_gif_set_src(img1, set_anim_src(9));
    lv_gif_set_src(img2, set_anim_src(9));
    lv_gif_set_src(img3, set_anim_src(9));
    lv_gif_set_src(img4, set_anim_src(9));
    lv_gif_set_src(img5, set_anim_src(9));
    lv_gif_set_src(img6, set_anim_src(9));

    // GIF original dimensions are 96x164
    int original_w = 96;
    int original_h = 164;
    
    // Scale to fit 800x480 screen — use ~75% zoom
    int clock_zoom = 200;  // 200/256 ≈ 78%
    lv_img_set_zoom(img1, clock_zoom);
    lv_img_set_zoom(img2, clock_zoom);
    lv_img_set_zoom(img3, clock_zoom);
    lv_img_set_zoom(img4, clock_zoom);
    lv_img_set_zoom(img5, clock_zoom);
    lv_img_set_zoom(img6, clock_zoom);

    int clock_visual_w = (original_w * clock_zoom) / 256;
    int clock_visual_h = (original_h * clock_zoom) / 256;
    int gap_digit = 2;   // Gap between digits in a pair
    int gap_group = 28;  // Gap between HH, MM, SS groups

    // Total visual width = 6 digits + 3 digit gaps + 2 group gaps
    int clock_total_w = (6 * clock_visual_w) + (3 * gap_digit) + (2 * gap_group);
    int screen_w = 800;
    int clock_visual_left = (screen_w - clock_total_w) / 2;
    int clock_start_x = clock_visual_left - (original_w - clock_visual_w) / 2;

    // Labels for HOURS, MINUTES, SECONDS
    int label_h = 24;
    int label_spacing_y = 12;
    int clock_zoom_gap = (original_h - clock_visual_h) / 2;
    int clock_box_y = label_h + label_spacing_y - clock_zoom_gap;

    // Position digits
    int pos1 = clock_start_x;
    int pos2 = pos1 + clock_visual_w + gap_digit;
    int pos3 = pos2 + clock_visual_w + gap_group;
    int pos4 = pos3 + clock_visual_w + gap_digit;
    int pos5 = pos4 + clock_visual_w + gap_group;
    int pos6 = pos5 + clock_visual_w + gap_digit;

    lv_obj_align(img1, LV_ALIGN_TOP_LEFT, pos1, clock_box_y);
    lv_obj_align(img2, LV_ALIGN_TOP_LEFT, pos2, clock_box_y);
    lv_obj_align(img3, LV_ALIGN_TOP_LEFT, pos3, clock_box_y);
    lv_obj_align(img4, LV_ALIGN_TOP_LEFT, pos4, clock_box_y);
    lv_obj_align(img5, LV_ALIGN_TOP_LEFT, pos5, clock_box_y);
    lv_obj_align(img6, LV_ALIGN_TOP_LEFT, pos6, clock_box_y);

    // Styled labels
    static lv_style_t style_label;
    static bool style_inited = false;
    if (!style_inited) {
        lv_style_init(&style_label);
        lv_style_set_text_color(&style_label, lv_color_white());
        lv_style_set_text_font(&style_label, &lv_font_montserrat_14);
        style_inited = true;
    }

    int clock_group_w = 2 * clock_visual_w + gap_digit;

    lv_obj_t *label_hours = lv_label_create(cont_clk);
    lv_obj_add_style(label_hours, &style_label, 0);
    lv_label_set_text(label_hours, "HOURS");
    lv_obj_set_width(label_hours, clock_group_w);
    lv_obj_set_style_text_align(label_hours, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_hours, LV_ALIGN_TOP_LEFT, clock_visual_left, 0);
    lv_obj_clear_flag(label_hours, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label_minutes = lv_label_create(cont_clk);
    lv_obj_add_style(label_minutes, &style_label, 0);
    lv_label_set_text(label_minutes, "MINUTES");
    lv_obj_set_width(label_minutes, clock_group_w);
    lv_obj_set_style_text_align(label_minutes, LV_TEXT_ALIGN_CENTER, 0);
    int mm_left = clock_visual_left + clock_group_w + gap_group;
    lv_obj_align(label_minutes, LV_ALIGN_TOP_LEFT, mm_left, 0);
    lv_obj_clear_flag(label_minutes, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label_seconds = lv_label_create(cont_clk);
    lv_obj_add_style(label_seconds, &style_label, 0);
    lv_label_set_text(label_seconds, "SECONDS");
    lv_obj_set_width(label_seconds, clock_group_w);
    lv_obj_set_style_text_align(label_seconds, LV_TEXT_ALIGN_CENTER, 0);
    int ss_left = clock_visual_left + 2 * (clock_group_w + gap_group);
    lv_obj_align(label_seconds, LV_ALIGN_TOP_LEFT, ss_left, 0);
    lv_obj_clear_flag(label_seconds, LV_OBJ_FLAG_CLICKABLE);

    // Date label below clock
    date_label = lv_label_create(parent);
    lv_label_set_text(date_label, "");
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_24, 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 100);
    lv_obj_clear_flag(date_label, LV_OBJ_FLAG_CLICKABLE);

    // Initial time
    update_time();
}

void FlipClock::update_time() {
    time_t raw_time = time(NULL);
    struct tm *ti = localtime(&raw_time);

    set_flip_time_anim(ti->tm_hour, ti->tm_min, ti->tm_sec);

    // Update date once per day
    if (date_label) {
        static int last_day = -1;
        if (ti->tm_mday != last_day) {
            last_day = ti->tm_mday;
            const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
            char date_buf[64];
            sprintf(date_buf, "%s, %s %d, %d", days[ti->tm_wday], months[ti->tm_mon], ti->tm_mday, ti->tm_year + 1900);
            lv_label_set_text(date_label, date_buf);
        }
    }
}

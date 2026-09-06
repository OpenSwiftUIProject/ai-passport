#include "demo.h"
#include "swift/PassportBridge.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_system.h"
#include "../assets/images/spark_rgb565.h"
#include <string.h>

// Static scene: no timers, workers or input callbacks retain these objects.
static lv_obj_t *s_screen;
static lv_obj_t *s_scene;
static unsigned s_nodes;
static passport_scene_geometry_t s_geometry;
static const lv_image_dsc_t SPARK_IMAGE = {
    .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                .w = 16, .h = 16, .stride = 32 },
    .data_size = sizeof(SPARK_RGB565), .data = (const uint8_t *)SPARK_RGB565,
};

static bool valid_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    // Bound LVGL coordinates, image scaling and per-scene allocation.
    return s_scene && s_nodes < 32 && x >= -32767 && x <= 32767 &&
           y >= -32767 && y <= 32767 && width > 0 && width <= 1024 &&
           height > 0 && height <= 1024;
}

bool passport_scene_begin(int32_t battery)
{
    if (s_screen) return false;
    lv_display_t *display = lv_display_get_default();
    if (!display) return false;
    s_geometry = (passport_scene_geometry_t) {
        .screen_width = lv_display_get_horizontal_resolution(display),
        .screen_height = lv_display_get_vertical_resolution(display),
        .top = 66, .leading = 12, .bottom = 30, .trailing = 12,
    };
    if (s_geometry.screen_width <= s_geometry.leading + s_geometry.trailing ||
        s_geometry.screen_height <= s_geometry.top + s_geometry.bottom ||
        s_geometry.screen_width > 1024 || s_geometry.screen_height > 1024) return false;
    s_screen = ui_pixel_screen_create("OpenSwiftUI");
    lv_obj_t *label = ui_pixel_label(s_screen, "", &lv_font_montserrat_14, UI_PAPER);
    lv_obj_align(label, LV_ALIGN_TOP_RIGHT, -12, 44);
    if (battery >= 0 && battery <= 100) lv_label_set_text_fmt(label, "Boot: %ld%%", (long)battery);
    else lv_label_set_text(label, "Boot: --%");
    s_scene = lv_obj_create(s_screen);
    lv_obj_remove_style_all(s_scene);
    lv_obj_set_pos(s_scene, s_geometry.leading, s_geometry.top);
    lv_obj_set_size(s_scene, s_geometry.screen_width - s_geometry.leading - s_geometry.trailing,
                   s_geometry.screen_height - s_geometry.top - s_geometry.bottom);
    lv_obj_remove_flag(s_scene, LV_OBJ_FLAG_SCROLLABLE);
    // Parent bounds clip child drawing. No screen-sized bitmap is allocated.
    s_nodes = 0;
    lv_screen_load(s_screen);
    return true;
}

bool passport_scene_get_geometry(passport_scene_geometry_t *geometry)
{
    if (!s_scene || !geometry) return false;
    *geometry = s_geometry;
    return true;
}

bool passport_scene_measure_image(const uint8_t *name, uint32_t length, int32_t *width, int32_t *height)
{
    if (!name || length != 5 || memcmp(name, "spark", 5) || !width || !height) return false;
    *width = SPARK_IMAGE.header.w;
    *height = SPARK_IMAGE.header.h;
    return true;
}

static bool copy_text(const uint8_t *text, uint32_t length, char buffer[96])
{
    if (!text || length > 95 || memchr(text, 0, length)) return false;
    memcpy(buffer, text, length);
    buffer[length] = '\0';
    return true;
}

bool passport_scene_measure_text(const uint8_t *text, uint32_t length, int32_t proposed_width, int32_t *width, int32_t *height)
{
    char buffer[96];
    if (!width || !height || proposed_width < -1 || proposed_width > 32767 || !copy_text(text, length, buffer)) return false;
    lv_point_t size;
    // The same font, wrapping and spacing are used when the label is rendered.
    lv_text_get_size(&size, buffer, &lv_font_montserrat_14, 0, 0,
                     proposed_width < 0 ? LV_COORD_MAX : proposed_width, LV_TEXT_FLAG_NONE);
    *width = proposed_width < 0 ? size.x : LV_MIN(size.x, proposed_width);
    *height = size.y;
    return true;
}

bool passport_scene_fill(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb, uint8_t alpha)
{
    if (!valid_rect(x, y, width, height)) return false;
    lv_obj_t *object = lv_obj_create(s_scene);
    lv_obj_remove_style_all(object);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, height);
    lv_obj_set_style_bg_color(object, lv_color_hex(rgb), 0);
    lv_obj_set_style_bg_opa(object, alpha, 0);
    ++s_nodes;
    return true;
}

bool passport_scene_image(int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t *name, uint32_t length)
{
    if (!valid_rect(x, y, width, height) || !name || length != 5 || memcmp(name, "spark", 5)) return false;
    lv_obj_t *object = lv_image_create(s_scene);
    lv_image_set_src(object, &SPARK_IMAGE);
    lv_image_set_pivot(object, 0, 0);
    lv_image_set_antialias(object, false);
    lv_image_set_scale_x(object, (uint32_t)width * 256 / 16);
    lv_image_set_scale_y(object, (uint32_t)height * 256 / 16);
    lv_obj_set_pos(object, x, y);
    ++s_nodes;
    return true;
}

bool passport_scene_text(int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t *text, uint32_t length, uint32_t rgb, uint8_t alpha)
{
    char buffer[96];
    if (!valid_rect(x, y, width, height) || !copy_text(text, length, buffer)) return false;
    lv_obj_t *object = lv_label_create(s_scene);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, height);
    lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP);
    lv_label_set_text(object, buffer); // LVGL owns a copy after this call.
    lv_obj_set_style_text_font(object, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(object, 0, 0);
    lv_obj_set_style_text_line_space(object, 0, 0);
    lv_obj_set_style_text_align(object, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(object, lv_color_hex(rgb), 0);
    lv_obj_set_style_text_opa(object, alpha, 0);
    ++s_nodes;
    return true;
}

void passport_scene_end(bool succeeded)
{
    if (!succeeded) {
        lv_obj_clean(s_scene);
        lv_obj_t *error = ui_pixel_label(s_scene, "View render failed", &lv_font_montserrat_14, UI_INK);
        lv_obj_center(error);
    }
    ESP_LOGI("openswiftui", "profile=%lu render=%s nodes=%u free_heap=%lu",
             (unsigned long)openswiftui_embedded_version(), succeeded ? "OK" : "FAIL",
             s_nodes, (unsigned long)esp_get_free_heap_size());
}

void demo_openswiftui_enter(void) { passport_content_enter(); }
void demo_openswiftui_exit(void)
{
    if (s_screen) lv_obj_delete(s_screen);
    s_screen = s_scene = NULL;
    s_nodes = 0;
}
void demo_openswiftui_key(bsp_btn_t btn, bsp_btn_ev_t ev) { (void)btn; (void)ev; }

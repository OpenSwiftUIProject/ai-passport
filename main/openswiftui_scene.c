#include "swift/PassportBridge.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_system.h"
#include "../assets/images/spark_rgb565.h"
#include <string.h>
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#if defined(PASSPORT_2048_SOAK) || !defined(ESP_PLATFORM)
static uint32_t scene_stack_free(void)
{
#ifdef ESP_PLATFORM
    return uxTaskGetStackHighWaterMark(NULL);
#else
    return 0;
#endif
}
#endif

// Page-owned scene; the app's serialized input dispatcher invokes Swift.
// Stop the page clock before releasing Swift state and the screen.
static lv_obj_t *s_screen;
static lv_obj_t *s_scene;
static lv_obj_t *s_error;
static unsigned s_nodes, s_node_limit, s_allocated;
static lv_obj_t *s_objects[64];
static uint8_t s_types[64];
typedef struct {
    bool valid;
    int32_t x, y, width, height;
    uint32_t rgb;
    uint8_t alpha;
} scene_properties_t;
static scene_properties_t s_properties[64];
static lv_timer_t *s_clock;
static void (*s_tick_callback)(uint32_t);
static uint32_t s_last_tick, s_frame_start, s_frames, s_max_tick;
static bool s_reported;
#ifdef PASSPORT_2048_SOAK
static bool s_measuring, s_flushed;
static uint32_t s_refreshes;
static void scene_refresh(lv_event_t *event)
{
    if (!s_measuring) return;
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_FLUSH_START) s_flushed = true;
    else if (code == LV_EVENT_REFR_READY && s_flushed) {
        ++s_refreshes;
        s_flushed = false;
    }
}
#endif
static passport_scene_geometry_t s_geometry;
static const lv_image_dsc_t SPARK_IMAGE = {
    .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                .w = 16, .h = 16, .stride = 32 },
    .data_size = sizeof(SPARK_RGB565), .data = (const uint8_t *)SPARK_RGB565,
};

static bool valid_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    // Bound LVGL coordinates, image scaling and per-scene allocation.
    return s_scene && s_nodes < s_node_limit && x >= -32767 && x <= 32767 &&
           y >= -32767 && y <= 32767 && width > 0 && width <= 1024 &&
           height > 0 && height <= 1024;
}

static bool scene_begin(int32_t battery, bool game)
{
    if (s_screen) return false;
    lv_display_t *display = lv_display_get_default();
    if (!display) return false;
    s_geometry = (passport_scene_geometry_t) {
        .screen_width = lv_display_get_horizontal_resolution(display),
        .screen_height = lv_display_get_vertical_resolution(display),
        .top = game ? 46 : 66, .leading = 12, .bottom = game ? 34 : 30, .trailing = 12,
    };
    if (s_geometry.screen_width <= s_geometry.leading + s_geometry.trailing ||
        s_geometry.screen_height <= s_geometry.top + s_geometry.bottom ||
        s_geometry.screen_width > 1024 || s_geometry.screen_height > 1024) return false;
    s_screen = ui_pixel_screen_create(game ? "2048" : "OpenSwiftUI");
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
    s_node_limit = game ? 64 : 32;
    lv_screen_load(s_screen);
    return true;
}

bool passport_scene_get_geometry(passport_scene_geometry_t *geometry)
{
    if (!s_scene || !geometry) return false;
    *geometry = s_geometry;
    return true;
}

bool passport_scene_reset(void)
{
    if (!s_scene) return false;
    if (s_error) { lv_obj_delete(s_error); s_error = NULL; }
    s_nodes = 0;
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

// Reuse command slots. Swift has already resolved identities and interpolation;
// objects here represent the current ordered display list, including alpha=0.
static lv_obj_t *scene_object(uint8_t kind)
{
    if (s_nodes < s_allocated && s_types[s_nodes] != kind) {
        lv_obj_delete(s_objects[s_nodes]);
        s_objects[s_nodes] = NULL;
    }
    lv_obj_t *object = s_nodes < s_allocated ? s_objects[s_nodes] : NULL;
    if (!object) {
        object = kind == 1 ? lv_obj_create(s_scene) :
                 kind == 2 ? lv_image_create(s_scene) : lv_label_create(s_scene);
        if (!object) return NULL;
        if (kind == 1) lv_obj_remove_style_all(object);
        if (kind == 2) {
            lv_image_set_src(object, &SPARK_IMAGE);
            lv_image_set_pivot(object, 0, 0);
            lv_image_set_antialias(object, false);
        } else if (kind == 3) {
            lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_font(object, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_letter_space(object, 0, 0);
            lv_obj_set_style_text_line_space(object, 0, 0);
            lv_obj_set_style_text_align(object, LV_TEXT_ALIGN_CENTER, 0);
        }
        lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_move_to_index(object, (int32_t)s_nodes);
        s_objects[s_nodes] = object;
        s_types[s_nodes] = kind;
        s_properties[s_nodes].valid = false;
        if (s_nodes == s_allocated) ++s_allocated;
    }
    return object;
}

// LVGL refreshes styles even when a setter receives the existing value.
// Keep stationary cells/labels out of the invalidation and redraw path.
static void scene_geometry(lv_obj_t *object, int32_t x, int32_t y, int32_t width, int32_t height)
{
    scene_properties_t *old = &s_properties[s_nodes];
    if (!old->valid || old->x != x || old->y != y) lv_obj_set_pos(object, x, y);
    if (!old->valid || old->width != width || old->height != height) {
        if (s_types[s_nodes] == 2) {
            lv_image_set_scale_x(object, (uint32_t)width * 256 / 16);
            lv_image_set_scale_y(object, (uint32_t)height * 256 / 16);
        } else lv_obj_set_size(object, width, height);
    }
    old->x = x; old->y = y; old->width = width; old->height = height;
}

static void scene_color(lv_obj_t *object, uint32_t rgb, uint8_t alpha)
{
    scene_properties_t *old = &s_properties[s_nodes];
    if (!old->valid || old->rgb != rgb) {
        if (s_types[s_nodes] == 1) lv_obj_set_style_bg_color(object, lv_color_hex(rgb), 0);
        else lv_obj_set_style_text_color(object, lv_color_hex(rgb), 0);
    }
    if (!old->valid || old->alpha != alpha) {
        if (s_types[s_nodes] == 1) lv_obj_set_style_bg_opa(object, alpha, 0);
        else lv_obj_set_style_text_opa(object, alpha, 0);
    }
    old->rgb = rgb; old->alpha = alpha;
}

bool passport_scene_fill(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb, uint8_t alpha)
{
    if (!valid_rect(x, y, width, height)) return false;
    lv_obj_t *object = scene_object(1);
    if (!object) return false;
    scene_geometry(object, x, y, width, height);
    scene_color(object, rgb, alpha);
    s_properties[s_nodes].valid = true;
    ++s_nodes;
    return true;
}

bool passport_scene_image(int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t *name, uint32_t length)
{
    if (!valid_rect(x, y, width, height) || !name || length != 5 || memcmp(name, "spark", 5)) return false;
    lv_obj_t *object = scene_object(2);
    if (!object) return false;
    scene_geometry(object, x, y, width, height);
    s_properties[s_nodes].valid = true;
    ++s_nodes;
    return true;
}

bool passport_scene_text(int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t *text, uint32_t length, uint32_t rgb, uint8_t alpha)
{
    char buffer[96];
    if (!valid_rect(x, y, width, height) || !copy_text(text, length, buffer)) return false;
    lv_obj_t *object = scene_object(3);
    if (!object) return false;
    scene_geometry(object, x, y, width, height);
    if (strcmp(lv_label_get_text(object), buffer)) lv_label_set_text(object, buffer);
    scene_color(object, rgb, alpha);
    s_properties[s_nodes].valid = true;
    ++s_nodes;
    return true;
}

void passport_scene_end(bool succeeded)
{
    if (!s_scene) return;
    if (!succeeded) {
        lv_obj_clean(s_scene);
        s_nodes = s_allocated = 0;
        memset(s_objects, 0, sizeof(s_objects));
        lv_obj_t *error = ui_pixel_label(s_scene, "View render failed", &lv_font_montserrat_14, UI_INK);
        lv_obj_center(error);
        s_error = error;
    } else {
        while (s_allocated > s_nodes) {
            --s_allocated;
            lv_obj_delete(s_objects[s_allocated]);
            s_objects[s_allocated] = NULL;
        }
    }
    if (s_clock) {
        if (s_frames++ == 0) s_frame_start = lv_tick_get();
    }
    if (!s_clock || !s_reported || !succeeded) {
    ESP_LOGI("openswiftui", "profile=%lu render=%s nodes=%u free_heap=%lu",
             (unsigned long)openswiftui_embedded_version(), succeeded ? "OK" : "FAIL",
             s_nodes, (unsigned long)esp_get_free_heap_size());
        s_reported = true;
    }
}

bool passport_scene_begin(int32_t battery) { return scene_begin(battery, false); }
bool passport_scene_begin_2048(int32_t battery) { return scene_begin(battery, true); }

void passport_scene_destroy(void)
{
    passport_scene_stop_clock();
    if (s_screen) lv_obj_delete(s_screen);
    s_screen = s_scene = s_error = NULL;
    s_nodes = s_node_limit = s_allocated = 0;
    s_reported = false;
    memset(s_objects, 0, sizeof(s_objects));
}

static void scene_tick(lv_timer_t *timer)
{
    (void)timer;
    uint32_t now = lv_tick_get();
    uint32_t elapsed = now - s_last_tick; // Unsigned subtraction handles tick wrap.
    s_last_tick = now;
    if (s_tick_callback) s_tick_callback(elapsed);
    uint32_t duration = lv_tick_get() - now;
    if (duration > s_max_tick) s_max_tick = duration;
}

bool passport_scene_start_clock(void (*callback)(uint32_t))
{
    if (!s_scene || s_clock || !callback) return false;
    s_tick_callback = callback;
    s_last_tick = lv_tick_get();
    s_frames = s_max_tick = 0;
    s_clock = lv_timer_create(scene_tick, 16, NULL);
    if (!s_clock) s_tick_callback = NULL;
#ifdef PASSPORT_2048_SOAK
    if (s_clock) lv_display_add_event_cb(lv_display_get_default(), scene_refresh, LV_EVENT_ALL, NULL);
#endif
    return s_clock != NULL;
}

void passport_scene_stop_clock(void)
{
#ifdef PASSPORT_2048_SOAK
    if (s_clock) lv_display_remove_event_cb_with_user_data(lv_display_get_default(), scene_refresh, NULL);
    s_measuring = false;
#endif
    if (s_clock) lv_timer_delete(s_clock);
    s_clock = NULL;
    s_tick_callback = NULL;
}

void passport_scene_clock_begin(void)
{
    passport_scene_clock_phase();
    s_frame_start = lv_tick_get();
    s_frames = 1;
    s_max_tick = 0;
#ifdef PASSPORT_2048_SOAK
    s_measuring = true;
    s_flushed = false;
    s_refreshes = 0;
#endif
}

void passport_scene_clock_phase(void)
{
    // Layout may take longer than a tick on the C3. Start elapsed time after
    // publishing each phase's first frame, excluding that preparatory work.
    s_last_tick = lv_tick_get();
    if (s_clock) lv_timer_reset(s_clock);
}

void passport_scene_clock_report(void)
{
#ifdef PASSPORT_2048_SOAK
    ESP_LOGI("2048-soak", "panel_refreshes=%lu", (unsigned long)s_refreshes);
    s_measuring = false;
#endif
#if defined(PASSPORT_2048_SOAK) || !defined(ESP_PLATFORM)
    ESP_LOGI("openswiftui", "animation frames=%lu elapsed_ms=%lu max_tick_ms=%lu free_heap=%lu stack_free=%lu",
             (unsigned long)s_frames, (unsigned long)(lv_tick_get() - s_frame_start),
             (unsigned long)s_max_tick, (unsigned long)esp_get_free_heap_size(), (unsigned long)scene_stack_free());
#endif
    s_frames = s_max_tick = 0;
}

void passport_scene_input_overflow(void)
{
#if defined(PASSPORT_2048_SOAK) || !defined(ESP_PLATFORM)
    ESP_LOGI("openswiftui", "animation input queue full; dropped=%u", 1u);
#endif
}

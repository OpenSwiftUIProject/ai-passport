// components/bsp/src/bsp_display_lvgl.c
// LVGL 接入单独成文件:不用 LVGL 的开发者删掉本文件 + idf_component.yml 里的两条依赖即可。
#include "bsp_display.h"
#include "bsp_pins.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

static const char *TAG = "bsp_lvgl";

static lv_display_t *s_disp;

lv_display_t *bsp_lvgl_init(void) {
    if (s_disp) return s_disp;
    if (!bsp_display_panel()) {
        ESP_LOGE(TAG, "请先成功调用 bsp_display_init()");
        return NULL;
    }

    const lvgl_port_cfg_t pc = ESP_LVGL_PORT_INIT_CONFIG();
    if (lvgl_port_init(&pc) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl_port_init 失败");
        return NULL;
    }

    const lvgl_port_display_cfg_t dc = {
        .panel_handle = bsp_display_panel(),
        .io_handle    = bsp_display_io(),
        // ⚠ C3 无 PSRAM,DMA 只能用内部 RAM(总共约 150KB)。
        // 20 行单缓冲 ≈ 9.6KB;若改成 40 行双缓冲(≈37.5KB)会把 I2S 等外设的
        // DMA 描述符挤到 NO_MEM。刷新略慢但稳。
        .buffer_size   = (uint32_t)BSP_LCD_W * 20,
        .double_buffer = false,
        .hres = BSP_LCD_W, .vres = BSP_LCD_H,
        // 旋转/镜像必须在这里配:esp_lvgl_port 注册显示时会重新下发 MADCTL,
        // 覆盖 bsp_display.c 里 esp_lcd_panel_mirror() 的设置。
        .rotation = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        // swap_bytes:LVGL 输出小端 RGB565,ST7789 走 SPI 要大端 → 需交换高低字节。
        .flags = { .buff_dma = true, .swap_bytes = true },
    };
    s_disp = lvgl_port_add_disp(&dc);
    if (!s_disp) { ESP_LOGE(TAG, "lvgl_port_add_disp 失败"); return NULL; }

    ESP_LOGI(TAG, "LVGL 就绪");
    return s_disp;
}

bool bsp_lvgl_lock(int timeout_ms) { return lvgl_port_lock(timeout_ms); }
void bsp_lvgl_unlock(void)         { lvgl_port_unlock(); }

typedef struct {
    bsp_display_row_sink_t sink;
    void *context;
    esp_err_t result;
    uint32_t pixels;
} capture_t;

static void capture_flush(lv_event_t *event)
{
    capture_t *capture = lv_event_get_user_data(event);
    if (capture->result != ESP_OK) return;
    const lv_area_t *area = lv_event_get_param(event);
    const lv_draw_buf_t *buffer = lv_display_get_buf_active(s_disp);
    // LVGL 9.5 sends FLUSH_START before the port swaps RGB565 bytes for SPI.
    // In partial mode buf_act is the row-strided buffer passed to flush_cb.
    if (!area || !buffer || !buffer->data || area->x1 < 0 || area->y1 < 0 ||
        area->x2 >= BSP_LCD_W || area->y2 >= BSP_LCD_H ||
        area->x2 < area->x1 || area->y2 < area->y1) {
        capture->result = ESP_ERR_INVALID_STATE;
        return;
    }
    uint32_t width = (uint32_t)lv_area_get_width(area);
    uint32_t height = (uint32_t)lv_area_get_height(area);
    uint32_t stride = buffer->header.stride;
    if (stride < width * 2 || (height - 1) * stride + width * 2 > buffer->data_size) {
        capture->result = ESP_ERR_INVALID_SIZE;
        return;
    }
    for (uint32_t row = 0; row < height; ++row) {
        capture->result = capture->sink((uint16_t)area->x1, (uint16_t)(area->y1 + row),
                                        (uint16_t)width, buffer->data + row * stride,
                                        capture->context);
        if (capture->result != ESP_OK) return;
        capture->pixels += width;
    }
}

esp_err_t bsp_display_capture(bsp_display_row_sink_t sink, void *context)
{
    if (!sink) return ESP_ERR_INVALID_ARG;
    if (!s_disp) return ESP_ERR_INVALID_STATE;
    if (!bsp_lvgl_lock(1000)) return ESP_ERR_TIMEOUT;
    capture_t capture = { .sink = sink, .context = context, .result = ESP_OK };
    if (lv_display_get_render_mode(s_disp) != LV_DISPLAY_RENDER_MODE_PARTIAL ||
        lv_display_get_color_format(s_disp) != LV_COLOR_FORMAT_RGB565 ||
        lv_display_get_rotation(s_disp) != LV_DISPLAY_ROTATION_0) {
        bsp_lvgl_unlock();
        return ESP_ERR_NOT_SUPPORTED;
    }
    lv_display_add_event_cb(s_disp, capture_flush, LV_EVENT_FLUSH_START, &capture);
    lv_obj_invalidate(lv_display_get_screen_active(s_disp));
    // Synchronous refresh runs the sink in this worker, not the LVGL timer or
    // button task. UI mutation/animations cannot interleave while locked.
    lv_refr_now(s_disp);
    lv_display_remove_event_cb_with_user_data(s_disp, capture_flush, &capture);
    if (capture.result == ESP_OK && capture.pixels != BSP_LCD_W * BSP_LCD_H) {
        capture.result = ESP_ERR_INVALID_SIZE;
    }
    bsp_lvgl_unlock();
    return capture.result;
}

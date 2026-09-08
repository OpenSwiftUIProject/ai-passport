#include "screen_capture.h"
#include "screen_protocol.h"
#include "bsp_display.h"
#include "bsp_pins.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "screen_capture";
static TaskHandle_t s_task;

typedef struct {
    uint32_t id;
    uint32_t pixels;
    int64_t deadline_us;
    char packet[4 * ((BSP_LCD_W * 2 + 2) / 3) + 96];
} transfer_t;

static esp_err_t send_packet(const char *packet, size_t length, int64_t deadline)
{
    if (esp_timer_get_time() >= deadline) return ESP_ERR_TIMEOUT;
    // Coordinate with ordinary stdout logs. Never hold this lock while trying
    // to acquire LVGL: other tasks may log while they own the LVGL lock.
    flockfile(stdout);
    int written = usb_serial_jtag_write_bytes(packet, length, pdMS_TO_TICKS(100));
    funlockfile(stdout);
    return written == (int)length ? ESP_OK : ESP_ERR_TIMEOUT;
}

static esp_err_t send_row(uint16_t x, uint16_t y, uint16_t width,
                          const uint8_t *pixels, void *context)
{
    transfer_t *transfer = context;
    size_t length = screen_encode_row(transfer->packet, sizeof(transfer->packet),
                                      transfer->id, x, y, width, pixels);
    if (!length) return ESP_ERR_INVALID_SIZE;
    esp_err_t result = send_packet(transfer->packet, length, transfer->deadline_us);
    if (result == ESP_OK) transfer->pixels += width;
    return result;
}

static void capture(uint32_t id)
{
    transfer_t transfer = { .id = id, .deadline_us = esp_timer_get_time() + 8000000 };
    int length = snprintf(transfer.packet, sizeof(transfer.packet),
                          "\nFPS1 BEGIN %08" PRIx32 " %d %d RGB565LE\n", id, BSP_LCD_W, BSP_LCD_H);
    esp_err_t result = send_packet(transfer.packet, (size_t)length, transfer.deadline_us);
    if (result == ESP_OK) result = bsp_display_capture(send_row, &transfer);
    if (result == ESP_OK) {
        length = snprintf(transfer.packet, sizeof(transfer.packet),
                           "FPS1 END %08" PRIx32 " %" PRIu32 "\n", id, transfer.pixels);
    } else {
        length = snprintf(transfer.packet, sizeof(transfer.packet),
                           "\nFPS1 ERROR %08" PRIx32 " %s\n", id, esp_err_to_name(result));
    }
    // A disconnected host must not keep the UI blocked. One bounded final
    // attempt allows connected hosts to receive an explicit error record.
    send_packet(transfer.packet, (size_t)length, esp_timer_get_time() + 200000);
    ESP_LOGI(TAG, "capture result=%s pixels=%" PRIu32 " free_heap=%" PRIu32 " stack_free=%u",
             esp_err_to_name(result), transfer.pixels, esp_get_free_heap_size(),
             (unsigned)uxTaskGetStackHighWaterMark(NULL));
}

typedef struct {
    uint16_t next_y;
    int64_t deadline_us;
} fap_transfer_t;

static esp_err_t send_fap_row(uint16_t x, uint16_t y, uint16_t width,
                             const uint8_t *pixels, void *context)
{
    fap_transfer_t *transfer = context;
    if (!screen_fap_advance(&transfer->next_y, BSP_LCD_W, BSP_LCD_H, x, y, width)) {
        return ESP_ERR_INVALID_STATE;
    }
    return send_packet((const char *)pixels, (size_t)width * 2, transfer->deadline_us);
}

static void capture_fap(void)
{
    // Take LVGL before stdout (the BSP lock and FILE lock are recursive).
    // Keep ordinary console logs out of the binary header/payload. The render
    // and row callbacks must not print while this transfer owns stdout.
    if (!bsp_lvgl_lock(1000)) return;
    flockfile(stdout);
    fflush(stdout);
    fap_transfer_t transfer = { .deadline_us = esp_timer_get_time() + 8000000 };
    char header[80];
    size_t length = screen_fap_header(header, sizeof(header), BSP_LCD_W, BSP_LCD_H);
    esp_err_t result = send_packet("\n", 1, transfer.deadline_us);
    if (result == ESP_OK) result = send_packet(header, length, transfer.deadline_us);
    if (result == ESP_OK) result = bsp_display_capture(send_fap_row, &transfer);
    funlockfile(stdout);
    bsp_lvgl_unlock();
    // Do not append diagnostic bytes to an incomplete binary payload.
    // A later request is still accepted; no reset or persistent state change.
    (void)result;
}

static void command_task(void *argument)
{
    (void)argument;
    char line[64];
    size_t used = 0;
    bool overflow = false;
    for (;;) {
        char byte;
        if (usb_serial_jtag_read_bytes(&byte, 1, pdMS_TO_TICKS(100)) != 1) continue;
        if (byte == '\r') continue;
        if (byte == '\n') {
            line[used] = '\0';
            uint32_t id;
            if (!overflow && screen_request_id(line, &id)) capture(id);
            else if (!overflow && screen_fap_request(line)) capture_fap();
            used = 0;
            overflow = false;
        } else if ((unsigned char)byte < 32 || (unsigned char)byte > 126) {
            overflow = true;
        } else if (used + 1 < sizeof(line)) {
            line[used++] = byte;
        } else {
            overflow = true;
        }
    }
}

esp_err_t screen_capture_start(void)
{
    if (s_task) return ESP_OK;
    usb_serial_jtag_driver_config_t config = { .tx_buffer_size = 2048, .rx_buffer_size = 256 };
    bool installed_here = !usb_serial_jtag_is_driver_installed();
    if (installed_here) {
        esp_err_t result = usb_serial_jtag_driver_install(&config);
        if (result != ESP_OK) return result;
    }
    // Console and the capture service must share the interrupt-backed driver.
    usb_serial_jtag_vfs_use_driver();
    // lv_refr_now draws on this stack: use the measured LVGL rendering budget
    // plus 2 KiB for capture records and USB/protocol calls.
    if (xTaskCreate(command_task, "screen_usb", CONFIG_BSP_LVGL_TASK_STACK_SIZE + 2048,
                    NULL, 3, &s_task) != pdPASS) {
        if (installed_here) {
            usb_serial_jtag_vfs_use_nonblocking();
            usb_serial_jtag_driver_uninstall();
        }
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "USB screenshot service ready (FPS1 / FAP_SCREENSHOT_V1)");
    return ESP_OK;
}

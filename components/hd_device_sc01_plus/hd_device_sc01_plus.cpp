#include "hd_device_sc01_plus.h"

#if CONFIG_SPIRAM_SUPPORT
#define LVGL_BUFFER_SIZE (TFT_HEIGHT * 20)  // Changed from WIDTH to HEIGHT
#else
#define LVGL_BUFFER_SIZE (TFT_HEIGHT * 20)  // Changed from WIDTH to HEIGHT
#endif

namespace esphome {
namespace hd_device {

static const char *const TAG = "HD_DEVICE";
static lv_disp_draw_buf_t draw_buf;
// Optimize buffer size
static lv_color_t *buf = (lv_color_t *)heap_caps_malloc(
    LVGL_BUFFER_SIZE * sizeof(lv_color_t), 
    MALLOC_CAP_DMA | MALLOC_CAP_32BIT
);

LGFX lcd;

lv_disp_t *indev_disp;
lv_group_t *group;

// Add boot time measurement
static unsigned long boot_start_time = 0;

// Add global pointer to instance
static HaDeckDevice* instance_ = nullptr;

void IRAM_ATTR flush_pixels(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    // Pre-calculate dimensions once
    const uint32_t w = (area->x2 - area->x1 + 1);
    const uint32_t h = (area->y2 - area->y1 + 1);
    const uint32_t len = w * h;

    // Use direct memory writes where possible
    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.writePixels((uint16_t *)color_p, len, true);  // Removed the &color_p->full
    lcd.endWrite();

    lv_disp_flush_ready(disp);
}

void IRAM_ATTR touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    static uint16_t last_x = 0, last_y = 0;
    static bool last_touched = false;
    static uint32_t last_read = 0;
    uint32_t now = millis();

    // Limit touch readings to every 5ms
    if (now - last_read < 5) {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = last_touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        return;
    }

    last_read = now;
    last_touched = lcd.getTouch(&last_x, &last_y);
    
    if (last_touched && instance_) {
        // Wake screen on touch if brightness is 0
        if (instance_->get_brightness() == 0) {
            instance_->set_brightness(100);
        }
    }
    
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = last_touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

void HaDeckDevice::lvgl_init_task(void *param) {
    lv_init();
    vTaskDelete(nullptr);
}

void HaDeckDevice::setup() {
    instance_ = this;  // Store instance pointer
    boot_start_time = millis();

    // Initialize LVGL and LCD in parallel using second core
    TaskHandle_t task_handle;
    xTaskCreatePinnedToCore(
        lvgl_init_task,
        "lvgl_init",
        4096,
        nullptr,
        1,
        &task_handle,
        0
    );

    // Initialize display on main core
    lcd.init();
    lcd.setBrightness(brightness_);
    
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LVGL_BUFFER_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.rotated = 1;
    disp_drv.sw_rotate = 1;
    disp_drv.flush_cb = flush_pixels;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    group = lv_group_create();
    lv_group_set_default(group);

    lv_theme_default_init(disp, lv_color_hex(0xFFEB3B), lv_color_hex(0xFF7043), 1, LV_FONT_DEFAULT);

    ESP_LOGD(TAG, "Boot completed in %lums", millis() - boot_start_time);
}

// ...existing code...

void HaDeckDevice::loop() {
    static unsigned long last_tick = 0;
    unsigned long now = millis();
    
    // Only handle LVGL updates
    if (now - last_tick >= 10) {
        lv_timer_handler();
        last_tick = now;
    }

    // System monitoring every 60 seconds
    if (now - time_ > 60000) {
        time_ = now;
        float cpu_freq = ESP.getCpuFreqMHz();
        uint32_t free_heap = esp_get_free_heap_size();
        uint32_t min_free_heap = esp_get_minimum_free_heap_size();
        uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        float heap_frag = 100.0 - (float)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) * 100.0 / free_heap;

        ESP_LOGD(TAG, "System Status:");
        ESP_LOGD(TAG, "  CPU: %.1f MHz", cpu_freq);
        ESP_LOGD(TAG, "  Free Heap: %u bytes (Min: %u bytes)", free_heap, min_free_heap);
        ESP_LOGD(TAG, "  Heap Fragmentation: %.1f%%", heap_frag);
        ESP_LOGD(TAG, "  Free PSRAM: %u bytes", free_psram);
        ESP_LOGD(TAG, "  Uptime: %lu ms", now);
    }
}

// ...rest of existing code...

float HaDeckDevice::get_setup_priority() const { return setup_priority::DATA; }

uint8_t HaDeckDevice::get_brightness() {
    return brightness_;
}

void HaDeckDevice::set_brightness(uint8_t value) {
    brightness_ = value;
    lcd.setBrightness(brightness_);
}

}  // namespace hd_device
}  // namespace esphome
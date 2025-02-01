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
    
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = last_touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

void HaDeckDevice::lvgl_init_task(void *param) {
    lv_init();
    vTaskDelete(nullptr);
}

void HaDeckDevice::setup() {
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

// Add display backlight auto-dimming
void HaDeckDevice::loop() {
    static unsigned long last_tick = 0;
    static unsigned long last_touch = 0;
    static uint8_t current_brightness = brightness_;
    unsigned long now = millis();
    
    // Increase minimum update time
    if (now - last_tick >= 10) {  // Changed from 5ms to 10ms
        lv_timer_handler();
        last_tick = now;
        
        // Check for touch to reset dimming timer
        uint16_t x, y;
        if (lcd.getTouch(&x, &y)) {
            last_touch = now;
            if (current_brightness != brightness_) {
                lcd.setBrightness(brightness_);
                current_brightness = brightness_;
            }
        }
        
        // Auto-dim after 30 seconds
        if (now - last_touch > 30000 && current_brightness > 20) {
            current_brightness = 20;
            lcd.setBrightness(current_brightness);
        }
    }

    // Reduce logging frequency
    if (now - time_ > 300000) {  // Changed from 60s to 300s
        time_ = now;
        ESP_LOGD(TAG, "Available heap: %d bytes", esp_get_free_heap_size());
    }
}

float HaDeckDevice::get_setup_priority() const { return setup_priority::DATA; }

uint8_t HaDeckDevice::get_brightness() {
    return brightness_;
}

void HaDeckDevice::set_brightness(uint8_t value) {
    brightness_ = value;  // Removed minimum brightness limit
    lcd.setBrightness(brightness_);
}

}  // namespace hd_device
}  // namespace esphome
#include "hd_device_sc01_plus.h"

#if CONFIG_SPIRAM_SUPPORT
#define LVGL_BUFFER_SIZE (TFT_WIDTH * 20)
#else
#define LVGL_BUFFER_SIZE (TFT_WIDTH * 10)
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
    uint16_t touchX, touchY;
    bool touched = lcd.getTouch(&touchX, &touchY);

    if (touched) {
        data->point.x = touchX;
        data->point.y = touchY;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void HaDeckDevice::setup() {
    // Initialize display first
    lcd.init();
    lcd.setBrightness(brightness_);

    // Initialize LVGL with optimized settings
    lv_init();
    
    // Use smaller buffer size
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_WIDTH * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.rotated = 1;
    disp_drv.sw_rotate = 1;
    disp_drv.flush_cb = flush_pixels;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.direct_mode = 1;  // Enable direct mode if supported
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    // Optimize touch input
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // Create input group
    group = lv_group_create();
    lv_group_set_default(group);

    // Apply theme after display setup
    lv_theme_default_init(disp, lv_color_hex(0xFFEB3B), lv_color_hex(0xFF7043), 1, LV_FONT_DEFAULT);
}

void HaDeckDevice::loop() {
    static unsigned long last_tick = 0;
    unsigned long now = millis();
    
    // Only update if enough time has passed
    if (now - last_tick >= 5) {  // 5ms minimum between updates
        lv_timer_handler();
        last_tick = now;
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
    brightness_ = std::max((uint8_t)20, value);  // Ensure minimum brightness of 20%
    lcd.setBrightness(brightness_);
}

}  // namespace hd_device
}  // namespace esphome
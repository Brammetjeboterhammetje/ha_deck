#include "hd_device_sc01_plus.h"

namespace esphome {
namespace hd_device {

static const char *const TAG = "HD_DEVICE";
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf = (lv_color_t *)heap_caps_malloc(TFT_HEIGHT * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);

LGFX lcd;

lv_disp_t *indev_disp;
lv_group_t *group;

void IRAM_ATTR flush_pixels(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    uint32_t len = w * h;

    lcd.startWrite();                            /* Start new TFT transaction */
    lcd.setAddrWindow(area->x1, area->y1, w, h); /* set the working window */
    lcd.writePixels((uint16_t *)&color_p->full, len, true);
    lcd.endWrite();                              /* terminate TFT transaction */

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
    lv_init();
    lv_theme_default_init(NULL, lv_color_hex(0xFFEB3B), lv_color_hex(0xFF7043), 1, LV_FONT_DEFAULT);

    lcd.init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_HEIGHT * 20);

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
    indev_drv.long_press_time = 1000;
    indev_drv.long_press_repeat_time = 300;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    group = lv_group_create();
    lv_group_set_default(group);

    lcd.setBrightness(brightness_);

    // Create a test UI
    lv_obj_t * screen = lv_scr_act();
    
    // Create a button
    lv_obj_t * btn = lv_btn_create(screen);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    
    // Add label to the button
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Touch me!");
    lv_obj_center(label);
    
    // Create a counter label
    counter_label_ = lv_label_create(screen);
    lv_label_set_text(counter_label_, "Counter: 0");
    lv_obj_align(counter_label_, LV_ALIGN_TOP_MID, 0, 20);
    
    // Add click event handler
    lv_obj_add_event_cb(btn, [](lv_event_t * e) {
        static int counter = 0;
        counter++;
        lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);
        char buf[32];
        snprintf(buf, sizeof(buf), "Counter: %d", counter);
        lv_label_set_text(label, buf);
    }, LV_EVENT_CLICKED, counter_label_);
}

void HaDeckDevice::loop() {
    lv_timer_handler();

    unsigned long ms = millis();
    if (ms - time_ > 60000) {
        time_ = ms;
        ESP_LOGCONFIG(TAG, "Free memory: %d bytes", esp_get_free_heap_size());
    }
}

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
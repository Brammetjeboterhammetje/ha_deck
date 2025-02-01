#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <algorithm>  // Add this for std::max
#include "LGFX.h"
#include "lvgl.h"

LV_IMG_DECLARE(bg_480x320);

namespace esphome {
namespace hd_device {

class HaDeckDevice : public Component
{
public:
    void setup() override;
    void loop() override;
    float get_setup_priority() const override;
    uint8_t get_brightness();
    void set_brightness(uint8_t value);
    void display_off() { lcd.sleep(); }
    void display_on() { lcd.wakeup(); }
    bool is_display_on() { return !lcd.isInSleep(); }
private:
    unsigned long time_ = 0;
    uint8_t brightness_ = 0;
    bool display_sleep_enabled_ = false;
    unsigned long last_touch_time_ = 0;
    static constexpr uint32_t DISPLAY_TIMEOUT = 60000; // 1 minute timeout
};

}  // namespace hd_device
}  // namespace esphome
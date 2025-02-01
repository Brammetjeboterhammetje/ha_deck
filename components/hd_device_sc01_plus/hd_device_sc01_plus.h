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
    uint32_t get_boot_time() const { return boot_start_time; }
private:
    unsigned long time_ = 0;
    uint8_t brightness_ = 0;
    uint8_t current_brightness = 0;
};

}  // namespace hd_device
}  // namespace esphome

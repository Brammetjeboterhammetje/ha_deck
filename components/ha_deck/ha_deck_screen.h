#pragma once
#include <vector>
#include "ha_deck_widget.h"
#include "hd_button.h"
#include "hd_slider.h"

namespace esphome {
namespace ha_deck {

class HaDeckScreen : public Component
{
public:
    ~HaDeckScreen();
    void setup() override;
    void loop() override;
    float get_setup_priority() const override;

    void set_name(std::string name);
    std::string get_name();
    void set_inactivity(uint32_t value);
    uint32_t get_inactivity();
    bool add_widget(HaDeckWidget *widget);

    void set_active(bool active);
    bool is_active() const { return active_; }
private:
    const char *TAG = "HD_SCREEN";
    std::string name_;
    uint32_t inactivity_ = 0;
    bool active_ = false;
    uint32_t last_activity_ = 0;
    std::vector<HaDeckWidget*> widgets_ = {};
};

}  // namespace ha_deck
}  // namespace esphome
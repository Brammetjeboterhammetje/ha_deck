#include "ha_deck_screen.h"

namespace esphome {
namespace ha_deck {

void HaDeckScreen::setup() { 
    
}

void HaDeckScreen::loop() {
    if (active_ && inactivity_ > 0) {
        if (millis() - last_activity_ > inactivity_) {
            ESP_LOGD(TAG, "Screen %s timeout reached", name_.c_str());
            set_active(false);
        }
    }
}

float HaDeckScreen::get_setup_priority() const { return setup_priority::AFTER_CONNECTION; }

void HaDeckScreen::set_name(std::string name) {
    name_ = name;
}
std::string HaDeckScreen::get_name() {
    return name_;
}

void HaDeckScreen::set_inactivity(uint32_t inactivity) {
    if (inactivity > 3600) {  // max 1 hour timeout
        ESP_LOGW(TAG, "Inactivity timeout too large, capping at 3600s");
        inactivity = 3600;
    }
    inactivity_ = inactivity * 1000;
}

uint32_t HaDeckScreen::get_inactivity() {
    return inactivity_;
}

bool HaDeckScreen::add_widget(HaDeckWidget *widget) {
    if (widget == nullptr) {
        ESP_LOGW(TAG, "Attempt to add null widget to screen %s", name_.c_str());
        return false;
    }
    widgets_.push_back(widget);
    return true;
}

void HaDeckScreen::set_active(bool active) {
    ESP_LOGD(this->TAG, "screen %s, set_active: %d", this->get_name().c_str(), active);
    active_ = active;

    for (auto widget : widgets_) {
        if (active) {
            widget->render();
        } else {
            widget->destroy();
        }
    }
}

HaDeckScreen::~HaDeckScreen() {
    // Clean up widgets if needed
    set_active(false);
}

}  // namespace ha_deck
}  // namespace esphome
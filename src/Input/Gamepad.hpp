/**
 * @file Gamepad.hpp
 * @brief Interfaces with physical Xbox-compatible gamepads using the XInput API.
 *
 * This module encapsulates hardware polling, normalizes raw analog signals,
 * and provides bitmask-based state verification for digital buttons.
 */
#pragma once
#include <windows.h>
#include <Xinput.h>

/**
 * @class Gamepad
 * @brief Manages state and hardware communication for a single XInput controller.
 */
class Gamepad
{
public:
    explicit Gamepad(int controller_index = 0);

    bool update();
    bool is_connected() const;

    float get_left_stick_y() const;
    float get_left_stick_x() const;
    float get_right_stick_x() const;
    float get_right_stick_y() const;
    float get_right_trigger() const;
    float get_left_trigger() const;

    bool is_button_pressed(WORD button_mask) const;

private:
    int index;
    bool connected;
    XINPUT_STATE state;

    float apply_deadzone(short raw_value, short deadzone_threshold) const;
};
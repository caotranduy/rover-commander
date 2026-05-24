/**
 * @file Gamepad.cpp
 * @brief Implementation of the Gamepad class.
 */
#include "Gamepad.hpp"
#include <cmath>
#include <algorithm>

Gamepad::Gamepad(int controller_index)
    : index(controller_index), connected(false)
{
    ZeroMemory(&state, sizeof(XINPUT_STATE));
}

bool Gamepad::update()
{
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    DWORD result = XInputGetState(index, &state);

    connected = (result == ERROR_SUCCESS);
    return connected;
}

bool Gamepad::is_connected() const
{
    return connected;
}

float Gamepad::get_left_stick_y() const
{
    if (!connected)
        return 0.0f;
    return apply_deadzone(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
}

float Gamepad::get_left_stick_x() const
{
    if (!connected)
        return 0.0f;
    return apply_deadzone(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
}

float Gamepad::get_right_stick_x() const
{
    if (!connected)
        return 0.0f;
    return apply_deadzone(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
}

float Gamepad::get_right_stick_y() const
{
    if (!connected)
        return 0.0f;
    return apply_deadzone(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
}

float Gamepad::get_right_trigger() const
{
    if (!connected)
        return 0.0f;

    BYTE raw = state.Gamepad.bRightTrigger;
    if (raw < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        return 0.0f;

    return static_cast<float>(raw - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) /
           (255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

float Gamepad::get_left_trigger() const
{
    if (!connected)
        return 0.0f;

    BYTE raw = state.Gamepad.bLeftTrigger;
    if (raw < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        return 0.0f;

    return static_cast<float>(raw - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) /
           (255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

bool Gamepad::is_button_pressed(WORD button_mask) const
{
    if (!connected)
        return false;
    return (state.Gamepad.wButtons & button_mask) != 0;
}

float Gamepad::apply_deadzone(short raw_value, short deadzone_threshold) const
{
    if (std::abs(raw_value) < deadzone_threshold)
    {
        return 0.0f;
    }

    short sign = (raw_value > 0) ? 1 : -1;
    float normalized_magnitude = static_cast<float>(std::abs(raw_value) - deadzone_threshold) /
                                 (32767.0f - deadzone_threshold);

    return std::clamp(normalized_magnitude * sign, -1.0f, 1.0f);
}
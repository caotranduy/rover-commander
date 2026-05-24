/**
 * @file GamepadMapper.cpp
 * @brief Implementation of the JSON-driven Gamepad Mapper.
 */
#include "GamepadMapper.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GamepadMapper::GamepadMapper(std::shared_ptr<CommandDispatcher> dispatcher, const nlohmann::json &config)
    : cmd_dispatcher(std::move(dispatcher)),
      app_config(config),
      current_mode(ManeuverType::FRONT_WHEEL_STEER) // Default routing profile
{
}

int8_t GamepadMapper::to_percent(float raw_value) const
{
    /* Clamp the float safely and scale to a [-100, 100] percentage integer */
    float clamped = std::clamp(raw_value, -1.0f, 1.0f);
    return static_cast<int8_t>(std::round(clamped * 100.0f));
}

WORD GamepadMapper::get_button_mask(const std::string &config_key) const
{
    std::string btn_name = app_config["input_mapping"]["gamepad"]["modes"][config_key];

    /* Naive string-to-macro reflection for dynamic button binding */
    if (btn_name == "DPAD_UP")
        return XINPUT_GAMEPAD_DPAD_UP;
    if (btn_name == "DPAD_DOWN")
        return XINPUT_GAMEPAD_DPAD_DOWN;
    if (btn_name == "DPAD_LEFT")
        return XINPUT_GAMEPAD_DPAD_LEFT;
    if (btn_name == "DPAD_RIGHT")
        return XINPUT_GAMEPAD_DPAD_RIGHT;

    return 0; // Unmapped or invalid
}

float GamepadMapper::get_axis_value(const Gamepad &gamepad, const std::string &config_key) const
{
    std::string axis_name = app_config["input_mapping"]["gamepad"]["axes"][config_key];

    /* Fetch hardware data dynamically based on the string alias */
    if (axis_name == "LEFT_STICK_Y")
        return gamepad.get_left_stick_y();
    if (axis_name == "LEFT_STICK_X")
        return gamepad.get_left_stick_x();

    // Note: You will need to add get_right_stick_x() and get_right_stick_y() to the Gamepad class!
    if (axis_name == "RIGHT_STICK_X")
        return gamepad.get_right_stick_x();
    if (axis_name == "RIGHT_STICK_Y")
        return gamepad.get_right_stick_y();

    return 0.0f;
}

void GamepadMapper::process_input(const Gamepad &gamepad)
{
    /* 1. Evaluate State Transitions (Mode Switching) */
    if (gamepad.is_button_pressed(get_button_mask("select_front_steer")))
    {
        current_mode = ManeuverType::FRONT_WHEEL_STEER;
    }
    else if (gamepad.is_button_pressed(get_button_mask("select_rear_steer")))
    {
        current_mode = ManeuverType::REAR_WHEEL_STEER;
    }
    else if (gamepad.is_button_pressed(get_button_mask("select_crab_walk")))
    {
        current_mode = ManeuverType::CRAB_WALK;
    }
    else if (gamepad.is_button_pressed(get_button_mask("select_point_turn")))
    {
        current_mode = ManeuverType::POINT_TURN;
    }

    /* 2. Execute Kinematics Mapping based on current state */
    switch (current_mode)
    {
    case ManeuverType::FRONT_WHEEL_STEER:
        handle_front_steer(gamepad);
        break;
    case ManeuverType::REAR_WHEEL_STEER:
        handle_rear_steer(gamepad);
        break;
    case ManeuverType::CRAB_WALK:
        handle_crab_walk(gamepad);
        break;
    case ManeuverType::POINT_TURN:
        handle_point_turn(gamepad);
        break;
    }
}

void GamepadMapper::handle_front_steer(const Gamepad &gamepad)
{
    int8_t power = to_percent(get_axis_value(gamepad, "drive_power"));
    int8_t steer = to_percent(get_axis_value(gamepad, "drive_steer"));
    cmd_dispatcher->dispatch_front_steer(power, steer);
}

void GamepadMapper::handle_rear_steer(const Gamepad &gamepad)
{
    int8_t power = to_percent(get_axis_value(gamepad, "drive_power"));
    int8_t steer = to_percent(get_axis_value(gamepad, "drive_steer"));
    cmd_dispatcher->dispatch_rear_steer(power, steer);
}

void GamepadMapper::handle_crab_walk(const Gamepad &gamepad)
{
    /* Retrieve Cartesian coordinates from the right stick */
    float x = get_axis_value(gamepad, "crab_vector_x");
    float y = get_axis_value(gamepad, "crab_vector_y");

    /* Calculate polar magnitude (throttle equivalent) */
    float magnitude = std::sqrt((x * x) + (y * y));
    int8_t packed_mag = to_percent(magnitude);

    /* Calculate polar angle using atan2.
     * atan2(x, y) treats straight up (y>0, x=0) as 0 radians.
     * Output range is [-PI, PI]. Dividing by PI normalizes it to [-1.0, 1.0]. */
    float angle_normalized = 0.0f;
    if (magnitude > 0.1f)
    { // Apply deadzone threshold to prevent division jitter
        angle_normalized = std::atan2(x, y) / M_PI;
    }

    int8_t packed_angle = to_percent(angle_normalized);
    cmd_dispatcher->dispatch_crab_walk(packed_mag, packed_angle);
}

void GamepadMapper::handle_point_turn(const Gamepad &gamepad)
{
    int8_t spin_vel = to_percent(get_axis_value(gamepad, "spin_velocity"));
    cmd_dispatcher->dispatch_point_turn(spin_vel);
}
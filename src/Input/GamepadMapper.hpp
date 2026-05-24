/**
 * @file GamepadMapper.hpp
 * @brief Translates raw XInput gamepad state into semantic intent commands.
 *
 * Utilizes dynamic JSON configuration to map physical inputs to abstract axes
 * and maneuver modes. Performs trigonometric transformations (e.g., Cartesian to Polar)
 * for advanced kinematics like Crab Walk, pushing results to the Command Dispatcher.
 */
#pragma once
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "Input/Gamepad.hpp"
#include "Network/CommandDispatcher.hpp"
#include "DTO/Command.hpp"

class GamepadMapper
{
public:
    /**
     * @brief Constructs the mapper with required dependencies.
     * @param dispatcher Pointer to the semantic command dispatcher (the "nm" module).
     * @param config Parsed JSON object containing the "input_mapping" block.
     */
    GamepadMapper(std::shared_ptr<CommandDispatcher> dispatcher, const nlohmann::json &config);

    /**
     * @brief Reads the current physical gamepad state and dispatches the corresponding maneuver.
     * @param gamepad Constant reference to the updated Gamepad hardware object.
     */
    void process_input(const Gamepad &gamepad);

private:
    std::shared_ptr<CommandDispatcher> cmd_dispatcher;
    nlohmann::json app_config;
    ManeuverType current_mode;

    /* Utility function to convert semantic -1.0 to 1.0 float ranges to -100 to 100 integer percentages */
    int8_t to_percent(float raw_value) const;

    /* Configuration parsers */
    WORD get_button_mask(const std::string &config_key) const;
    float get_axis_value(const Gamepad &gamepad, const std::string &config_key) const;

    /* Dedicated maneuver handlers */
    void handle_front_steer(const Gamepad &gamepad);
    void handle_rear_steer(const Gamepad &gamepad);
    void handle_crab_walk(const Gamepad &gamepad);
    void handle_point_turn(const Gamepad &gamepad);
};
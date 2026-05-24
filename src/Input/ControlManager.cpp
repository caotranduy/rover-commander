/**
 * @file ControlManager.cpp
 * @brief Implementation of the input management and orchestration layer.
 */
#include "ControlManager.hpp"
#include <iostream>

ControlManager::ControlManager(std::shared_ptr<SystemState> state, const nlohmann::json &config)
    : system_state(std::move(state)),
      app_config(config),
      current_mode(ControlMode::GAMEPAD), // Set Gamepad as the default profile upon launch
      gamepad(0)                          // Initialize hardware connection binding to slot 0
{
    /* Instantiate the internal semantic command dispatcher (the "nm" module) */
    dispatcher = std::make_shared<CommandDispatcher>(system_state);

    /* Construct the gamepad mapper, injecting its structural network and configuration dependencies */
    gamepad_mapper = std::make_unique<GamepadMapper>(dispatcher, app_config);
}

void ControlManager::set_control_mode(ControlMode mode)
{
    /* Critical: Force a full safety halt before re-wiring input streams to avoid legacy kinetic signals */
    dispatcher->dispatch_emergency_stop();

    current_mode = mode;

    std::cout << "[ControlManager] Active control profile mutated to: "
              << (mode == ControlMode::GAMEPAD ? "GAMEPAD" : "KEYBOARD_MOUSE") << "\n";
}

ControlMode ControlManager::get_current_mode() const
{
    return current_mode;
}

void ControlManager::update()
{
    if (current_mode == ControlMode::GAMEPAD)
    {
        /* Poll hardware registers and check for physical connection stability */
        if (!gamepad.update() || !gamepad.is_connected())
        {
            handle_gamepad_failsafe();
            return;
        }

        /* Hardware state is secure, pass raw buffers to the semantic translator */
        gamepad_mapper->process_input(gamepad);
    }
    else
    {
        /* TODO: Invoke KeyboardMapper processing mechanics here when ready */

        /* Placeholder fallback to ensure the rover does not drift blindly in unmapped states */
        dispatcher->dispatch_emergency_stop();
    }
}

void ControlManager::handle_gamepad_failsafe()
{
    std::cerr << "[CRITICAL FAILSAFE] Connection to Gamepad lost! Engaging immediate braking.\n";

    /* Inject an override emergency stop command directly to the SystemState */
    dispatcher->dispatch_emergency_stop();

    /* Migrate state to Keyboard/Mouse profile to unlock alternative operator intervention */
    set_control_mode(ControlMode::KEYBOARD_MOUSE);
}
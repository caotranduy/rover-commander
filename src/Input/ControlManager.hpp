/**
 * @file ControlManager.hpp
 * @brief Orchestrates input mappers, device lifecycles, and safety failovers.
 *
 * Manages active input states (Gamepad vs Keyboard) and automatically triggers
 * emergency procedures if a hardware disconnect event occurs during runtime.
 */
#pragma once
#include <memory>
#include <nlohmann/json.hpp>
#include "SystemState.hpp"
#include "Input/Gamepad.hpp"
#include "Input/GamepadMapper.hpp"
#include "Network/CommandDispatcher.hpp"

/**
 * @brief Supported human interface device categories.
 */
enum class ControlMode
{
    GAMEPAD,
    KEYBOARD_MOUSE
};

class ControlManager
{
public:
    /**
     * @brief Constructs the input orchestrator.
     * @param state Shared pointer to the centralized thread-safe SystemState.
     * @param config Reference to the parsed application configuration JSON.
     */
    ControlManager(std::shared_ptr<SystemState> state, const nlohmann::json &config);

    /**
     * @brief High-frequency polling method to be executed by the main execution loop.
     */
    void update();

    /**
     * @brief Hot-swaps the active input source and updates state machines safely.
     * @param mode Target control method to switch to.
     */
    void set_control_mode(ControlMode mode);

    /**
     * @brief Inspects the active input mode.
     */
    ControlMode get_current_mode() const;

private:
    std::shared_ptr<SystemState> system_state;
    std::shared_ptr<CommandDispatcher> dispatcher; // Semantic network API instance (the "nm" module)
    nlohmann::json app_config;

    ControlMode current_mode;
    Gamepad gamepad;
    std::unique_ptr<GamepadMapper> gamepad_mapper; // Isolated translation layer for XInput

    /**
     * @brief Safely halts rover operations and fallbacks to secondary control structures.
     */
    void handle_gamepad_failsafe();
};
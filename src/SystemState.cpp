/**
 * @file SystemState.cpp
 * @brief Implementation of the thread-safe SystemState class for intent commands.
 */
#include "SystemState.hpp"
#include <cstring>

SystemState::SystemState()
{
    /* Zero-initialize the structure memory for safe initial state */
    std::memset(&current_cmd, 0, sizeof(RoverIntentCommand));

    /* Enforce default routing to the chassis base actor */
    current_cmd.actor_id = 0x01;

    /* Set the default driving profile to standard Ackermann steering */
    current_cmd.maneuver_type = static_cast<uint8_t>(ManeuverType::FRONT_WHEEL_STEER);
}

void SystemState::set_rover_command(const RoverIntentCommand &cmd)
{
    /* Acquire lock to eliminate read-write data races */
    std::lock_guard<std::mutex> lock(state_mutex);

    current_cmd = cmd;

    /* Safeguard actor mapping against external input corruption */
    current_cmd.actor_id = 0x01;
}

RoverIntentCommand SystemState::get_rover_command() const
{
    /* Acquire lock to ensure atomic copy of the internal struct */
    std::lock_guard<std::mutex> lock(state_mutex);

    return current_cmd;
}
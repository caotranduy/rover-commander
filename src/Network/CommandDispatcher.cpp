/**
 * @file CommandDispatcher.cpp
 * @brief Implementation of the semantic Command Dispatcher.
 */
#include "CommandDispatcher.hpp"
#include <algorithm>

CommandDispatcher::CommandDispatcher(std::shared_ptr<SystemState> state)
    : system_state(std::move(state)) {}

WheelActorCommand CommandDispatcher::build_base_dto(ManeuverType type, int8_t throttle, int8_t steering)
{
    WheelActorCommand cmd;

    // Clear memory and set static identifiers
    std::memset(&cmd, 0, sizeof(WheelActorCommand));
    cmd.actor_id = 0x01;
    cmd.emergency_brake = 0;

    // Set maneuver enum
    cmd.maneuver_type = static_cast<uint8_t>(type);

    // Clamp semantic values to strict [-100, 100] ranges to prevent integer overflow
    cmd.throttle = static_cast<int8_t>(std::clamp(static_cast<int>(throttle), -100, 100));
    cmd.steering = static_cast<int8_t>(std::clamp(static_cast<int>(steering), -100, 100));

    return cmd;
}

void CommandDispatcher::dispatch_front_steer(int8_t power, int8_t steer)
{
    auto cmd = build_base_dto(ManeuverType::FRONT_WHEEL_STEER, power, steer);
    system_state->set_actor_command(cmd);
}

void CommandDispatcher::dispatch_rear_steer(int8_t power, int8_t steer)
{
    auto cmd = build_base_dto(ManeuverType::REAR_WHEEL_STEER, power, steer);
    system_state->set_actor_command(cmd);
}

void CommandDispatcher::dispatch_crab_walk(int8_t magnitude, int8_t vector_angle)
{
    // For Crab Walk:
    // Throttle holds the movement speed (vector magnitude).
    // Steering holds the target direction (-100 to 100 maps to -180 to 180 degrees at the Brain).
    auto cmd = build_base_dto(ManeuverType::CRAB_WALK, magnitude, vector_angle);
    system_state->set_actor_command(cmd);
}

void CommandDispatcher::dispatch_point_turn(int8_t spin_velocity)
{
    // For Point Turn:
    // Throttle is forced to 0 (no forward movement).
    // Steering determines rotation direction and speed.
    auto cmd = build_base_dto(ManeuverType::POINT_TURN, 0, spin_velocity);
    system_state->set_actor_command(cmd);
}

void CommandDispatcher::dispatch_emergency_stop()
{
    auto cmd = build_base_dto(ManeuverType::FRONT_WHEEL_STEER, 0, 0);
    cmd.emergency_brake = 1; // Trigger hardware halt flag
    system_state->set_actor_command(cmd);
}
/**
 * @file CommandDispatcher.hpp
 * @brief Semantic API for generating and dispatching intent-based rover commands.
 *
 * Exposes high-level maneuver functions (e.g., Crab Walk, Point Turn). Translates
 * clamped abstract values (-100 to 100) into packed DTOs and injects them into
 * the thread-safe SystemState for asynchronous UDP transmission.
 */
#pragma once
#include <memory>
#include <cstdint>
#include "SystemState.hpp"
#include "../DTO/Command.hpp"

class CommandDispatcher
{
public:
    explicit CommandDispatcher(std::shared_ptr<SystemState> state);

    /* Standard driving with front wheels steering */
    void dispatch_front_steer(int8_t power, int8_t steer);

    /* Forklift-style driving with rear wheels steering */
    void dispatch_rear_steer(int8_t power, int8_t steer);

    /* Omni-directional translation based on a 2D vector */
    void dispatch_crab_walk(int8_t magnitude, int8_t vector_angle);

    /* Zero-radius rotation in place */
    void dispatch_point_turn(int8_t spin_velocity);

    /* Immediate physical and software halt */
    void dispatch_emergency_stop();

private:
    std::shared_ptr<SystemState> system_state;

    /* Helper to build the base structure and enforce limits */
    WheelActorCommand build_base_dto(ManeuverType type, int8_t throttle, int8_t steering);
};
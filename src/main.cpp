/**
 * @file main.cpp
 * @brief Entry point for the Rover Commander application.
 *
 * Initializes the dependency graph, spawns the network transceiver thread,
 * and executes the primary 50Hz input polling loop.
 */
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <string>

#include "ConfigLoader.hpp"
#include "SystemState.hpp"
#include "Input/ControlManager.hpp"
#include "Network/UdpTransceiver.hpp"

#define XINPUT_CONFIG_PATH "config/gamepad_xinput_mapper.json"

/**
 * @brief Helper function to stringify the semantic maneuver type for console debugging.
 */
const char *maneuver_to_string(ManeuverType type)
{
    switch (type)
    {
    case ManeuverType::FRONT_WHEEL_STEER:
        return "FRONT_STEER";
    case ManeuverType::REAR_WHEEL_STEER:
        return "REAR_STEER ";
    case ManeuverType::CRAB_WALK:
        return "CRAB_WALK  ";
    case ManeuverType::POINT_TURN:
        return "POINT_TURN ";
    default:
        return "UNKNOWN    ";
    }
}

int main()
{
    std::cout << "Booting Rover Commander...\n";

    try
    {
        /* 1. Load dynamic configuration */
        auto config = ConfigLoader::load(XINPUT_CONFIG_PATH);
        std::string target_ip = config["network"]["rover_ip"];
        int target_port = config["network"]["udp_port"];

        /* 2. Initialize the thread-safe centralized state manager */
        auto system_state = std::make_shared<SystemState>();

        /* 3. Initialize and start the background UDP networking thread */
        UdpTransceiver transceiver(target_ip, target_port, 8081, system_state);
        transceiver.start();

        /* 4. Initialize the input orchestration layer */
        ControlManager control_manager(system_state, config);

        std::cout << "System online. Polling input at 50Hz. Press Ctrl+C to exit.\n";
        std::cout << "=============================================================\n";

        /* 5. Primary execution loop */
        while (true)
        {
            /* Tick the control manager to read hardware and map kinematics */
            control_manager.update();

            /* Fetch the latest packed DTO from the state manager for rendering */
            RoverIntentCommand cmd = system_state->get_rover_command();

            /* Output the semantic command structure (Overwrites the same line using \r) */
            printf("\r[Mode: %s] | Throttle: %4d | Steer: %4d | E-Brake: %d | UDP Seq: %6u",
                   maneuver_to_string(static_cast<ManeuverType>(cmd.maneuver_type)),
                   cmd.throttle,
                   cmd.steering,
                   cmd.emergency_brake,
                   cmd.header.sequence_num);

            /* Sleep to enforce a ~50Hz deterministic loop cycle (20ms) */
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n[FATAL SYSTEM ERROR] " << e.what() << "\n";
        return -1;
    }

    return 0;
}
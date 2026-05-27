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

#define XINPUT_CONFIG_PATH "E:\\.rover-rasberry\\rover-comander\\config\\gamepad_xinput_mapper.json"

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
        auto config = ConfigLoader::load(XINPUT_CONFIG_PATH);
        std::string target_ip = config["network"]["rover_ip"];
        int target_port = config["network"]["udp_port"];

        auto system_state = std::make_shared<SystemState>();

        /* Khởi tạo hạ tầng mạng */
        UdpTransceiver transceiver(target_ip, target_port, 8081, system_state);

        /* THỰC THI KỊCH BẢN HANDSHAKE NGAY ĐẦU VÒNG ĐỜI */
        if (!transceiver.perform_handshake())
        {
            std::cerr << "[FATAL] Handshake protocol failed. Terminating system application.\n";
            return -1; // Ngắt chương trình ngay tại đây nếu không xác thực được xe
        }

        /* Xác thực thành công -> Kích hoạt luồng chạy ngầm đa luồng 50Hz */
        transceiver.start();

        ControlManager control_manager(system_state, config);
        std::cout << "System online. Entering deterministic 50Hz control loop...\n";
        std::cout << "=============================================================\n";

        /* Vòng lặp test hiện tại (Giữ nguyên logic cũ của bạn) */
        while (true)
        {
            control_manager.update();
            auto commands = system_state->get_all_commands();

            for (const auto &cmd : commands)
            {
                if (cmd.actor_id == 0x01)
                {
                    printf("\r[Mode: %s] | Throttle: %3d | Steer: %3d | E-Brake: %d | UDP Seq: %6u",
                           maneuver_to_string(static_cast<ManeuverType>(cmd.maneuver_type)),
                           cmd.throttle,
                           cmd.steering,
                           cmd.emergency_brake,
                           cmd.header.sequence_num);
                    break;
                }
            }
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
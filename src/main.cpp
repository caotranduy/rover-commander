/**
 * @file main.cpp
 * @brief Entry point for the Rover Commander application.
 *
 * Initializes the dependency graph, executes secure handshake protocols,
 * spawns the background GStreamer pipeline and Node.js Web bridge, and
 * orchestrates the deterministic 50Hz input polling loop.
 */
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>
#include <shellapi.h> // Required for ShellExecuteA (opening the browser)

#include "ConfigLoader.hpp"
#include "SystemState.hpp"
#include "Input/ControlManager.hpp"
#include "Network/UdpTransceiver.hpp"
#include "Video/VideoManager.hpp"

#define XINPUT_CONFIG_PATH ".\\config\\gamepad_xinput_mapper.json"
#define WEB_SERVER_SCRIPT_PATH ".\\WebUI\\server.js"
#define WEB_HTML_UI_PATH ".\\WebUI\\index.html"

/**
 * @brief Helper function to stringify the semantic maneuver type for console debugging.
 * * @param type The operational maneuver enum.
 * @return const char* String representation padded for aligned console output.
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

/**
 * @brief Spawns the Node.js WebUI bridge server as a hidden child process.
 * * @param pi Reference to the PROCESS_INFORMATION structure to receive process handles.
 * @retval true If Node.js server was successfully created.
 * @retval false If initialization failed (e.g., node executable not found).
 */
bool spawn_node_bridge(PROCESS_INFORMATION &pi)
{
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Keep the Node.js console window hidden

    std::string cmd = "node.exe " + std::string(WEB_SERVER_SCRIPT_PATH);
    std::vector<char> cmd_buffer(cmd.begin(), cmd.end());
    cmd_buffer.push_back('\0');

    if (!CreateProcessA(NULL, cmd_buffer.data(), NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        std::cerr << "[Main Orchestrator] ERROR: Failed to spawn Node.js Web bridge. OS Error: "
                  << GetLastError() << "\n";
        return false;
    }

    std::cout << "[Main Orchestrator] Node.js WebSocket Server spawned ngầm thành công.\n";
    return true;
}

int main()
{
    std::cout << "Booting Rover Commander...\n";

    // Track OS handles for the Node.js process to ensure clean exit resource harvesting
    PROCESS_INFORMATION node_process_info;
    ZeroMemory(&node_process_info, sizeof(node_process_info));
    bool is_node_running = false;

    try
    {
        auto config = ConfigLoader::load(XINPUT_CONFIG_PATH);
        std::string target_ip = config["network"]["rover_ip"];
        int target_port = config["network"]["udp_port"];

        auto system_state = std::make_shared<SystemState>();

        /* 1. INITIALIZE NETWORK LAYER */
        UdpTransceiver transceiver(target_ip, target_port, 8081, system_state);

        /* 2. EXECUTE SECURE HANDSHAKE AT START OF LIFECYCLE */
        if (!transceiver.perform_handshake())
        {
            std::cerr << "[FATAL] Handshake protocol failed. Terminating system application.\n";
            return -1;
        }

        /* 3. VERIFIED CONNECTION -> SPIN UP MEDIA STREAMING ENGINE */
        // Intercepts UDP port 5000 from Rover and relays to internal TCP port 5001
        VideoManager video_manager(50001, 50002);
        if (!video_manager.start())
        {
            std::cerr << "[WARNING] GStreamer pipeline failed. Video streaming unavailable.\n";
        }

        /* 4. SPIN UP NODE.JS WEBSOCKET BRIDGE CLIENT */
        is_node_running = spawn_node_bridge(node_process_info);

        /* 5. AUTO-OPEN THE WEB UI IN THE DEFAULT BROWSER */
        // ShellExecuteA commands Windows OS to handle the index.html with its registered browser
        ShellExecuteA(NULL, "open", WEB_HTML_UI_PATH, NULL, NULL, SW_SHOWNORMAL);
        std::cout << "[Main Orchestrator] Triggered Default Browser to open Web UI.\n";

        /* 6. START MULTI-THREADED ASYNCHRONOUS NETWORK TRANSMISSION (50Hz) */
        transceiver.start();

        ControlManager control_manager(system_state, config);
        std::cout << "System online. Entering deterministic 50Hz control loop...\n";
        std::cout << "=============================================================\n";

        /* 7. PRIMARY POLLING ENGINE */
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

        // Resource Cleanup during panic/unwind states
        if (is_node_running)
        {
            TerminateProcess(node_process_info.hProcess, 0);
            CloseHandle(node_process_info.hProcess);
            CloseHandle(node_process_info.hThread);
        }
        return -1;
    }

    // Standard Graceful Shutdown Resource Harvesting
    if (is_node_running)
    {
        TerminateProcess(node_process_info.hProcess, 0);
        CloseHandle(node_process_info.hProcess);
        CloseHandle(node_process_info.hThread);
    }
    return 0;
}
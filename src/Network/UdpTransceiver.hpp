/**
 * @file UdpTransceiver.hpp
 * @brief Asynchronous UDP networking module for the Rover Commander.
 *
 * Handles continuous 50Hz transmission of control DTOs to the Rover Brain
 * and asynchronous reception of telemetry data, utilizing asio's io_context.
 */
#pragma once
#include <asio.hpp>
#include <string>
#include <atomic>
#include <memory>
#include <thread>
#include <array>
#include "DTO/Command.hpp"
#include "SystemState.hpp"

class UdpTransceiver
{
public:
    /**
     * @brief Initializes the UDP Transceiver.
     * @param target_ip IP address of the Rover Brain.
     * @param target_port UDP Port of the Rover Brain.
     * @param local_port Local port to bind for receiving telemetry.
     * @param state Shared pointer to the thread-safe SystemState.
     */
    UdpTransceiver(const std::string &target_ip, int target_port, int local_port,
                   std::shared_ptr<SystemState> state);
    ~UdpTransceiver();

    void start();
    void stop();

private:
    void start_transmit_timer();
    void start_receive();

    std::shared_ptr<SystemState> system_state; // Shared thread-safe state manager

    asio::io_context io_context;
    asio::ip::udp::socket socket;
    asio::ip::udp::endpoint target_endpoint;
    asio::steady_timer transmit_timer;

    std::atomic<uint32_t> sequence_counter; // Replay attack protection counter
    std::atomic<bool> is_running;           // Thread safety flag for the I/O loop

    std::array<uint8_t, 1024> recv_buffer;   // Buffer for incoming telemetry packets
    asio::ip::udp::endpoint remote_endpoint; // Remote sender endpoint info

    std::thread io_thread; // Dedicated thread for asynchronous network polling
};
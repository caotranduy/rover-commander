/**
 * @file UdpTransceiver.cpp
 * @brief Implementation of the asynchronous UDP Transceiver.
 */
#include "UdpTransceiver.hpp"
#include <iostream>

#define ROVER_MAGIC_KEY 0x524F5652 // ASCII characters for "ROVR"

UdpTransceiver::UdpTransceiver(const std::string &target_ip, int target_port, int local_port,
                               std::shared_ptr<SystemState> state)
    : system_state(std::move(state)),
      socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), local_port)),
      transmit_timer(io_context),
      sequence_counter(0),
      is_running(false)
{
    // Resolve target endpoint destination
    target_endpoint = asio::ip::udp::endpoint(asio::ip::make_address(target_ip), target_port);
}

UdpTransceiver::~UdpTransceiver()
{
    stop();
}

void UdpTransceiver::start()
{
    if (is_running.exchange(true))
        return; // Prevent double initialization

    start_transmit_timer();
    start_receive();

    // Run the Asio event loop inside a dedicated background thread
    io_thread = std::thread([this]()
                            { io_context.run(); });
}

void UdpTransceiver::stop()
{
    if (!is_running.exchange(false))
        return; // Avoid stopping if not active

    io_context.stop(); // Clear outstanding asynchronous handlers
    if (io_thread.joinable())
    {
        io_thread.join(); // Gracefully synchronize thread destruction
    }
}

void UdpTransceiver::start_transmit_timer()
{
    if (!is_running)
        return;

    // Schedule next transmission cycle with 20ms offset (50Hz frequency)
    transmit_timer.expires_after(std::chrono::milliseconds(20));

    transmit_timer.async_wait([this](const asio::error_code &error)
                              {
        if (!error && is_running) {
            
            // Fetch the latest thread-safe semantic command via abstract type alias
            WheelActorCommand cmd = system_state->get_rover_command();
            
            // Populate the security block before packing over the network
            cmd.header.magic_key = ROVER_MAGIC_KEY;
            cmd.header.sequence_num = ++sequence_counter;

            // Stream the fixed-size binary structure to the socket interface
            asio::error_code send_error;
            socket.send_to(asio::buffer(&cmd, sizeof(WheelActorCommand)), target_endpoint, 0, send_error);

            if (send_error) {
                std::cerr << "[UDP TX Error] " << send_error.message() << "\n";
            }

            // Loop back asynchronously to maintain the fixed transmission rate
            start_transmit_timer();
        } });
}

void UdpTransceiver::start_receive()
{
    if (!is_running)
        return;

    // Listen for telemetry notifications from the edge device non-blockingly
    socket.async_receive_from(
        asio::buffer(recv_buffer), remote_endpoint,
        [this](const asio::error_code &error, std::size_t bytes_recvd)
        {
            if (!error && is_running)
            {

                // IP Whitelisting: Isolate and drop external threat networks
                if (remote_endpoint.address() == target_endpoint.address())
                {
                    // Packet verified from valid source endpoint. Telemetry parsing logic goes here.
                }
            }

            // Queue up the next receive state handler immediately
            start_receive();
        });
}
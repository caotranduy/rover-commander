/**
 * @file UdpTransceiver.cpp
 * @brief Implementation of the asynchronous UDP Transceiver.
 */
#include "UdpTransceiver.hpp"
#include <iostream>

#define ROVER_MAGIC_KEY 0x524F5652 // ASCII characters for "ROVR"

UdpTransceiver::UdpTransceiver(const std::string &target_ip, int target_port, int local_port, std::shared_ptr<SystemState> state)
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

    transmit_timer.expires_after(std::chrono::milliseconds(20));

    transmit_timer.async_wait([this](const asio::error_code &error)
                              {
        if (!error && is_running) {
            
            /* Lấy bản sao danh sách toàn bộ các lệnh đang Active từ mọi Actor */
            auto active_commands = system_state->get_all_commands();
            
            /* Duyệt qua từng lệnh và nã đạn UDP độc lập */
            for (auto& cmd : active_commands) {
                // Đóng dấu bảo mật và cấp số thứ tự ĐỘC LẬP cho từng gói tin
                cmd.header.magic_key = ROVER_MAGIC_KEY;
                cmd.header.sequence_num = ++sequence_counter;

                asio::error_code send_error;
                socket.send_to(asio::buffer(&cmd, sizeof(WheelActorCommand)), target_endpoint, 0, send_error);

                if (send_error) {
                    std::cerr << "[UDP TX Error] Actor ID 0x" << std::hex << (int)cmd.actor_id 
                              << ": " << send_error.message() << std::dec << "\n";
                }
            }

            // Lặp lại chu kỳ 50Hz
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

bool UdpTransceiver::perform_handshake()
{
    /* 1. Đóng gói cấu trúc gói tin bắt tay */
    RoverHandshakePacket handshake_packet;
    std::memset(&handshake_packet, 0, sizeof(RoverHandshakePacket));
    handshake_packet.handshake_key = 0x484E4453; // Set token pattern

    /* 2. Bắn phát súng kích hoạt đến IP của xe */
    asio::error_code send_error;
    socket.send_to(asio::buffer(&handshake_packet, sizeof(RoverHandshakePacket)), target_endpoint, 0, send_error);

    if (send_error)
    {
        std::cerr << "[Handshake Error] Failed to send token: " << send_error.message() << "\n";
        return false;
    }

    std::cout << "[Handshake] Token dispatched. Awaiting Brain validation (3s timeout)...\n";

    /* 3. Thiết lập biến trạng thái và bộ hẹn giờ Timeout 3 giây */
    bool handshake_success = false;
    asio::steady_timer timeout_timer(io_context);
    timeout_timer.expires_after(std::chrono::seconds(3));

    // Nếu hết 3 giây mà chưa nhận được gói tin, hủy bỏ hành động lắng nghe của Socket
    timeout_timer.async_wait([&](const asio::error_code &error)
                             {
        if (!error) {
            std::cerr << "[Handshake Timeout] Rover Brain did not respond within 3 seconds.\n";
            socket.cancel(); 
        } });

    /* 4. Đăng ký luồng hứng gói tin phản hồi (Echo) từ xe dội về */
    RoverHandshakePacket response_packet;
    asio::ip::udp::endpoint sender_endpoint;

    socket.async_receive_from(
        asio::buffer(&response_packet, sizeof(RoverHandshakePacket)), sender_endpoint,
        [&](const asio::error_code &error, std::size_t bytes_recvd)
        {
            if (!error)
            {
                /* Tắt bộ đếm thời gian lùi vì đã nhận được hàng */
                timeout_timer.cancel();

                /* Kiểm tra tính toàn vẹn: Đúng kích thước và đúng Key xác thực */
                if (bytes_recvd == sizeof(RoverHandshakePacket) && response_packet.handshake_key == 0x484E4453)
                {
                    std::cout << "[Handshake Secured] Verified connection with Brain at: "
                              << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << "\n";
                    handshake_success = true;
                }
                else
                {
                    std::cerr << "[Handshake Malformed] Packet signature validation failed.\n";
                }
            }
        });

    /* 5. Chạy luồng khóa đồng bộ tạm thời cho đến khi 1 trong 2 Handler (nhận được hoặc hết giờ) hoàn thành */
    io_context.run();

    /* Cực kỳ quan trọng: Reset lại trạng thái để io_context có thể tái sử dụng cho luồng background sau này */
    io_context.restart();

    return handshake_success;
}
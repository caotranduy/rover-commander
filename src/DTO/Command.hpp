/**
 * @file Command.hpp
 * @brief Defines the semantic data transfer objects (DTOs) and network protocol structures.
 *
 * This file contains tightly packed binary structures used for UDP transmission
 * between the Rover Commander and the Rover Brain. It employs a semantic intent-based
 * approach rather than explicit hardware angles, ensuring loose coupling.
 */
#pragma once
#include <cstdint>

#pragma pack(push, 1)

/**
 * @brief Header block for all UDP packets to ensure security and mitigate replay attacks.
 */
struct UdpSecurityHeader
{
    uint32_t magic_key;    // Mật khẩu tĩnh (ví dụ: 0x524F5652 - "ROVR") để lọc rác UDP.
    uint32_t sequence_num; // Số thứ tự gói tin. RB sẽ từ chối nếu số này < số cũ (Chống Replay Attack).
};

/**
 * @brief Enumeration of supported kinematic maneuvers.
 */
enum class ManeuverType : uint8_t
{
    FRONT_WHEEL_STEER = 0, // Lái như ô tô bình thường (Chạy tốc độ cao)
    REAR_WHEEL_STEER = 1,  // Lái bằng bánh sau, văng đuôi (Dùng để căn chỉnh đầu xe)
    CRAB_WALK = 2,         // Đi ngang như cua (Cả 6 bánh cùng bẻ một góc)
    POINT_TURN = 3         // Xoay tròn tại chỗ (Bánh trái tiến, bánh phải lùi)
};

/**
 * @brief Payload conveying the commander's high-level intent to the rover.
 */
struct RoverIntentCommand
{
    UdpSecurityHeader header; // Nhúng Header bảo mật vào đầu gói tin (Bắt buộc)

    uint8_t actor_id = 0x01; // Bắt buộc là 0x01 (Định danh cho Khung gầm/Base)
    uint8_t maneuver_type;   // Lấy từ enum ManeuverType ở trên

    //  dùng int8_t (-100 đến 100):
    // - Dấu âm: Lùi / Rẽ Trái
    // - Dấu dương: Tiến / Rẽ Phải
    // - Số 0: Đứng im / Thẳng lái
    int8_t throttle; // Phần trăm ga: -100 (lùi tối đa) đến 100 (tiến tối đa)
    int8_t steering; // Phần trăm góc bẻ lái: -100 (trái tối đa) đến 100 (phải tối đa)

    uint8_t emergency_brake; // Cờ phanh khẩn cấp: 1 = Phanh cứng, 0 = Bình thường
};
typedef RoverIntentCommand WheelActorCommand;

struct RoverHandshakePacket
{
    uint32_t handshake_key; ///< Authentication token passphrase.
    uint8_t reserved[8];    ///< Padding bytes for size matching and future telemetry.
};

#pragma pack(pop)
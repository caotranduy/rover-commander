#pragma once
#include <mutex>
#include <unordered_map>
#include <vector>
#include "DTO/Command.hpp"

class SystemState
{
public:
    SystemState() = default;

    /**
     * @brief Cập nhật hoặc thêm mới lệnh cho một Actor cụ thể.
     * Tự động trích xuất actor_id từ bên trong struct truyền vào.
     */
    void set_actor_command(const RoverIntentCommand &cmd);

    /**
     * @brief Lấy danh sách toàn bộ các lệnh đang active của tất cả Actor.
     * @return Một vector chứa bản sao an toàn của các gói DTO.
     */
    std::vector<RoverIntentCommand> get_all_commands() const;

private:
    /* Dictionary lưu trữ trạng thái: Key = Actor ID, Value = Command DTO */
    std::unordered_map<uint8_t, RoverIntentCommand> actor_commands;
    mutable std::mutex state_mutex;
};
#include "SystemState.hpp"

void SystemState::set_actor_command(const RoverIntentCommand &cmd)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    /* Ghi đè (hoặc tạo mới) lệnh vào dictionary dựa trên ID của Actor */
    actor_commands[cmd.actor_id] = cmd;
}

std::vector<RoverIntentCommand> SystemState::get_all_commands() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    std::vector<RoverIntentCommand> snapshot;
    snapshot.reserve(actor_commands.size()); // Tối ưu hóa cấp phát bộ nhớ

    /* Trích xuất toàn bộ value từ dictionary đẩy vào vector */
    for (const auto &pair : actor_commands)
    {
        snapshot.push_back(pair.second);
    }

    return snapshot;
}
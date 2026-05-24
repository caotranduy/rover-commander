cmake_minimum_required(VERSION 3.20)
project(RoverCommander VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Tìm kiếm các thư viện từ vcpkg
find_package(asio CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 2. KHAI BÁO TARGET VÀ LIỆT KÊ MÃ NGUỒN (Phải đặt ở đây)
# Lưu ý: Không cần liệt kê ConfigLoader.hpp hay Command.hpp vì chúng là header
add_executable(RoverCommander
    src/main.cpp
    src/SystemState.cpp
    src/Input/Gamepad.cpp
    src/Input/GamepadMapper.cpp
    src/Input/ControlManager.cpp
    src/Network/CommandDispatcher.cpp
    src/Network/UdpTransceiver.cpp
)

# 3. Cấu hình Include path cho target
target_include_directories(RoverCommander PRIVATE src)

# 4. Cấu hình macro (Bắt buộc cho ASIO trên Windows)
target_compile_definitions(RoverCommander PRIVATE ASIO_STANDALONE WIN32_LEAN_AND_MEAN)

# 5. Liên kết các thư viện vào target
target_link_libraries(RoverCommander 
    PRIVATE 
    asio::asio
    nlohmann_json::nlohmann_json
    ws2_32
    xinput
)
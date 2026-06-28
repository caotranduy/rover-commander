/**
 * @file VideoManager.cpp
 * @brief Implementation of the VideoManager class.
 */

#include "VideoManager.hpp"
#include <iostream>
#include <vector>

#define GSTREAM_EXE "E:\\.rover-rasberry\\gstream\\msvc_x86_64\\bin\\gst-launch-1.0.exe"

VideoManager::VideoManager(int udp_port, int tcp_port)
    : m_udp_port(udp_port), m_tcp_port(tcp_port), m_is_running(false)
{
    // Initialize the PROCESS_INFORMATION memory block to zero
    ZeroMemory(&m_process_info, sizeof(m_process_info));
}

VideoManager::~VideoManager()
{
    // Ensure the background process is terminated upon object destruction
    stop();
}

bool VideoManager::start()
{
    if (m_is_running)
    {
        return true; // Pipeline is already active
    }

    /* Configure startup info to hide the console window completely */
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    /* * Construct the GStreamer command line string.
     * Pipeline logic: UDP source -> Strip RTP -> Parse H264 -> Byte-stream -> TCP Server Sink
     */
    std::string pipeline_cmd =
        std::string(GSTREAM_EXE) +
        " --gst-plugin-path=.\\gstream\\lib\\gstreamer-1.0" + // Ép đường dẫn plugin
        " udpsrc port=" + std::to_string(m_udp_port) +
        " caps=\"application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96\""
        " ! rtph264depay ! h264parse ! video/x-h264, stream-format=byte-stream"
        " ! udpsink host=127.0.0.1 port=" +
        std::to_string(m_tcp_port);

    /* * Windows CreateProcessA API requires a mutable character buffer.
     * We convert the std::string into a std::vector<char> and append a null terminator.
     */
    std::vector<char> cmd_buffer(pipeline_cmd.begin(), pipeline_cmd.end());
    cmd_buffer.push_back('\0');

    /* Spawn the hidden background process */
    if (!CreateProcessA(
            NULL,              // Application name (NULL to use command line buffer)
            cmd_buffer.data(), // Command line string
            NULL,              // Process handle not inheritable
            NULL,              // Thread handle not inheritable
            FALSE,             // Set handle inheritance to FALSE
            CREATE_NO_WINDOW,  // Creation flags: Do not create a console window
            NULL,              // Use parent's environment block
            NULL,              // Use parent's starting directory
            &si,               // Pointer to STARTUPINFO structure
            &m_process_info    // Pointer to PROCESS_INFORMATION structure (stores handles)
            ))
    {
        std::cerr << "[VideoManager] FATAL: Failed to start GStreamer pipeline. OS Error Code: "
                  << GetLastError() << "\n";
        return false;
    }

    m_is_running = true;
    std::cout << "[VideoManager] GStreamer Pipeline Active. [UDP: " << m_udp_port
              << " -> TCP: " << m_tcp_port << "]\n";
    return true;
}

void VideoManager::stop()
{
    if (!m_is_running)
    {
        return;
    }

    std::cout << "[VideoManager] Terminating background GStreamer process...\n";

    /* Send a hard termination signal to the child process */
    TerminateProcess(m_process_info.hProcess, 0);

    /* Close process and thread handles to release system memory */
    CloseHandle(m_process_info.hProcess);
    CloseHandle(m_process_info.hThread);

    // Reset process info structure for safety
    ZeroMemory(&m_process_info, sizeof(m_process_info));
    m_is_running = false;
}
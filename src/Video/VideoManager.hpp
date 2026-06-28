/**
 * @file VideoManager.hpp
 * @brief Defines the VideoManager class for orchestrating GStreamer pipelines.
 *
 * This file contains the class definition for managing a background
 * GStreamer process on Windows, bridging a UDP RTP video stream to a
 * local TCP raw H.264 stream for Web UI consumption.
 */
#pragma once
#include <string>
#include <windows.h>

/**
 * @class VideoManager
 * @brief Manages the lifecycle of a background GStreamer video processing pipeline.
 *
 * The VideoManager class encapsulates the Windows API calls required to spawn,
 * monitor, and safely terminate a hidden GStreamer process (gst-launch-1.0.exe).
 * It acts as a media bridge, receiving UDP packets from the Rover Brain and
 * forwarding parsed H.264 byte-streams to a local TCP socket.
 */
class VideoManager
{
public:
    /**
     * @brief Constructs a new Video Manager object.
     *
     * @param udp_port The local UDP port to listen for incoming RTP packets from the Rover Brain.
     * @param tcp_port The local TCP port to serve the raw H.264 stream to the WebSocket bridge.
     */
    VideoManager(int udp_port, int tcp_port);

    /**
     * @brief Destroys the Video Manager object.
     *
     * Automatically calls stop() to ensure that the background GStreamer process
     * is safely terminated if it is still running, preventing zombie processes.
     */
    ~VideoManager();

    /**
     * @brief Starts the GStreamer pipeline in the background.
     *
     * Constructs the gst-launch-1.0 command and creates a hidden child process
     * using the Windows CreateProcess API. The process runs without a console window.
     *
     * @retval true If the pipeline was successfully started.
     * @retval false If the pipeline failed to start (e.g., executable not found in PATH).
     */
    bool start();

    /**
     * @brief Terminates the running GStreamer pipeline.
     *
     * Sends a termination signal to the child process and closes all associated
     * OS handles to prevent resource leaks.
     */
    void stop();

private:
    int m_udp_port;                     ///< The UDP port for incoming RTP stream.
    int m_tcp_port;                     ///< The TCP port for outgoing H.264 stream.
    PROCESS_INFORMATION m_process_info; ///< Windows API structure holding process and thread handles.
    bool m_is_running;                  ///< Internal state indicating if the child process is active.
};
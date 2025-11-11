/*
 * HUB APPLICATION STUB (Component 3, Part 1)
 *
 * This is a standalone C++ application. It is the "brain" or "gateway".
 *
 * WHAT IT DOES:
 * 1.  Runs a TCP Client to connect to the REAPER Plugin on port 9001.
 * 2.  Listens for simple text messages (e.g., "VOL 0 0.75\n").
 * 3.  Runs an OSC Server on port 9000 (for the Qt GUI to connect to).
 * 4.  Translates the text message into an OSC message (e.g., /track/1/volume 0.75f).
 * 5.  Broadcasts the OSC message to all connected GUI clients.
 *
 * DEPENDENCIES:
 * - C++ Sockets library (e.g., <sys/socket.h> on Linux, Winsock on Windows)
 * - An OSC library (e.g., oscpack, tinyosc)
 *
 * COMPILE:
 * Compile as a standalone executable, linking against the OSC library and pthreads.
 * g++ hub_app_stub.cpp -o hub -lpthread -loscpack (example)
 */

// --- C/C++ Standard Libraries ---
#include <iostream>
#include <string>
#include <thread>
#include <cstring>

// --- OS-specific Networking ---
#ifdef __APPLE__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#else
// Linux headers (your existing code)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// --- OSC Library (oscpack example) ---
// You must have oscpack headers and link the library.
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"

// --- Globals ---
#define OSC_BROADCAST_PORT 9000
#define REAPER_PLUGIN_PORT 9001

UdpTransmitSocket* g_osc_socket = nullptr;

// --- Main Application ---
int main() {
    std::cout << "Starting Hub Application..." << std::endl;

    // --- 1. Initialize OSC Server (UdpSocket for broadcasting) ---
    // This socket will SEND OSC messages to the Qt GUI
    try {
        g_osc_socket = new UdpTransmitSocket(IpEndpointName("127.0.0.1", OSC_BROADCAST_PORT));
        std::cout << "Hub: OSC server broadcasting to 127.0.0.1:" << OSC_BROADCAST_PORT << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Hub: Error initializing OSC socket: " << e.what() << std::endl;
        return 1;
    }
    // std::cout << "Hub: [Stub] OSC server initialized." << std::endl;


    // --- 2. Initialize TCP Client (to connect to REAPER) ---
    int ipc_sock = -1;
    sockaddr_in plugin_addr = {};
    plugin_addr.sin_family = AF_INET;
    plugin_addr.sin_port = htons(REAPER_PLUGIN_PORT);
    inet_pton(AF_INET, "127.0.0.1", &plugin_addr.sin_addr);

    // Connection loop
    while (true) {
        try {
            ipc_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (ipc_sock < 0) {
                throw std::runtime_error("Failed to create socket");
            }

            std::cout << "Hub: Connecting to REAPER plugin on 127.0.0.1:" << REAPER_PLUGIN_PORT << "..." << std::endl;
            if (connect(ipc_sock, (sockaddr*)&plugin_addr, sizeof(plugin_addr)) < 0) {
                throw std::runtime_error("Failed to connect");
            }

            std::cout << "Hub: Connected to REAPER plugin!" << std::endl;

            // --- 3. Main processing loop ---
            char read_buffer[1024];
            std::string line_buffer;

            while (true) {
                int bytes_read = recv(ipc_sock, read_buffer, sizeof(read_buffer) - 1, 0);
                if (bytes_read <= 0) {
                    std::cout << "Hub: REAPER plugin disconnected." << std::endl;
                    close(ipc_sock);
                    break; // Break to outer loop to reconnect
                }

                read_buffer[bytes_read] = '\0';
                line_buffer += read_buffer;

                // Process all complete lines (ending in '\n')
                size_t newline_pos;
                while ((newline_pos = line_buffer.find('\n')) != std::string::npos) {
                    std::string message = line_buffer.substr(0, newline_pos);
                    line_buffer.erase(0, newline_pos + 1);

                    // --- 4. Translate and Broadcast ---
                    std::cout << "Hub: Received IPC: " << message << std::endl;

                    // Parse the simple "VOL TRACK VOL" message
                    char command[4];
                    int track_index;
                    float volume;

                    if (sscanf(message.c_str(), "%3s %d %f", command, &track_index, &volume) == 3) {
                        if (strcmp(command, "VOL") == 0) {
                            // Translate to OSC
                            // e.g., "VOL 0 0.75" -> /track/1/volume 0.75f
                            // (Adding 1 to index for user-friendly track numbers)
                            std::string osc_address = "/track/" + std::to_string(track_index + 1) + "/volume";

                            std::cout << "Hub: Sending OSC: " << osc_address << " " << volume << std::endl;

                            // --- Use oscpack to send the message ---

                            char osc_buffer[1024];
                            osc::OutboundPacketStream p(osc_buffer, sizeof(osc_buffer));

                            p << osc::BeginBundleImmediate
                              << osc::BeginMessage(osc_address.c_str())
                              << volume
                              << osc::EndMessage
                              << osc::EndBundle;

                            g_osc_socket->Send(p.Data(), p.Size());

                        }
                    }
                }
            }
        } catch (std::exception& e) {
            std::cerr << "Hub: Error: " << e.what() << ". Retrying in 5s..." << std::endl;
            if (ipc_sock >= 0) close(ipc_sock);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    delete g_osc_socket;
    return 0;
}

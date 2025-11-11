/*
 * HUB APPLICATION - MANUAL OSC CONSTRUCTION
 */

// --- C/C++ Standard Libraries ---
#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <vector>

// --- OS-specific Networking ---
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// --- Globals ---
#define OSC_BROADCAST_PORT 9000
#define REAPER_PLUGIN_PORT 9001

int g_osc_socket = -1;

// Manual OSC message construction
std::vector<char> buildOscMessage(const std::string& address, float value) {
    std::vector<char> buffer;

    // Address pattern (null-terminated, padded to 4 bytes)
    buffer.insert(buffer.end(), address.begin(), address.end());
    buffer.push_back('\0');
    while (buffer.size() % 4 != 0) {
        buffer.push_back('\0');
    }

    // Type tag (",f" null-terminated, padded to 4 bytes)
    buffer.push_back(',');
    buffer.push_back('f');
    buffer.push_back('\0');
    buffer.push_back('\0'); // Padding to 4 bytes

    // Float value (big-endian, 4 bytes)
    union {
        float f;
        uint32_t i;
    } converter;
    converter.f = value;

    // Convert to big-endian
    uint32_t be_value = htonl(converter.i);
    char* value_bytes = reinterpret_cast<char*>(&be_value);
    buffer.insert(buffer.end(), value_bytes, value_bytes + 4);

    return buffer;
}

// --- Main Application ---
int main() {
    std::cout << "Starting Hub Application..." << std::endl;

    // --- 1. Initialize OSC Socket (for broadcasting) ---
    g_osc_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_osc_socket < 0) {
        std::cerr << "Hub: Error creating OSC socket" << std::endl;
        return 1;
    }

    // Enable broadcast
    int broadcast = 1;
    setsockopt(g_osc_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    sockaddr_in osc_addr = {};
    osc_addr.sin_family = AF_INET;
    osc_addr.sin_port = htons(OSC_BROADCAST_PORT);
    inet_pton(AF_INET, "127.0.0.1", &osc_addr.sin_addr);

    std::cout << "Hub: OSC server broadcasting to 127.0.0.1:" << OSC_BROADCAST_PORT << std::endl;

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
                    std::cout << "Hub: Raw message length: " << message.length() << " bytes" << std::endl;

                    // Parse the simple "VOL TRACK VOL" message
                    char command[4];
                    int track_index;
                    float volume;

                    if (sscanf(message.c_str(), "%3s %d %f", command, &track_index, &volume) == 3) {
                        if (strcmp(command, "VOL") == 0) {
                            // Translate to OSC
                            // e.g., "VOL 0 0.75" -> /track/1/volume 0.75f
                            std::string osc_address = "/track/" + std::to_string(track_index + 1) + "/volume";

                            std::cout << "Hub: Sending OSC: " << osc_address << " " << volume << std::endl;

                            // Build OSC message manually
                            auto osc_message = buildOscMessage(osc_address, volume);

                            // Send OSC message
                            sendto(g_osc_socket, osc_message.data(), osc_message.size(), 0,
                                   (sockaddr*)&osc_addr, sizeof(osc_addr));
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

    if (g_osc_socket >= 0) close(g_osc_socket);
    return 0;
}

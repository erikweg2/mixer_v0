/*
 * HUB APPLICATION - FIXED PORTS - OPTIMIZED
 */

// --- C/C++ Standard Libraries ---
#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/tcp.h>

// --- Globals ---
#define OSC_RECEIVE_PORT 9000    // Hub receives from GUI on this port
#define OSC_SEND_PORT 9002       // Hub sends to GUI on this port
#define REAPER_PLUGIN_PORT 9001

int g_osc_receive_socket = -1;
int g_osc_send_socket = -1;
int g_ipc_sock = -1;

void optimizeSocket(int socket) {
    int no_delay = 1;
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(no_delay));

    int buf_size = 65536;
    setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
    setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
}

// Manual OSC message construction
std::vector<char> buildOscMessage(const std::string& address, float value) {
    std::vector<char> buffer;

    // Address pattern
    buffer.insert(buffer.end(), address.begin(), address.end());
    buffer.push_back('\0');
    while (buffer.size() % 4 != 0) {
        buffer.push_back('\0');
    }

    // Type tag
    buffer.push_back(',');
    buffer.push_back('f');
    buffer.push_back('\0');
    buffer.push_back('\0');

    // Float value (big-endian)
    union {
        float f;
        uint32_t i;
    } converter;
    converter.f = value;

    uint32_t be_value = htonl(converter.i);
    char* value_bytes = reinterpret_cast<char*>(&be_value);
    buffer.insert(buffer.end(), value_bytes, value_bytes + 4);

    return buffer;
}

// Parse OSC message from GUI
bool parseOscFromGui(const char* data, size_t size, int& track_index, float& volume) {
    if (size < 8) return false;

    // Parse address
    const char* ptr = data;
    std::string address = ptr;

    if (address.find("/track/") == 0 && address.find("/volume") != std::string::npos) {
        // Extract track number
        size_t start = strlen("/track/");
        size_t end = address.find("/volume");
        if (end != std::string::npos) {
            std::string track_str = address.substr(start, end - start);
            try {
                track_index = std::stoi(track_str);
            } catch (...) {
                return false;
            }

            // Find type tag
            int address_len = address.length() + 1;
            int padded_addr_len = (address_len + 3) & ~3;

            if (padded_addr_len + 4 <= (int)size) {
                const char* type_tag = ptr + padded_addr_len;
                if (type_tag[0] == ',' && type_tag[1] == 'f' && type_tag[2] == '\0') {
                    int type_tag_len = 4;
                    int data_offset = padded_addr_len + type_tag_len;

                    if (data_offset + 4 <= (int)size) {
                        const unsigned char* float_data =
                            reinterpret_cast<const unsigned char*>(ptr + data_offset);

                        union {
                            uint32_t i;
                            float f;
                        } converter;

                        converter.i = (float_data[0] << 24) |
                                      (float_data[1] << 16) |
                                      (float_data[2] << 8) |
                                      float_data[3];

                        volume = converter.f;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// --- Main Application ---
int main() {
    std::cout << "Starting Hub Application (Fixed Ports - Optimized)..." << std::endl;
    std::cout << "Hub: Receiving from GUI on port " << OSC_RECEIVE_PORT << std::endl;
    std::cout << "Hub: Sending to GUI on port " << OSC_SEND_PORT << std::endl;

    // --- 1. Initialize OSC Receive Socket (from GUI) ---
    g_osc_receive_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_osc_receive_socket < 0) {
        std::cerr << "Hub: Error creating OSC receive socket" << std::endl;
        return 1;
    }

    int reuse = 1;
    setsockopt(g_osc_receive_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Bind to receive from GUI
    sockaddr_in osc_receive_addr = {};
    osc_receive_addr.sin_family = AF_INET;
    osc_receive_addr.sin_addr.s_addr = INADDR_ANY;
    osc_receive_addr.sin_port = htons(OSC_RECEIVE_PORT);

    if (bind(g_osc_receive_socket, (sockaddr*)&osc_receive_addr, sizeof(osc_receive_addr)) < 0) {
        std::cerr << "Hub: Error binding OSC receive socket to port " << OSC_RECEIVE_PORT << std::endl;
        close(g_osc_receive_socket);
        return 1;
    }

    // --- 2. Initialize OSC Send Socket (to GUI) ---
    g_osc_send_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_osc_send_socket < 0) {
        std::cerr << "Hub: Error creating OSC send socket" << std::endl;
        close(g_osc_receive_socket);
        return 1;
    }

    int broadcast = 1;
    setsockopt(g_osc_send_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    sockaddr_in osc_send_addr = {};
    osc_send_addr.sin_family = AF_INET;
    osc_send_addr.sin_port = htons(OSC_SEND_PORT);
    inet_pton(AF_INET, "127.0.0.1", &osc_send_addr.sin_addr);

    // --- 3. Main processing loop ---
    while (true) {
        try {
            // Connect to REAPER plugin
            g_ipc_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (g_ipc_sock < 0) {
                throw std::runtime_error("Failed to create socket");
            }

            // Optimize TCP socket for low latency
            optimizeSocket(g_ipc_sock);

            sockaddr_in plugin_addr = {};
            plugin_addr.sin_family = AF_INET;
            plugin_addr.sin_port = htons(REAPER_PLUGIN_PORT);
            inet_pton(AF_INET, "127.0.0.1", &plugin_addr.sin_addr);

            std::cout << "Hub: Connecting to REAPER plugin on 127.0.0.1:" << REAPER_PLUGIN_PORT << "..." << std::endl;
            if (connect(g_ipc_sock, (sockaddr*)&plugin_addr, sizeof(plugin_addr)) < 0) {
                throw std::runtime_error("Failed to connect");
            }

            std::cout << "Hub: Connected to REAPER plugin!" << std::endl;

            // Setup polling for multiple sockets
            struct pollfd fds[2];
            fds[0].fd = g_ipc_sock;           // Plugin TCP connection
            fds[0].events = POLLIN;
            fds[1].fd = g_osc_receive_socket; // GUI OSC messages
            fds[1].events = POLLIN;

            char read_buffer[1024];
            std::string line_buffer;

            while (true) {
                // Reduced poll timeout from 100ms to 10ms for more responsive polling
                int poll_result = poll(fds, 2, 10);

                if (poll_result < 0) {
                    throw std::runtime_error("Poll error");
                }

                if (poll_result == 0) {
                    continue; // Timeout, no data
                }

                // Check for data from REAPER plugin (STATE UPDATES)
                if (fds[0].revents & POLLIN) {
                    int bytes_read = recv(g_ipc_sock, read_buffer, sizeof(read_buffer) - 1, 0);
                    if (bytes_read <= 0) {
                        std::cout << "Hub: REAPER plugin disconnected." << std::endl;
                        close(g_ipc_sock);
                        break;
                    }

                    read_buffer[bytes_read] = '\0';
                    line_buffer += read_buffer;

                    // Process all complete lines
                    size_t newline_pos;
                    while ((newline_pos = line_buffer.find('\n')) != std::string::npos) {
                        std::string message = line_buffer.substr(0, newline_pos);
                        line_buffer.erase(0, newline_pos + 1);

                        // Only process VOL messages (state updates from REAPER)
                        if (message.find("VOL ") == 0) {
                            std::cout << "Hub: Received STATE from Plugin: " << message << std::endl;

                            // Parse "VOL TRACK VOL" message with higher precision
                            char command[4];
                            int track_index;
                            float volume;

                            // Use %f for full precision
                            if (sscanf(message.c_str(), "%3s %d %f", command, &track_index, &volume) == 3) {
                                if (strcmp(command, "VOL") == 0) {
                                    std::string osc_address = "/track/" + std::to_string(track_index) + "/volume";
                                    std::cout << "Hub: Sending STATE to GUI: " << osc_address << " " << volume << std::endl;

                                    auto osc_message = buildOscMessage(osc_address, volume);
                                    sendto(g_osc_send_socket, osc_message.data(), osc_message.size(), 0,
                                           (sockaddr*)&osc_send_addr, sizeof(osc_send_addr));
                                }
                            }
                        }
                    }
                }

                // Check for data from GUI (CONTROL COMMANDS)
                if (fds[1].revents & POLLIN) {
                    sockaddr_in gui_addr;
                    socklen_t gui_addr_len = sizeof(gui_addr);
                    int bytes_read = recvfrom(g_osc_receive_socket, read_buffer, sizeof(read_buffer) - 1, 0,
                                              (sockaddr*)&gui_addr, &gui_addr_len);

                    if (bytes_read > 0) {
                        int track_index;
                        float volume;
                        if (parseOscFromGui(read_buffer, bytes_read, track_index, volume)) {
                            std::cout << "Hub: Received COMMAND from GUI: Track " << track_index << " Volume " << volume << std::endl;

                            // IMMEDIATE forwarding to REAPER plugin - no buffering
                            char message[256];
                            snprintf(message, sizeof(message), "SET_VOL %d %.6f\n", track_index, volume);

                            // Send immediately without waiting for poll cycle
                            ssize_t sent = send(g_ipc_sock, message, strlen(message), MSG_DONTWAIT);
                            if (sent > 0) {
                                std::cout << "Hub: Sent COMMAND to Plugin: " << message;
                            } else {
                                std::cerr << "Hub: Failed to send command to plugin" << std::endl;
                            }
                        }
                    }
                }
            }
        } catch (std::exception& e) {
            std::cerr << "Hub: Error: " << e.what() << ". Retrying in 5s..." << std::endl;
            if (g_ipc_sock >= 0) close(g_ipc_sock);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    if (g_osc_receive_socket >= 0) close(g_osc_receive_socket);
    if (g_osc_send_socket >= 0) close(g_osc_send_socket);
    return 0;
}

/*
 * REAPER PLUGIN WITH DEBUG OUTPUT
 */

#include "WDL/wdltypes.h"
#include <thread>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <set>

extern "C" {
#include "reaper_plugin.h"
}

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define PLUGIN_PORT 9001

// Global function pointers
void (*ShowConsoleMsg)(const char*) = nullptr;
double (*GetMediaTrackInfo_Value)(MediaTrack* track, const char* parmname) = nullptr;
int (*GetNumTracks)() = nullptr;
MediaTrack* (*GetTrack)(int index) = nullptr;
int (*CSurf_TrackFromID)(int id, bool mcpView) = nullptr;

class CSurf_IPC : public IReaperControlSurface {
private:
    std::atomic<bool> m_running{false};
    std::thread m_server_thread;
    int m_server_socket{-1};
    std::mutex m_clients_mutex;
    std::set<int> m_client_sockets;

    void sendToAllClients(const std::string& message) {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        if (ShowConsoleMsg) {
            char debug[256];
            snprintf(debug, sizeof(debug), "Sending to %zu clients: %s", m_client_sockets.size(), message.c_str());
            ShowConsoleMsg(debug);
        }

        for (int client_socket : m_client_sockets) {
            int result = send(client_socket, message.c_str(), message.length(), 0);
            if (result <= 0) {
                if (ShowConsoleMsg) {
                    ShowConsoleMsg("Failed to send to client\n");
                }
            }
        }
    }

    void handleClient(int client_socket) {
        if (ShowConsoleMsg) {
            ShowConsoleMsg("New client connected\n");
        }

        // Send initial state of all tracks
        if (GetNumTracks && GetTrack && GetMediaTrackInfo_Value) {
            int num_tracks = GetNumTracks();
            for (int i = 0; i < num_tracks; i++) {
                MediaTrack* track = GetTrack(i);
                if (track) {
                    double volume = GetMediaTrackInfo_Value(track, "D_VOL");
                    char msg[256];
                    snprintf(msg, sizeof(msg), "VOL %d %.3f\n", i, volume);
                    send(client_socket, msg, strlen(msg), 0);

                    if (ShowConsoleMsg) {
                        char debug[256];
                        snprintf(debug, sizeof(debug), "Sent initial volume for track %d: %.3f\n", i, volume);
                        ShowConsoleMsg(debug);
                    }
                }
            }
        }

        // Keep connection open and wait for more data or keep alive
        char buffer[1024];
        while (m_running) {
            int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_read <= 0) {
                if (ShowConsoleMsg) {
                    ShowConsoleMsg("Client disconnected\n");
                }
                break; // Client disconnected or error
            }

            buffer[bytes_read] = '\0';
// Echo back for testing, or process incoming commands

#ifdef _WIN32
            Sleep(100);
#else
            usleep(100000);
#endif
        }

        // Clean up client
        {
            std::lock_guard<std::mutex> lock(m_clients_mutex);
            m_client_sockets.erase(client_socket);
        }
#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
    }

    void runServer() {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

        m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_server_socket < 0) {
            if (ShowConsoleMsg) ShowConsoleMsg("Failed to create server socket\n");
            return;
        }

        // Set socket options
        int opt = 1;
        setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PLUGIN_PORT);

        if (bind(m_server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            if (ShowConsoleMsg) ShowConsoleMsg("Failed to bind server socket\n");
#ifdef _WIN32
            closesocket(m_server_socket);
#else
            close(m_server_socket);
#endif
            return;
        }

        listen(m_server_socket, 5); // Allow multiple pending connections

        if (ShowConsoleMsg) ShowConsoleMsg("TCP Server started on port 9001\n");

        while (m_running) {
            sockaddr_in client_addr{};
#ifdef _WIN32
            int client_len = sizeof(client_addr);
#else
            socklen_t client_len = sizeof(client_addr);
#endif

            int client_socket = accept(m_server_socket, (sockaddr*)&client_addr, &client_len);
            if (client_socket >= 0) {
                {
                    std::lock_guard<std::mutex> lock(m_clients_mutex);
                    m_client_sockets.insert(client_socket);
                }

                // Handle client in this thread for simplicity
                handleClient(client_socket);
            }

// Small delay to prevent busy waiting
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100000);
#endif
        }

        // Clean up all client connections
        {
            std::lock_guard<std::mutex> lock(m_clients_mutex);
            for (int client_socket : m_client_sockets) {
#ifdef _WIN32
                closesocket(client_socket);
#else
                close(client_socket);
#endif
            }
            m_client_sockets.clear();
        }

#ifdef _WIN32
        closesocket(m_server_socket);
        WSACleanup();
#else
        close(m_server_socket);
#endif
    }

public:
    CSurf_IPC() {
        if (ShowConsoleMsg) ShowConsoleMsg("CSurf_IPC constructor\n");
        m_running = true;
        m_server_thread = std::thread(&CSurf_IPC::runServer, this);
    }

    ~CSurf_IPC() {
        m_running = false;
        if (m_server_thread.joinable()) {
            m_server_thread.join();
        }
    }

    virtual const char* GetTypeString() override { return "IPC_CSURF"; }
    virtual const char* GetDescString() override { return "IPC CSurf Test"; }
    virtual const char* GetConfigString() override { return ""; }
    virtual void Run() override {
        // This gets called regularly (about 30 times per second)
    }

    virtual void SetSurfaceVolume(MediaTrack* track, double volume) override {
        // This gets called when a track volume changes in REAPER
        if (ShowConsoleMsg) {
            char msg[256];
            snprintf(msg, sizeof(msg), "SetSurfaceVolume called: track=%p, volume=%.3f\n", track, volume);
            ShowConsoleMsg(msg);
        }

        // Find the track index
        if (GetNumTracks && GetTrack) {
            int num_tracks = GetNumTracks();
            for (int i = 0; i < num_tracks; i++) {
                if (GetTrack(i) == track) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "VOL %d %.3f\n", i, volume);
                    sendToAllClients(msg);

                    if (ShowConsoleMsg) {
                        char debug[256];
                        snprintf(debug, sizeof(debug), "Sent volume update: track %d = %.3f\n", i, volume);
                        ShowConsoleMsg(debug);
                    }
                    return;
                }
            }
        }

        if (ShowConsoleMsg) {
            ShowConsoleMsg("Could not find track index for volume update\n");
        }
    }

    virtual void SetSurfacePan(MediaTrack *track, double pan) override {}
    virtual void SetSurfaceMute(MediaTrack *track, bool mute) override {}
    virtual void SetSurfaceSelected(MediaTrack *track, bool selected) override {}
    virtual void SetSurfaceSolo(MediaTrack *track, bool solo) override {}
    virtual void SetSurfaceRecArm(MediaTrack *track, bool arm) override {}
    virtual void SetPlayState(bool play, bool pause, bool rec) override {}
    virtual void SetRepeatState(bool rep) override {}
    virtual void SetTrackTitle(MediaTrack *track, const char *title) override {}
    virtual bool GetTouchState(MediaTrack *track, int idx) override { return false; }
};

static IReaperControlSurface* CSurf_Create(const char* type_string, const char* config_string, int* errStats) {
    if (errStats) *errStats = 0;
    return new CSurf_IPC();
}

static HWND ShowConfig(const char* type_string, HWND parent, const char* initConfigString) {
    return NULL;
}

static reaper_csurf_reg_t csurf_reg = {
    "IPC_CSURF",
    "IPC CSurf Test",
    CSurf_Create,
    ShowConfig
};

extern "C" {
REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec) {
    if (!rec) return 0;

    if (rec->GetFunc) {
        ShowConsoleMsg = (void (*)(const char*))rec->GetFunc("ShowConsoleMsg");
        GetMediaTrackInfo_Value = (double (*)(MediaTrack*, const char*))rec->GetFunc("GetMediaTrackInfo_Value");
        GetNumTracks = (int (*)())rec->GetFunc("GetNumTracks");
        GetTrack = (MediaTrack* (*)(int))rec->GetFunc("GetTrack");
        CSurf_TrackFromID = (int (*)(int, bool))rec->GetFunc("CSurf_TrackFromID");
    }

    if (!ShowConsoleMsg) return 0;

    rec->Register("csurf", &csurf_reg);
    ShowConsoleMsg("=== IPC Control Surface with Debug Output Loaded ===\n");
    return 1;
}
}

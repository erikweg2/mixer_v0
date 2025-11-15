/*
 * REAPER PLUGIN - 12-BIT RESOLUTION SUPPORT
 */

#include "WDL/wdltypes.h"
#include <thread>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <set>
#include <map>

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
void (*SetMediaTrackInfo_Value)(MediaTrack* track, const char* parmname, double newValue) = nullptr;
int (*GetNumTracks)() = nullptr;
MediaTrack* (*GetTrack)(int index) = nullptr;
MediaTrack* (*GetMasterTrack)(int proj) = nullptr;
bool (*TrackList_AdjustWindows)(bool isMinor) = nullptr;

class CSurf_IPC : public IReaperControlSurface {
private:
    std::atomic<bool> m_running{false};
    std::thread m_server_thread;
    int m_server_socket{-1};
    std::mutex m_clients_mutex;
    std::set<int> m_client_sockets;

    // Feedback prevention
    std::atomic<bool> m_ignore_callbacks{false};
    std::map<MediaTrack*, int> m_track_map;
    int m_next_track_id{1};

    int getTrackId(MediaTrack* track) {
        if (GetMasterTrack && track == GetMasterTrack(0)) {
            return 0;
        }

        auto it = m_track_map.find(track);
        if (it != m_track_map.end()) {
            return it->second;
        }

        int new_id = m_next_track_id++;
        m_track_map[track] = new_id;

        if (ShowConsoleMsg) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Assigned ID %d to new track %p\n", new_id, track);
            ShowConsoleMsg(msg);
        }

        return new_id;
    }

    MediaTrack* getTrackById(int track_id) {
        if (track_id == 0) {
            return GetMasterTrack ? GetMasterTrack(0) : nullptr;
        }

        // Search for track with this ID
        for (auto& pair : m_track_map) {
            if (pair.second == track_id) {
                return pair.first;
            }
        }

        // If not found in map, try by index (track_id 1 = index 0, etc.)
        if (GetNumTracks && GetTrack) {
            int num_tracks = GetNumTracks();
            if (track_id > 0 && track_id <= num_tracks) {
                MediaTrack* track = GetTrack(track_id - 1);
                if (track) {
                    m_track_map[track] = track_id;
                    return track;
                }
            }
        }

        return nullptr;
    }

    void sendToAllClients(const std::string& message) {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        for (int client_socket : m_client_sockets) {
            send(client_socket, message.c_str(), message.length(), 0);
        }
    }

    void sendTrackVolume(MediaTrack* track, int client_socket = -1) {
        if (!track || !GetMediaTrackInfo_Value) return;

        double volume = GetMediaTrackInfo_Value(track, "D_VOL");
        int track_id = getTrackId(track);

        // Send with high precision (6 decimal places)
        char msg[256];
        snprintf(msg, sizeof(msg), "VOL %d %.6f\n", track_id, volume);

        if (client_socket >= 0) {
            send(client_socket, msg, strlen(msg), 0);
        } else {
            sendToAllClients(msg);
        }

        if (ShowConsoleMsg) {
            char debug[256];
            snprintf(debug, sizeof(debug), "Sent: %s", msg);
            ShowConsoleMsg(debug);
        }
    }

    void setTrackVolume(int track_id, float volume) {
        MediaTrack* track = getTrackById(track_id);
        if (!track || !SetMediaTrackInfo_Value) {
            if (ShowConsoleMsg) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Cannot find track with ID %d\n", track_id);
                ShowConsoleMsg(msg);
            }
            return;
        }

        if (ShowConsoleMsg) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Setting track %d volume to %.6f\n", track_id, volume);
            ShowConsoleMsg(msg);
        }

        // IGNORE callbacks while we set the volume programmatically
        m_ignore_callbacks = true;

        // Set the volume in REAPER with full precision
        SetMediaTrackInfo_Value(track, "D_VOL", volume);

        // Refresh UI
        if (TrackList_AdjustWindows) {
            TrackList_AdjustWindows(false);
        }

// Small delay to ensure the set operation completes
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif

        // RE-ENABLE callbacks
        m_ignore_callbacks = false;
    }

    void processClientCommand(const std::string& command) {
        if (ShowConsoleMsg) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Processing command: %s\n", command.c_str());
            ShowConsoleMsg(msg);
        }

        // Parse SET_VOL commands: "SET_VOL track_id volume" with high precision
        if (command.find("SET_VOL") == 0) {
            int track_id;
            float volume;
            if (sscanf(command.c_str(), "SET_VOL %d %f", &track_id, &volume) == 2) {
                setTrackVolume(track_id, volume);
            } else {
                if (ShowConsoleMsg) {
                    ShowConsoleMsg("Failed to parse SET_VOL command\n");
                }
            }
        }
    }

    void handleClient(int client_socket) {
        if (ShowConsoleMsg) {
            ShowConsoleMsg("New client connected\n");
        }

        // Send initial track volumes with high precision
        if (GetMasterTrack) {
            MediaTrack* master_track = GetMasterTrack(0);
            sendTrackVolume(master_track, client_socket);
        }

        if (GetNumTracks && GetTrack) {
            int num_tracks = GetNumTracks();
            for (int i = 0; i < num_tracks; i++) {
                MediaTrack* track = GetTrack(i);
                sendTrackVolume(track, client_socket);
            }
        }

        // Handle client communication
        char buffer[1024];
        std::string line_buffer;

        while (m_running) {
            int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_read <= 0) {
                if (ShowConsoleMsg) {
                    ShowConsoleMsg("Client disconnected\n");
                }
                break;
            }

            buffer[bytes_read] = '\0';
            line_buffer += buffer;

            // Process complete lines
            size_t newline_pos;
            while ((newline_pos = line_buffer.find('\n')) != std::string::npos) {
                std::string command = line_buffer.substr(0, newline_pos);
                line_buffer.erase(0, newline_pos + 1);

                processClientCommand(command);
            }

#ifdef _WIN32
            Sleep(10);
#else
            usleep(10000);
#endif
        }

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

        listen(m_server_socket, 5);
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
                handleClient(client_socket);
            }

#ifdef _WIN32
            Sleep(100);
#else
            usleep(100000);
#endif
        }

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
    virtual void Run() override {}

    virtual void SetSurfaceVolume(MediaTrack* track, double volume) override {
        // IGNORE callbacks if we're currently setting volumes programmatically
        if (m_ignore_callbacks) {
            if (ShowConsoleMsg) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Ignoring callback (programmatic change): track=%p, volume=%.6f\n", track, volume);
                ShowConsoleMsg(msg);
            }
            return;
        }

        // This is a genuine user change from REAPER - send to clients
        if (ShowConsoleMsg) {
            char msg[256];
            snprintf(msg, sizeof(msg), "User changed volume: track=%p, volume=%.6f\n", track, volume);
            ShowConsoleMsg(msg);
        }

        sendTrackVolume(track);
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
        SetMediaTrackInfo_Value = (void (*)(MediaTrack*, const char*, double))rec->GetFunc("SetMediaTrackInfo_Value");
        GetNumTracks = (int (*)())rec->GetFunc("GetNumTracks");
        GetTrack = (MediaTrack* (*)(int))rec->GetFunc("GetTrack");
        GetMasterTrack = (MediaTrack* (*)(int))rec->GetFunc("GetMasterTrack");
        TrackList_AdjustWindows = (bool (*)(bool))rec->GetFunc("TrackList_AdjustWindows");
    }

    if (!ShowConsoleMsg) return 0;

    rec->Register("csurf", &csurf_reg);
    ShowConsoleMsg("=== IPC Control Surface - 12-Bit Resolution Support Loaded ===\n");
    return 1;
}
}

/*
 * REAPER PLUGIN WITH PROPER TRACK IDENTIFICATION
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
int (*GetNumTracks)() = nullptr;
MediaTrack* (*GetTrack)(int index) = nullptr;
MediaTrack* (*GetMasterTrack)(int proj) = nullptr;
GUID* (*GetTrackGUID)(MediaTrack* track) = nullptr;
const char* (*GetTrackName)(MediaTrack* track, char* buf, int buf_sz) = nullptr;

class CSurf_IPC : public IReaperControlSurface {
private:
    std::atomic<bool> m_running{false};
    std::thread m_server_thread;
    int m_server_socket{-1};
    std::mutex m_clients_mutex;
    std::set<int> m_client_sockets;
    std::map<MediaTrack*, int> m_track_cache;

    int findTrackIndex(MediaTrack* track) {
        // Check cache first
        auto it = m_track_cache.find(track);
        if (it != m_track_cache.end()) {
            return it->second;
        }

        // Check if it's the master track
        if (GetMasterTrack && track == GetMasterTrack(0)) {
            m_track_cache[track] = 0; // Master is track 0
            return 0;
        }

        // Scan all tracks to find this one
        if (GetNumTracks && GetTrack) {
            int num_tracks = GetNumTracks();
            for (int i = 0; i < num_tracks; i++) {
                MediaTrack* current_track = GetTrack(i);
                if (current_track == track) {
                    m_track_cache[track] = i + 1; // Regular tracks start from 1
                    return i + 1;
                }
            }
        }

        // If we can't find it, assign a temporary negative ID
        int temp_id = -(int)m_track_cache.size() - 1;
        m_track_cache[track] = temp_id;
        return temp_id;
    }

    void sendToAllClients(const std::string& message) {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        for (int client_socket : m_client_sockets) {
            send(client_socket, message.c_str(), message.length(), 0);
        }
    }

    void handleClient(int client_socket) {
        if (ShowConsoleMsg) {
            ShowConsoleMsg("New client connected\n");
        }

        // Clear cache and rebuild
        m_track_cache.clear();

        // Send master track first (track 0)
        if (GetMasterTrack && GetMediaTrackInfo_Value) {
            MediaTrack* master_track = GetMasterTrack(0);
            if (master_track) {
                double volume = GetMediaTrackInfo_Value(master_track, "D_VOL");
                char msg[256];
                snprintf(msg, sizeof(msg), "VOL 0 %.3f\n", volume);
                send(client_socket, msg, strlen(msg), 0);
                m_track_cache[master_track] = 0;
            }
        }

        // Send all regular tracks
        if (GetNumTracks && GetTrack && GetMediaTrackInfo_Value) {
            int num_tracks = GetNumTracks();
            for (int i = 0; i < num_tracks; i++) {
                MediaTrack* track = GetTrack(i);
                if (track) {
                    double volume = GetMediaTrackInfo_Value(track, "D_VOL");
                    char msg[256];
                    snprintf(msg, sizeof(msg), "VOL %d %.3f\n", i + 1, volume); // Tracks start from 1
                    send(client_socket, msg, strlen(msg), 0);
                    m_track_cache[track] = i + 1;
                }
            }
        }

        // Keep connection open
        char buffer[1024];
        while (m_running) {
            int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_read <= 0) {
                break;
            }
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100000);
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
        if (m_server_socket < 0) return;

        int opt = 1;
        setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PLUGIN_PORT);

        if (bind(m_server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
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
        int track_index = findTrackIndex(track);

        if (ShowConsoleMsg) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Volume: track_index=%d, volume=%.3f\n", track_index, volume);
            ShowConsoleMsg(msg);
        }

        char msg[256];
        snprintf(msg, sizeof(msg), "VOL %d %.3f\n", track_index, volume);
        sendToAllClients(msg);
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
        GetMasterTrack = (MediaTrack* (*)(int))rec->GetFunc("GetMasterTrack");
    }

    if (!ShowConsoleMsg) return 0;

    rec->Register("csurf", &csurf_reg);
    ShowConsoleMsg("=== IPC Control Surface with Fixed Track IDs ===\n");
    return 1;
}
}

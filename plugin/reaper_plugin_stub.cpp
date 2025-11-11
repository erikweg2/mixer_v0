/*
 * CORRECT CONTROL SURFACE IMPLEMENTATION WITH EXACT STRUCTURE
 */

#include "WDL/wdltypes.h"

extern "C" {
#include "reaper_plugin.h"
}

class CSurf_IPC : public IReaperControlSurface {
public:
    virtual const char* GetTypeString() { return "IPC_CSURF"; }
    virtual const char* GetDescString() { return "IPC CSurf Test"; }
    virtual const char* GetConfigString() { return ""; }
    virtual void Run() {}
    virtual void SetSurfaceVolume(MediaTrack* track, double volume) {}
    virtual void SetSurfacePan(MediaTrack *track, double pan) {}
    virtual void SetSurfaceMute(MediaTrack *track, bool mute) {}
    virtual void SetSurfaceSelected(MediaTrack *track, bool selected) {}
    virtual void SetSurfaceSolo(MediaTrack *track, bool solo) {}
    virtual void SetSurfaceRecArm(MediaTrack *track, bool arm) {}
    virtual void SetPlayState(bool play, bool pause, bool rec) {}
    virtual void SetRepeatState(bool rep) {}
    virtual void SetTrackTitle(MediaTrack *track, const char *title) {}
    virtual bool GetTouchState(MediaTrack *track, int idx) { return false; }
};

static IReaperControlSurface* CSurf_Create(const char* type_string, const char* config_string, int* errStats) {
    if (errStats) *errStats = 0; // No errors
    return new CSurf_IPC();
}

// ShowConfig function - can be NULL if no configuration dialog
static HWND ShowConfig(const char* type_string, HWND parent, const char* initConfigString) {
    return NULL; // No configuration dialog
}

// EXACT structure matching the SDK definition
static reaper_csurf_reg_t csurf_reg = {
    "IPC_CSURF",           // type_string
    "IPC CSurf Test",      // desc_string
    CSurf_Create,          // create function
    ShowConfig             // ShowConfig function (can be NULL)
};

extern "C" {
REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec) {
    if (!rec) return 0;

    void* (*GetFunc)(const char* name) = rec->GetFunc;
    if (!GetFunc) return 0;

    void (*ShowMsg)(const char*) = (void (*)(const char*))GetFunc("ShowConsoleMsg");
    if (ShowMsg) {
        ShowMsg("=== Control Surface with EXACT Structure ===\n");
    }

    // Register using the structure
    rec->Register("csurf", &csurf_reg);

    return 1;
}
}

/*
 * MINIMAL REAPER PLUGIN TEST
 */

#include "WDL/wdltypes.h"

extern "C" {
#define REAPER_PLUGIN_WANT_FUNC_DEF
#include "reaper_plugin_functions.h"
}

#include "reaper_plugin.h"

class CSurf_IPC : public IReaperControlSurface {
public:
    virtual const char* GetTypeString() override { return "IPC_CSURF"; }
    virtual const char* GetDescString() override { return "IPC CSurf Test"; }
    virtual const char* GetConfigString() override { return ""; }
    virtual void Run() override {}
    virtual void SetSurfaceVolume(MediaTrack* track, double volume) override {}
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

static IReaperControlSurface* CSurf_Create(const char* type_string, const char* config_string, int* size) {
    return new CSurf_IPC();
}

static reaper_csurf_reg_t csurf_reg = { CSurf_Create };

extern "C" {
REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec) {
    if (!rec) return 0;
    if (rec->caller_version != REAPER_PLUGIN_VERSION) return 0;

    // Get function pointers
    if (rec->GetFunc) {
        ShowConsoleMsg = (decltype(ShowConsoleMsg))rec->GetFunc("ShowConsoleMsg");
        GetMediaTrackInfo_Value = (decltype(GetMediaTrackInfo_Value))rec->GetFunc("GetMediaTrackInfo_Value");
    }

    if (!ShowConsoleMsg) return 0;

    rec->Register("csurf", &csurf_reg);
    ShowConsoleMsg("Minimal IPC CSurf loaded!\n");
    return 1;
}
}

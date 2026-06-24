//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GUICntr.cpp
 Version     : V2.00
 By          : Wey. Silver Grid

 Description : GUI controller - adapter layer delegating to the new GForm system.
               Keeps backward compatibility for existing form code while
               all internals are handled by gform::.

               DEPRECATED: New code should use GForm.h directly.
               This file will be removed in a future cleanup phase.

 Date       : 2026.06.24 (v2.0 - adapter for gform)
              2023.12.05 (v1.10 - original implementation)
*/
//-----------------------------------------------------------------------------//-----------------------------------------------------------------------------
#include "GUICntr.h"

//  New GForm system 
#include "GForm.h"
#include "GFormPlatform.h"

#ifndef __vmSIMULATOR__
 #include "IndLed.h"
 #include "KeyBoard.h"
 #include "RNGen.h"
 #include "DevRegs.h"
 #include <cmsis_os.h>
#else
 #include <Windows.h>
 #include <mutex>
 #include "WM.h"
#endif

#include "GUI.h"
#include "GWinTypes.h"
#include "GUIMessage.h"

//  Form headers (for GFormsList registration) 
#include "GPSplashForm.h"
#include "GPMenuForm.h"
#include "GPMainForm.h"
#include "GPEventBrowserForm.h"
#include "GPFatalForm.h"  // used by GUIShowFatalMessage

#include <string.h>

#ifndef __vmSIMULATOR__
  void KEYDB_OnChanged(const TKeyState& key);
#endif

//=============================================================================
// Constants
//-----------------------------------------------------------------------------
constexpr auto TIME_MS_LCDBG = 120000;   // LCD backlight timeout (ms)

//=============================================================================
// Global state (kept for backward compatibility)
//-----------------------------------------------------------------------------
volatile TGUIState FGUIState;

#ifndef __vmSIMULATOR__
  // RTOS mutex initialized via osMutexNew in GUIStart()
#else
  std::mutex gui_mutex;   // Simulator thread safety for FGUIState idle check
#endif

//=============================================================================
// Form registration list  registers all known forms into gform::
//-----------------------------------------------------------------------------
// Each entry maps a window ID to its GWinForm.
// FormRegistrar auto-registration (Phase 2) will make this obsolete.
//-----------------------------------------------------------------------------
struct FormEntry {
    uint32_t        id;
    const GWinForm* form;
    const char*     name;
};

// Forms now self-register via FormRegistrar in their own .cpp files.
// s_formTable is kept as a fallback for forms that don't use FormRegistrar
// (e.g., forms not yet migrated, or forms that need late registration).
static constexpr FormEntry s_formTable[] = {
    { WID_SplashForm,     &FSplashForm,       "Splash"        },
    { WID_MenuForm,       &FMainMenuForm,     "Menu"          },
    { WID_MainForm,       &FMainForm,         "Main"          },
    { WID_EventListForm,  &FEventBrowserForm, "EventBrowser"  },
};

static constexpr size_t kNumForms = sizeof(s_formTable) / sizeof(s_formTable[0]);

//=============================================================================
// Adapter: register all known forms into GForm system
//=============================================================================
static void RegisterAllForms()
{
    for (size_t i = 0; i < kNumForms; ++i) {
        if (s_formTable[i].form != nullptr) {
            gform::RegisterForm(
                static_cast<gform::FormId>(s_formTable[i].id),
                s_formTable[i].form,
                s_formTable[i].name
            );
        }
    }
}

//=============================================================================
// Adapter: GUITick  delegate to gform::Tick
//=============================================================================
void GUITick(void)
{
#ifndef __vmSIMULATOR__
    // Keyboard polling (real device only)
    TKeyState key;
    if (0 != KeyBoard_Tick()) {
        KeyBoard_Read(key);
        if (0 != key.ucKey)
            KEYDB_OnChanged(key);
        FGUIState.ucCurIRKey = 0;
        RNG_Init(GUI_GetTime());
    }
#endif

    //  Idle timeout check 
    bool needSwitchToMain = false;

#ifndef __vmSIMULATOR__
    osMutexWait(FGUIState.mutexGUI, osWaitForever);
#else
    std::unique_lock<std::mutex> lock(gui_mutex);
#endif

    if ((0 < FGUIState.uKeyIdle) &&
        ((uint32_t)GUI_GetTime() > FGUIState.uKeyIdle)) {
        FGUIState.uKeyIdle = 0;
#ifndef __vmSIMULATOR__
        INDLED_LCDBG(0);
#endif
        // Switch to main if we're not already there
        if (gform::GetCurrentFormId() != WID_MainForm) {
            needSwitchToMain = true;
        }
    }

#ifndef __vmSIMULATOR__
    osMutexRelease(FGUIState.mutexGUI);
#else
    lock.unlock();
#endif

    if (needSwitchToMain) {
        gform::ReplaceForm(WID_MainForm, nullptr);
    }

    //  Delegate tick to GForm system 
    gform::Tick();
}

//=============================================================================
// Adapter: GUIStart  init GUI and open splash
//=============================================================================
void GUIStart()
{
    // Clear global state
    memset((void*)&FGUIState, 0, sizeof(FGUIState));

#ifndef __vmSIMULATOR__
    INDLED_Init();
    KeyBoard_Init();
#endif

    // Init emWin
    GUI_Init();
#ifdef __vmSIMULATOR__
    WM_MULTIBUF_Enable(1);
#endif

    GUI_SetBkColor(GUI_BLACK);
    GUI_Clear();

#ifndef __vmSIMULATOR__
#if osCMSIS >= 0x20000U
    FGUIState.mutexGUI = osMutexNew(nullptr);
#else
    FGUIState.mutexGUI = osMutexCreate(nullptr);
#endif
#endif

    //  Initialize the new GForm system 
    gform::Init();

    //  Register all forms 
    RegisterAllForms();

    //  Decide initial form 
#ifndef __vmSIMULATOR__
    if (0 != GetRSTSrc(RRS_PORRST) && 0 == GetRSTSrc(RRS_IWDGRST)) {
        gform::PushForm(WID_SplashForm, nullptr);
    } else {
        gform::PushForm(WID_MainForm, nullptr);
    }

    if (0 == GetRSTSrc(RRS_IWDGRST)) {
        INDLED_LCDBG(1);
        FGUIState.uKeyIdle = GUI_GetTime() + TIME_MS_LCDBG;
    }

    // Collect reset source flags
    if (GetRSTSrc(RRS_IWDGRST) != 0)  FGUIState.uStartupTime |= 1;
    if (GetRSTSrc(RRS_PORRST)  != 0)  FGUIState.uStartupTime |= 2;
    if (GetRSTSrc(RRS_PINRST)  != 0)  FGUIState.uStartupTime |= 4;
    if (GetRSTSrc(RRS_SFTRST)  != 0)  FGUIState.uStartupTime |= 8;
    if (GetRSTSrc(RRS_LPWRRST) != 0)  FGUIState.uStartupTime |= 16;
#else
    // Simulator always starts with splash
    gform::PushForm(WID_SplashForm, nullptr);
#endif

    FGUIState.ucGUIOk = GUI_OK;
}

//=============================================================================
// Adapter: GUICenter  main GUI loop
//=============================================================================
void GUICenter()
{
    int iTickCount = 0;
    while (1) {
        //  GForm-based tick 
        GUITick();

#ifndef __vmSIMULATOR__
        osDelay(10);
        if (iTickCount > 10) {
            INDLED_CommLEDCntr();
            iTickCount = 0;
        }
#else
        iTickCount++;
        Sleep(10);
#endif

#ifdef RRS_GUITASK
        SetRSTSrc(RRS_GUITASK);
#endif
    }
}

//=============================================================================
// Adapter: GUIFormOpen
//=============================================================================
bool GUIFormOpen(uint32_t uFormId, const void* para)
{
    // Validate form id range
    if (WID_FORMBEGIN > uFormId) return false;

    size_t idx = uFormId - WID_FORMBEGIN;
    if (idx >= kNumForms) return false;

    gform::PushForm(static_cast<gform::FormId>(uFormId), para);
    return true;
}

//=============================================================================
// Adapter: GUIFormClose
//=============================================================================
bool GUIFormClose(const void* para)
{
    (void)para;
    gform::PopForm();

    // If stack is now empty, open MainForm as fallback
    if (gform::IsStackEmpty()) {
        gform::PushForm(WID_MainForm, nullptr);
    }
    return true;
}

//=============================================================================
// Adapter: GUISendMessage
//=============================================================================
void GUISendMessage(uint32_t uMsgIdent, uint32_t uParam, uint32_t uData)
{
    gform::SendMsg(
        static_cast<uint16_t>(uMsgIdent),
        static_cast<uint16_t>(uParam),
        static_cast<int32_t>(uData)
    );
}

//=============================================================================
// Adapter: GUIModeChanged
//=============================================================================
void GUIModeChanged(void)
{
    gform::ReplaceForm(WID_SplashForm, nullptr);
}

//=============================================================================
// Adapter: GUIKeyEnevt (simulator only)
//=============================================================================
#ifdef __vmSIMULATOR__
void GUIKeyEnevt(uint32_t uKey, uint32_t uPressedCnt)
{
    gform::KeyEvent(uKey, uPressedCnt);
}
#endif

//=============================================================================
// Adapter: IRKBD_Received (real device only)
//=============================================================================
#ifndef __vmSIMULATOR__
void IRKBD_Received(uint8_t ucKeyCode)
{
    FGUIState.ucCurIRKey = ucKeyCode;
}
#endif

//=============================================================================
// Adapter: GUIShowFatalMessage
//=============================================================================
void GUIShowFatalMessage(const char* pcMsg)
{
#ifndef __vmSIMULATOR__
    FFatalForm.pShow(pcMsg);
#endif
}

//=============================================================================
// Keyboard callback (real device only)
//=============================================================================
#ifndef __vmSIMULATOR__
void KEYDB_OnChanged(const TKeyState& key)
{
    FGUIState.uKeyIdle = GUI_GetTime() + TIME_MS_LCDBG;

    if (false == INDLED_GetLCDBG()) {
        INDLED_LCDBG(1);
        return;
    }

    uint32_t uMsgId;
    if (KEYST_RELEASE == key.ucState)
        uMsgId = GM_KEYUP;
    else if (KEYST_DOWN == key.ucState)
        uMsgId = GM_KEYDOWN;
    else if (KEYST_PRESS == key.ucState)
        uMsgId = GM_KEYPRESS;
    else
        uMsgId = 0;

    if (0 != uMsgId) {
        gform::SendMsg(
            static_cast<uint16_t>(uMsgId),
            key.ucKey,
            key.uwPressCnt
        );
    } else if (gform::GetCurrentFormId() == WID_CONSOLE && 0 == key.ucState) {
        GUIFormClose(nullptr);
        GUIFormOpen(WID_MainForm, nullptr);
    }
}
#endif

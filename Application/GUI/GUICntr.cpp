//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GUICntr.cpp
 Version     : V2.04
 By          : Wey. Silver Grid

 Description : GUI controller - adapter layer delegating to the new GForm system.
               Keeps backward compatibility for existing form code while
               all internals are handled by gfc::.

               DEPRECATED: New code should use GFormCentra.h directly.
               This file will be removed in a future cleanup phase.

 Date       : 2026.07.08 (v2.04 — register GDataListForm in s_formTableSim)
              2026.07.08 (v2.03 — GUIFormOpen validation via gfc::FindForm,
                          replaces hardcoded idx>=15 that blocked WID 115..119)
              2026.07.08 (v2.02 — idle timeout now queries GM_IDLE_EXPIRE via
                          gfc::QueryIdleExpire before switching to MainForm)
              2026.06.25 (v2.01 — added touch polling support)
              2026.06.24 (v2.0  — adapter for gform)
              2023.12.05 (v1.10 — original implementation)
*/
//-----------------------------------------------------------------------------//-----------------------------------------------------------------------------
#include "GUICntr.h"

//  New GForm system 
#include "GFormCentra.h"
#include "GFormCentraPlatform.h"

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
#include "GSplashForm.h"
#include "GMenuForm.h"
#include "GMainForm.h"
#include "GLogListForm.h"
#include "GDataListForm.h"
#include "GFatalForm.h"  // used by GUIShowFatalMessage

// MCU-only forms (require FreeRTOS / STM32 HAL / rtc.h)
#ifndef __vmSIMULATOR__
#include "GDevInfoForm.h"
#include "GMessageForm.h"
#include "GUICtrlConfigDialog.h"
#include "GUIUARTConfigDialog.h"
//#include "GConfigForm.h"
#include "GGuageForm.h"
#include "GLoginDialog.h"
#include "GTimeDialog.h"
//#include "GWLGListForm.h"
//#include "GWLGViewForm.h"
#include "GUIMisc.h"
#endif

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
// Form registration list  registers all known forms into gfc::
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
static constexpr FormEntry s_formTableSim[] = {
    { WID_SplashForm,       &FSplashForm,         "Splash"         },
    { WID_MenuForm,         &FMainMenuForm,       "Menu"           },
    { WID_MainForm,         &FMainForm,           "Main"           },
    { WID_DataListForm,     &FDataListForm,       "DataList"       },
    { WID_LogListForm,      &FLogListForm,        "LogList"        },
};

#ifndef __vmSIMULATOR__
static constexpr FormEntry s_formTableMcu[] = {
    { WID_CTRLConfigForm,   &FCTRLConfigForm,     "CTRLConfig"     },
    { WID_UARTConfigForm,   &FUARTConfigForm,     "UARTConfig"     },
    { WID_DevInfoForm,      &FDevInfoForm,        "DevInfo"        },
    { WID_MessageForm,      &FMessageForm,        "Message"        },
//    { WID_ConfigForm,       &FConfigForm,         "Config"         },
    { WID_GuageForm,        &FGuageForm,          "Guage"          },
    { WID_LoginDialog,      &FLoginDialog,        "Login"          },
    { WID_TimeDialog,       &FTimeDialog,         "Time"           },
    { WID_LogListForm,      &FLogListForm,        "LogList"        },
//    { WID_WLGListForm,      &FWavelogListForm,    "WLGList"        },
//    { WID_WLGViewForm,      &FWavelogViewForm,    "WLGView"        },
};
#endif

static constexpr size_t kNumSimForms = sizeof(s_formTableSim) / sizeof(s_formTableSim[0]);
#ifndef __vmSIMULATOR__
static constexpr size_t kNumMcuForms = sizeof(s_formTableMcu) / sizeof(s_formTableMcu[0]);
#endif

//=============================================================================
// Adapter: register all known forms into GForm system
//=============================================================================
static void RegisterAllForms()
{
    // Sim-compatible forms (always available)
    for (size_t i = 0; i < kNumSimForms; ++i) {
        if (s_formTableSim[i].form != nullptr) {
            gfc::RegisterForm(
                static_cast<gfc::FormId>(s_formTableSim[i].id),
                s_formTableSim[i].form,
                s_formTableSim[i].name
            );
        }
    }
#ifndef __vmSIMULATOR__
    // MCU-only forms (require FreeRTOS / STM32 HAL)
    for (size_t i = 0; i < kNumMcuForms; ++i) {
        if (s_formTableMcu[i].form != nullptr) {
            gfc::RegisterForm(
                static_cast<gfc::FormId>(s_formTableMcu[i].id),
                s_formTableMcu[i].form,
                s_formTableMcu[i].name
            );
        }
    }
#endif
}

//=============================================================================
// Adapter: GUITick  delegate to gfc::Tick
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

#if GUI_SUPPORT_TOUCH
    // ── Touch polling ───────────────────────────────────────────────────
    {
        static GUI_PID_STATE s_lastState = {};
        static bool          s_hadTouch  = false;

        GUI_TOUCH_Exec();
        GUI_PID_STATE state;
        if (GUI_TOUCH_GetState(&state)) {
            if (!s_hadTouch) {
                // First reading: treat as touch down
                gfc::TouchEvent(TOUCH_DOWN,
                                  static_cast<uint16_t>(state.x),
                                  static_cast<uint16_t>(state.y));
                FGUIState.uKeyIdle = GUI_GetTime() + TIME_MS_LCDBG;
            } else if (state.Pressed != s_lastState.Pressed) {
                // Pressed state changed
                gfc::TouchEvent(state.Pressed ? TOUCH_DOWN : TOUCH_UP,
                                  static_cast<uint16_t>(state.x),
                                  static_cast<uint16_t>(state.y));
                FGUIState.uKeyIdle = GUI_GetTime() + TIME_MS_LCDBG;
            } else if (state.Pressed &&
                       (state.x != s_lastState.x || state.y != s_lastState.y)) {
                // Position changed while pressed
                gfc::TouchEvent(TOUCH_MOVE,
                                  static_cast<uint16_t>(state.x),
                                  static_cast<uint16_t>(state.y));
                FGUIState.uKeyIdle = GUI_GetTime() + TIME_MS_LCDBG;
            }
            s_lastState = state;
            s_hadTouch  = true;
        } else {
            // Touch lost (finger lifted completely)
            if (s_hadTouch && s_lastState.Pressed) {
                gfc::TouchEvent(TOUCH_UP,
                                  static_cast<uint16_t>(s_lastState.x),
                                  static_cast<uint16_t>(s_lastState.y));
            }
            s_hadTouch = false;
        }
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
        // INDLED_LCDBG(0);
#endif
        // Switch to main if we're not already there and the current form
        // does not veto (e.g. an editor with unsaved changes). The form
        // signals a veto by filling GM_IDLE_EXPIRE.Data.v > 0.
        if (gfc::GetCurrentFormId() != WID_MainForm) {
            if (!gfc::QueryIdleExpire()) {
                needSwitchToMain = true;
            }
        }
    }

#ifndef __vmSIMULATOR__
    osMutexRelease(FGUIState.mutexGUI);
#else
    lock.unlock();
#endif

    if (needSwitchToMain) {
        gfc::ReplaceForm(WID_MainForm, nullptr);
    }

    //  Delegate tick to GForm system 
    gfc::Tick();
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
  INDLED_LCDBG( 1 );
#endif

#ifndef __vmSIMULATOR__
#if osCMSIS >= 0x20000U
    FGUIState.mutexGUI = osMutexNew(nullptr);
#else
    FGUIState.mutexGUI = osMutexCreate(nullptr);
#endif
#endif

    //  Initialize the new GForm system 
    gfc::Init();

    //  Register all forms 
    RegisterAllForms();

    //  Decide initial form 
#ifndef __vmSIMULATOR__
//    if (0 != GetRSTSrc(RRS_PORRST) && 0 == GetRSTSrc(RRS_IWDGRST)) {
        gfc::PushForm(WID_SplashForm, nullptr);
//    } else {
//        gfc::PushForm(WID_MainForm, nullptr);
//    }

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
    gfc::PushForm(WID_SplashForm, nullptr);
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
    // Validate via the GFormCentra registry: any registered form id is accepted,
    // unregistered ids are rejected. This replaces a stale hardcoded range check
    // (idx >= 15) that blocked WID_DataListForm..WID_AboutForm (115..119).
    if (nullptr == gfc::FindForm(static_cast<gfc::FormId>(uFormId))) {
        return false;
    }

    gfc::PushForm(static_cast<gfc::FormId>(uFormId), para);
    return true;
}

//=============================================================================
// Adapter: GUIFormClose
//=============================================================================
bool GUIFormClose(const void* para)
{
    (void)para;
    gfc::PopForm();

    // If stack is now empty, open MainForm as fallback
    if (gfc::IsStackEmpty()) {
        gfc::PushForm(WID_MainForm, nullptr);
    }
    return true;
}

//=============================================================================
// Adapter: GUISendMessage
//=============================================================================
void GUISendMessage(uint32_t uMsgIdent, uint32_t uParam, uint32_t uData)
{
    gfc::SendMsg(
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
    gfc::ReplaceForm(WID_SplashForm, nullptr);
}

//=============================================================================
// Adapter: GUIKeyEnevt (simulator only)
//=============================================================================
#ifdef __vmSIMULATOR__
void GUIKeyEnevt(uint32_t uKey, uint32_t uPressedCnt)
{
    gfc::KeyEvent(uKey, uPressedCnt);
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
        gfc::SendMsg(
            static_cast<uint16_t>(uMsgId),
            key.ucKey,
            key.uwPressCnt
        );
    } else if (gfc::GetCurrentFormId() == WID_CONSOLE && 0 == key.ucState) {
        GUIFormClose(nullptr);
        GUIFormOpen(WID_MainForm, nullptr);
    }
}
#endif

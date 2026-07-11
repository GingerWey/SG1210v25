//-----------------------------------------------------------------------------
/*
 File        : GFormCentra.h
 Version     : V1.02
 By          : Wey. Silver Grid

 Description : GForm — Unified GUI Form management system.
               Replaces GUICntr and FormManager with a clean namespace-based
               API. Supports push/replace/pop navigation, dynamic form
               registration, and message dispatch.

               Key types:
                 FormId          — uint16_t identifier for each form
                 FormTransition  — kPush, kReplace, kPop
                 FormRecord      — metadata + GWinForm* callback pointer

               Usage:
                 1. Each form .cpp registers via FormRegistrar (GFormCentraRegistrar.h).
                 2. Call gfc::Init() once at startup.
                 3. Call gfc::Tick() every 10ms from the GUI loop.
                 4. Navigate via gfc::PushForm / ReplaceForm / PopForm.
                 5. Send messages via gfc::SendMessage / KeyEvent.

 Date        : 2026.06.24 (V1.00 — initial GFormCentra system)
              2026.06.25 (V1.01 — added TouchEvent API for touch screen)
              2026.07.08 (V1.02 — added QueryIdleExpire for GM_IDLE_EXPIRE query chain)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GFC_H
#define GUI_GFC_H

#include <cstdint>
#include <cstddef>

//-----------------------------------------------------------------------------
// Include real type definitions (not forward decls — these are typedef structs)
#include "GUIMessage.h"
#include "GWinTypes.h"

//-----------------------------------------------------------------------------
// Window ID range
//-----------------------------------------------------------------------------
constexpr uint16_t kFormIdBase    = 200;
constexpr uint16_t kFormIdMax     = 231;   // Max 32 forms (231 - 200 + 1)
constexpr uint16_t kFormIdInvalid = 0xFFFF;

//-----------------------------------------------------------------------------
// Legacy Window IDs — kept for backward compatibility with GUICntr.h
//-----------------------------------------------------------------------------
#ifndef WID_FORMBEGIN
  constexpr uint16_t WID_FORMBEGIN      = 100;
  constexpr uint16_t WID_SplashForm     = 100;
  constexpr uint16_t WID_MenuForm       = 101;
  constexpr uint16_t WID_MainForm       = 102;
  constexpr uint16_t WID_EventListForm  = 103;
  constexpr uint16_t WID_CTRLConfigForm = 104;
  constexpr uint16_t WID_UARTConfigForm = 105;
  constexpr uint16_t WID_DevInfoForm    = 106;
  constexpr uint16_t WID_MessageForm    = 107;
  constexpr uint16_t WID_ConfigForm     = 108;
  constexpr uint16_t WID_GuageForm      = 109;
  constexpr uint16_t WID_LoginDialog    = 111;
  constexpr uint16_t WID_TimeDialog     = 112;
  constexpr uint16_t WID_WLGListForm    = 113;
  constexpr uint16_t WID_WLGViewForm    = 114;
  constexpr uint16_t WID_DataListForm   = 115;
  constexpr uint16_t WID_SystemLogForm  = 116;
  constexpr uint16_t WID_DeviceTestForm = 117;
  constexpr uint16_t WID_EthernetConfigForm = 118;
  constexpr uint16_t WID_AboutForm      = 119;
  constexpr uint16_t WID_LogListForm    = 120;
#endif

//=============================================================================
namespace gfc {

//-----------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------
using FormId = uint16_t;

//-----------------------------------------------------------------------------
// Transition mode for OpenForm / navigation
//-----------------------------------------------------------------------------
enum class FormTransition : uint8_t {
    kPush,      // Push new form onto stack, show it
    kReplace,   // Pop current form, then push and show new form
    kPop        // Close current, restore previous from stack
};

//-----------------------------------------------------------------------------
// Metadata record for a registered form
//-----------------------------------------------------------------------------
struct FormRecord {
    const GWinForm* callbacks;   // The four C callbacks (pInit/pShow/pClose/pMsg)
    const char*     name;        // Debug name (UTF-8), may be nullptr
    uint16_t        flags;       // Reserved for animation / transition hints
};

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
constexpr size_t kMaxForms  = 32;   // Registry capacity
constexpr size_t kMaxStack  = 16;   // Navigation stack depth

//=============================================================================
// Lifecycle
//=============================================================================

/// Initialize the GFormCentra system. Must be called once before any other function.
/// Clears the registry, stack, and pending message queue.
void Init();

/// Periodic tick (call every ~10ms from the GUI loop).
/// Drains pending messages and delivers GM_TIMER_TICK to the current form.
void Tick();

/// Run one iteration of the main loop. Returns current stack depth.
/// For backward compatibility with the old GUICenter() pattern.
size_t Run();

//=============================================================================
// Registration
//=============================================================================

/// Register a form. Called automatically by FormRegistrar constructors.
/// Re-registering the same id overwrites the previous entry.
void RegisterForm(FormId id, const GWinForm* form, const char* name);

/// Look up a registered form by id. Returns nullptr if not found.
const FormRecord* FindForm(FormId id);

/// Unregister a form.
void UnregisterForm(FormId id);

//=============================================================================
// Navigation
//=============================================================================

/// Open a form with explicit transition control.
void OpenForm(FormId id, const void* para = nullptr,
              FormTransition transition = FormTransition::kPush);

/// Push a new form onto the stack and show it.
inline void PushForm(FormId id, const void* para = nullptr) {
    OpenForm(id, para, FormTransition::kPush);
}

/// Replace the current form with a new one (pop + push).
inline void ReplaceForm(FormId id, const void* para = nullptr) {
    OpenForm(id, para, FormTransition::kReplace);
}

/// Close the current form and restore the previous one.
inline void PopForm() {
    OpenForm(kFormIdInvalid, nullptr, FormTransition::kPop);
}

/// Close the current form without showing a replacement.
/// If stack has a previous form, it is restored.
void CloseCurrentForm();

/// Get the id of the currently active form (top of stack).
/// Returns kFormIdInvalid if stack is empty.
FormId GetCurrentFormId();

//=============================================================================
// Stack introspection
//=============================================================================

/// Number of entries on the navigation stack.
size_t StackDepth();

/// Is the navigation stack empty?
bool IsStackEmpty();

/// Peek at a form id on the stack (0 = top, 1 = one below, etc.).
/// Returns kFormIdInvalid if depth is out of range.
FormId GetStackForm(size_t depth);

/// True if the given form is anywhere on the stack.
bool IsFormOnStack(FormId id);

//=============================================================================
// Messaging
//=============================================================================

/// Send a message to the current (top) form.
/// @param msgId   Message identifier (GM_PAINT, GM_KEYDOWN, etc.)
/// @param param   Optional parameter
/// @param value   Optional integer data value
void SendMsg(uint16_t msgId, uint16_t param = 0, int32_t value = 0);

/// Send a message with a pointer payload to the current form.
void SendMsgPtr(uint16_t msgId, uint16_t param, const void* data);

/// Post a message for deferred delivery on the next Tick() call.
/// Safe to call from interrupt / non-GUI thread context.
void PostMsg(uint16_t msgId, uint16_t param = 0, int32_t value = 0);

/// Broadcast a message to ALL forms currently on the stack (bottom to top).
void BroadcastMsg(uint16_t msgId, uint16_t param = 0, int32_t value = 0);

/// Query the current form whether it wants to block the idle-timeout return
/// to MainForm. Sends GM_IDLE_EXPIRE to the current form; returns true if the
/// form vetoed by setting Data.v > 0.
bool QueryIdleExpire();

/// Handle a key event (translate raw key + press count to GM_KEYDOWN/UP/PRESS).
void KeyEvent(uint32_t key, uint32_t pressedCnt);

/// Handle a touch event (translate action + coordinates to GM_TOUCH).
/// Only active when GUI_SUPPORT_TOUCH is enabled in GUIConf.h.
/// @param action  TOUCH_DOWN, TOUCH_MOVE, or TOUCH_UP
/// @param x       X coordinate (display pixels)
/// @param y       Y coordinate (display pixels)
void TouchEvent(uint16_t action, uint16_t x, uint16_t y);

}  // namespace gfc

//-----------------------------------------------------------------------------
#endif  // GUI_GFC_H

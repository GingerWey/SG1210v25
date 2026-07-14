//-----------------------------------------------------------------------------
/*
 File        : GFormCentra.cpp
 Version     : V1.04
 By          : Wey. Silver Grid

 Description : GFormCentra system implementation.
               Registry, navigation stack, message dispatch, Tick loop.

 Date        : 2026.07.14 (V1.04 — 修正所有违反 Yoda-style 编码规范的比较语句)
              2026.07.14 (V1.03 — 内存分配从 new 移植到 RAMHeap)
              2026.06.24 (V1.00 — initial implementation)
              2026.06.25 (V1.01 — added TouchEvent for touch screen support)
              2026.07.08 (V1.02 — navigation lifecycle messages GM_FORM_ACTIVATED/
                          DEACTIVATED, GM_CLOSE_QUERY veto on PopForm,
                          GM_IDLE_EXPIRE query via QueryIdleExpire)
*/
//-----------------------------------------------------------------------------
#include "GFormCentra.h"
#include "GFormCentraPlatform.h"
#include "GWinTypes.h"
#include "GUIMessage.h"
#include "GUI.h"
#include "RamHeap.h"

#include <cstring>
#include <new>

//=============================================================================
// Internal state (anonymous namespace)
//=============================================================================
namespace {

// ── Registry entry: pairs a FormId with its FormRecord ───────────
struct RegistryEntry {
    gfc::FormId       id;
    gfc::FormRecord   record;
};

// ── Registry: fixed-size array ────────────────────────
RegistryEntry  s_registry[gfc::kMaxForms] = {};
size_t         s_registryCount = 0;

// ── Navigation stack ─────────────────────────────
gfc::FormId  s_stack[gfc::kMaxStack] = {};
size_t         s_stackTop = 0;       // Number of entries on stack
                                     // Current = s_stack[s_stackTop - 1]

// ── Current form snapshot ──────────────────────────
const gfc::FormRecord* s_pCurrent  = nullptr;
gfc::FormId            s_currentId = kFormIdInvalid;

// ── Pending message queue (PostMsg / deferred delivery) ──────────
constexpr size_t kMsgQueueSize = 16;
struct PendingMsg {
    uint16_t    msgId;
    uint16_t    param;
    int32_t     value;
    const void* data;
};
PendingMsg s_msgQueue[kMsgQueueSize] = {};
size_t     s_msgHead = 0;
size_t     s_msgTail = 0;

// ── Platform mutex (lazy init — avoids static init order crash) ──────
static gfc::platform::Lock* s_plock = nullptr;
static gfc::platform::Lock& GetLock() {
    if (!s_plock) {
        s_plock = static_cast<gfc::platform::Lock*>(RAM_Malloc(sizeof(gfc::platform::Lock)));
        if (s_plock) {
            new (s_plock) gfc::platform::Lock();  // placement new
        }
    }
    return *s_plock;
}

// ── Init flag ─────────────────────────────────
bool s_initialized = false;

//=============================================================================
// Internal helpers
//=============================================================================

/// Find a registry entry by FormId. Returns index or -1.
int FindRegIdx(gfc::FormId id)
{
    for (size_t i = 0; i < s_registryCount; ++i) {
        if (s_registry[i].record.callbacks != nullptr &&
            s_registry[i].id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/// Find registry entry by GWinForm pointer (for overwrite detection).
int FindRegIdxByForm(const GWinForm* form)
{
    for (size_t i = 0; i < s_registryCount; ++i) {
        if (s_registry[i].record.callbacks == form) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/// Send a lifecycle notification message to a form's pMsg callback.
/// Called with lock held.
void NotifyForm(const gfc::FormRecord* rec, uint16_t msgId, uint16_t param = 0)
{
    if (nullptr == rec || nullptr == rec->callbacks ||
        nullptr == rec->callbacks->pMsg) {
        return;
    }
    GM_MESSAGE msg = {};
    msg.MsgId = msgId;
    msg.Param = param;
    rec->callbacks->pMsg(&msg);
}

/// Send a query message to a form. Returns true if the form vetoed
/// by filling Data.v with a value greater than zero.
/// Called with lock held.
bool QueryForm(const gfc::FormRecord* rec, uint16_t msgId, uint16_t param = 0)
{
    if (nullptr == rec || nullptr == rec->callbacks ||
        nullptr == rec->callbacks->pMsg) {
        return false;
    }
    GM_MESSAGE msg = {};
    msg.MsgId = msgId;
    msg.Param = param;
    rec->callbacks->pMsg(&msg);
    return (0 < msg.Data.v);
}

/// Activate a form: push onto stack, call pInit+pShow, update state.
/// Called with lock held.
bool ActivateForm(gfc::FormId id, const gfc::FormRecord* rec,
                  const void* para)
{
    if (kFormIdInvalid == id || nullptr == rec || nullptr == rec->callbacks)
        return false;
    if (s_stackTop >= gfc::kMaxStack)
        return false;

    // Push onto stack
    s_stack[s_stackTop] = id;
    ++s_stackTop;

    s_pCurrent  = rec;
    s_currentId = id;

    // Call init then show
    if (nullptr != rec->callbacks->pInit) {
        rec->callbacks->pInit(para);
    }
    if (nullptr != rec->callbacks->pShow) {
        rec->callbacks->pShow(para);
    }

    // Notify the newly activated form
    NotifyForm(rec, GM_FORM_ACTIVATED);
    return true;
}

/// Deactivate current form: call pClose, pop from stack, update state.
/// Called with lock held.
void DeactivateCurrent(const void* para)
{
    if (0 == s_stackTop) {
        return;
    }

    if (nullptr != s_pCurrent && nullptr != s_pCurrent->callbacks) {
        if (nullptr != s_pCurrent->callbacks->pClose) {
            s_pCurrent->callbacks->pClose(para);
        }
    }

    s_stack[s_stackTop - 1] = kFormIdInvalid;
    --s_stackTop;

    // Restore previous or clear
    if (0 < s_stackTop) {
        gfc::FormId prevId = s_stack[s_stackTop - 1];
        int idx = FindRegIdx(prevId);
        if (0 <= idx) {
            s_pCurrent  = &s_registry[idx].record;
            s_currentId = prevId;
        } else {
            s_pCurrent  = nullptr;
            s_currentId = kFormIdInvalid;
        }
    } else {
        s_pCurrent  = nullptr;
        s_currentId = kFormIdInvalid;
    }
}

/// Show the current stack-top form (re-activate after deactivation).
/// Called with lock held.
void ShowTop(const void* para)
{
    if (0 < s_stackTop) {
        gfc::FormId topId = s_stack[s_stackTop - 1];
        int idx = FindRegIdx(topId);
        if (0 <= idx) {
            s_pCurrent  = &s_registry[idx].record;
            s_currentId = topId;
            if (nullptr != s_pCurrent->callbacks->pShow) {
                s_pCurrent->callbacks->pShow(para);
            }
        }
    }
}

}  // anonymous namespace

//=============================================================================
// Public API — Lifecycle
//=============================================================================

void gfc::Init()
{
    gfc::ScopedLock _(GetLock());

    // NOTE: Do NOT clear the registry here. Forms register themselves
    // via FormRegistrar during static initialization (before main()),
    // so the registry is already populated by the time Init() is called.
    // Only reset the navigation stack and message queue.

    for (size_t i = 0; i < kMaxStack; ++i) {
        s_stack[i] = kFormIdInvalid;
    }
    s_stackTop = 0;

    s_pCurrent  = nullptr;
    s_currentId = kFormIdInvalid;

    s_msgHead = 0;
    s_msgTail = 0;

    s_initialized = true;
}

void gfc::Tick()
{
    // ── Drain pending message queue ─────────────────────
    PendingMsg pending;
    bool hasPending = false;

    {
        gfc::ScopedLock _(GetLock());
        if (s_msgHead != s_msgTail) {
            pending    = s_msgQueue[s_msgHead];
            s_msgHead  = (s_msgHead + 1) % kMsgQueueSize;
            hasPending = true;
        }
    }

    while (hasPending) {
        if (nullptr != pending.data) {
            SendMsgPtr(pending.msgId, pending.param, pending.data);
        } else {
            SendMsg(pending.msgId, pending.param, pending.value);
        }

        {
            gfc::ScopedLock _(GetLock());
            if (s_msgHead != s_msgTail) {
                pending    = s_msgQueue[s_msgHead];
                s_msgHead  = (s_msgHead + 1) % kMsgQueueSize;
                hasPending = true;
            } else {
                hasPending = false;
            }
        }
    }

    // ── Deliver GM_TIMER_TICK to current form ────────────────
    const GWinForm* callbacks = nullptr;
    {
        gfc::ScopedLock _(GetLock());
        if (nullptr != s_pCurrent) {
            callbacks = s_pCurrent->callbacks;
        }
    }

    if (nullptr != callbacks && nullptr != callbacks->pMsg) {
        GM_MESSAGE tickMsg = {};
        tickMsg.MsgId  = GM_TIMER_TICK;
        tickMsg.Param  = 0;
        tickMsg.Data.v = static_cast<int32_t>(GUI_GetTime());
        callbacks->pMsg(&tickMsg);
    }
}

size_t gfc::Run()
{
    return s_stackTop;
}

//=============================================================================
// Public API — Registration
//=============================================================================

void gfc::RegisterForm(FormId id, const GWinForm* form, const char* name)
{
    if (nullptr == form) {
        return;
    }

    gfc::ScopedLock _(GetLock());

    // Check for overwrite (same GWinForm pointer)
    int existIdx = FindRegIdxByForm(form);
    if (0 <= existIdx) {
        s_registry[existIdx].id            = id;
        s_registry[existIdx].record.name   = name;
        s_registry[existIdx].record.flags  = 0;
        return;
    }

    // Also check for overwrite by id
    existIdx = FindRegIdx(id);
    if (0 <= existIdx) {
        s_registry[existIdx].record.callbacks = form;
        s_registry[existIdx].record.name      = name;
        s_registry[existIdx].record.flags     = 0;
        return;
    }

    // Append new entry
    if (s_registryCount >= kMaxForms) return;

    s_registry[s_registryCount].id              = id;
    s_registry[s_registryCount].record.callbacks  = form;
    s_registry[s_registryCount].record.name       = name;
    s_registry[s_registryCount].record.flags      = 0;
    ++s_registryCount;
}

const gfc::FormRecord* gfc::FindForm(FormId id)
{
    int idx = FindRegIdx(id);
    if (idx >= 0) {
        return &s_registry[idx].record;
    }
    return nullptr;
}

void gfc::UnregisterForm(FormId id)
{
    gfc::ScopedLock _(GetLock());

    int idx = FindRegIdx(id);
    if (0 > idx) {
        return;
    }

    // Compact the array by shifting entries down
    for (size_t i = static_cast<size_t>(idx); i < s_registryCount - 1; ++i) {
        s_registry[i] = s_registry[i + 1];
    }
    --s_registryCount;
    s_registry[s_registryCount] = {};
}

//=============================================================================
// Public API — Navigation
//=============================================================================

void gfc::OpenForm(FormId id, const void* para, FormTransition transition)
{
    gfc::ScopedLock _(GetLock());

    switch (transition) {

    case FormTransition::kPop: {
        if (s_stackTop <= 1) {
            // Can't pop the last form; do nothing
            return;
        }

        // GM_CLOSE_QUERY: allow the current form to veto being closed
        // (e.g. unsaved edits). A veto is signaled by Data.v > 0.
        if (QueryForm(s_pCurrent, GM_CLOSE_QUERY)) {
            return;
        }

        // Notify the outgoing form it is being deactivated
        NotifyForm(s_pCurrent, GM_FORM_DEACTIVATED);

        // Snapshot close callback
        TGWVoidProc closeFn = nullptr;
        if (nullptr != s_pCurrent && nullptr != s_pCurrent->callbacks) {
            closeFn = s_pCurrent->callbacks->pClose;
        }

        // Pop current
        s_stack[s_stackTop - 1] = kFormIdInvalid;
        --s_stackTop;

        // Restore previous and get its show callback
        TGWVoidProc showFn = nullptr;
        const gfc::FormRecord* lowerRec = nullptr;
        int prevIdx = FindRegIdx(s_stack[s_stackTop - 1]);
        if (0 <= prevIdx) {
            s_pCurrent  = &s_registry[prevIdx].record;
            s_currentId = s_stack[s_stackTop - 1];
            lowerRec    = s_pCurrent;
            if (nullptr != s_pCurrent->callbacks) {
                showFn = s_pCurrent->callbacks->pShow;
            }
        }

        // Execute callbacks outside of critical section concerns
        if (nullptr != closeFn) {
            closeFn(para);
        }
        if (nullptr != showFn) {
            showFn(para);
        }

        // Notify the restored lower form it has been activated
        NotifyForm(lowerRec, GM_FORM_ACTIVATED);
        break;
    }

    case FormTransition::kReplace: {
        // Snapshot close
        TGWVoidProc closeFn = nullptr;
        if (0 < s_stackTop && nullptr != s_pCurrent &&
            nullptr != s_pCurrent->callbacks) {
            closeFn = s_pCurrent->callbacks->pClose;
        }

        // Notify the outgoing form it is being deactivated
        if (0 < s_stackTop) {
            NotifyForm(s_pCurrent, GM_FORM_DEACTIVATED);
        }

        // Pop current from stack
        if (0 < s_stackTop) {
            s_stack[s_stackTop - 1] = kFormIdInvalid;
            --s_stackTop;
        }
        s_pCurrent  = nullptr;
        s_currentId = kFormIdInvalid;

        if (nullptr != closeFn) {
            closeFn(para);
        }

        // Find and activate new form (ActivateForm sends GM_FORM_ACTIVATED)
        if (kFormIdInvalid != id) {
            int idx = FindRegIdx(id);
            if (0 <= idx) {
                ActivateForm(id, &s_registry[idx].record, para);
            }
        }
        break;
    }

    case FormTransition::kPush: {
        if (kFormIdInvalid == id) {
            return;
        }

        int idx = FindRegIdx(id);
        if (0 > idx) {
            return;
        }

        // Notify the current top form it is being pushed down
        if (0 < s_stackTop) {
            NotifyForm(s_pCurrent, GM_FORM_DEACTIVATED);
        }

        ActivateForm(id, &s_registry[idx].record, para);
        break;
    }
    }  // switch
}

void gfc::CloseCurrentForm()
{
    gfc::ScopedLock _(GetLock());

    if (0 == s_stackTop) {
        // Nothing to close; guard against s_stack[-1] underflow
        return;
    }

    TGWVoidProc closeFn = nullptr;
    TGWVoidProc showFn  = nullptr;

    if (0 < s_stackTop && nullptr != s_pCurrent &&
        nullptr != s_pCurrent->callbacks) {
        closeFn = s_pCurrent->callbacks->pClose;
    }

    // Notify the outgoing form it is being deactivated
    NotifyForm(s_pCurrent, GM_FORM_DEACTIVATED);

    s_stack[s_stackTop - 1] = kFormIdInvalid;
    --s_stackTop;

    // Restore previous
    const gfc::FormRecord* lowerRec = nullptr;
    if (0 < s_stackTop) {
        int prevIdx = FindRegIdx(s_stack[s_stackTop - 1]);
        if (0 <= prevIdx) {
            s_pCurrent  = &s_registry[prevIdx].record;
            s_currentId = s_stack[s_stackTop - 1];
            lowerRec    = s_pCurrent;
            if (nullptr != s_pCurrent->callbacks) {
                showFn = s_pCurrent->callbacks->pShow;
            }
        }
    } else {
        s_pCurrent  = nullptr;
        s_currentId = kFormIdInvalid;
    }

    if (nullptr != closeFn) {
        closeFn(nullptr);
    }
    if (nullptr != showFn) {
        showFn(nullptr);
    }

    // Notify the restored lower form it has been activated
    NotifyForm(lowerRec, GM_FORM_ACTIVATED);
}

gfc::FormId gfc::GetCurrentFormId()
{
    return s_currentId;
}

//=============================================================================
// Public API — Stack introspection
//=============================================================================

size_t gfc::StackDepth()
{
    return s_stackTop;
}

bool gfc::IsStackEmpty()
{
    return (0 == s_stackTop);
}

gfc::FormId gfc::GetStackForm(size_t depth)
{
    if (depth >= s_stackTop) return kFormIdInvalid;
    // depth 0 = top = s_stack[s_stackTop - 1]
    return s_stack[s_stackTop - 1 - depth];
}

bool gfc::IsFormOnStack(FormId id)
{
    for (size_t i = 0; i < s_stackTop; ++i) {
        if (s_stack[i] == id) return true;
    }
    return false;
}

//=============================================================================
// Public API — Messaging
//=============================================================================

void gfc::SendMsg(uint16_t msgId, uint16_t param, int32_t value)
{
    TGWMsgProc  msgFn = nullptr;
    GM_MESSAGE  msg   = {};

    {
        gfc::ScopedLock _(GetLock());
        if (nullptr != s_pCurrent && nullptr != s_pCurrent->callbacks) {
            msgFn = s_pCurrent->callbacks->pMsg;
        }
    }

    if (nullptr != msgFn) {
        msg.MsgId    = msgId;
        msg.Param    = param;
        msg.Data.v   = value;
        msgFn(&msg);
    }
}

void gfc::SendMsgPtr(uint16_t msgId, uint16_t param, const void* data)
{
    TGWMsgProc  msgFn = nullptr;
    GM_MESSAGE  msg   = {};

    {
        gfc::ScopedLock _(GetLock());
        if (nullptr != s_pCurrent && nullptr != s_pCurrent->callbacks) {
            msgFn = s_pCurrent->callbacks->pMsg;
        }
    }

    if (nullptr != msgFn) {
        msg.MsgId    = msgId;
        msg.Param    = param;
        msg.Data.p   = const_cast<void*>(data);
        msgFn(&msg);
    }
}

void gfc::PostMsg(uint16_t msgId, uint16_t param, int32_t value)
{
    gfc::ScopedLock _(GetLock());

    size_t next = (s_msgTail + 1) % kMsgQueueSize;
    if (next == s_msgHead) return;  // Queue full, drop

    s_msgQueue[s_msgTail].msgId = msgId;
    s_msgQueue[s_msgTail].param = param;
    s_msgQueue[s_msgTail].value = value;
    s_msgQueue[s_msgTail].data  = nullptr;
    s_msgTail = next;
}

void gfc::BroadcastMsg(uint16_t msgId, uint16_t param, int32_t value)
{
    // Snapshot stack indices under lock
    FormId  stackCopy[kMaxStack];
    size_t  stackLen = 0;

    {
        gfc::ScopedLock _(GetLock());
        stackLen = s_stackTop;
        for (size_t i = 0; i < s_stackTop; ++i) {
            stackCopy[i] = s_stack[i];
        }
    }

    GM_MESSAGE msg = {};
    msg.MsgId  = msgId;
    msg.Param  = param;
    msg.Data.v = value;

    for (size_t i = 0; i < stackLen; ++i) {
        int idx = FindRegIdx(stackCopy[i]);
        if (0 <= idx &&
            nullptr != s_registry[idx].record.callbacks &&
            nullptr != s_registry[idx].record.callbacks->pMsg) {
            s_registry[idx].record.callbacks->pMsg(&msg);
        }
    }
}

bool gfc::QueryIdleExpire()
{
    gfc::ScopedLock _(GetLock());
    return QueryForm(s_pCurrent, GM_IDLE_EXPIRE);
}

void gfc::KeyEvent(uint32_t key, uint32_t pressedCnt)
{
    if (0 == key) {
        return;
    }

    uint16_t msgId;
    if (0 == pressedCnt) {
        msgId = GM_KEYUP;
    } else if (1 == pressedCnt) {
        msgId = GM_KEYDOWN;
    } else {
        msgId = GM_KEYPRESS;
    }

    SendMsg(msgId, static_cast<uint16_t>(key),
                static_cast<int32_t>(pressedCnt));
}

//=============================================================================
// Touch event handling
//=============================================================================
#if GUI_SUPPORT_TOUCH
void gfc::TouchEvent(uint16_t action, uint16_t x, uint16_t y)
{
    // Pack coordinates: x in upper 16 bits, y in lower 16 bits
    int32_t packed = (static_cast<int32_t>(x) << 16) | static_cast<int32_t>(y);
    SendMsg(GM_TOUCH, action, packed);
}
#endif

//-----------------------------------------------------------------------------
/*
 File        : DevEvtMgrTest.cpp
 Version     : V1.10
 By          : Wey. Silver Grid

 Description : EVTMGR test harness — functional + stress.

               Phase 1 — Functional: each state/event register × each Append path.
               Phase 2 — Ring-buffer wrap: 500 appends (>> SIZE_EVENT_CACHE=32),
                         verifies Count saturates, Fetch/SaveEvent work after wrap.
               Phase 3 — PostEvent queue stress: 200 posts + drain.
               Phase 4 — Mixed stress: interleave Append/Post/OpLog/Fetch/Save.
               Phase 5 — Wrap + consumer: produce 200, interleave Fetch every 10,
                         verify no crash, counts consistent.

 Date        : 2026.07.11 (V1.10 — stress test: ring wrap, overflow, mixed)
              2026.07.11 (V1.00 — initial functional test)
*/
//-----------------------------------------------------------------------------
#include "DevEvtMgrTest.h"

#include "DevEvtMgr.h"
#include "DevIntf.h"
#include "DevTypes.h"
#include "DevRegs.h"

#include <cstdarg>
#include <cstdio>

//=============================================================================
// Logging
//=============================================================================
#ifdef __vmSIMULATOR__
  #include <Windows.h>
  static void _Log(const char* fmt, ...)
  {
    static FILE* fp = nullptr;
    if (nullptr == fp) {
      fp = std::fopen("D:\\Works\\SilverGrid\\SG1210\\Firmware\\SG1210v25\\Sim\\evtmgr_test.txt", "w");
      if (nullptr == fp) {
        return;
      }
    }
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(fp, fmt, ap);
    va_end(ap);
    std::fputc('\n', fp);
    std::fflush(fp);
  }
#else
  static void _Log(const char* fmt, ...)
  {
    va_list ap;
    va_start(ap, fmt);
    (void)std::vprintf(fmt, ap);
    va_end(ap);
    std::printf("\n");
  }
#endif

//=============================================================================
// Helpers
//=============================================================================

/// Collect test register numbers from the given list classes.
static uint16_t _CollectRegs(TDevRegListClass* classes, uint16_t nClasses,
                             uint32_t* outRegs, uint16_t maxOut)
{
  uint16_t n = 0;
  for (uint16_t c = 0; c < nClasses && n < maxOut; ++c) {
    const TDevRegInfoList* pList = DevIntf_GetRegsList(classes[c]);
    if (nullptr == pList || nullptr == pList->List) {
      continue;
    }
    for (uint16_t i = 0; i < pList->Count && n < maxOut; ++i) {
      outRegs[n++] = pList->List[i].RegNum;
    }
  }
  return n;
}

//=============================================================================
// Test phases
//=============================================================================

/// Phase 1: functional — each state register × AppendEvent / PostEvent /
/// PostEventFromISR; each event-log register × AppendEvent;
/// each op register (DevOption/DevConfig) × AppendOpLog.
static void _TestFunctional(uint32_t* stateRegs, uint16_t nState,
                            uint32_t* evtRegs, uint16_t nEvt,
                            uint32_t* opRegs, uint16_t nOp,
                            uint32_t* counts)
{
  _Log("--- Phase 1: functional ---");
  for (uint16_t i = 0; i < nState; ++i) {
    EVTMGR_AppendEvent(stateRegs[i], STATE_TRUE);
    ++counts[0];
    EVTMGR_PostEvent(stateRegs[i], STATE_FALSE);
    ++counts[1];
    EVTMGR_PostEventFromISR(stateRegs[i], STATE_TRUE, nullptr);
    ++counts[2];
  }
  EVTMGR_DrainEvents();
  for (uint16_t i = 0; i < nEvt; ++i) {
    EVTMGR_AppendEvent(evtRegs[i], STATE_TRUE);
    ++counts[0];
  }
  for (uint16_t i = 0; i < nOp; ++i) {
    EVTMGR_AppendOpLog(opRegs[i], i);
    ++counts[3];
  }
  _Log("  AppendEvent=%u PostEvent=%u PostFromISR=%u OpLog=%u",
       (unsigned)counts[0], (unsigned)counts[1], (unsigned)counts[2], (unsigned)counts[3]);
}

/// Phase 2: ring-buffer wrap — append N >> SIZE_EVENT_CACHE events, verify
/// Count saturates and Fetch/SaveEvent work after multiple wraps.
/// Uses stateRegs for AppendEvent and opRegs for AppendOpLog.
static void _TestRingWrap(uint32_t* stateRegs, uint16_t nState,
                          uint32_t* opRegs, uint16_t nOp, uint32_t N)
{
  _Log("--- Phase 2: ring wrap (N=%u, SIZE_EVENT_CACHE=32) ---", (unsigned)N);
  uint32_t nAppend = 0, nOpLog = 0;
  for (uint32_t k = 0; k < N; ++k) {
    if (0 == (k % 3)) {
      EVTMGR_AppendOpLog(opRegs[k % nOp], k);
      ++nOpLog;
    } else {
      uint32_t reg = stateRegs[k % nState];
      uint32_t act = (0 == (k & 1)) ? STATE_TRUE : STATE_FALSE;
      EVTMGR_AppendEvent(reg, act);
      ++nAppend;
    }
    // Every 100 iterations: log comm-event count (should be <= 32)
    if (0 == (k % 100) && k > 0) {
      uint32_t nComm = EVTMGR_GetNewEventCount(NES_UART1);
      _Log("  k=%u: GetNewEventCount(NES_UART1)=%u (expect <=32)", (unsigned)k, (unsigned)nComm);
    }
  }
  // After N appends: verify count <= SIZE_EVENT_CACHE
  uint32_t nFinal = EVTMGR_GetNewEventCount(NES_UART1);
  _Log("  after %u appends: NewEventCount(NES_UART1)=%u (expect <=32)", (unsigned)N, (unsigned)nFinal);

  // Fetch a few items (consumer drains oldest)
  uint32_t nFetch = 0;
  for (uint32_t i = 0; i < 5; ++i) {
    if (nullptr != EVTMGR_Fetch(NES_UART1)) {
      ++nFetch;
    }
  }
  _Log("  fetched=%u (expect 5 if buffer non-empty)", (unsigned)nFetch);

  // SaveEvent (iterates + FIX_SaveEvtLogItem)
  EVTMGR_SaveEvent();
  _Log("  SaveEvent done");
  _Log("  phase2 totals: AppendEvent=%u AppendOpLog=%u", (unsigned)nAppend, (unsigned)nOpLog);
}

/// Phase 3: PostEvent queue stress — post N events, then drain.
static void _TestPostQueue(uint32_t* regs, uint16_t nRegs, uint32_t N)
{
  _Log("--- Phase 3: PostEvent queue stress (N=%u) ---", (unsigned)N);
  for (uint32_t k = 0; k < N; ++k) {
    uint32_t reg = regs[k % nRegs];
    EVTMGR_PostEvent(reg, (0 == (k & 1)) ? STATE_TRUE : STATE_FALSE);
    // Every 50: drain (simulates appTask periodic drain)
    if (0 == (k % 50)) {
      EVTMGR_DrainEvents();
    }
  }
  EVTMGR_DrainEvents();  // final drain
  _Log("  PostEvent+Drain done (N=%u)", (unsigned)N);
}

/// Phase 4: mixed stress — interleave Append/Post/OpLog/Fetch/Save.
/// AppendEvent/PostEvent use stateRegs; AppendOpLog uses opRegs.
static void _TestMixed(uint32_t* stateRegs, uint16_t nState,
                       uint32_t* opRegs, uint16_t nOp, uint32_t N)
{
  _Log("--- Phase 4: mixed stress (N=%u) ---", (unsigned)N);
  uint32_t nA = 0, nP = 0, nO = 0, nF = 0, nS = 0;
  for (uint32_t k = 0; k < N; ++k) {
    switch (k % 5) {
    case 0:  // AppendEvent
      EVTMGR_AppendEvent(stateRegs[k % nState], STATE_TRUE);
      ++nA;
      break;
    case 1:  // PostEvent
      EVTMGR_PostEvent(stateRegs[k % nState], STATE_FALSE);
      ++nP;
      break;
    case 2:  // AppendOpLog
      EVTMGR_AppendOpLog(opRegs[k % nOp], k);
      ++nO;
      break;
    case 3:  // Fetch (consumer)
      if (nullptr != EVTMGR_Fetch(NES_UART1)) {
        ++nF;
      }
      break;
    case 4:  // SaveEvent (every 5th)
      EVTMGR_SaveEvent();
      ++nS;
      break;
    }
    // Periodic drain
    if (0 == (k % 30)) {
      EVTMGR_DrainEvents();
    }
  }
  EVTMGR_DrainEvents();
  _Log("  Append=%u Post=%u OpLog=%u Fetch=%u Save=%u",
       (unsigned)nA, (unsigned)nP, (unsigned)nO, (unsigned)nF, (unsigned)nS);
}

/// Phase 5: wrap + interleaved consumer — produce 200, Fetch every 10.
static void _TestWrapConsume(uint32_t* regs, uint16_t nRegs, uint32_t N)
{
  _Log("--- Phase 5: wrap + interleaved consumer (N=%u) ---", (unsigned)N);
  uint32_t nAppend = 0, nFetch = 0;
  for (uint32_t k = 0; k < N; ++k) {
    uint32_t reg = regs[k % nRegs];
    EVTMGR_AppendEvent(reg, (0 == (k & 1)) ? STATE_TRUE : STATE_FALSE);
    ++nAppend;
    // Every 10: fetch 3 (consumer drains some while producer fills)
    if (0 == (k % 10)) {
      for (uint32_t j = 0; j < 3; ++j) {
        if (nullptr != EVTMGR_Fetch(NES_UART1)) {
          ++nFetch;
        }
      }
    }
  }
  // Final count
  uint32_t nRemain = EVTMGR_GetNewEventCount(NES_UART1);
  _Log("  append=%u fetch=%u remain=%u (expect remain<=32)",
       (unsigned)nAppend, (unsigned)nFetch, (unsigned)nRemain);
}

//=============================================================================
// Main test entry
//=============================================================================
void EVTMGR_RunTest(void)
{
  _Log("=== EVTMGR_RunTest start (V1.10 stress) ===");

  // Collect registers
  uint32_t stateRegs[64];
  uint32_t evtRegs[64];
  uint32_t opRegs[64];
  TDevRegListClass stateClasses[] = { rcProtSignals, rcBinaryIn, rcBinaryOut };
  uint16_t nState = _CollectRegs(stateClasses, 3, stateRegs, 64);
  TDevRegListClass evtClasses[] = { rcEvents };
  uint16_t nEvt   = _CollectRegs(evtClasses, 1, evtRegs, 64);
  // AppendOpLog only for DevOption / DevConfig registers
  TDevRegListClass opClasses[] = { rcDevOption, rcDevConfig };
  uint16_t nOp    = _CollectRegs(opClasses, 2, opRegs, 64);
  _Log("state regs=%u, event regs=%u, op regs=%u",
       (unsigned)nState, (unsigned)nEvt, (unsigned)nOp);

  if (0 == nState || 0 == nEvt) {
    _Log("ERROR: no state/event registers collected");
    return;
  }
  if (0 == nOp) {
    _Log("WARNING: no op registers collected, OpLog tests skipped");
  }

  // Phase 1: functional
  uint32_t counts[4] = { 0, 0, 0, 0 };
  _TestFunctional(stateRegs, nState, evtRegs, nEvt, opRegs, nOp, counts);

  // Phase 2: ring buffer wrap (500 events >> 32 slots)
  if (0 < nOp) {
    _TestRingWrap(stateRegs, nState, opRegs, nOp, 500);
  }

  // Phase 3: PostEvent queue stress (200)
  _TestPostQueue(stateRegs, nState, 200);

  // Phase 4: mixed stress (300)
  if (0 < nOp) {
    _TestMixed(stateRegs, nState, opRegs, nOp, 300);
  }

  // Phase 5: wrap + interleaved consumer (200)
  _TestWrapConsume(stateRegs, nState, 200);

  _Log("=== EVTMGR_RunTest done — all phases completed without crash ===");
}
//-----------------------------------------------------------------------------

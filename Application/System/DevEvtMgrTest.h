//-----------------------------------------------------------------------------
/*
 File        : DevEvtMgrTest.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : EVTMGR test harness — exercises AppendEvent / PostEvent /
               PostEventFromISR with state registers (ProtSign / DigInput /
               RelayOutput) and AppendOpLog with event-log registers.
               Verifies the #1/#2/#3 fixes (null guards, FieldData clamp,
               mutex + ISR-deferred queue) do not crash and append events.

 Date        : 2026.07.11 (V1.00 — initial)
*/
//-----------------------------------------------------------------------------
#ifndef DEV_EVTMGR_TEST_H
#define DEV_EVTMGR_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/// Run the EVTMGR test once. Call from appTask (MCU) or SimAppTask (Sim).
void EVTMGR_RunTest(void);

#ifdef __cplusplus
}
#endif

#endif  // DEV_EVTMGR_TEST_H
//-----------------------------------------------------------------------------

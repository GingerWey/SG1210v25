//-----------------------------------------------------------------------------
/*
 File        : GLogListForm.cpp
 Version     : V1.04
 By          : Wey. Silver Grid

 Description : LogList form — log query form.
               Displays device logs (event/alarm/fault) from EVTMGR, browsable
               by category. Style aligned with DataListForm (outer/inner frames
               + right scrollbar + key throttle) plus a top Caption showing the
               current log category name. Per SG1210v25 form design spec V.

               Behaviour:
                 - Opened from GMenuForm Fatal-Log item (WID_LogListForm); starts
                   at mltAutoCtrl.
                 - ESC -> PopForm back to GMenuForm.
                 - UP/DOWN navigate log items (held key auto-repeats, throttled).
                 - LEFT / right-swipe-beyond-Fault -> next category or GMainForm.
                 - RIGHT / left-swipe-beyond-Event -> prev category or GWavelogForm.
                   (swipe cycles mltDeviceLog<->mltDevStatus<->mltAutoCtrl; beyond the ends
                   it switches forms per spec V.5.1)
                 - Static display: load on entry, reload on category switch.
                 - LEFT/RIGHT respond to initial keydown only (no key-repeat)
                   to avoid rapid category cycling while key is held.

 Date        : 2026.07.13 (V1.03 — Caption left edge: add 4px gap from frame border
              (LL_CAP_ICON_LEFT_GAP); scrollbar: when items don't exceed visible rows,
              clear scrollbar lane with panel color and redraw outer frame to hide
              stale thumb)
              2026.07.11 (V1.02 — fix category switch: clear stale rows beyond
              item count in _UpdateList; LEFT/RIGHT ignore key-repeat (bRepeat)
              to prevent fast-cycling; _RedrawCaption inset to avoid outer-frame
              border conflict/flicker, no outer-frame redraw)
              2026.07.11 (V1.01 — optimize category switch: partial redraw only, add _RedrawCaption)
              2026.07.10 (V1.00 — initial implementation)
*/
//-----------------------------------------------------------------------------
#include "GLogListForm.h"

#include "GUI.h"
#include "GFormCentra.h"
#include "GUIMessage.h"
#include "GWinTypes.h"
#include "GFormCentraRegistrar.h"

#include "FontSGRes.h"
#include "CSGDraw.h"
#include "GUIPicture.h"
#include "Graphics/ImageRes.h"
#include "Strings/TextStrs.h"

#include "DevIntf.h"
#include "DevRegInfo.h"
#include "DevRegs.h"
#include "DevTypes.h"
#include "DevDebug.h"
#include "RamHeap.h"
#include "DevFixed.h"
#include "DevEvtMgr.h"

#include <cstdio>
#include <cstring>
#include <GUI_Type.h>
#include <GUIConf.h>

//=============================================================================
// Layout
//=============================================================================
// Outer frame / panel (涂色区). Encloses caption + row table + scrollbar.
#define LL_LIST_X          15
#define LL_LIST_Y          4
#define LL_LIST_W          290
#define LL_LIST_H          232
#define LL_LIST_X1         (LL_LIST_X + LL_LIST_W - 1)   // 304
#define LL_LIST_Y1         (LL_LIST_Y + LL_LIST_H - 1)   // 235

// Caption bar (log category name) at the top of the panel
#define LL_CAP_H           28
#define LL_CAP_Y0          LL_LIST_Y                     // 4
#define LL_CAP_Y1          (LL_CAP_Y0 + LL_CAP_H - 1)    // 25
#define LL_CAP_SEP         (LL_CAP_Y1 + 1)               // 26
#define LL_CAP_ICON_W      20
#define LL_CAP_ICON_X0     (LL_INNER_X1 - LL_CAP_ICON_W + 1)  // right end of caption
#define LL_CAP_ICON_X1     LL_INNER_X1
#define LL_CAP_ICON_Y0     (LL_CAP_Y0 + (LL_CAP_H - 20) / 2)   // vertically centered

// Scrollbar lane (right side of panel)
#define LL_SCRL_W          8
#define LL_SCRL_GAP        4
#define LL_SCRL_X0         (LL_LIST_X1 - LL_SCRL_W + 1)  // 297
#define LL_SCRL_X1         LL_LIST_X1                     // 304

// Inner frame (row table). Below caption; right edge before scrollbar gap.
#define LL_FRAME_GAP       4
#define LL_CAP_ICON_LEFT_GAP  4
#define LL_INNER_X         (LL_LIST_X + LL_FRAME_GAP)               // 19
#define LL_INNER_Y         (LL_CAP_SEP + 1 + LL_FRAME_GAP)          // 31
#define LL_INNER_X1        (LL_SCRL_X0 - LL_SCRL_GAP - 1)           // 292
#define LL_INNER_Y1        (LL_LIST_Y1 - LL_FRAME_GAP)              // 231

// Row content — inset from inner frame
#define LL_CONTENT_PAD     3
#define LL_CONTENT_X0      (LL_INNER_X + LL_CONTENT_PAD)            // 22
#define LL_CONTENT_Y0      (LL_INNER_Y + LL_CONTENT_PAD)            // 34
#define LL_CONTENT_X1      (LL_INNER_X1 - LL_CONTENT_PAD)           // 289
#define LL_CONTENT_Y1      (LL_INNER_Y1 - LL_CONTENT_PAD)           // 228

#define LL_ROW_H           36
#define LL_VISIBLE         ((LL_CONTENT_Y1 - LL_CONTENT_Y0 + 1) / LL_ROW_H)  // 5
#define LL_SWIPE_PX        30
#define LL_KEY_REPEAT_INIT_MS  300
#define LL_KEY_REPEAT_MS       120

//=============================================================================
// Colors (aligned with DataListForm)
//=============================================================================
constexpr auto crAccent      = 0x00B5F2;
constexpr auto crText        = 0xECECEC;
constexpr auto crSelFrame    = 0x4DC9FC;
constexpr auto crSeparator   = 0x2F5CA6;
constexpr auto crListPanel   = 0x031635;
constexpr auto crOuterFrame  = 0x2F8FD0;
constexpr auto crInnerFrame  = 0x0A2240;
constexpr auto crScrollTrack = 0x081E3A;
constexpr auto crScrollThumb = 0x2F5CA6;
constexpr auto crCaption     = 0x00B5F2;
constexpr auto ftItem        = GUI_FONT_16LTH_CHN;

//=============================================================================
// Log categories (cycle order per spec V.5.1)
//=============================================================================
static const TEventLogType kLogCats[] = { mltAutoCtrl, mltDevStatus, mltDeviceLog };
#define LL_CAT_COUNT  (sizeof(kLogCats) / sizeof(kLogCats[0]))

static uint8_t _CatIndex(TEventLogType t)
{
  for (uint8_t i = 0; i < LL_CAT_COUNT; ++i) {
    if (kLogCats[i] == t) {
      return i;
    }
  }
  return 0;
}

//=============================================================================
// Form state (heap-allocated in _Init, freed in _Close)
//=============================================================================
struct TLogFormState {
  TEventLogType  eType;
  uint16_t       uCount;
  uint16_t       uTopItem;
  uint16_t       uCurItem;
  uint32_t       uLastKeyTick;
  uint16_t       uKeyRepeat;
  TEventWithProperty eventList[LL_VISIBLE];
#if GUI_SUPPORT_TOUCH
  int16_t        touchStartX;
  int16_t        touchStartY;
  int16_t        touchLastY;
  bool           touchActive;
#endif
};

static TLogFormState* s_pState = nullptr;

//=============================================================================
// Helpers
//=============================================================================
static void _MakeRect(GUI_RECT* pR, int x, int y, int w, int h)
{
  pR->x0 = x;
  pR->y0 = y;
  pR->x1 = x + w - 1;
  pR->y1 = y + h - 1;
}

static void _RowRect(GUI_RECT* pR, uint16_t uIdx)
{
  int y = LL_CONTENT_Y0 + (int)(uIdx - s_pState->uTopItem) * LL_ROW_H;
  _MakeRect(pR, LL_CONTENT_X0, y, LL_CONTENT_X1 - LL_CONTENT_X0 + 1, LL_ROW_H);
}

/// Re-fetch the visible log page from EVTMGR/FIX into the cache.
static void _GetEventList(void)
{
  if (nullptr == s_pState) {
    return;
  }
  memset(s_pState->eventList, 0, sizeof(s_pState->eventList));
  for (uint32_t i = 0; i < LL_VISIBLE; ++i) {
    TEventLogItem* pEvent = &(s_pState->eventList[i].EvtLog);
    if (0 != FIX_ReadEvtLogItem(s_pState->eType,
                                i + s_pState->uTopItem, pEvent, ERD_BACKWARD)) {
      pEvent->Summary.State.RegNum = 0;
    }
  }
}

static TEventWithProperty* _GetEventInfo(uint16_t uIdx)
{
  if (nullptr == s_pState) {
    return nullptr;
  }
  if (uIdx < s_pState->uTopItem ||
      uIdx >= s_pState->uTopItem + LL_VISIBLE) {
    return nullptr;
  }
  TEventWithProperty* pEvent = s_pState->eventList + (uIdx - s_pState->uTopItem);
  if (0 == pEvent->EvtLog.Summary.State.RegNum) {
    return nullptr;
  }
  return pEvent;
}

static void _ChangeType(TEventLogType newType)
{
  if (nullptr == s_pState) {
    return;
  }
  s_pState->eType   = newType;
  s_pState->uCount  = EVTMGR_GetEventCount(newType);
  s_pState->uTopItem = 0;
  s_pState->uCurItem = 0;
  // Clear key repeat state on category change
  s_pState->uKeyRepeat = 0;
  s_pState->uLastKeyTick = 0;
}

//=============================================================================
// Drawing
//=============================================================================
static void _DrawOuterFrame(void)
{
  GUI_SetColor(crOuterFrame);
  GUI_DrawRoundedRect(LL_LIST_X, LL_LIST_Y, LL_LIST_X1, LL_LIST_Y1, 3);
}

static void _DrawCaption(void)
{
  uint32_t uCapId = 0, uIconId = 0;
  switch (s_pState->eType) {
  case mltDeviceLog:
      uCapId = idEventCatalog1;
      uIconId = picIdxLV_LogC20x20Cyan;
      break;
  case mltDevStatus:
      uCapId = idEventCatalog2;
      uIconId = picIdxLV_LogB20x20Cyan;
      break;
  case mltAutoCtrl:
      uCapId = idEventCatalog3;
      uIconId = picIdxLV_LogA20x20Cyan;
      break;
  default: break;
  }

  int iconX0 = LL_LIST_X + LL_FRAME_GAP + LL_CAP_ICON_LEFT_GAP;
  GUI_DrawPicture(&picMAUAtlascsg, iconX0, LL_CAP_ICON_Y0,
      uIconId, 100);

  const char* pStr = GetMultiLangString(uCapId);
  if (nullptr != pStr) {
    GUI_RECT r;

    // Leave room for the menu icon on the right
    int capTextX0 = LL_LIST_X + LL_FRAME_GAP + LL_CAP_ICON_LEFT_GAP + 24;
    int capTextW  = LL_CAP_ICON_X0 - capTextX0;
    _MakeRect(&r, capTextX0, LL_CAP_Y0, capTextW, LL_CAP_H);
    GUI_SetFont(ftItem);
    GUI_SetColor(crCaption);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);
    GUI_DispStringInRect(pStr, &r, GUI_TA_LEFT | GUI_TA_VCENTER);
  }
#if GUI_SUPPORT_TOUCH
  // Menu icon at the right end of the caption bar
  GUI_DrawPicture(&picMAUAtlascsg, LL_CAP_ICON_X0, LL_CAP_ICON_Y0,
                  picIdxMA_Menu20x20Cyan, 100);
#endif
  // Caption separator line
  GUI_SetColor(crSeparator);
  GUI_DrawHLine(LL_CAP_SEP, LL_LIST_X + LL_FRAME_GAP, LL_INNER_X1);
}

/// Redraw Caption area only (inset to avoid outer frame border)
static void _RedrawCaption(void)
{
  // Clear caption background only (inset from outer frame border by FRAME_GAP)
  GUI_SetColor(crListPanel);
  int capX0 = LL_LIST_X + LL_FRAME_GAP;     // 19 (inset from left border)
  int capY0 = LL_LIST_Y + LL_FRAME_GAP;     // 8 (inset from top border)
  int capX1 = LL_INNER_X1;                  // 292 (right boundary of inner frame)
  int capY1 = LL_CAP_SEP - 1;               // 25 (just before separator)
  GUI_FillRect(capX0, capY0, capX1, capY1);
  _DrawCaption();
}

static void _DrawScrollbar(void)
{
  if (nullptr == s_pState) {
    return;
  }

  // If items don't exceed visible rows, clear scrollbar lane (don't draw thumb)
  if (s_pState->uCount <= LL_VISIBLE) {
    GUI_SetColor(crListPanel);
    GUI_FillRect(LL_SCRL_X0, LL_INNER_Y, LL_SCRL_X1, LL_INNER_Y1);
    _DrawOuterFrame();
    return;
  }

  uint16_t maxTop = s_pState->uCount - LL_VISIBLE;
  int trackY0 = LL_INNER_Y;
  int trackY1 = LL_INNER_Y1;
  int trackH  = trackY1 - trackY0 + 1;
  int thumbH  = trackH * LL_VISIBLE / s_pState->uCount;
  if (thumbH < 12) {
    thumbH = 12;
  }
  int travel  = trackH - thumbH;
  int thumbY  = trackY0 + travel * (int)s_pState->uTopItem / (int)maxTop;
  int thumbY1 = thumbY + thumbH - 1;
  if (thumbY1 > trackY1) {
    thumbY1 = trackY1;
    thumbY  = thumbY1 - thumbH + 1;
  }
  GUI_SetColor(crScrollTrack);
  GUI_FillRect(LL_SCRL_X0, trackY0, LL_SCRL_X1, trackY1);
  GUI_SetColor(crScrollThumb);
  GUI_FillRoundedRect(LL_SCRL_X0 + 2, thumbY, LL_SCRL_X1 - 2, thumbY1, 2);
  _DrawOuterFrame();
}

static void _FlushForm(void)
{
  CSG_DrawPicture(IMAGE_BACKGROUND, 0, 0, 0, 100, nullptr);
  GUI_SetColor(crListPanel);
  GUI_FillRect(LL_LIST_X, LL_LIST_Y, LL_LIST_X1, LL_LIST_Y1);
  GUI_SetColor(crInnerFrame);
  GUI_DrawRoundedRect(LL_INNER_X, LL_INNER_Y, LL_INNER_X1, LL_INNER_Y1, 2);
  _DrawCaption();
  _DrawOuterFrame();
}

/// Draw one log item (2 lines: timestamp + name/desp).
static void _DrawItem(uint16_t uIdx)
{
  if (nullptr == s_pState) {
    return;
  }
  if (uIdx >= s_pState->uCount ||
      uIdx < s_pState->uTopItem ||
      uIdx >= s_pState->uTopItem + LL_VISIBLE) {
    return;
  }
  GUI_RECT rRow;
  _RowRect(&rRow, uIdx);
  bool bSelected = (uIdx == s_pState->uCurItem);

  GUI_SetColor(crListPanel);
  GUI_FillRect(rRow.x0, rRow.y0, rRow.x1, rRow.y1);

  TEventWithProperty* pEvent = _GetEventInfo(uIdx);
  if (nullptr == pEvent) {
    return;
  }
  if (nullptr == pEvent->EvtProp) {
    if (0 != EVTMGR_GetEventDesp(pEvent)) {
      return;
    }
    if (nullptr == pEvent->RegInfo || 0 == pEvent->RegInfo->RegNum) {
      return;
    }
  }
  const TDevRegInfoItem* pInfo = DevIntf_GetRegInfo(pEvent->RegInfo->RegNum);
  if (nullptr == pInfo) {
    return;
  }
  const char* pcRegName = (nullptr != pInfo->pName) ? pInfo->pName
                          : GetMultiLangString(pInfo->NameStrId);
  if (nullptr == pcRegName) {
    return;
  }

  // Line 1: timestamp (left, +4px gap from selection frame)
  char szLine[48];
  snprintf(szLine, sizeof(szLine), "%04u-%02u-%02u %02u:%02u:%02u:%03u",
           pEvent->EvtLog.Summary.Time.Year,
           pEvent->EvtLog.Summary.Time.Month,
           pEvent->EvtLog.Summary.Time.Day,
           pEvent->EvtLog.Summary.Time.Hours,
           pEvent->EvtLog.Summary.Time.Minutes,
           pEvent->EvtLog.Summary.Time.Seconds,
           pEvent->EvtLog.Summary.Time.milSecs);
  int iGap = 6;
  GUI_RECT r1;
  _MakeRect(&r1, rRow.x0 + iGap, rRow.y0 + 2,
            rRow.x1 - rRow.x0 + 1 - iGap, LL_ROW_H / 2);
  GUI_SetFont(ftItem);
  GUI_SetColor(bSelected ? crAccent : crText);
  GUI_SetTextMode(GUI_TEXTMODE_TRANS);
  GUI_DispStringInRect(szLine, &r1, GUI_TA_LEFT | GUI_TA_VCENTER);

  // Line 2: name + desp (left) + FieldData value (right)
  snprintf(szLine, sizeof(szLine), "%s %s", pcRegName, pEvent->EvtDesp);
  // Reserve right area for FieldData if present
  int iRightW = 0;
  char szVal[24] = { 0 };
  if (nullptr != pEvent->EvtProp && 0 < pEvent->EvtProp->NbrOfData) {
    uint32_t uFdReg = pEvent->EvtProp->FieldDataReg[0];
    if (0 != uFdReg) {
      const TDevRegInfoItem* pFdInfo = DevIntf_GetRegInfo(uFdReg);
      if (nullptr != pFdInfo) {
        if (REG_REAL == REG_TYPE(uFdReg)) {
          char szFmt[12];
          snprintf(szFmt, sizeof(szFmt), "%%0.%uf", pFdInfo->Decimal);
          float fVal = _GetRealReg(uFdReg);
          int pos = snprintf(szVal, sizeof(szVal), szFmt, fVal);
          const char* pDim = RINF_GetDIMNameEx(pFdInfo);
          if (nullptr != pDim && (size_t)pos < sizeof(szVal)) {
            snprintf(szVal + pos, sizeof(szVal) - pos, "%s", pDim);
          }
          iRightW = 80;
        } else {
          // Non-real register: show raw value
          uint32_t uVal = DevReg_Read(uFdReg);
          snprintf(szVal, sizeof(szVal), "%u", (unsigned)uVal);
          iRightW = 60;
        }
      }
    }
  }

  GUI_RECT r2;
  _MakeRect(&r2, rRow.x0 + iGap, rRow.y0 + LL_ROW_H / 2,
            rRow.x1 - rRow.x0 + 1 - iGap - iRightW, LL_ROW_H / 2 - 2);
  GUI_SetColor(bSelected ? crAccent : crText);
  GUI_DispStringInRect(szLine, &r2, GUI_TA_LEFT | GUI_TA_VCENTER);

  // FieldData value (right-aligned)
  if (0 < iRightW && 0 != szVal[0]) {
    GUI_RECT rVal;
    _MakeRect(&rVal, rRow.x1 - iRightW + 1, rRow.y0 + LL_ROW_H / 2,
              iRightW, LL_ROW_H / 2 - 2);
    GUI_SetColor(bSelected ? crAccent : crText);
    GUI_DispStringInRect(szVal, &rVal, GUI_TA_RIGHT | GUI_TA_VCENTER);
  }

  if (bSelected) {
    GUI_SetColor(crSelFrame);
    GUI_DrawRoundedFrame(rRow.x0, rRow.y0, rRow.x1, rRow.y1, 3, 1);
  }
}

static void _UpdateList(void)
{
  _GetEventList();
  for (uint16_t i = 0; i < LL_VISIBLE; ++i) {
    uint16_t idx = s_pState->uTopItem + i;
    if (idx >= s_pState->uCount) {
      // Clear rows beyond item count (empty row)
      GUI_RECT rRow;
      _RowRect(&rRow, idx);
      GUI_SetColor(crListPanel);
      GUI_FillRect(rRow.x0, rRow.y0, rRow.x1, rRow.y1);
    } else {
      _DrawItem(idx);
    }
  }
}

static void _RefreshValues(void)
{
  _GetEventList();
  for (uint16_t i = 0; i < LL_VISIBLE; ++i) {
    uint16_t idx = s_pState->uTopItem + i;
    if (idx >= s_pState->uCount) {
      break;
    }
    _DrawItem(idx);
  }
}

//=============================================================================
// Navigation
//=============================================================================
static void _EnsureCursorVisible(void)
{
  if (s_pState->uCurItem < s_pState->uTopItem) {
    s_pState->uTopItem = s_pState->uCurItem;
  } else if (s_pState->uCurItem >= s_pState->uTopItem + LL_VISIBLE) {
    s_pState->uTopItem = s_pState->uCurItem - LL_VISIBLE + 1;
  }
}

static void _ClampTop(void)
{
  if (s_pState->uCount <= LL_VISIBLE) {
    s_pState->uTopItem = 0;
    return;
  }
  uint16_t maxTop = s_pState->uCount - LL_VISIBLE;
  if (s_pState->uTopItem > maxTop) {
    s_pState->uTopItem = maxTop;
  }
}

/// Cycle to next category (right), wrap around within the 3 categories.
static void _CycleNext(void)
{
  uint8_t idx = _CatIndex(s_pState->eType);
  idx = (idx + 1) % LL_CAT_COUNT;
  _ChangeType(kLogCats[idx]);
  // Partial redraw: Caption + Row table + Scrollbar (no full screen flush)
  _RedrawCaption();
  _UpdateList();
  _DrawScrollbar();
}

/// Cycle to prev category (left), wrap around within the 3 categories.
static void _CyclePrev(void)
{
  uint8_t idx = _CatIndex(s_pState->eType);
  idx = (0 == idx) ? (LL_CAT_COUNT - 1) : (idx - 1);
  _ChangeType(kLogCats[idx]);
  // Partial redraw: Caption + Row table + Scrollbar (no full screen flush)
  _RedrawCaption();
  _UpdateList();
  _DrawScrollbar();
}

//=============================================================================
// Key handling (throttled auto-repeat, mirrors DataListForm)
//=============================================================================
static void _OnKey(uint16_t uwKey, bool bRepeat)
{
  if (nullptr == s_pState) {
    return;
  }
  if (bRepeat) {
    uint32_t now = GUI_GetTime();
    uint32_t threshold = (0 == s_pState->uKeyRepeat) ? LL_KEY_REPEAT_INIT_MS
                                                     : LL_KEY_REPEAT_MS;
    if (now - s_pState->uLastKeyTick < threshold) {
      return;
    }
    s_pState->uLastKeyTick = now;
    ++s_pState->uKeyRepeat;
  } else {
    s_pState->uLastKeyTick = GUI_GetTime();
    s_pState->uKeyRepeat   = 0;
  }

  switch (uwKey) {
  case KEY_UP: {
    uint16_t old = s_pState->uCurItem;
    uint16_t oldTop = s_pState->uTopItem;
    if (0 < s_pState->uCurItem) {
      --s_pState->uCurItem;
      _EnsureCursorVisible();
    } else if (0 < s_pState->uTopItem) {
      --s_pState->uTopItem;
    } else {
      break;
    }
    if (s_pState->uTopItem != oldTop) {
      _UpdateList();
      _DrawScrollbar();
    } else {
      _DrawItem(old);
      _DrawItem(s_pState->uCurItem);
    }
    break;
  }
  case KEY_DOWN: {
    uint16_t old = s_pState->uCurItem;
    uint16_t oldTop = s_pState->uTopItem;
    if (s_pState->uCurItem + 1 < s_pState->uCount) {
      ++s_pState->uCurItem;
      _EnsureCursorVisible();
    } else if (s_pState->uCount > LL_VISIBLE &&
               s_pState->uTopItem < s_pState->uCount - LL_VISIBLE) {
      ++s_pState->uTopItem;
    } else {
      break;
    }
    if (s_pState->uTopItem != oldTop) {
      _UpdateList();
      _DrawScrollbar();
    } else {
      _DrawItem(old);
      _DrawItem(s_pState->uCurItem);
    }
    break;
  }
  case KEY_LEFT:
    if (false == bRepeat) {
      _CyclePrev();
    }
    break;
  case KEY_RIGHT:
    if (false == bRepeat) {
      _CycleNext();
    }
    break;
  default:
    break;
  }
}

static void _OnKeyUp(uint16_t uwKey)
{
  if (nullptr != s_pState) {
    s_pState->uKeyRepeat = 0;
  }
  if (KEY_ESCAPE == uwKey) {
    gfc::PopForm();
  }
}

//=============================================================================
// Touch handling (Sim only)
//=============================================================================
#if GUI_SUPPORT_TOUCH
static void _OnTouch(uint16_t action, int16_t x, int16_t y)
{
  if (nullptr == s_pState) {
    return;
  }
  switch (action) {
  case TOUCH_DOWN:
    s_pState->touchActive  = true;
    s_pState->touchStartX  = x;
    s_pState->touchStartY  = y;
    s_pState->touchLastY   = y;
    if (x >= LL_CONTENT_X0 && x <= LL_CONTENT_X1 &&
        y >= LL_CONTENT_Y0 &&
        y <  LL_CONTENT_Y0 + LL_VISIBLE * LL_ROW_H) {
      uint16_t idx = s_pState->uTopItem +
                     (uint16_t)((y - LL_CONTENT_Y0) / LL_ROW_H);
      if (s_pState->uCount <= idx) {
        idx = (0 < s_pState->uCount) ? s_pState->uCount - 1 : 0;
      }
      if (s_pState->uCurItem != idx) {
        uint16_t old = s_pState->uCurItem;
        s_pState->uCurItem = idx;
        _DrawItem(old);
        _DrawItem(s_pState->uCurItem);
      }
    }
    break;

  case TOUCH_MOVE:
    if (s_pState->touchActive) {
      int16_t dy = y - s_pState->touchLastY;
      if (dy >= LL_ROW_H) {
        uint16_t shift = (uint16_t)(dy / LL_ROW_H);
        if (s_pState->uTopItem >= shift) {
          s_pState->uTopItem -= shift;
        } else {
          s_pState->uTopItem = 0;
        }
        s_pState->touchLastY = y;
        _ClampTop();
        _UpdateList();
        _DrawScrollbar();
      } else if (dy <= -LL_ROW_H) {
        uint16_t shift = (uint16_t)((-dy) / LL_ROW_H);
        if (s_pState->uCount > LL_VISIBLE) {
          uint16_t maxTop = s_pState->uCount - LL_VISIBLE;
          uint16_t newTop = s_pState->uTopItem + shift;
          s_pState->uTopItem = (newTop > maxTop) ? maxTop : newTop;
        }
        s_pState->touchLastY = y;
        _UpdateList();
        _DrawScrollbar();
      }
    }
    break;

  case TOUCH_UP:
    if (s_pState->touchActive) {
      // Menu icon hit-test (right end of caption bar)
      if (x >= LL_CAP_ICON_X0 && x <= LL_CAP_ICON_X1 &&
          y >= LL_CAP_ICON_Y0 && y <= LL_CAP_ICON_Y0 + 20 - 1) {
        s_pState->touchActive = false;
        gfc::PopForm();
        return;
      }
      int16_t dx = x - s_pState->touchStartX;
      int16_t dy = y - s_pState->touchStartY;
      int16_t adx = (0 <= dx) ? dx : (int16_t)(-dx);
      int16_t ady = (0 <= dy) ? dy : (int16_t)(-dy);
      if (LL_SWIPE_PX <= adx && ady < adx) {
        if (0 < dx) {
          _CycleNext();
        } else {
          _CyclePrev();
        }
        if (nullptr == s_pState) {
          return;
        }
      }
      s_pState->touchActive = false;
    }
    break;

  default:
    break;
  }
}
#endif  // GUI_SUPPORT_TOUCH

//=============================================================================
// Tick
//=============================================================================
static void _OnTick(uint32_t uNow)
{
  (void)uNow;
  // GLogListForm is static display — no automatic refresh.
  // Updates only on user interaction (key press, touch swipe, category switch).
}

//=============================================================================
// Form lifecycle
//=============================================================================
static void _Init(const void* argument)
{
  (void)argument;
  if (nullptr != s_pState) {
#ifndef __vmSIMULATOR__
    RAM_Free(s_pState);
#else
    delete s_pState;
#endif
    s_pState = nullptr;
  }

  s_pState = static_cast<TLogFormState*>(RAM_Malloc(sizeof(TLogFormState)));
  DEV_ASSERT(nullptr == s_pState, GFC_OutOfMem);
  if (nullptr == s_pState) {
    return;
  }

  memset(s_pState, 0, sizeof(TLogFormState));
  // Initial category from the menu argument (e.g. mltAutoCtrl for Fatal-Log);
  // default to mltAutoCtrl when no argument is supplied.
  TEventLogType eInit = (nullptr != argument)
                        ? (TEventLogType)(uintptr_t)argument
                        : mltAutoCtrl;
  _ChangeType(eInit);
}

static void _Show(const void* argument)
{
  (void)argument;
  if (nullptr == s_pState) {
    return;
  }
  _ClampTop();
  _FlushForm();
  _UpdateList();
  _DrawScrollbar();
}

static void _Close(const void* argument)
{
  (void)argument;
  if (nullptr != s_pState) {
#ifndef __vmSIMULATOR__
    RAM_Free(s_pState);
#else
    delete s_pState;
#endif
    s_pState = nullptr;
  }
}

static void _OnMessage(GM_MESSAGE* pMsg)
{
  if (nullptr == pMsg || nullptr == s_pState) {
    return;
  }
  switch (pMsg->MsgId) {
  case GM_TIMER_TICK:
    _OnTick(static_cast<uint32_t>(pMsg->Data.v));
    break;
  case GM_KEYDOWN:
    _OnKey(pMsg->Param, false);
    pMsg->MsgId = 0;
    break;
  case GM_KEYPRESS:
    _OnKey(pMsg->Param, true);
    pMsg->MsgId = 0;
    break;
  case GM_KEYUP:
    _OnKeyUp(pMsg->Param);
    pMsg->MsgId = 0;
    break;
#if GUI_SUPPORT_TOUCH
  case GM_TOUCH: {
    int32_t packed = pMsg->Data.v;
    int16_t x = (int16_t)((packed >> 16) & 0xFFFF);
    int16_t y = (int16_t)(packed & 0xFFFF);
    _OnTouch(pMsg->Param, x, y);
    pMsg->MsgId = 0;
    break;
  }
#endif
  default:
    break;
  }
}

//=============================================================================
// Form descriptor + registration (WID_LogListForm = the Fatal-Log menu target)
//=============================================================================
const GWinForm FLogListForm = {
  _Init,
  _Show,
  _Close,
  _OnMessage
};

static const gfc::FormRegistrar kRegLogList(WID_LogListForm,
                                            &FLogListForm,
                                            "LogList");
//-----------------------------------------------------------------------------

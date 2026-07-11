//-----------------------------------------------------------------------------
/*
 File        : GPDataListForm.cpp
 Version     : V1.08
 By          : Wey. Silver Grid

 Description : DataList form implementation.
               Live register list — displays register names and real-time
               values, grouped by register class. Manual owner-draw list
               with key + touch navigation.
               Per SG1210v25 form design spec section IV.

               Style aligned with MainForm / MenuForm (new style):
                 - Background image shows through (no opaque list panel)
                 - Cyan group headers + separator line
                 - Light-grey register text
                 - Hollow rounded-rect selection frame (no fill)
                 - Flicker-free 1s refresh: value cells redrawn only when
                   their displayed value actually changes (per-row cache)

               Layer0: full-screen background (IMAGE_BACKGROUND)
               Layer1: grouped register list (name left, value/state right)

               Behaviour:
                 - Opened from GMenuForm (menu item 1, WID_DataListForm)
                 - ESC / RIGHT key / right-swipe -> PopForm back to GMenuForm
                 - LEFT key / left-swipe      -> PushForm WID_LogListForm (next)
                 - UP/DOWN move cursor (skips group headers); list scrolls
                 - Values refresh every 1s (GM_TIMER_TICK)

 Date        : 2026.07.10 (V1.08 — add inner/outer frames: bright outer frame
                          (dimmer than selection) encloses panel+scrollbar; dark
                          inner frame encloses the row table only; row table
                          inset from panel (T/B/L gap) and content inset from
                          inner frame)
              2026.07.09 (V1.07 — throttle key auto-repeat: first press immediate,
                          held key repeats after 300ms then every 120ms. Fixes
                          MCU moving many rows per tap (driver fires GM_KEYPRESS
                          every ~10ms with no initial delay))
              2026.07.09 (V1.06 — fix UP scroll leaving top group header hidden
                          (cursor at first reg now scrolls viewport up one row);
                          handle GM_KEYPRESS so held UP/DOWN auto-repeats)
              2026.07.09 (V1.05 — per-row cache allocated in _Init (RAMHeap on
                          MCU / new on Sim) and freed in _Close; allocation
                          failure caught with DEV_ASSERT(GFC_OutOfMem))
              2026.07.09 (V1.04 — modern scrollbar on right lane (8px + gap):
                          subtle track + rounded thumb reflecting viewport;
                          row content narrowed to leave the scrollbar lane)
              2026.07.09 (V1.03 — indent register names under group header;
                          center state icons in value cell; Sim-only test
                          value injection each refresh cycle)
              2026.07.09 (V1.02 — new-style palette per MainForm/MenuForm;
                          hollow selection frame over background image;
                          flicker-free refresh via per-row value cache)
              2026.07.09 (V1.01 — fix scroll redraw at view top/bottom)
              2026.07.08 (V1.00 — initial implementation)
*/
//-----------------------------------------------------------------------------
#include "GDataListForm.h"

#include "GUI.h"
#include "GFormCentra.h"
#include "GUIMessage.h"
#include "GWinTypes.h"
#include "GFormCentraRegistrar.h"

#include "FontSGRes.h"
#include "GUIPicture.h"
#include "CSGDraw.h"
#include "PictureRes.h"
#include "Graphics/ImageRes.h"
#include "Strings/TextStrs.h"
#include "GUIMisc.h"

#include "DevIntf.h"
#include "DevRegInfo.h"
#include "DevRegs.h"
#include "DevTypes.h"
#include "DevDebug.h"
#include "RamHeap.h"

#include <cstdio>
#include <cstring>
#include <GUI_Type.h>
#include <GUIConf.h>

//=============================================================================
// Layout constants (spec IV.4.3.2: list area -> 320x240 screen)
//=============================================================================
// Outer frame / panel (涂色区). Encloses the row table AND the scrollbar.
#define DL_LIST_X          15
#define DL_LIST_Y          10
#define DL_LIST_W          290
#define DL_LIST_H          220
#define DL_LIST_X1         (DL_LIST_X + DL_LIST_W - 1)   // 304
#define DL_LIST_Y1         (DL_LIST_Y + DL_LIST_H - 1)   // 229

// Scrollbar lane (right side of panel)
#define DL_SCRL_W          8                          // scrollbar width
#define DL_SCRL_GAP        4                          // gap between row table and scrollbar
#define DL_SCRL_X0         (DL_LIST_X1 - DL_SCRL_W + 1)            // 297
#define DL_SCRL_X1         DL_LIST_X1                              // 304

// Inner frame (row table boundary). Inset from the panel on top/bottom/left;
// right edge sits at the scrollbar gap — does NOT include the scrollbar.
#define DL_FRAME_GAP       4   // gap between outer frame and inner frame (T/B/L)
#define DL_INNER_X         (DL_LIST_X + DL_FRAME_GAP)               // 19
#define DL_INNER_Y         (DL_LIST_Y + DL_FRAME_GAP + 3)           // 17
#define DL_INNER_X1        (DL_SCRL_X0 - DL_SCRL_GAP - 1)           // 292
#define DL_INNER_Y1        (DL_LIST_Y1 - DL_FRAME_GAP - 3)          // 222

// Row content — inset from the inner frame (spacing between content and frame)
#define DL_CONTENT_PAD     3
#define DL_CONTENT_X0      (DL_INNER_X + DL_CONTENT_PAD)            // 22
#define DL_CONTENT_Y0      (DL_INNER_Y + DL_CONTENT_PAD)            // 20
#define DL_CONTENT_X1      (DL_INNER_X1 - DL_CONTENT_PAD)           // 289
#define DL_CONTENT_Y1      (DL_INNER_Y1 - DL_CONTENT_PAD)           // 219

#define DL_ROW_H           20
#define DL_VISIBLE         ((DL_CONTENT_Y1 - DL_CONTENT_Y0 + 1) / DL_ROW_H)  // 10

// Text margins within the content area
#define DL_MARGIN          4      // left/right inner margin
#define DL_NAME_INDENT     12     // register-name indent under its group header
#define DL_VAL_W           90     // value column width
#define DL_NAME_X0         (DL_CONTENT_X0 + DL_MARGIN)              // 26
#define DL_VAL_X1          (DL_CONTENT_X1 - DL_MARGIN)              // 285
#define DL_VAL_X0          (DL_VAL_X1 - DL_VAL_W + 1)               // 196

#define DL_REFRESH_MS      1000   // value refresh period (spec IV.4.2)
#define DL_SWIPE_PX        30     // horizontal swipe threshold
#define DL_KEY_REPEAT_INIT_MS  300   // initial delay before first auto-repeat
#define DL_KEY_REPEAT_MS       120   // auto-repeat interval after initial delay

//=============================================================================
// Colors — new style, aligned with MainForm / MenuForm
//=============================================================================
constexpr auto crAccent      = 0x00B5F2;   // cyan — group headers, selected text
constexpr auto crText        = 0xECECEC;   // light grey — register names & values
constexpr auto crSelFrame    = 0x4DC9FC;   // selection frame (hollow, brightest)
constexpr auto crSeparator   = 0x2F5CA6;   // group separator line
constexpr auto crListPanel   = 0x031635;   // solid panel (涂色区)
constexpr auto crOuterFrame  = 0x2F8FD0;   // outer frame — bright, dimmer than selection
constexpr auto crInnerFrame  = 0x0A2240;   // inner frame — dark
constexpr auto crScrollTrack = 0x081E3A;   // scrollbar lane (subtle, below panel)
constexpr auto crScrollThumb = 0x2F5CA6;   // scrollbar thumb (muted blue)
constexpr auto ftItemName    = GUI_FONT_16LTH_CHN;
constexpr auto ftItemValue   = GUI_FONT_16LTH_CHN;

//=============================================================================
// Row model — flat list of group-header rows and register rows
//=============================================================================
enum RowKind : uint8_t {
  kGroup = 0,
  kReg   = 1
};

struct Row {
  uint8_t  kind;
  uint32_t strId;    // group name string id (kGroup only)
  uint32_t regNum;   // register number    (kReg only)
};

static const Row s_rows[] = {
  // Group 1 — AC measure (idDVGroup01)
  { kGroup, idDVGroup01, 0 },
  { kReg,   0, REG_RL_Uin  },
  { kReg,   0, REG_RL_Uout },
  { kReg,   0, REG_RL_ACFreq },
  { kReg,   0, REG_RL_VOFreq },
  // Group 2 — Battery (idDVGroup02)
  { kGroup, idDVGroup02, 0 },
  { kReg,   0, REG_RL_BCHRG_Pbus },
  { kReg,   0, REG_RL_BCHRG_Ibus },
  { kReg,   0, REG_RL_BCHRG_Ibus_Max },
  { kReg,   0, REG_RL_BTOUT_Pbus },
  { kReg,   0, REG_RL_BTOUT_Ibus },
  { kReg,   0, REG_RL_BTOUT_Ibus_Max },
  { kReg,   0, REG_RL_BAT_CAPLevel },
  { kReg,   0, REG_RL_BAT_TEMPERATRUE },
  // Group 3 — Realtime clock (idDVGroup03)
  { kGroup, idDVGroup03, 0 },
  { kReg,   0, REG_RL_RTC_TEMP },
  { kReg,   0, REG_RL_RTC_Vbat },
  // Group 4 — Status signals (idDVGroup04)
  { kGroup, idDVGroup04, 0 },
  { kReg,   0, REG_DI0 },
  { kReg,   0, REG_DI1 },
  { kReg,   0, REG_DI2 },
  { kReg,   0, REG_DI3 },
  { kReg,   0, REG_DI4 },
  { kReg,   0, REG_DI5 },
  { kReg,   0, REG_DI6 },
  { kReg,   0, REG_DI7 },
  // Group 5 — Control signals (idDVGroup05)
  { kGroup, idDVGroup05, 0 },
  { kReg,   0, REG_RELAY0 },
  { kReg,   0, REG_RELAY1 },
  { kReg,   0, REG_RELAY2 },
  { kReg,   0, REG_RELAY3 },
};
static const uint16_t s_rowCount = sizeof(s_rows) / sizeof(s_rows[0]);

//=============================================================================
// Form state
//=============================================================================
typedef struct tagDataListState {
  uint16_t uTopItem;       // index of first visible row
  uint16_t uCurItem;       // cursor row index (always a kReg row)
  uint32_t uLastRefresh;   // GUI_GetTime() of last value refresh
  uint32_t uLastKeyTick;   // GUI_GetTime() of last accepted key action
  uint16_t uKeyRepeat;     // auto-repeat count since last GM_KEYDOWN (0 = first press)
#if GUI_SUPPORT_TOUCH
  int16_t  touchStartX;
  int16_t  touchStartY;
  int16_t  touchLastY;     // last move Y for drag-scroll anchoring
  bool     touchActive;
#endif
} TDataListState;

static TDataListState m_State;

//=============================================================================
// Per-row value cache — drives flicker-free refresh (skip unchanged cells).
// Allocated in _Init (RAMHeap on MCU / new on Sim) and freed in _Close.
//=============================================================================
struct RowCache {
  bool    valid;
  bool    isIO;        // true = IO state register, false = REAL
  uint8_t ioState;     // last IO state drawn
  char    text[28];    // last REAL text drawn
};
static RowCache* s_pCache = nullptr;

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

/// Row vertical extent for visible index. Rows live in the content area
/// (inside the inner frame, inset by DL_CONTENT_PAD) so row clears never
/// overwrite the inner/outer frame borders.
static void _RowRect(GUI_RECT* pR, uint16_t uIdx)
{
  int y = DL_CONTENT_Y0 + (int)(uIdx - m_State.uTopItem) * DL_ROW_H;
  _MakeRect(pR, DL_CONTENT_X0, y, DL_CONTENT_X1 - DL_CONTENT_X0 + 1, DL_ROW_H);
}

/// Register display name: prefer multilang string, fallback to pName.
static const char* _GetRegName(uint32_t uRegNum)
{
  const TDevRegInfoItem* pProp = DevIntf_GetRegInfo(uRegNum);
  if (nullptr == pProp) {
    return nullptr;
  }
  const char* pName = GetMultiLangString(pProp->NameStrId);
  if (nullptr == pName) {
    pName = pProp->pName;
  }
  return pName;
}

/// Format a REAL register value with Decimal places + dimension suffix.
static void _FormatReal(uint32_t uRegNum, const TDevRegInfoItem* pProp,
                        char* buf, size_t len)
{
  if (nullptr == pProp || nullptr == buf || 0 == len) {
    return;
  } else {
    buf[0] = 0;
  }
 char szFmt[12];
  snprintf(szFmt, sizeof(szFmt), "%%0.%uf", pProp->Decimal);
  float fValue = _GetRealReg(uRegNum);
  int pos = snprintf(buf, len, szFmt, fValue);
  if (pos < 0) {
    pos = 0;
  }
  const char* pDim = RINF_GetDIMNameEx(pProp);
  if (nullptr != pDim && (size_t)pos < len) {
    snprintf(buf + pos, len - pos, " %s", pDim);
  }
}

/// Snapshot the current displayed value of a row into the cache.
static void _CacheRow(uint16_t idx)
{
  if (nullptr == s_pCache || idx >= s_rowCount) {
    return;
  }
  uint32_t regNum = s_rows[idx].regNum;
  RowCache& c = s_pCache[idx];
  if (REG_IOSTATE == REG_TYPE(regNum)) {
    c.isIO = true;
    c.ioState = _GetIOStateReg(regNum);
  } else {
    c.isIO = false;
    _FormatReal(regNum, DevIntf_GetRegInfo(regNum), c.text, sizeof(c.text));
  }
  c.valid = true;
}

/// True if the row's displayed value differs from the cached snapshot.
static bool _ValueChanged(uint16_t idx)
{
  if (nullptr == s_pCache || idx >= s_rowCount) {
    return true;
  }
  RowCache& c = s_pCache[idx];
  if (!c.valid) {
    return true;
  }
  uint32_t regNum = s_rows[idx].regNum;
  if (REG_IOSTATE == REG_TYPE(regNum)) {
    return (!c.isIO) || (c.ioState != _GetIOStateReg(regNum));
  }
  char buf[28];
  _FormatReal(regNum, DevIntf_GetRegInfo(regNum), buf, sizeof(buf));
  return (c.isIO) || (0 != strcmp(c.text, buf));
}

/// Draw the value/state cell of a register row at the given top y.
/// @param bClear  true = clear the value cell with the background image first
///                (used by refresh); false = row background already cleared
///                (used by _DrawRow). The clear rect is inset vertically so it
///                never erases the selection frame border.
static void _DrawRegValue(uint32_t uRegNum, int y, bool bSelected, bool bClear)
{
  GUI_RECT vRect;
  _MakeRect(&vRect, DL_VAL_X0, y + 1, DL_VAL_W, DL_ROW_H - 2);

  if (bClear) {
    GUI_SetColor(crListPanel);
    GUI_FillRect(vRect.x0, vRect.y0, vRect.x1, vRect.y1);
  }

  const TDevRegInfoItem* pProp = DevIntf_GetRegInfo(uRegNum);
  if (nullptr == pProp) {
    return;
  }

  if (REG_REAL == REG_TYPE(uRegNum)) {
    char szBuf[28];
    _FormatReal(uRegNum, pProp, szBuf, sizeof(szBuf));
    GUI_SetFont(ftItemValue);
    GUI_SetColor(bSelected ? crAccent : crText);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);
    GUI_DispStringInRect(szBuf, &vRect, GUI_TA_RIGHT | GUI_TA_VCENTER);
  } else if (REG_IOSTATE == REG_TYPE(uRegNum)) {
    const auto stValue = _GetIOStateReg(uRegNum);
    int imgIdx = (STATE_TRUE == stValue)
                 ? picIdxLV_CheckMark16x16Red
                 : picIdxLV_CrossMark16x16Green;
    // Center the 16x16 mark in the value cell (horizontal) and row (vertical)
    int ix = vRect.x0 + (DL_VAL_W - 16) / 2;
    int iy = y + (DL_ROW_H - 16) / 2;
    GUI_DrawPicture(&picMAUAtlascsg, ix, iy, imgIdx, 100);
  }
}

//=============================================================================
// Drawing
//=============================================================================
/// Outer frame (bright, dimmer than selection) — encloses panel + scrollbar.
/// Drawn in _FlushForm and re-drawn at the end of _DrawScrollbar so it stays
/// crisp over the scrollbar track lane.
static void _DrawOuterFrame(void)
{
  GUI_SetColor(crOuterFrame);
  GUI_DrawRoundedRect(DL_LIST_X, DL_LIST_Y, DL_LIST_X1, DL_LIST_Y1, 3);
}

static void _FlushForm(void)
{
  // Background image (drawn once) + solid panel (涂色区) + inner frame + outer frame.
  // The solid panel lets row/value clears use cheap GUI_FillRect instead of
  // re-decoding the full background image per clear (which exhausted the emWin
  // heap under refresh load and crashed).
  CSG_DrawPicture(IMAGE_BACKGROUND, 0, 0, 0, 100, nullptr);
  GUI_SetColor(crListPanel);
  GUI_FillRect(DL_LIST_X, DL_LIST_Y, DL_LIST_X1, DL_LIST_Y1);
  // Inner frame (dark) — row table boundary (excludes scrollbar)
  GUI_SetColor(crInnerFrame);
  GUI_DrawRoundedRect(DL_INNER_X, DL_INNER_Y, DL_INNER_X1, DL_INNER_Y1, 2);
  // Outer frame (bright) — encloses panel + scrollbar
  _DrawOuterFrame();
}

/// Modern scrollbar on the right lane: subtle track + rounded thumb whose
/// position/size reflect the viewport (uTopItem / visible / rowCount).
/// Row clears never touch the scrollbar lane, so this only needs to be
/// (re)drawn on _Show and whenever uTopItem changes. The outer frame is
/// re-drawn at the end to repair the border over the track lane.
static void _DrawScrollbar(void)
{
  if (s_rowCount <= DL_VISIBLE) {
    return;  // everything fits — no scrollbar
  }
  uint16_t maxTop = s_rowCount - DL_VISIBLE;

  int trackY0 = DL_LIST_Y + 1;   // inset 1px from the outer frame
  int trackY1 = DL_LIST_Y1 - 1;
  int trackH  = trackY1 - trackY0 + 1;
  int thumbH  = trackH * DL_VISIBLE / s_rowCount;
  if (thumbH < 12) {
    thumbH = 12;
  }
  int travel  = trackH - thumbH;
  int thumbY  = trackY0 + travel * (int)m_State.uTopItem / (int)maxTop;
  int thumbY1 = thumbY + thumbH - 1;
  if (thumbY1 > trackY1) {
    thumbY1 = trackY1;
    thumbY  = thumbY1 - thumbH + 1;
  }

  // Subtle track lane
  GUI_SetColor(crScrollTrack);
  GUI_FillRect(DL_SCRL_X0, trackY0, DL_SCRL_X1, trackY1);
  // Rounded thumb, inset within the lane
  GUI_SetColor(crScrollThumb);
  GUI_FillRoundedRect(DL_SCRL_X0 + 2, thumbY, DL_SCRL_X1 - 2, thumbY1, 2);
  // Repair the outer frame border over the track lane
  _DrawOuterFrame();
}

static void _DrawRow(uint16_t uIdx)
{
  if (uIdx >= s_rowCount) {
    return;
  }
  if (uIdx < m_State.uTopItem ||
      uIdx >= m_State.uTopItem + DL_VISIBLE) {
    return;
  }

  GUI_RECT rRow;
  _RowRect(&rRow, uIdx);
  const Row* pRow = &s_rows[uIdx];
  bool bSelected = (uIdx == m_State.uCurItem);

  // Clear row area with solid panel fill (cheap, no image re-decode)
  GUI_SetColor(crListPanel);
  GUI_FillRect(rRow.x0, rRow.y0, rRow.x1, rRow.y1);

  if (kGroup == pRow->kind) {
    // Group header: cyan name + thin separator line beneath
    const char* pStr = GetMultiLangString(pRow->strId);
    if (nullptr != pStr) {
      GUI_RECT rTxt;
      _MakeRect(&rTxt, rRow.x0 + DL_MARGIN, rRow.y0,
                (rRow.x1 - DL_MARGIN) - (rRow.x0 + DL_MARGIN) + 1, DL_ROW_H);
      GUI_SetFont(ftItemName);
      GUI_SetColor(crAccent);
      GUI_SetTextMode(GUI_TEXTMODE_TRANS);
      GUI_DispStringInRect(pStr, &rTxt, GUI_TA_LEFT | GUI_TA_VCENTER);
    }
    GUI_SetColor(crSeparator);
    GUI_DrawHLine(rRow.y1, rRow.x0 + DL_MARGIN, rRow.x1 - DL_MARGIN);
    return;
  }

  // Register row: name (left, indented under its group) + value/state (right)
  const char* pName = _GetRegName(pRow->regNum);
  if (nullptr != pName) {
    int nameX0 = DL_NAME_X0 + DL_NAME_INDENT;
    GUI_RECT rName;
    _MakeRect(&rName, nameX0, rRow.y0,
              DL_VAL_X0 - nameX0, DL_ROW_H);
    GUI_SetFont(ftItemName);
    GUI_SetColor(bSelected ? crAccent : crText);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);
    GUI_DispStringInRect(pName, &rName, GUI_TA_LEFT | GUI_TA_VCENTER);
  }
  _DrawRegValue(pRow->regNum, rRow.y0, bSelected, false);

  // Selection: hollow rounded-rect frame (new style, no fill)
  if (true == bSelected) {
    GUI_SetColor(crSelFrame);
    GUI_DrawRoundedFrame(rRow.x0, rRow.y0, rRow.x1, rRow.y1, 3, 1);
  }

  _CacheRow(uIdx);
}

static void _UpdateList(void)
{
  for (uint16_t i = 0; i < DL_VISIBLE; ++i) {
    uint16_t idx = m_State.uTopItem + i;
    if (idx >= s_rowCount) {
      break;
    }
    _DrawRow(idx);
  }
}

/// Refresh visible register rows whose displayed value changed (1s tick).
/// Unchanged cells are left untouched — flicker-free for steady states.
static void _RefreshValues(void)
{
  for (uint16_t i = 0; i < DL_VISIBLE; ++i) {
    uint16_t idx = m_State.uTopItem + i;
    if (idx >= s_rowCount) {
      break;
    }
    if (kReg != s_rows[idx].kind) {
      continue;
    }
    if (false == _ValueChanged(idx)) {
      continue;
    }
    int y = DL_CONTENT_Y0 + (int)i * DL_ROW_H;
    bool bSelected = (idx == m_State.uCurItem);
    _DrawRegValue(s_rows[idx].regNum, y, bSelected, true);
    _CacheRow(idx);
  }
}

//=============================================================================
// Navigation
//=============================================================================
static void _GoBack(void)
{
  gfc::PopForm();
}

static void _GoNext(void)
{
  gfc::PushForm(WID_LogListForm, nullptr);
}

static void _EnsureCursorVisible(void)
{
  if (m_State.uCurItem < m_State.uTopItem) {
    m_State.uTopItem = m_State.uCurItem;
  } else if (m_State.uCurItem >= m_State.uTopItem + DL_VISIBLE) {
    m_State.uTopItem = m_State.uCurItem - DL_VISIBLE + 1;
  }
}

static void _ClampTop(void)
{
  if (s_rowCount <= DL_VISIBLE) {
    m_State.uTopItem = 0;
    return;
  }
  uint16_t maxTop = s_rowCount - DL_VISIBLE;
  if (m_State.uTopItem > maxTop) {
    m_State.uTopItem = maxTop;
  }
}

/// Next register row (skipping group headers) in direction dir (+1/-1).
/// Returns idx unchanged if no register row exists in that direction.
static uint16_t _NextReg(uint16_t idx, int dir)
{
  int i = (int)idx + dir;
  while (i >= 0 && i < (int)s_rowCount) {
    if (kReg == s_rows[i].kind) {
      return (uint16_t)i;
    }
    i += dir;
  }
  return idx;
}

//=============================================================================
// Key handling
//=============================================================================
// Throttle auto-repeat so a quick tap moves exactly one row while a held key
// repeats at a comfortable rate. The MCU keyboard scans every ~10ms and fires
// GM_KEYPRESS continuously with no initial delay; without throttling, a single
// tap would move many rows. Decoupled from the driver cadence:
//   - GM_KEYDOWN (first press): act immediately, (re)start repeat state.
//   - GM_KEYPRESS (held): act only after DL_KEY_REPEAT_INIT_MS, then every
//     DL_KEY_REPEAT_MS.
static void _OnKey(uint16_t uwKey, bool bRepeat)
{
  if (true == bRepeat) {
    uint32_t now = GUI_GetTime();
    uint32_t threshold = (0 == m_State.uKeyRepeat) ? DL_KEY_REPEAT_INIT_MS
                                                    : DL_KEY_REPEAT_MS;
    if (now - m_State.uLastKeyTick < threshold) {
      return;  // too soon — swallow this repeat
    }
    m_State.uLastKeyTick = now;
    ++m_State.uKeyRepeat;
  } else {
    m_State.uLastKeyTick = GUI_GetTime();
    m_State.uKeyRepeat   = 0;
  }

  switch (uwKey) {
  case KEY_UP: {
    uint16_t old = m_State.uCurItem;
    uint16_t oldTop = m_State.uTopItem;
    uint16_t nxt = _NextReg(m_State.uCurItem, -1);
    if (nxt != m_State.uCurItem) {
      // Cursor can move up to the previous register row
      m_State.uCurItem = nxt;
      _EnsureCursorVisible();
    } else if (0 < m_State.uTopItem) {
      // At the first register row (cursor can't move up): scroll the viewport
      // up one row so the row above (e.g. a group header) becomes visible.
      --m_State.uTopItem;
    } else {
      break;
    }
    if (m_State.uTopItem != oldTop) {
      _UpdateList();
      _DrawScrollbar();
    } else {
      _DrawRow(old);
      _DrawRow(m_State.uCurItem);
    }
    break;
  }
  case KEY_DOWN: {
    uint16_t old = m_State.uCurItem;
    uint16_t oldTop = m_State.uTopItem;
    uint16_t nxt = _NextReg(m_State.uCurItem, +1);
    if (nxt != m_State.uCurItem) {
      // Cursor can move down to the next register row
      m_State.uCurItem = nxt;
      _EnsureCursorVisible();
    } else if (s_rowCount > DL_VISIBLE &&
               m_State.uTopItem < s_rowCount - DL_VISIBLE) {
      // At the last register row (cursor can't move down): scroll the viewport
      // down one row to reveal rows below.
      ++m_State.uTopItem;
    } else {
      break;
    }
    if (m_State.uTopItem != oldTop) {
      _UpdateList();
      _DrawScrollbar();
    } else {
      _DrawRow(old);
      _DrawRow(m_State.uCurItem);
    }
    break;
  }
  case KEY_LEFT:
    _GoNext();
    break;
  case KEY_RIGHT:
    _GoBack();
    break;
  default:
    break;
  }
}

static void _OnKeyUp(uint16_t uwKey)
{
  // Release: reset repeat state so the next tap is a fresh first press.
  m_State.uKeyRepeat = 0;
  if (KEY_ESCAPE == uwKey) {
    _GoBack();
  }
}

//=============================================================================
// Touch handling (Sim only — GUI_SUPPORT_TOUCH)
//=============================================================================
#if GUI_SUPPORT_TOUCH
static void _OnTouch(uint16_t action, int16_t x, int16_t y)
{
  switch (action) {
  case TOUCH_DOWN:
    m_State.touchActive  = true;
    m_State.touchStartX  = x;
    m_State.touchStartY  = y;
    m_State.touchLastY   = y;
    // Hit-test: move cursor to tapped register row (skip group headers)
    if (x >= DL_CONTENT_X0 && x <= DL_CONTENT_X1 &&
        y >= DL_CONTENT_Y0 &&
        y <  DL_CONTENT_Y0 + DL_VISIBLE * DL_ROW_H) {
      uint16_t idx = m_State.uTopItem +
                     (uint16_t)((y - DL_CONTENT_Y0) / DL_ROW_H);
      if (idx >= s_rowCount) {
        idx = s_rowCount - 1;
      }
      if (kGroup == s_rows[idx].kind) {
        uint16_t n = _NextReg(idx, +1);
        if (n != idx) {
          idx = n;
        }
      }
      if (idx != m_State.uCurItem) {
        uint16_t old = m_State.uCurItem;
        m_State.uCurItem = idx;
        _DrawRow(old);
        _DrawRow(m_State.uCurItem);
      }
    }
    break;

  case TOUCH_MOVE:
    if (true == m_State.touchActive) {
          int16_t dy = y - m_State.touchLastY;
      // Vertical drag-scroll: shift uTopItem by rows when drag exceeds ROW_H
      if (dy >= DL_ROW_H) {
        uint16_t shift = (uint16_t)(dy / DL_ROW_H);
        if (m_State.uTopItem >= shift) {
          m_State.uTopItem -= shift;
        } else {
          m_State.uTopItem = 0;
        }
        m_State.touchLastY = y;
        _ClampTop();
        _UpdateList();
        _DrawScrollbar();
      } else if (dy <= -DL_ROW_H) {
        uint16_t shift = (uint16_t)((-dy) / DL_ROW_H);
        if (s_rowCount > DL_VISIBLE) {
          uint16_t maxTop = s_rowCount - DL_VISIBLE;
          uint16_t newTop = m_State.uTopItem + shift;
          m_State.uTopItem = (newTop > maxTop) ? maxTop : newTop;
        }
        m_State.touchLastY = y;
        _UpdateList();
        _DrawScrollbar();
      }
    }
    break;

  case TOUCH_UP:
    if (true == m_State.touchActive) {
      int16_t dx = x - m_State.touchStartX;
      int16_t dy = y - m_State.touchStartY;
      int16_t adx = (dx >= 0) ? dx : (int16_t)(-dx);
      int16_t ady = (dy >= 0) ? dy : (int16_t)(-dy);
      // Horizontal swipe -> switch forms
      if (adx >= DL_SWIPE_PX && adx > ady) {
        if (dx > 0) {
          _GoBack();       // right-swipe -> back to Menu
        } else {
          _GoNext();       // left-swipe  -> FatalLog
        }
      }
      m_State.touchActive = false;
    }
    break;

  default:
    break;
  }
}
#endif // GUI_SUPPORT_TOUCH

//=============================================================================
// Tick
//=============================================================================
// Note: register values are provided by the Sim appTask background thread
// (SimAppTask.cpp), which ports the MCU appTask and injects Sim signals.
// _OnTick only refreshes the displayed value cells.

static void _OnTick(uint32_t uNow)
{
  if ((uNow - m_State.uLastRefresh) >= DL_REFRESH_MS) {
    m_State.uLastRefresh = uNow;
    _RefreshValues();
  }
}

//=============================================================================
// Form lifecycle callbacks
//=============================================================================
static void _Init(const void* argument)
{
  (void)argument;
  memset(&m_State, 0, sizeof(m_State));
  // Allocate the per-row cache (RAMHeap on MCU, new on Sim); freed in _Close.
  if (nullptr != s_pCache) {
#ifndef __vmSIMULATOR__
    RAM_Free(s_pCache);
#else
    delete[] s_pCache;
#endif
    s_pCache = nullptr;
  }
#ifndef __vmSIMULATOR__
  s_pCache = static_cast<RowCache*>(RAM_Malloc(sizeof(RowCache) * s_rowCount));
#else
  s_pCache = new RowCache[s_rowCount];
#endif
  DEV_ASSERT(nullptr == s_pCache, GFC_OutOfMem);
  m_State.uTopItem = 0;
  // Start cursor on the first register row (group headers are non-selectable)
  m_State.uCurItem = (s_rowCount > 1) ? 1 : 0;
}

static void _Show(const void* argument)
{
  (void)argument;
  _ClampTop();
  _FlushForm();
  _UpdateList();
  _DrawScrollbar();
  m_State.uLastRefresh = GUI_GetTime();
}

static void _Close(const void* argument)
{
  (void)argument;
  if (nullptr != s_pCache) {
#ifndef __vmSIMULATOR__
    RAM_Free(s_pCache);
#else
    delete[] s_pCache;
#endif
    s_pCache = nullptr;
  }
}

static void _OnMessage(GM_MESSAGE* pMsg)
{
  if (nullptr == pMsg) {
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
    // Key held down — repeat the navigation action (throttled in _OnKey)
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
// Form descriptor + auto-registration
//=============================================================================
const GWinForm FDataListForm = {
  _Init,
  _Show,
  _Close,
  _OnMessage
};

static const gfc::FormRegistrar kRegDataList(WID_DataListForm,
                                             &FDataListForm,
                                             "DataList");
//-----------------------------------------------------------------------------

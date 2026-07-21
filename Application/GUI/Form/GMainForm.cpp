//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GPMainForm.cpp
 Version     : V2.05
 By          : Wey. Silver Grid

 Description : Main operation form. Displays measurements, status graphics,
               and date/time.  Per SG1210v21 form design spec section II.

   Layer0 (drawn once): background image, Caption bar/divider, Logo,
     measurement labels (4), status labels (2), controller area.
   Layer1 (redrawn periodically): measurement values (4), status values (2),
     status wires (3), status icons (6), status text, date/time.

   Layering strategy (no memory device — avoids 153 KB SRAM cost on MCU):
     - Layer0 is drawn to screen once in _Show().
     - Layer1 uses GUI_TEXTMODE_TRANS for all text: new glyph pixels replace
       old ones transparently.  Since numeric values are right-aligned and
       monospaced within a fixed region, old trailing digits are naturally
       overwritten by the fresh string's pixels.
     - Solid-colour background areas (Caption bar) are filled with the known
       background colour before drawing new text.
     - CSG image background areas do NOT attempt pixel-permanent restoration;
       transparent text mode plus the fixed-width region minimises artifacts.

   Update scheduling (spec II.2):
     - Date/time:   every 60 s
     - Measurement: every 10 s (or immediate on value change)
     - Status data: every 10 s (or immediate on value change)
     - Graphics:    every  1 s (IO-state driven, or +/-10 % deadband)
     - Status text: every  1 s

 Date        : 2026.07.02 (V2.05 — Yoda-style + brace-style compliance)
              2026.07.01 (V2.04 — coordinated sync to 表单设计.md)
              2026.07.01 (V2.03 — animated wire highlight 20%→80% one-way)
              2026.06.29 (V2.02 — removed full-screen memdev; TRANS-based Layer1)
              2026.06.29 (V2.01 — GUI_MEMDEV for Layer0/Layer1 isolation)
              2026.06.29 (V2.00 — rewritten per form design spec)
*/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include "GMainForm.h"

#include "GUI.h"
#include "GFormCentra.h"
#include "GUIMessage.h"
#include "GWinTypes.h"
#include "GFormCentraRegistrar.h"

#include "FontSGRes.h"
#include "GUIPicture.h"
#include "PictureRes.h"
#include "Graphics/ImageRes.h"
#include "Strings/TextStrs.h"
#include "GUIMisc.h"

#include "DevRegs.h"

#ifndef __vmSIMULATOR__
  #include "rtc.h"
#else
  #include <Windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <cstdio>
#include <DevTypes.h>
#include <Dev_Cfg.h>
#include <GUI_Type.h>
#include <GUIConf.h>

//=============================================================================
// Update intervals (ms) — per spec II.2.1
//=============================================================================
#define UPDATE_CLOCK_MS     60000
#define UPDATE_MEAS_MS      2000
#define UPDATE_STAT_MS      2000
#define UPDATE_GRAPHIC_MS   1000
#define UPDATE_TEXT_MS      1000

#ifndef __vmSIMULATOR__
template<typename T> T min(T a, T b) { return (a <  b? a : b); }
template<typename T> T max(T a, T b) { return (a >= b? a : b); }
#endif

//=============================================================================
// Layout — Layer0 — per spec II.2.3.1
// Coordinate format (x, y, w, h).  emWin calls use (x, y, x+w-1, y+h-1).
//=============================================================================

// Background
#define MF_BKG_X            0
#define MF_BKG_Y            0

// Caption back panel (40 px tall, solid colour #041736)
#define MF_CAP_X            0
#define MF_CAP_Y            0
#define MF_CAP_W            320
#define MF_CAP_H            40
#define MF_CAP_COLOR        0x041736

// Caption divider line
#define MF_SEP_X            0
#define MF_SEP_Y            40
#define MF_SEP_W            320
#define MF_SEP_H            1
#define MF_SEP_COLOR        0x2F5CA6

//// Logo
//#define MF_LOGO_X           121
//#define MF_LOGO_Y           220
//#define MF_LOGO_IDX         picIdxMA_Logo78x18

// Measurement data labels — static, drawn once in Layer0
//   输入电压 / 输出电压: 24 px font (top Caption area)
//   充电电流 / 放电电流: 16 px font (lower status area)
#define MF_LBL_UIN_X        10
#define MF_LBL_UIN_Y        8
#define MF_LBL_UIN_W        60
#define MF_LBL_UIN_H        24

#define MF_LBL_UOUT_X       170
#define MF_LBL_UOUT_Y       8
#define MF_LBL_UOUT_W       60
#define MF_LBL_UOUT_H       24

#define MF_LBL24_FONT       GUI_FONT_24LTH_CHN
#define MF_LBL24_COLOR      0xECECEC
#define MF_LBL24_ALIGN      (GUI_TA_LEFT | GUI_TA_VCENTER)

//// Status data labels (2 × 16 px font) — static
//#define MF_LBL_TMP_X        10
//#define MF_LBL_TMP_Y        170
//#define MF_LBL_TMP_W        96   //40
//#define MF_LBL_TMP_H        16
//
//#define MF_LBL_BAT_X        10
//#define MF_LBL_BAT_Y        190
//#define MF_LBL_BAT_W        96   //40
//#define MF_LBL_BAT_H        16

//#define MF_LBL_CHG_X        218
//#define MF_LBL_CHG_Y        170
//#define MF_LBL_CHG_W        40
//#define MF_LBL_CHG_H        16
//
//#define MF_LBL_DIS_X        218
//#define MF_LBL_DIS_Y        190
//#define MF_LBL_DIS_W        40
//#define MF_LBL_DIS_H        16

//#define MF_LBL16_FONT       GUI_FONT_16LTH_CHN
//#define MF_LBL16_COLOR      0xECECEC
//#define MF_LBL16_ALIGN      (GUI_TA_LEFT | GUI_TA_BOTTOM)

// Controller area (rounded rect border, no fill)
#define MF_CTRL_X             105
#define MF_CTRL_Y             65
#define MF_CTRL_W             110
#define MF_CTRL_H             130
#define MF_CTRL_RADIUS        10
#define MF_CTRL_COLOR         0x1485B3
                             
#define MF_CTRL_TAB_HSEP_X1  (MF_CTRL_X + 1)
#define MF_CTRL_TAB_HSEP_X2  (MF_CTRL_X + MF_CTRL_W - 2)
#define MF_CTRL_TAB_HSEP_Y   (MF_CTRL_Y + MF_CTRL_H - MF_CTRL_TAB_HSEP_H)
#define MF_CTRL_TAB_HSEP_W   (MF_CTRL_W)
#define MF_CTRL_TAB_HSEP_H    20

#define MF_CTRL_TAB_VSEP_X   (MF_CTRL_X + MF_CTRL_W / 2)
#define MF_CTRL_TAB_VSEP_Y1  (MF_CTRL_TAB_HSEP_Y + 1)
#define MF_CTRL_TAB_VSEP_Y2  (MF_CTRL_Y + MF_CTRL_H  - 1)

// 
#define MF_CTRL_TAB_ROW1_Y   (MF_CTRL_TAB_HSEP_Y + 1)
#define MF_CTRL_TAB_COL1_X1  (MF_CTRL_X + MF_CTRL_RADIUS)
#define MF_CTRL_TAB_COL1_X2  (MF_CTRL_X + MF_CTRL_W / 2)
#define MF_CTRL_TAB_COL2_X1  (MF_CTRL_TAB_COL1_X2 + 2)
#define MF_CTRL_TAB_COL2_X2  (MF_CTRL_X + MF_CTRL_W - MF_CTRL_RADIUS)

//=============================================================================
// Layout — Layer1 — per spec II.2.3.2
//=============================================================================
// Measurement values — solid background #031635
//   输入电压 / 输出电压: 24 px font (top Caption area)
//   充电电流 / 放电电流: 16 px font (lower status area)
#define MF_VAL_UIN_X        70
#define MF_VAL_UIN_Y        12
#define MF_VAL_UIN_W        80
#define MF_VAL_UIN_H        24
#define MF_VAL_BG           0x031635     // value background colour

#define MF_VAL_UOUT_X       230
#define MF_VAL_UOUT_Y       12
#define MF_VAL_UOUT_W       80
#define MF_VAL_UOUT_H       24

#define MF_VAL24_FONT       GUI_FONT_24LTH_CHN
#define MF_VAL24_COLOR      0xECECEC
#define MF_VAL24_ALIGN      (GUI_TA_RIGHT | GUI_TA_VCENTER)

//// Status values (2 × 16 px font) — over CSG image background
////   Use GUI_TEXTMODE_TRANS: new text pixels overwrite old ones.
//#define MF_VAL_TMP_X        50
//#define MF_VAL_TMP_Y        170
//#define MF_VAL_TMP_W        38
//#define MF_VAL_TMP_H        16

//#define MF_VAL_BAT_X        50
//#define MF_VAL_BAT_Y        190
//#define MF_VAL_BAT_W        38
//#define MF_VAL_BAT_H        16

//#define MF_VAL_CHG_X        260
//#define MF_VAL_CHG_Y        170
//#define MF_VAL_CHG_W        48
//#define MF_VAL_CHG_H        16
//
//#define MF_VAL_DIS_X        260
//#define MF_VAL_DIS_Y        190
//#define MF_VAL_DIS_W        50
//#define MF_VAL_DIS_H        16

#define MF_VAL_BAT_X          (MF_CTRL_TAB_COL1_X2 - MF_VAL_BAT_W - 3)
#define MF_VAL_BAT_Y          (MF_CTRL_TAB_ROW1_Y + 1)
#define MF_VAL_BAT_W          40
#define MF_VAL_BAT_H          16

#define MF_VAL_BAT_BG         0x001C38
#define MF_VAL16_FONT         GUI_FONT_ASCII16B
#define MF_VAL16_ALIGN        (GUI_TA_RIGHT | GUI_TA_VCENTER)

// Status connection wires (3 thin rectangles)
#define MF_WIRE_IN_X        62
#define MF_WIRE_IN_Y        100
#define MF_WIRE_IN_W        58
#define MF_WIRE_IN_H        4

#define MF_WIRE_OUT_X       200
#define MF_WIRE_OUT_Y       100
#define MF_WIRE_OUT_W       51
#define MF_WIRE_OUT_H       4

#define MF_WIRE_BAT_X       158
#define MF_WIRE_BAT_Y       136
#define MF_WIRE_BAT_W       4
#define MF_WIRE_BAT_H       15

#define MF_WIRE_GRAY        0x526670
#define MF_WIRE_GRAD_C      0x00B5F2
#define MF_WIRE_GRAD_M      0xF0F0F0

// Status icons (6 atlas sub-pictures)
#define MF_ICON_AC_X        30
#define MF_ICON_AC_Y        77
#define MF_ICON_AC_IDX      picIdxMA_ACPow32x59Cyan

#define MF_ICON_CTRL_X      120
#define MF_ICON_CTRL_Y      74
#define MF_ICON_CTRL_IDX    picIdxMA_CtrlG80x62Cyan

#define MF_ICON_LED1_X      (MF_ICON_CTRL_X + 13)
#define MF_ICON_LED1_Y      (MF_ICON_CTRL_Y + 19)
#define MF_ICON_LED2_X      (MF_ICON_CTRL_X + 34)
#define MF_ICON_LED2_Y      (MF_ICON_CTRL_Y + 33)
#define MF_ICON_LED3_X      (MF_ICON_CTRL_X + 55)
#define MF_ICON_LED3_Y      (MF_ICON_CTRL_Y + 19)
#define MF_ICON_RLED_IDX    picIdxMA_CtrlLED13x13Red

#define MF_ICON_BRKR_X      250
#define MF_ICON_BRKR_Y      76
#define MF_ICON_BRKR_IDX    picIdxMA_Brkr56x60Cyan

#define MF_ICON_BAT_X       (MF_WIRE_BAT_X - 12)
#define MF_ICON_BAT_Y       (MF_WIRE_BAT_Y + MF_WIRE_BAT_H)

// Icon area of Controller
#define MF_CTRL_ICON_X      (MF_CTRL_TAB_COL1_X2 + 4) // 270
#define MF_CTRL_ICON_Y      (MF_CTRL_TAB_ROW1_Y + 1) // 221

#define MF_ICON_HEAT_X      MF_CTRL_ICON_X // 270
#define MF_ICON_HEAT_Y      MF_CTRL_ICON_Y // 221
#define MF_ICON_HEAT_IDX    picIdxMA_Fire16x16

#define MF_ICON_FAN_X       (MF_CTRL_ICON_X + 20) // 293
#define MF_ICON_FAN_Y       MF_CTRL_ICON_Y        // 221
#define MF_ICON_FAN_IDX     picIdxMA_Fan16x16Cyan

// Icon saturation state: 100=on, 10=off. 
// Used for change detection & deadband.
#define MF_ICON_SAT_ON      100
#define MF_ICON_SAT_OFF     10

// // Status text (bypass / coil state)
// #define MF_STAT_X           216
// #define MF_STAT_Y           160
// #define MF_STAT_W           100
// #define MF_STAT_H           25
// #define MF_STAT_FONT        GUI_FONT_24LTH_CHN
// #define MF_STAT_ALIGN       (GUI_TA_HCENTER | GUI_TA_VCENTER)
// #define MF_STAT_COLOR_ON    0x25B92C
// #define MF_STAT_COLOR_OFF   0xF06D0D
 #define MF_STAT_BG          0x001838     // status value background

// Date / time
#define MF_CLOCK_X          10
#define MF_CLOCK_Y          221
#define MF_CLOCK_W          120
#define MF_CLOCK_H          16
#define MF_CLOCK_FONT       GUI_FONT_ASCII16B

#define MF_CLOCK_COLOR      0xBDBDBD
#define MF_CLOCK_ALIGN      (GUI_TA_LEFT | GUI_TA_VCENTER)
#define MF_CLOCK_BG         0x001028     // clock background

// ---- Button (2.3.3) ----
// Menu button — atlas icon, 20×20, 70 % saturation
#define MF_BTN_MENU_X        286
#define MF_BTN_MENU_Y        218
#define MF_BTN_MENU_W        20
#define MF_BTN_MENU_H        20
#define MF_BTN_MENU_IDX      picIdxMA_Menu20x20Cyan
#define MF_BTN_MENU_SAT      30

// Alpha values (0=transparent, 255=opaque) — per spec II.2.3.1
#define MF_CAP_ALPHA        51      // 20% opacity = 255*0.20
#define MF_SEP_ALPHA        38      // 15% opacity = 255*0.15
#define MF_CTRL_ALPHA       20      //  8% opacity = 255*0.08

// Deadband
#define MF_DEADBAND_PCT     0.10f
#define MF_VOLT_THRESHOLD   30.0f

//=============================================================================
// Colour thresholds — per spec II.2.3.2 item 2
//=============================================================================
#define MF_TEMP_HIGH        40.0f
#define MF_TEMP_LOW         -10.0f
#define MF_TEMP_COLOR_HI    0xFF1010
#define MF_TEMP_COLOR_LO    0xFF1010
#define MF_TEMP_COLOR_OK    0x3DF83D

#define MF_BAT_HIGH         85.0f
#define MF_BAT_LOW          60.0f
#define MF_BAT_COLOR_HI     0x3DF83D
#define MF_BAT_COLOR_MID    0xFF4020
#define MF_BAT_COLOR_LO     0xFF1010

static void _DrawWireBat(void);
//=============================================================================
// Battery icon index lookup — per spec II.2.3.2 item 4
// 4-level hysteresis (implemented below after m_State declaration)
//=============================================================================
static int _GetBatteryIconIdx(int rLevel);

//=============================================================================
// Form state — caches previous values for change detection & deadband
//=============================================================================
typedef struct tagMainFormState {
  uint32_t uLastClockTick;
  uint32_t uLastMeasTick;
  uint32_t uLastStatTick;
  uint32_t uLastGraphicTick;
  uint32_t uLastTextTick;

  float    rLastUin;
  float    rLastUout;
  float    rLastChgI;
  float    rLastDisI;

  float    rLastTemp;
  float    rLastBatLevel;

  float    rLastWireUin;
  float    rLastWireUout;
  int      iLastBatIconIdx;
  char     iLastBatSat;
  char     iLastCharge     = MF_ICON_SAT_OFF;;
  int      iLastAcSat      = MF_ICON_SAT_OFF;
  TStateReg stLastHeaterSt = MF_ICON_SAT_OFF;
  TStateReg stLastFanSt    = MF_ICON_SAT_OFF;

  TDevStateReg  stLastCoil;
  TStateReg   stLastPassby;
  uint8_t stCrtlLED[3] = { MF_ICON_SAT_OFF, MF_ICON_SAT_OFF, MF_ICON_SAT_OFF };

  //char     szClock[32];
  uint8_t  ucLastMinute = 0; //

  enum emBattery {
      batIdel = 0,
      batCharge = 10,
      batDischarge = 20
  } stBattery = batIdel;

  // Wire highlight animation: one-way 20%→80%, resets to 20%
  int    uWireHLPct = 20;   // current highlight position (% of wire width)

  float rChargeI    = 9.26,
        rDischargeI = 3.51;  // current charge current

} TMainFormState;

static TMainFormState m_State;

//=============================================================================
// Battery icon lookup table — per spec II.2.3.2 item 4
// Ordered high→low.  Hysteresis: rise at threshold, drop at threshold - 2%.
//=============================================================================
struct TBatIconLevel {
  uint8_t   iThreshold;   // rLevel ≥ this → use this icon
  uint32_t  uColor;       // value colour (tint)
  uint8_t   iIconIdx;     // atlas sub-picture index
};
constexpr int kBatRiseDropDelta = 2; // 2% deadband for drop threshold

static const TBatIconLevel kBatIconTable[] = {
  { 95, 0x80FF80, picIdxMA_Battery32x20C5 },
  { 80, 0x00FF00, picIdxMA_Battery32x20C4 },
  { 50, 0xFFFF00, picIdxMA_Battery32x20C3 },
  { 20, 0xFFA500, picIdxMA_Battery32x20C2 },
  { 0,  0xFF0000, picIdxMA_Battery32x20C1 }, // lowest: always stay if reached
};
static const int kBatIconCount = sizeof(kBatIconTable) / sizeof(kBatIconTable[0]);

//=============================================================================
// Battery icon index with hysteresis.
//   Rise:  rLevel ≥ iRiseThreshold → immediately step up
//   Drop:  rLevel < iDropThreshold (2% deadband) → step down
//=============================================================================
static int _GetBatteryIconIdx(int iLevel)
{
  int iLast = m_State.iLastBatIconIdx;

  // Search table high→low.  
  // If currently at a level, stay unless dropped below hysteresis.
  for (int i = 0; i < kBatIconCount - 1; ++i) {
    auto pLvl = &kBatIconTable[i];

    if (pLvl->iIconIdx == iLast) {
      // Currently at this level — stay unless dropped below hysteresis
      if (iLevel < pLvl->iThreshold - kBatRiseDropDelta) {
        continue;  // drop to next level
      }
      return pLvl->iIconIdx;
    }

    // Below this level — rise if threshold met
    if (iLevel >= pLvl->iThreshold) {
      return pLvl->iIconIdx;
    }
  }

  // Fallback (should not reach here)
  return kBatIconTable[kBatIconCount-1].iIconIdx;
}

//=============================================================================
// Helpers
//=============================================================================

//-----------------------------------------------------------------------------
// Convert (x, y, w, h) to emWin GUI_RECT (x0, y0, x1, y1)
//-----------------------------------------------------------------------------
static void _MakeRect(GUI_RECT* pR, int x, int y, int w, int h)
{
  pR->x0 = x;
  pR->y0 = y;
  pR->x1 = x + w - 1;
  pR->y1 = y + h - 1;
}

//-----------------------------------------------------------------------------
// Check if value crossed threshold ±10% deadband (prevents flicker)
//-----------------------------------------------------------------------------
static bool _DeadbandCrossed(float rOld, float rNew, float rThreshold)
{
  float rDelta = rThreshold * MF_DEADBAND_PCT;
  if (rOld < rThreshold)
    return (rNew > rThreshold + rDelta);
  else
    return (rNew < rThreshold - rDelta);
}

//-----------------------------------------------------------------------------
// Fill a rectangle with the Caption solid background colour.
// Used to "erase" old Layer1 text inside the Caption bar before redrawing.
//-----------------------------------------------------------------------------
static void _EraseValueRect(int x, int y, int w, int h)
{
  GUI_SetColor(MF_VAL_BG);
  GUI_FillRect(x, y, x + w - 1, y + h - 1);
}

//=============================================================================
// Numeric formatting — convert register values to display strings
//=============================================================================

// "228.1V" — 3 digits integer, 1 decimal + unit
static void _FmtVoltage(char* pBuf, size_t n, float rVal)
{
  snprintf(pBuf, n, "%0.1fV", rVal);
}

#if defined(MF_VAL_DIS_X) || defined(MF_VAL_CHG_X)
// "10.69A" — 2 digits integer, 2 decimals + unit
static void _FmtCurrent(char* pBuf, size_t n, float rVal)
{
  snprintf(pBuf, n, "%0.2fA", rVal);
}
#endif

#ifdef MF_VAL_TMP_X
// Uses localised format string from idMainFmSt04 (e.g. "%.1f℃")
static void _FmtTemp(char* pBuf, size_t n, float rVal)
{
  //auto szFmt = GetMultiLangString(idMainFmSt04);
  //if (szFmt )
  //  snprintf(pBuf, n, szFmt, rVal);
    snprintf(pBuf, n, "%0.1f", rVal);
}
#endif

#ifdef MF_VAL_BAT_X
// "85.3%" — 2 digits integer, 1 decimal + '%' unit
static void _FmtBattery(char* pBuf, size_t n, float rVal)
{
    if ( 99.9f <rVal)
      snprintf(pBuf, n, "%d%%", int(rVal + 0.5));
    else
      snprintf(pBuf, n, "%0.1f%%", rVal);
}
#endif

//=============================================================================
// Drawing — Layer0 (drawn once to screen in _Show)
//   绘制背景 + Caption + 控制器边框 + 全部静态 Label
//=============================================================================

static void _DrawLayer0(void)
{
  // Background image — full opacity
  GUI_DrawPicture(IMAGE_BACKGROUND, MF_BKG_X, MF_BKG_Y, 0, 100);

  // --- Caption back panel (20% opacity on top of CSG background) ---
  GUI_EnableAlpha(1);
//  GUI_SetAlpha(MF_CAP_ALPHA);
//  GUI_SetColor(MF_CAP_COLOR);
//  GUI_FillRect(MF_CAP_X, MF_CAP_Y,
//               MF_CAP_X + MF_CAP_W - 1, MF_CAP_Y + MF_CAP_H - 1);

  // Caption divider (15% opacity)
  GUI_SetAlpha(MF_SEP_ALPHA);
  GUI_SetColor(MF_SEP_COLOR);
  GUI_FillRect(MF_SEP_X, MF_SEP_Y,
               MF_SEP_X + MF_SEP_W - 1, MF_SEP_Y + MF_SEP_H - 1);
  GUI_EnableAlpha(0);

#ifdef MF_LOGO_IDX
  // Logo — full opacity (emWin FillRect above may have changed clip)
  SetFullClip();
  GUI_DrawPicture(CSG_WINATLAS, MF_LOGO_X, MF_LOGO_Y, MF_LOGO_IDX, 100);
#endif

  // --- Controller area — rounded rect border only, no fill ---
  GUI_SetColor(MF_CTRL_COLOR);
  GUI_SetPenSize(1);
  GUI_DrawRoundedRect(MF_CTRL_X, MF_CTRL_Y,
                      MF_CTRL_X + MF_CTRL_W - 1,
                      MF_CTRL_Y + MF_CTRL_H - 1, MF_CTRL_RADIUS);

  GUI_DrawHLine(MF_CTRL_TAB_HSEP_Y, 
                 MF_CTRL_TAB_HSEP_X1, 
                 MF_CTRL_TAB_HSEP_X2);
  GUI_DrawVLine(MF_CTRL_TAB_VSEP_X, 
                 MF_CTRL_TAB_VSEP_Y1,
                 MF_CTRL_TAB_VSEP_Y2);

  // Static labels — full opacity, transparent text
  const char* pStr;
  GUI_RECT    r;

  GUI_SetTextMode(GUI_TEXTMODE_TRANS);

  // Voltage labels (24 px font — top Caption area)
  GUI_SetFont(MF_LBL24_FONT);
  GUI_SetColor(MF_LBL24_COLOR);

  #define DR_LBL24(id, x,y,w,h) do { \
    pStr = GetMultiLangString(id);   \
    if (nullptr != pStr) { _MakeRect(&r,x,y,w,h); GUI_DispStringInRect(pStr,&r,MF_LBL24_ALIGN); } \
  } while(0)

  DR_LBL24(idMainLabel01, MF_LBL_UIN_X,  MF_LBL_UIN_Y,  MF_LBL_UIN_W,  MF_LBL_UIN_H);
  DR_LBL24(idMainLabel02, MF_LBL_UOUT_X, MF_LBL_UOUT_Y, MF_LBL_UOUT_W, MF_LBL_UOUT_H);

  #undef DR_LBL24

#ifdef MF_LBL16_FONT
  // Current & status labels (16 px font — lower area)
  GUI_SetFont(MF_LBL16_FONT);
  GUI_SetColor(MF_LBL16_COLOR);
#endif

#ifdef MF_LBL16_ALIGN
  #define DR_LBL16(id, x,y,w,h) do { \
    pStr = GetMultiLangString(id);   \
    if (nullptr != pStr) {           \ 
       _MakeRect(&r,x,y,w,h);        \
       GUI_DispStringInRect(pStr,&r,MF_LBL16_ALIGN); \
    } \
  } while(0)
#endif

#ifdef MF_LBL_CHG_X
  DR_LBL16(idMainLabel03, MF_LBL_CHG_X,  MF_LBL_CHG_Y,  MF_LBL_CHG_W,  MF_LBL_CHG_H);
#endif
#ifdef MF_LBL_CHG_X
  DR_LBL16(idMainLabel04, MF_LBL_DIS_X, MF_LBL_DIS_Y, MF_LBL_DIS_W, MF_LBL_DIS_H);
#endif
#ifdef MF_LBL_CHG_X
  DR_LBL16(idMainLabel05, MF_LBL_TMP_X, MF_LBL_TMP_Y, MF_LBL_TMP_W, MF_LBL_TMP_H);
#endif
#ifdef MF_LBL_CHG_X
  DR_LBL16(idMainLabel06, MF_LBL_BAT_X, MF_LBL_BAT_Y, MF_LBL_BAT_W, MF_LBL_BAT_H);
#endif

#ifdef DR_LBL16
  #undef DR_LBL16
#endif

  // Menu button (2.3.3) — bottom-right, 70 % saturation, touch → GMenuForm
#if GUI_SUPPORT_TOUCH
  GUI_DrawPicture(CSG_WINATLAS, MF_BTN_MENU_X, MF_BTN_MENU_Y,
                  MF_BTN_MENU_IDX, MF_BTN_MENU_SAT);
#endif
}

//=============================================================================
// Drawing — Layer1: measurement values
//   Voltage values: inside Caption bar (solid background → fill-then-draw)
//   Current values: lower status area (over CSG bg → fill+TRANS)
//=============================================================================

// Forward decl — _DrawValueTrans defined later (status values section)
static void _DrawValueTrans(const char* pStr, const GUI_FONT* pFont,
                            int x, int y, int w, int h,
                            GUI_COLOR color, int align, GUI_COLOR bg);

// ---- Draw a value on solid Caption background (erase then TRANS text) ----
// Used for voltage values inside the Caption bar (24px font, MF_VAL_BG fill)
static void _DrawValueSolid(const char* pStr, const GUI_FONT* pFont,
                            int x, int y, int w, int h,
                            GUI_COLOR color, int align)
{
  _EraseValueRect(x, y, w, h);     // restore solid Caption background

  GUI_RECT r;
  _MakeRect(&r, x, y, w, h);
  GUI_SetFont(pFont);
  GUI_SetColor(color);
  GUI_SetTextMode(GUI_TEXTMODE_TRANS);
  GUI_DispStringInRect(pStr, &r, align);
}

// ---- Update 4 measurement values (voltage ×2, current ×2) ----
// Voltage: _DrawValueSolid (Caption area, 24px), Current: _DrawValueTrans (lower area, 16px)
// Also derives battery charge/discharge state from current direction
static void _UpdateMeasValues(bool bForce)
{
  char  szBuf[16];
  float rVal;

  rVal = _GetRealReg(REG_RL_Uin);
  if (bForce || rVal != m_State.rLastUin) {
    m_State.rLastUin = rVal;
    _FmtVoltage(szBuf, sizeof(szBuf), rVal);
    _DrawValueSolid(szBuf, MF_VAL24_FONT,
                    MF_VAL_UIN_X, MF_VAL_UIN_Y, MF_VAL_UIN_W, MF_VAL_UIN_H,
                    MF_VAL24_COLOR, MF_VAL24_ALIGN);
  }

  rVal = _GetRealReg(REG_RL_Uout);
  if (bForce || rVal != m_State.rLastUout) {
    m_State.rLastUout = rVal;
    _FmtVoltage(szBuf, sizeof(szBuf), rVal);
    _DrawValueSolid(szBuf, MF_VAL24_FONT,
                    MF_VAL_UOUT_X, MF_VAL_UOUT_Y, MF_VAL_UOUT_W, MF_VAL_UOUT_H,
                    MF_VAL24_COLOR, MF_VAL24_ALIGN);
  }

  // Update Battery ststus
  auto rChgI = _GetRealReg(REG_RL_BCHRG_Ibus),
       rDisI = _GetRealReg(REG_RL_BTOUT_Ibus);
  TMainFormState::emBattery stBat = TMainFormState::batIdel;
  if (0.5f < rChgI && 0.5f < rDisI)
    {
      if (rChgI > rDisI)
          stBat = TMainFormState::batCharge;
      else
          stBat = TMainFormState::batDischarge;
  } else
      stBat = TMainFormState::batIdel;

  if( m_State.stBattery != stBat ) {
      m_State.stBattery = stBat;
      _DrawWireBat();
    }

#ifdef MF_VAL_CHG_X
    if (bForce || rChgI != m_State.rLastChgI) {
    m_State.rLastChgI = rChgI;
    _FmtCurrent(szBuf, sizeof(szBuf), rChgI);
    _DrawValueTrans(szBuf, MF_VAL16_FONT,
                    MF_VAL_CHG_X, MF_VAL_CHG_Y, MF_VAL_CHG_W, MF_VAL_CHG_H,
                    MF_VAL24_COLOR, MF_VAL16_ALIGN, MF_STAT_BG);
  }
#endif

#ifdef MF_VAL_DIS_X
  if (bForce || rDisI != m_State.rLastDisI) {
    m_State.rLastDisI = rDisI;
    _FmtCurrent(szBuf, sizeof(szBuf), rDisI);
    _DrawValueTrans(szBuf, MF_VAL16_FONT,
                    MF_VAL_DIS_X, MF_VAL_DIS_Y, MF_VAL_DIS_W, MF_VAL_DIS_H,
                    MF_VAL24_COLOR, MF_VAL16_ALIGN, MF_STAT_BG);
  }
#endif
}

//=============================================================================
// Drawing — Layer1: status values
//   Over CSG image background → GUI_TEXTMODE_TRANS, no explicit erase.
//   Right-aligned + fixed-width region keeps old-pixel-bleed minimal.
//=============================================================================

#ifdef MF_VAL_TMP_X
// ---- Threshold-based colour: temperature (>40℃ orange, >0℃ green, else orange) ----
static GUI_COLOR _TempColor(float rTemp)
{
  if (rTemp > MF_TEMP_HIGH) return MF_TEMP_COLOR_HI;
  if (rTemp > MF_TEMP_LOW)  return MF_TEMP_COLOR_OK;
  return MF_TEMP_COLOR_LO;
}
#endif

#ifdef MF_VAL_BAT_X
// ---- Threshold-based colour: battery (>80% green, >50% cyan, else orange) ----
static GUI_COLOR _BatColor(int iLevel)
{

  // Search table high→low.
  // If currently at a level, stay unless dropped below hysteresis.
  for (const auto& level : kBatIconTable) {
      // Currently at this level — stay unless dropped below hysteresis
      if (level.iThreshold <= iLevel ) {
          return level.uColor;
      }
  }

  // Fallback (should not reach here)
  return kBatIconTable[kBatIconCount - 1].uColor;
}
#endif

// ---- Draw a value over CSG background (fill solid bg then TRANS text) ----
// Used for current values + temperature + battery (16px font, MF_STAT_BG fill)
static void _DrawValueTrans(const char* pStr, const GUI_FONT* pFont,
                            int x, int y, int w, int h,
                            GUI_COLOR color, int align, GUI_COLOR bg)
{
  GUI_SetColor(bg);
  GUI_FillRect(x, y, x + w - 1, y + h - 1);

  GUI_RECT r;
  _MakeRect(&r, x, y, w, h);
  GUI_SetFont(pFont);
  GUI_SetColor(color);
  GUI_SetTextMode(GUI_TEXTMODE_TRANS);
  GUI_DispStringInRect(pStr, &r, align);
}

// ---- Update temperature + battery status values (16px, over CSG background) ----
static void _UpdateStatValues(bool bForce)
{
  char  szBuf[16];
  float rVal;

#ifdef MF_VAL_TMP_X
  rVal = _GetRealReg(REG_RL_BAT_TEMPERATRUE); //REG_RL_RTC_TEMP);
  if (bForce || rVal != m_State.rLastTemp) {
    m_State.rLastTemp = rVal;
    _FmtTemp(szBuf, sizeof(szBuf), rVal);
    _DrawValueTrans(szBuf, MF_VAL16_FONT,
                    MF_VAL_TMP_X, MF_VAL_TMP_Y, MF_VAL_TMP_W, MF_VAL_TMP_H,
                    _TempColor(rVal), MF_VAL16_ALIGN, MF_STAT_BG);
  }
#endif

#ifdef MF_VAL_BAT_X
  rVal = _GetRealReg(REG_RL_BAT_CAPLevel);
  if (bForce || rVal != m_State.rLastBatLevel) {
    m_State.rLastBatLevel = rVal;
    _FmtBattery(szBuf, sizeof(szBuf), rVal);
    _DrawValueTrans(szBuf, MF_VAL16_FONT,
                    MF_VAL_BAT_X, MF_VAL_BAT_Y, MF_VAL_BAT_W, MF_VAL_BAT_H,
                    _BatColor(rVal), MF_VAL16_ALIGN, MF_VAL_BAT_BG);
  }
#endif
}

//=============================================================================
// Drawing — Layer1: status connection wires
//   These are opaque rectangles; drawn directly over whatever is underneath.
//=============================================================================
static void _DrawWireHighlightH( int x, int y, int w, int h, int pct )
{

    int const kHalfW = 7;
    int cx = x + (int)(w * pct / 100);
    int x1 = max(x, cx - kHalfW);
    int x2 = min(x + w, cx + kHalfW) - 1;

    GUI_DrawGradientH(x1, y, cx, y + h - 1,
        MF_WIRE_GRAD_C, MF_WIRE_GRAD_M);
    GUI_DrawGradientH(cx + 1, y, x2, y + h - 1,
        MF_WIRE_GRAD_M, MF_WIRE_GRAD_C);
}

// ---- Draw horizontal connection wire ----
// rVolt < 30V: solid gray;  rVolt ≥ 30V: cyan base + animated gradient highlight
// iHLPct: highlight centre position as % of wire width (20→80 one-way, then resets)
static void _DrawWireH(int x, int y, int w, int h, float rVolt, int iHLPct)
{
  if (MF_VOLT_THRESHOLD > rVolt) {
    GUI_SetColor(MF_WIRE_GRAY);
    GUI_FillRect(x, y, x + w - 1, y + h - 1);
  } else {
    GUI_SetColor(MF_WIRE_GRAD_C);
    GUI_FillRect(x, y, x + w - 1, y + h - 1);

    // Animated highlight: iHLPct is % of wire width from left edge (10→90)
    _DrawWireHighlightH(x, y, w, h, iHLPct);

    if (60 < iHLPct)
        _DrawWireHighlightH(x, y, w, h, iHLPct - 50);
  }
}

// ---- Draw vertical battery wire with charge/discharge gradient ----
// Charge: cyan→white upward;  Discharge: white→cyan downward;  Idle: solid cyan
static void _DrawWireBat(int x, int y, int w, int h)
{

  GUI_SetColor(MF_WIRE_GRAD_C);
  GUI_FillRect(x, y, x + w - 1, y + h - 1);
  
  GUI_COLOR crStart, crEnd;
  if (TMainFormState::batIdel != m_State.stBattery) {
    if (TMainFormState::batCharge == m_State.stBattery) {
        crStart = MF_WIRE_GRAD_C;
        crEnd = MF_WIRE_GRAD_M;
    } else {
        crStart = MF_WIRE_GRAD_M;
        crEnd = MF_WIRE_GRAD_C;  
    }

  int cy = y + h / 2;
  GUI_DrawGradientV(x, cy - 6, x + w - 1, cy - 1,
      crStart, crEnd);
  GUI_DrawGradientV(x, cy, x + w - 1, cy + 5,
      crStart, crEnd);
  // GUI_DrawGradientV(x, y, x + w - 1, y + h - 1,
  //    MF_WIRE_GRAD_C, MF_WIRE_GRAD_M);
  }
}

// ---- Draw battery wire at fixed MF_WIRE_BAT position ----
static void _DrawWireBat( void )
{

  _DrawWireBat(MF_WIRE_BAT_X, MF_WIRE_BAT_Y, MF_WIRE_BAT_W, MF_WIRE_BAT_H);
}

// ---- Update 3 status connection wires (input, output, battery) ----
// Animates wire highlight when voltage ≥ threshold; deadband avoids flicker
static void _UpdateWires(bool bForce)
{
  
  float rUin  = _GetRealReg(REG_RL_Uin);
  float rUout = _GetRealReg(REG_RL_Uout);

  // Animate wire highlight: moves 10%→90% one-way, then resets to 10%
  // Animate only when at least one wire is "live" (voltage ≥ threshold)
  bool bLive = (rUin >= MF_VOLT_THRESHOLD || rUout >= MF_VOLT_THRESHOLD);
  if (true == bLive) {
    m_State.uWireHLPct += 5;  // 10% per tick (1 s)
    if (90 < m_State.uWireHLPct) {
      m_State.uWireHLPct -= 50;  // reset to start
    }
  }

  if (bForce || bLive || _DeadbandCrossed(m_State.rLastWireUin, rUin, MF_VOLT_THRESHOLD)) {
    m_State.rLastWireUin = rUin;
    _DrawWireH(MF_WIRE_IN_X, MF_WIRE_IN_Y, MF_WIRE_IN_W, MF_WIRE_IN_H,
               rUin, m_State.uWireHLPct);
  }

  if (bForce || bLive || _DeadbandCrossed(m_State.rLastWireUout, rUout, MF_VOLT_THRESHOLD)) {
    m_State.rLastWireUout = rUout;
    _DrawWireH(MF_WIRE_OUT_X, MF_WIRE_OUT_Y, MF_WIRE_OUT_W, MF_WIRE_OUT_H,
               rUout, m_State.uWireHLPct);
  }

  if (bForce) {
    _DrawWireBat();
  }
}

//=============================================================================
// Drawing — Layer1: status icons
//   GUI_DrawPicture draws opaque pixels; saturation handles dim/bright.
//=============================================================================
// update Controller LED state (3 × 13px) from IO state registers
static void _UpdateCtrlLEDs(bool bForce)
{
    TStateReg stPassby = _GetIOStateReg(REG_stPassby);
    uint8_t stLED[3];

    // If in Passby mode, force LED1 & LED3 on, LED2 off; else update from voltage & IO state
    if (STATE_TRUE == stPassby) {
        stLED[0] = MF_ICON_SAT_ON;  // Passby LED on
        stLED[1] = MF_ICON_SAT_OFF; 
        stLED[2] = MF_ICON_SAT_ON;
    } else  {
        // Update LED state from voltage thresholds and IO state
        for (int i = 0; i < 3; ++i) {
            stLED[i]  = m_State.stCrtlLED[i];
        }

        // Update LED1 (AC input) & LED3 (Breaker Power) from voltage thresholds, LED2 (Inverter output) from IO state
        float rUin  = _GetRealReg(REG_RL_Uin);
        float rUout = _GetRealReg(REG_RL_Uout);
        TStateReg stInvertorOk = _GetIOStateReg(REG_stInvertorOk);

        // LED1: AC input LED on if voltage > threshold, off if < threshold
        if (MF_ICON_SAT_ON != stLED[0] && MF_VOLT_THRESHOLD * 1.1 < rUin) {
            stLED[0] = MF_ICON_SAT_ON; // ACin LED on
        } else if (MF_ICON_SAT_OFF != stLED[0] && MF_VOLT_THRESHOLD * 0.9 > rUin) {
            stLED[0] = MF_ICON_SAT_OFF; // ACin LED off
        }

        // LED3: Breaker Power LED on if voltage > threshold, off if < threshold
        if (MF_ICON_SAT_ON != stLED[2] && MF_VOLT_THRESHOLD * 1.1 < rUout) {
            stLED[2] = MF_ICON_SAT_ON;  // Breaker Power LED on
        } else if (MF_ICON_SAT_OFF != stLED[2] && MF_VOLT_THRESHOLD * 0.9 > rUout) {
            stLED[2] = MF_ICON_SAT_OFF; // Breaker Power LED off
        }

        // LED2: Inverter output LED on if IO state is true, off if false
        if( STATE_TRUE == stInvertorOk ) {
            stLED[1] = MF_ICON_SAT_ON;  // Inverter output LED on
        } else {
            stLED[1] = MF_ICON_SAT_OFF;  // Inverter output LED off
        }
    }

    // No change in LED state, skip redraw
    if ( false == bForce &&
         stLED[0] == m_State.stCrtlLED[0] && 
         stLED[1] == m_State.stCrtlLED[1] && 
         stLED[2] == m_State.stCrtlLED[2] )
        return;

    // Erase old controller icon (background + 3 LEDs) and redraw new one
    GUI_DrawPicture(CSG_WINATLAS, MF_ICON_CTRL_X, MF_ICON_CTRL_Y, picIdxMA_CtrlG80x62Cyan);

    // Draw 3 LEDs (red) if on, else leave green LEDs (part of controller icon) visible
    if (MF_ICON_SAT_ON == stLED[0]) {
        GUI_DrawPicture(CSG_WINATLAS, MF_ICON_LED1_X, MF_ICON_LED1_Y, MF_ICON_RLED_IDX);
    }

    if (MF_ICON_SAT_ON == stLED[1]) {
        GUI_DrawPicture(CSG_WINATLAS, MF_ICON_LED2_X, MF_ICON_LED2_Y, MF_ICON_RLED_IDX);
    }
    
    if (MF_ICON_SAT_ON == stLED[2]) {
        GUI_DrawPicture(CSG_WINATLAS, MF_ICON_LED3_X, MF_ICON_LED3_Y, MF_ICON_RLED_IDX);
    }

    // update cached LED state
    for (int i = 0; i < 3; ++i) {
       m_State.stCrtlLED[i] = stLED[i];
    }
}
// ---- Update 6 status icons (AC input, controller, breaker, battery, heater, fan) ----
// Saturation dims/brightens based on voltage, IO state, or battery level
static void _UpdateIcons(bool bForce)
{

  float rUin = _GetRealReg(REG_RL_Uin);
  float rBat = _GetRealReg(REG_RL_BAT_CAPLevel);

  // AC input — saturation depends on voltage
  // 1. iAcSat  = m_State.iLastAcSat
  // 2. If Uin > MF_VOLT_THRESHOLD * 1.1 and m_State.iLastAcSat == MF_ICON_SAT_OFF then
  //    iAcSat = MF_ICON_SAT_ON (bright)
  // 3. If Uin < MF_VOLT_THRESHOLD * 0.9 and m_State.iLastAcSat == MF_ICON_SAT_ON then
  //    iAcSat = MF_ICON_SAT_OFF (dim)
  // 4. if( iAcSat != m_State.iLastAcSat ) then redraw AC input icon with new saturation
  int iAcSat = m_State.iLastAcSat;
  if (rUin > MF_VOLT_THRESHOLD * 1.1f && MF_ICON_SAT_ON != iAcSat) {
      iAcSat = MF_ICON_SAT_ON; // bright
  } else if (rUin < MF_VOLT_THRESHOLD * 0.9f && MF_ICON_SAT_OFF != iAcSat) {
      iAcSat = MF_ICON_SAT_OFF; // dim
  }

  if (iAcSat != m_State.iLastAcSat || true == bForce) {
      m_State.iLastAcSat = iAcSat;
      GUI_DrawPicture(CSG_WINATLAS, MF_ICON_AC_X, MF_ICON_AC_Y,
          MF_ICON_AC_IDX, iAcSat);
  }

  // Controller
  _UpdateCtrlLEDs(bForce);
 
  // Breaker — always 100 %
  if (true == bForce)
    GUI_DrawPicture(CSG_WINATLAS, MF_ICON_BRKR_X, MF_ICON_BRKR_Y,
                    MF_ICON_BRKR_IDX, 100);

  // Battery — index + saturation from charge level
  bool bDrawBat = false;
  int iBatIdx = _GetBatteryIconIdx(rBat);
  int iBatSat = (rBat > 10.0f) ? MF_ICON_SAT_ON : MF_ICON_SAT_OFF;
  if (true == bForce || 
       iBatIdx != m_State.iLastBatIconIdx || 
       iBatSat != m_State.iLastBatSat) {
    m_State.iLastBatIconIdx = iBatIdx;
    m_State.iLastBatSat     = iBatSat;

    bDrawBat = true;
  }

  // Charge/discharge arrow
  // 
  if (TMainFormState::batCharge == m_State.stBattery) {
       if( MF_ICON_SAT_ON != m_State.iLastCharge) {
          m_State.iLastCharge = MF_ICON_SAT_ON;
          bDrawBat = true;
      }
  } else if (MF_ICON_SAT_OFF != m_State.iLastCharge) {
          m_State.iLastCharge = MF_ICON_SAT_OFF;
          bDrawBat = true;
  }

  // Draw battery icon if index or saturation changed, or if forced
  if (true == bDrawBat) {
      GUI_DrawPicture(CSG_WINATLAS, MF_ICON_BAT_X, MF_ICON_BAT_Y,
                      iBatIdx, iBatSat);

      // Draw charge/discharge arrow on top of battery icon
      if (MF_ICON_SAT_ON == m_State.iLastCharge) {
          GUI_DrawPicture(CSG_WINATLAS, 
                          MF_ICON_BAT_X + 10, MF_ICON_BAT_Y + 3,
                          picIdxMA_BatCharge11X13, 100);
      }
  }

  // Heater — on/off from IO state
  TStateReg uHt = _GetIOStateReg(REG_stHeater);
  if (true == bForce || uHt != m_State.stLastHeaterSt) {
    m_State.stLastHeaterSt = uHt;
    int iSat = (STATE_TRUE == uHt) ? MF_ICON_SAT_ON : MF_ICON_SAT_OFF;
    GUI_DrawPicture(CSG_WINATLAS, MF_ICON_HEAT_X, MF_ICON_HEAT_Y,
                    MF_ICON_HEAT_IDX, iSat);
  }

  // Fan — on/off from IO state
  TStateReg uFn = _GetIOStateReg(REG_stFan);
  if (true == bForce || uFn != m_State.stLastFanSt) {
    m_State.stLastFanSt = uFn;
    int iSat = (STATE_TRUE == uFn) ? MF_ICON_SAT_ON : MF_ICON_SAT_OFF;
    GUI_DrawPicture(CSG_WINATLAS, MF_ICON_FAN_X, MF_ICON_FAN_Y,
                    MF_ICON_FAN_IDX, iSat);
  }
}

//=============================================================================
// Drawing — Layer1: status text (coil state)
//   Over CSG image background → GUI_TEXTMODE_TRANS, no explicit erase.
//=============================================================================

// static uint32_t _CoilToStrId(int iCoil)
// {
//   switch (iCoil) {
//   case COIL_Passby:   return idMainStat06;
//   case COIL_KeepOff:  return idMainStat05;
//   case COIL_KeepOn:   return idMainStat04;
//   default:            return 0;
//   }
// }

// static void _UpdateStatText(bool bForce)
// {
//   TDevStateReg iCoil  = GetCoilState(0);
//   TStateReg uPassby = _GetIOStateReg(REG_stPassby);

//   if (bForce || iCoil != m_State.stLastCoil || uPassby != m_State.stLastPassbySt) {
//     m_State.stLastCoil     = iCoil;
//     m_State.stLastPassbySt = uPassby;

//     GUI_COLOR cr = (uPassby == STATE_TRUE) ? MF_STAT_COLOR_ON : MF_STAT_COLOR_OFF;

//     uint32_t uStrId = _CoilToStrId(iCoil);
//     const char* pStr = (uStrId != 0) ? GetMultiLangString(uStrId) : "";
//     if (nullptr == pStr) pStr = "";

//     // Fill background, then draw text
//     GUI_SetColor(MF_STAT_BG);
//     GUI_FillRect(MF_STAT_X, MF_STAT_Y,
//                  MF_STAT_X + MF_STAT_W - 1, MF_STAT_Y + MF_STAT_H - 1);

//     GUI_RECT r;
//     _MakeRect(&r, MF_STAT_X, MF_STAT_Y, MF_STAT_W, MF_STAT_H);
//     GUI_SetFont(MF_STAT_FONT);
//     GUI_SetColor(cr);
//     GUI_SetTextMode(GUI_TEXTMODE_TRANS);
//     GUI_DispStringInRect(pStr, &r, MF_STAT_ALIGN);
//   }
// }

//=============================================================================
// Drawing — Layer1: date / time
//   Over CSG image background → GUI_TEXTMODE_TRANS.
//   Left-aligned; trailing space in format string clears old rightmost chars.
//=============================================================================

// ---- Format date/time string "yyyy-MM-dd HH:mm" (MCU: RTC, Sim: GetLocalTime) ----
// Returns true if minute changed (or bForce); caches ucLastMinute to avoid redraws
static bool _GetClockStr(bool bForce, char* pBuf, size_t nLen)
{
#ifndef __vmSIMULATOR__
  TDateTimeType dt;
  RTC_GetTime(&dt);
  
  if( bForce || m_State.ucLastMinute != dt.Minutes )
    {
    snprintf(pBuf, nLen, "%04u-%02u-%02u %02u:%02u",
             dt.Year, dt.Month, dt.Day, dt.Hours, dt.Minutes);
      
    m_State.ucLastMinute = dt.Minutes;
      
    return true;
    }
#else
  SYSTEMTIME st;
  GetLocalTime(&st);

  if( bForce || m_State.ucLastMinute != st.wMinute )
    {
    snprintf(pBuf, nLen, "%04u-%02u-%02u %02u:%02u",
           st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);

    m_State.ucLastMinute = st.wMinute;
      
    return true;
    }
#endif
    
  return false;
}

// ---- Redraw clock display if minute changed (single-byte compare, not strcmp) ----
static void _UpdateClock(bool bForce)
{
  
  // Wey. 2026.6.30
  // 优化AI代码：
  // 1.TMainFormState只保存minute，只有它变，时间才会变; 
  // 2.减少存贮空间占用
  // 3.降低比较MCU负荷
  // 3.提高效率
  
  char szNew[20];
  //_GetClockStr(szNew, sizeof(szNew));

  if (true == _GetClockStr(bForce, szNew, sizeof(szNew)) ) {//bForce || 0 != strcmp(szNew, m_State.szClock)) {
    //strncpy(m_State.szClock, szNew, sizeof(m_State.szClock) - 1);

    // Fill background, then draw text
    GUI_SetColor(MF_CLOCK_BG);
    GUI_FillRect(MF_CLOCK_X, MF_CLOCK_Y,
                 MF_CLOCK_X + MF_CLOCK_W - 1, MF_CLOCK_Y + MF_CLOCK_H - 1);

    GUI_RECT r;
    _MakeRect(&r, MF_CLOCK_X, MF_CLOCK_Y, MF_CLOCK_W, MF_CLOCK_H);
    GUI_SetFont(MF_CLOCK_FONT);
    GUI_SetColor(MF_CLOCK_COLOR);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);
    GUI_DispStringInRect(szNew, &r, MF_CLOCK_ALIGN);
  }
}

//=============================================================================
// Update scheduler
//=============================================================================

// ---- Tick scheduler: dispatch to sub-updaters at their configured intervals ----
static void _UpdateAll(bool bForce)
{
  
//#ifdef __vmSIMULATOR__
  static auto fLevle = 5.5f, fTemp = -38.5f;
  // Debug only: inject fake measurement values for Sim visual development
  _SetRealReg(REG_RL_Uin,  228.12);
  _SetRealReg(REG_RL_Uout, 229.87);
  _SetRealReg(REG_RL_BCHRG_Ibus, m_State.rChargeI);
  _SetRealReg(REG_RL_BTOUT_Ibus, m_State.rDischargeI);
  //#endif
  
  uint32_t uNow = GUI_GetTime();

  // Update clock at 1 s interval (or bForce)
  if (bForce || uNow - m_State.uLastClockTick >= UPDATE_CLOCK_MS) {
    m_State.uLastClockTick = uNow;
    _UpdateClock(bForce);
  }
  
  // Update measurement values at same interval (1 s) to keep highlight animation in sync
  if (bForce || uNow - m_State.uLastMeasTick >= UPDATE_MEAS_MS) {
    m_State.uLastMeasTick = uNow;
    _UpdateMeasValues(bForce);
  }
  
  // Update temperature + battery status values at same interval (1 s) to keep highlight animation in sync
  if (bForce || uNow - m_State.uLastStatTick >= UPDATE_STAT_MS) {
    // Debug only
    _SetRealReg(REG_RL_BAT_CAPLevel, fLevle); 
    static auto iChrgInc = 1.5f;
    fLevle += iChrgInc; 
    if (fLevle > 100) {
        iChrgInc = -1.5f;

        m_State.rChargeI    = 9.26;
        m_State.rDischargeI = 33.98;
    } else if (fLevle < 5.0f) {
       iChrgInc = 1.5f;

       m_State.rChargeI    = 7.28;
       m_State.rDischargeI = 1.54;
    }

    _SetRealReg(REG_RL_BAT_TEMPERATRUE, fTemp);
    fTemp += 0.5;
    if (fTemp > 45) {
       fTemp -= 83;
    }

    m_State.uLastStatTick = uNow;
    _UpdateStatValues(bForce);
    }

  // Update wires + icons at same interval (1 s) to keep highlight animation in sync
  if (bForce || uNow - m_State.uLastGraphicTick >= UPDATE_GRAPHIC_MS) {
    m_State.uLastGraphicTick = uNow;
    _UpdateWires(bForce);
    _UpdateIcons(bForce);
  }
  
  // if (bForce || uNow - m_State.uLastTextTick >= UPDATE_TEXT_MS) {
  //   m_State.uLastTextTick = uNow;
  //   _UpdateStatText(bForce);
  // }
}

//=============================================================================
// Form lifecycle
//=============================================================================

// ---- GWinForm: initialise form state ----
static void _Init(const void* argument)
{
  (void)argument;
  memset(&m_State, 0, sizeof(m_State));
  m_State.uWireHLPct = 20.0f;

  // Debug
  m_State.rChargeI = 9.76;
  m_State.rDischargeI = 2.18;
}

// ---- GWinForm: draw Layer0 + all Layer1 elements (bForce = true) ----
static void _Show(const void* argument)
{
  (void)argument;

  //GUI_SetBkColor(GUI_BLACK);
  //GUI_Clear();

  // Draw Layer0 once to screen
  _DrawLayer0();

  // Draw all Layer1 elements on top
  _UpdateAll(true);
}

// ---- GWinForm: release resources (nothing to free currently) ----
static void _Close(const void* argument)
{
  (void)argument;
}

// ---- GWinForm GM_TIMER_TICK handler: incremental update (bForce = false) ----
static void _OnTick(uint32_t uTick)
{
  (void)uTick;
  _UpdateAll(false);
}

// ---- Keyboard handler: Enter→MenuForm, Esc→SplashForm ----
static void _OnKeyUp(uint16_t uwKey)
{
  switch (uwKey) {
  case KEY_ENTER:  gfc::PushForm(WID_MenuForm, nullptr);    break;
  case KEY_ESCAPE: gfc::ReplaceForm(WID_SplashForm, nullptr); break;
  default: break;
  }
}

// ---- Touch handler: any tap → MenuForm ----
static void _OnTouch(uint16_t action, uint16_t x, uint16_t y)
{
  if (action != TOUCH_UP) {
     return;
  }

  // Menu button (2.3.3) — 20×20 icon at (286,218)
  if (x >= MF_BTN_MENU_X && x < MF_BTN_MENU_X + MF_BTN_MENU_W &&
      y >= MF_BTN_MENU_Y && y < MF_BTN_MENU_Y + MF_BTN_MENU_H) {
    gfc::PushForm(WID_MenuForm, nullptr);
  }
}

// ---- GWinForm message dispatch: tick / key / touch → handler ----
static void _OnMessage(GM_MESSAGE* pMsg)
{
  if (nullptr == pMsg) return;

  switch (pMsg->MsgId) {
  case GM_TIMER_TICK:
    _OnTick(static_cast<uint32_t>(pMsg->Data.v));
    break;
  case GM_KEYUP:
    if (pMsg->Param) { _OnKeyUp(pMsg->Param); pMsg->MsgId = 0; }
    break;
#if GUI_SUPPORT_TOUCH
  case GM_TOUCH: {
    uint16_t x = static_cast<uint16_t>((pMsg->Data.v >> 16) & 0xFFFF);
    uint16_t y = static_cast<uint16_t>(pMsg->Data.v & 0xFFFF);
    _OnTouch(pMsg->Param, x, y);
    pMsg->MsgId = 0;
    break;
  }
#endif
  default: break;
  }
}

//=============================================================================
// Form descriptor
//=============================================================================
const GWinForm FMainForm = { _Init, _Show, _Close, _OnMessage };

static const gfc::FormRegistrar kRegMain(WID_MainForm, &FMainForm, "Main");
//-----------------------------------------------------------------------------

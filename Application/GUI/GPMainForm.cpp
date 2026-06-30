//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GPMainForm.cpp
 Version     : V2.02
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
     - CSG image background areas do NOT attempt pixel-perfect restoration;
       transparent text mode plus the fixed-width region minimises artifacts.

   Update scheduling (spec II.2):
     - Date/time:   every 60 s
     - Measurement: every 10 s (or immediate on value change)
     - Status data: every 10 s (or immediate on value change)
     - Graphics:    every  1 s (IO-state driven, or +/-10 % deadband)
     - Status text: every  1 s

 Date        : 2026.06.29 (V2.02 — removed full-screen memdev; TRANS-based Layer1)
              2026.06.29 (V2.01 — GUI_MEMDEV for Layer0/Layer1 isolation)
              2026.06.29 (V2.00 — rewritten per form design spec)
*/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include "GPMainForm.h"

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

//=============================================================================
// Update intervals (ms) — per spec II.2.1
//=============================================================================
#define UPDATE_CLOCK_MS     60000
#define UPDATE_MEAS_MS      10000
#define UPDATE_STAT_MS      10000
#define UPDATE_GRAPHIC_MS   1000
#define UPDATE_TEXT_MS      1000

//=============================================================================
// Layout — Layer0 — per spec II.2.3.1
// Coordinate format (x, y, w, h).  emWin calls use (x, y, x+w-1, y+h-1).
//=============================================================================

// Background
#define MF_BKG_X            0
#define MF_BKG_Y            0

// Caption back panel (71 px tall, solid colour #041736)
#define MF_CAP_X            0
#define MF_CAP_Y            0
#define MF_CAP_W            320
#define MF_CAP_H            63
#define MF_CAP_COLOR        0x041736

// Caption divider line
#define MF_SEP_X            0
#define MF_SEP_Y            64
#define MF_SEP_W            320
#define MF_SEP_H            1
#define MF_SEP_COLOR        0x2F5CA6

//// Logo
//#define MF_LOGO_X           121
//#define MF_LOGO_Y           220
//#define MF_LOGO_IDX         picIdxMA_Logo78x18

// Measurement data labels (4 × 24 px font) — static, drawn once in Layer0
#define MF_LBL_UIN_X        10
#define MF_LBL_UIN_Y        5
#define MF_LBL_UIN_W        60
#define MF_LBL_UIN_H        24

#define MF_LBL_UOUT_X       170
#define MF_LBL_UOUT_Y       5
#define MF_LBL_UOUT_W       60
#define MF_LBL_UOUT_H       24

#define MF_LBL_CHG_X        10
#define MF_LBL_CHG_Y        33
#define MF_LBL_CHG_W        60
#define MF_LBL_CHG_H        24

#define MF_LBL_DIS_X        170
#define MF_LBL_DIS_Y        33
#define MF_LBL_DIS_W        60
#define MF_LBL_DIS_H        24

#define MF_LBL24_FONT       GUI_FONT_24LTH_CHN
#define MF_LBL24_COLOR      0xECECEC
#define MF_LBL24_ALIGN      (GUI_TA_LEFT | GUI_TA_VCENTER)

// Status data labels (2 × 16 px font) — static
#define MF_LBL_TMP_X        10
#define MF_LBL_TMP_Y        170
#define MF_LBL_TMP_W        40
#define MF_LBL_TMP_H        17

#define MF_LBL_BAT_X        10
#define MF_LBL_BAT_Y        190
#define MF_LBL_BAT_W        40
#define MF_LBL_BAT_H        17

#define MF_LBL16_FONT       GUI_FONT_16LTH_CHN
#define MF_LBL16_COLOR      0xECECEC
#define MF_LBL16_ALIGN      (GUI_TA_LEFT | GUI_TA_VCENTER)

// Controller area (rounded rect border, no fill)
#define MF_CTRL_X           110
#define MF_CTRL_Y           75
#define MF_CTRL_W           100
#define MF_CTRL_H           130
#define MF_CTRL_RADIUS      10
#define MF_CTRL_COLOR       0x1485B3

//=============================================================================
// Layout — Layer1 — per spec II.2.3.2
//=============================================================================

// Measurement values (4 × 24 px font) — solid background #031635
#define MF_VAL_UIN_X        70
#define MF_VAL_UIN_Y        9
#define MF_VAL_UIN_W        80
#define MF_VAL_UIN_H        24
#define MF_VAL_BG           0x031635     // value background colour

#define MF_VAL_UOUT_X       230
#define MF_VAL_UOUT_Y       9
#define MF_VAL_UOUT_W       80
#define MF_VAL_UOUT_H       24

#define MF_VAL_CHG_X        70
#define MF_VAL_CHG_Y        37
#define MF_VAL_CHG_W        80
#define MF_VAL_CHG_H        24

#define MF_VAL_DIS_X        230
#define MF_VAL_DIS_Y        37
#define MF_VAL_DIS_W        80
#define MF_VAL_DIS_H        24

#define MF_VAL24_FONT       GUI_FONT_24LTH_CHN
#define MF_VAL24_COLOR      0xECECEC
#define MF_VAL24_ALIGN      (GUI_TA_RIGHT | GUI_TA_VCENTER)

// Status values (2 × 16 px font) — over CSG image background
//   Use GUI_TEXTMODE_TRANS: new text pixels overwrite old ones.
#define MF_VAL_TMP_X        50
#define MF_VAL_TMP_Y        172
#define MF_VAL_TMP_W        56
#define MF_VAL_TMP_H        16

#define MF_VAL_BAT_X        50
#define MF_VAL_BAT_Y        192
#define MF_VAL_BAT_W        56
#define MF_VAL_BAT_H        16

#define MF_VAL16_FONT       GUI_FONT_16LTH_CHN
#define MF_VAL16_ALIGN      (GUI_TA_RIGHT | GUI_TA_VCENTER)

// Status connection wires (3 thin rectangles)
#define MF_WIRE_IN_X        62
#define MF_WIRE_IN_Y        120
#define MF_WIRE_IN_W        60
#define MF_WIRE_IN_H        4

#define MF_WIRE_OUT_X       199
#define MF_WIRE_OUT_Y       120
#define MF_WIRE_OUT_W       51
#define MF_WIRE_OUT_H       4

#define MF_WIRE_BAT_X       158
#define MF_WIRE_BAT_Y       146
#define MF_WIRE_BAT_W       4
#define MF_WIRE_BAT_H       26

#define MF_WIRE_GRAY        0x526670
#define MF_WIRE_GRAD_C      0x00B5F2
#define MF_WIRE_GRAD_M      0xF0F0F0

// Status icons (6 atlas sub-pictures)
#define MF_ICON_AC_X        30
#define MF_ICON_AC_Y        92
#define MF_ICON_AC_IDX      picIdxMA_ACPow32x59Cyan

#define MF_ICON_CTRL_X      121
#define MF_ICON_CTRL_Y      85
#define MF_ICON_CTRL_IDX    picIdxMA_Ctrl78x61Cyan

#define MF_ICON_BRKR_X      250
#define MF_ICON_BRKR_Y      91
#define MF_ICON_BRKR_IDX    picIdxMA_Brkr56x60Cyan

#define MF_ICON_BAT_X       138
#define MF_ICON_BAT_Y       172

#define MF_ICON_HEAT_X      270
#define MF_ICON_HEAT_Y      219
#define MF_ICON_HEAT_IDX    picIdxMA_Fire16x16

#define MF_ICON_FAN_X       293
#define MF_ICON_FAN_Y       220
#define MF_ICON_FAN_IDX     picIdxMA_Fan16x16Cyan

#define MF_ICON_SAT_ON      100
#define MF_ICON_SAT_OFF     10

// Status text (bypass / coil state)
#define MF_STAT_X           216
#define MF_STAT_Y           160
#define MF_STAT_W           100
#define MF_STAT_H           25
#define MF_STAT_FONT        GUI_FONT_24LTH_CHN
#define MF_STAT_ALIGN       (GUI_TA_HCENTER | GUI_TA_VCENTER)
#define MF_STAT_COLOR_ON    0x25B92C
#define MF_STAT_COLOR_OFF   0xF06D0D
#define MF_STAT_BG          0x001838     // status value background

// Date / time
#define MF_CLOCK_X          4
#define MF_CLOCK_Y          220
#define MF_CLOCK_W          116
#define MF_CLOCK_H          16
#define MF_CLOCK_FONT       GUI_FONT_AA4_ASCII16B

#define MF_CLOCK_COLOR      0xBDBDBD
#define MF_CLOCK_ALIGN      (GUI_TA_LEFT | GUI_TA_VCENTER)
#define MF_CLOCK_BG         0x001028     // clock background

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
#define MF_TEMP_LOW         0.0f
#define MF_TEMP_COLOR_HI    0xF06D0D
#define MF_TEMP_COLOR_OK    0x16E83D

#define MF_BAT_HIGH         80.0f
#define MF_BAT_MID          50.0f
#define MF_BAT_COLOR_HI     0x16E83D
#define MF_BAT_COLOR_MID    0x00B5F2
#define MF_BAT_COLOR_LO     0xF06D0D

static void _DrawWireBat(void);
    //=============================================================================
// Battery icon index lookup — per spec II.2.3.2 item 4
//=============================================================================
static int _GetBatteryIconIdx(float rLevel)
{
  if (rLevel > 90.0f) return picIdxMA_Battey44x24C5;
  if (rLevel > 70.0f) return picIdxMA_Battey44x24C4;
  if (rLevel > 50.0f) return picIdxMA_Battey44x24C3;
  if (rLevel > 30.0f) return picIdxMA_Battey44x24C2;
  return picIdxMA_Battey44x24C1;
}

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
  int      iLastBatSat;
  int      iLastAcSat;
  uint32_t uLastHeaterSt;
  uint32_t uLastFanSt;

  int      iLastCoil;
  uint32_t uLastPassbySt;

  //char     szClock[32];
  uint8_t  ucLastMinute = 0; //

  enum emBattery {
      batIdel = 0,
      batCharge = 10,
      batDischarge = 20
  } stBattery = batIdel;

} TMainFormState;

static TMainFormState m_State;

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
// Numeric formatting
//=============================================================================
static void _FmtVoltage(char* pBuf, size_t n, float rVal)
{
  snprintf(pBuf, n, "%0.1fV", rVal);
}
static void _FmtCurrent(char* pBuf, size_t n, float rVal)
{
  snprintf(pBuf, n, "%0.2fA", rVal);
}
static void _FmtTemp(char* pBuf, size_t n, float rVal)
{
  auto szFmt = GetMultiLangString(idMainFmSt04);
  if (szFmt )
    snprintf(pBuf, n, szFmt, rVal);
}
static void _FmtBattery(char* pBuf, size_t n, float rVal)
{
  snprintf(pBuf, n, "%0.1f%%", rVal);
}

//=============================================================================
// Drawing — Layer0 (drawn once to screen in _Show)
//=============================================================================

static void _DrawLayer0(void)
{
  // Background image — full opacity
  GUI_DrawPicture(&picbkg320x240Lcsg, MF_BKG_X, MF_BKG_Y, 0, 100);

  // --- Caption back panel (20% opacity on top of CSG background) ---
  GUI_EnableAlpha(1);
  GUI_SetAlpha(MF_CAP_ALPHA);
  GUI_SetColor(MF_CAP_COLOR);
  GUI_FillRect(MF_CAP_X, MF_CAP_Y,
               MF_CAP_X + MF_CAP_W - 1, MF_CAP_Y + MF_CAP_H - 1);

  // Caption divider (15% opacity)
  GUI_SetAlpha(MF_SEP_ALPHA);
  GUI_SetColor(MF_SEP_COLOR);
  GUI_FillRect(MF_SEP_X, MF_SEP_Y,
               MF_SEP_X + MF_SEP_W - 1, MF_SEP_Y + MF_SEP_H - 1);
  GUI_EnableAlpha(0);

#ifdef MF_LOGO_IDX
  // Logo — full opacity
  GUI_DrawPicture(&picMAUAtlascsg, MF_LOGO_X, MF_LOGO_Y, MF_LOGO_IDX, 100);
#endif

  // --- Controller area — rounded rect border only, no fill ---
  GUI_SetColor(MF_CTRL_COLOR);
  GUI_SetPenSize(1);
  GUI_DrawRoundedRect(MF_CTRL_X, MF_CTRL_Y,
                      MF_CTRL_X + MF_CTRL_W - 1,
                      MF_CTRL_Y + MF_CTRL_H - 1, MF_CTRL_RADIUS);

  // Static measurement labels — full opacity
  GUI_SetFont(MF_LBL24_FONT);
  GUI_SetColor(MF_LBL24_COLOR);
  GUI_SetTextMode(GUI_TEXTMODE_TRANS);

  const char* pStr;
  GUI_RECT    r;
  int a24 = MF_LBL24_ALIGN;

  #define DR_LBL(id, x,y,w,h) do { \
    pStr = GetMultiLangString(id);  \
    if (nullptr != pStr) { _MakeRect(&r,x,y,w,h); GUI_DispStringInRect(pStr,&r,a24); } \
  } while(0)

  DR_LBL(idMainLabel01, MF_LBL_UIN_X,  MF_LBL_UIN_Y,  MF_LBL_UIN_W,  MF_LBL_UIN_H);
  DR_LBL(idMainLabel02, MF_LBL_UOUT_X, MF_LBL_UOUT_Y, MF_LBL_UOUT_W, MF_LBL_UOUT_H);
  DR_LBL(idMainLabel03, MF_LBL_CHG_X,  MF_LBL_CHG_Y,  MF_LBL_CHG_W,  MF_LBL_CHG_H);
  DR_LBL(idMainLabel04, MF_LBL_DIS_X,  MF_LBL_DIS_Y,  MF_LBL_DIS_W,  MF_LBL_DIS_H);

  // Static status labels
  GUI_SetFont(MF_LBL16_FONT);
  GUI_SetColor(MF_LBL16_COLOR);
  int a16 = MF_LBL16_ALIGN;

  DR_LBL(idMainLabel05, MF_LBL_TMP_X, MF_LBL_TMP_Y, MF_LBL_TMP_W, MF_LBL_TMP_H);
  DR_LBL(idMainLabel06, MF_LBL_BAT_X, MF_LBL_BAT_Y, MF_LBL_BAT_W, MF_LBL_BAT_H);
  #undef DR_LBL
}

//=============================================================================
// Drawing — Layer1: measurement values
//   These are inside the Caption bar (solid background → fill-then-draw)
//=============================================================================

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
  if( 0.5f < rChgI && 0.5f < rDisI )
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

  if (bForce || rChgI != m_State.rLastChgI) {
    m_State.rLastChgI = rChgI;
    _FmtCurrent(szBuf, sizeof(szBuf), rChgI);
    _DrawValueSolid(szBuf, MF_VAL24_FONT,
                    MF_VAL_CHG_X, MF_VAL_CHG_Y, MF_VAL_CHG_W, MF_VAL_CHG_H,
                    MF_VAL24_COLOR, MF_VAL24_ALIGN);
  }

  if (bForce || rDisI != m_State.rLastDisI) {
    m_State.rLastDisI = rDisI;
    _FmtCurrent(szBuf, sizeof(szBuf), rDisI);
    _DrawValueSolid(szBuf, MF_VAL24_FONT,
                    MF_VAL_DIS_X, MF_VAL_DIS_Y, MF_VAL_DIS_W, MF_VAL_DIS_H,
                    MF_VAL24_COLOR, MF_VAL24_ALIGN);
  }
}

//=============================================================================
// Drawing — Layer1: status values
//   Over CSG image background → GUI_TEXTMODE_TRANS, no explicit erase.
//   Right-aligned + fixed-width region keeps old-pixel-bleed minimal.
//=============================================================================

static GUI_COLOR _TempColor(float rTemp)
{
  if (rTemp > MF_TEMP_HIGH) return MF_TEMP_COLOR_HI;
  if (rTemp > MF_TEMP_LOW)  return MF_TEMP_COLOR_OK;
  return MF_TEMP_COLOR_HI;
}

static GUI_COLOR _BatColor(float rLevel)
{
  if (rLevel > MF_BAT_HIGH) return MF_BAT_COLOR_HI;
  if (rLevel > MF_BAT_MID)  return MF_BAT_COLOR_MID;
  return MF_BAT_COLOR_LO;
}

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

static void _UpdateStatValues(bool bForce)
{
  char  szBuf[16];
  float rVal;

  rVal = _GetRealReg(REG_RL_RTC_TEMP);
  if (bForce || rVal != m_State.rLastTemp) {
    m_State.rLastTemp = rVal;
    _FmtTemp(szBuf, sizeof(szBuf), rVal);
    _DrawValueTrans(szBuf, MF_VAL16_FONT,
                    MF_VAL_TMP_X, MF_VAL_TMP_Y, MF_VAL_TMP_W, MF_VAL_TMP_H,
                    _TempColor(rVal), MF_VAL16_ALIGN, MF_STAT_BG);
  }

  rVal = _GetRealReg(REG_RL_BCHRG_Level);
  if (bForce || rVal != m_State.rLastBatLevel) {
    m_State.rLastBatLevel = rVal;
    _FmtBattery(szBuf, sizeof(szBuf), rVal);
    _DrawValueTrans(szBuf, MF_VAL16_FONT,
                    MF_VAL_BAT_X, MF_VAL_BAT_Y, MF_VAL_BAT_W, MF_VAL_BAT_H,
                    _BatColor(rVal), MF_VAL16_ALIGN, MF_STAT_BG);
  }
}

//=============================================================================
// Drawing — Layer1: status connection wires
//   These are opaque rectangles; drawn directly over whatever is underneath.
//=============================================================================

static void _DrawWireH(int x, int y, int w, int h, float rVolt)
{
  //if (rVolt < MF_VOLT_THRESHOLD) {
  //  GUI_SetColor(MF_WIRE_GRAY);
  //  GUI_FillRect(x, y, x + w - 1, y + h - 1);
  //} else {
    {
    GUI_SetColor(MF_WIRE_GRAD_C);
    GUI_FillRect(x, y, x + w - 1, y + h - 1);

    int cx = x + w / 2;
    GUI_DrawGradientH(cx - 9, y, cx, y + h - 1,
                      MF_WIRE_GRAD_C, MF_WIRE_GRAD_M);
    GUI_DrawGradientH(cx + 1, y, cx + 10, y + h - 1,
        MF_WIRE_GRAD_M, MF_WIRE_GRAD_C);
    // GUI_DrawGradientH(x, y, x + w - 1, y + h - 1,
    //    MF_WIRE_GRAD_C, MF_WIRE_GRAD_M);
  }
}

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
  GUI_DrawGradientV(x, cy - 10, x + w - 1, cy - 1,
      crStart, crEnd);
  GUI_DrawGradientV(x, cy, x + w - 1, cy + 9,
      crStart, crEnd);
  // GUI_DrawGradientV(x, y, x + w - 1, y + h - 1,
  //    MF_WIRE_GRAD_C, MF_WIRE_GRAD_M);
  }
}

static void _DrawWireBat( void )
{

  _DrawWireBat(MF_WIRE_BAT_X, MF_WIRE_BAT_Y, MF_WIRE_BAT_W, MF_WIRE_BAT_H);
}

static void _UpdateWires(bool bForce)
{
  float rUin  = _GetRealReg(REG_RL_Uin);
  float rUout = _GetRealReg(REG_RL_Uout);

  if (bForce || _DeadbandCrossed(m_State.rLastWireUin, rUin, MF_VOLT_THRESHOLD)) {
    m_State.rLastWireUin = rUin;
    _DrawWireH(MF_WIRE_IN_X, MF_WIRE_IN_Y, MF_WIRE_IN_W, MF_WIRE_IN_H, rUin);
  }

  if (bForce || _DeadbandCrossed(m_State.rLastWireUout, rUout, MF_VOLT_THRESHOLD)) {
    m_State.rLastWireUout = rUout;
    _DrawWireH(MF_WIRE_OUT_X, MF_WIRE_OUT_Y, MF_WIRE_OUT_W, MF_WIRE_OUT_H, rUout);
  }

  if (bForce) {
    _DrawWireBat();
  }
}

//=============================================================================
// Drawing — Layer1: status icons
//   GUI_DrawPicture draws opaque pixels; saturation handles dim/bright.
//=============================================================================

static void _UpdateIcons(bool bForce)
{
  float rUin = _GetRealReg(REG_RL_Uin);
  float rBat = _GetRealReg(REG_RL_BCHRG_Level);

  // AC input — saturation depends on voltage
  int iAcSat = (rUin > MF_VOLT_THRESHOLD) ? MF_ICON_SAT_ON : MF_ICON_SAT_OFF;
  if (bForce || iAcSat != m_State.iLastAcSat) {
    m_State.iLastAcSat = iAcSat;
    GUI_DrawPicture(&picMAUAtlascsg, MF_ICON_AC_X, MF_ICON_AC_Y,
                    MF_ICON_AC_IDX, iAcSat);
  }

  // Controller — always 100 %
  if (bForce)
    GUI_DrawPicture(&picMAUAtlascsg, MF_ICON_CTRL_X, MF_ICON_CTRL_Y,
                    MF_ICON_CTRL_IDX, 100);

  // Breaker — always 100 %
  if (bForce)
    GUI_DrawPicture(&picMAUAtlascsg, MF_ICON_BRKR_X, MF_ICON_BRKR_Y,
                    MF_ICON_BRKR_IDX, 100);

  // Battery — index + saturation from charge level
  int iBatIdx = _GetBatteryIconIdx(rBat);
  int iBatSat = (rBat > 10.0f) ? MF_ICON_SAT_ON : MF_ICON_SAT_OFF;
  if (bForce || iBatIdx != m_State.iLastBatIconIdx || iBatSat != m_State.iLastBatSat) {
    m_State.iLastBatIconIdx = iBatIdx;
    m_State.iLastBatSat     = iBatSat;
    GUI_DrawPicture(&picMAUAtlascsg, MF_ICON_BAT_X, MF_ICON_BAT_Y, iBatIdx, iBatSat);
  }

  // Heater — on/off from IO state
  uint32_t uHt = _GetIOStateReg(REG_stHeater);
  if (bForce || uHt != m_State.uLastHeaterSt) {
    m_State.uLastHeaterSt = uHt;
    int iSat = (uHt == STATE_TRUE) ? MF_ICON_SAT_ON : MF_ICON_SAT_OFF;
    GUI_DrawPicture(&picMAUAtlascsg, MF_ICON_HEAT_X, MF_ICON_HEAT_Y,
                    MF_ICON_HEAT_IDX, iSat);
  }

  // Fan — on/off from IO state
  uint32_t uFn = _GetIOStateReg(REG_stFan);
  if (bForce || uFn != m_State.uLastFanSt) {
    m_State.uLastFanSt = uFn;
    int iSat = (uFn == STATE_TRUE) ? MF_ICON_SAT_ON : MF_ICON_SAT_OFF;
    GUI_DrawPicture(&picMAUAtlascsg, MF_ICON_FAN_X, MF_ICON_FAN_Y,
                    MF_ICON_FAN_IDX, iSat);
  }
}

//=============================================================================
// Drawing — Layer1: status text (coil state)
//   Over CSG image background → GUI_TEXTMODE_TRANS, no explicit erase.
//=============================================================================

static uint32_t _CoilToStrId(int iCoil)
{
  switch (iCoil) {
  case COIL_Passby:   return idMainStat06;
  case COIL_KeepOff:  return idMainStat05;
  case COIL_KeepOn:   return idMainStat04;
  default:            return 0;
  }
}

static void _UpdateStatText(bool bForce)
{
  int      iCoil  = GetCoilState(0);
  uint32_t uPassby = _GetIOStateReg(REG_stPassby);

  if (bForce || iCoil != m_State.iLastCoil || uPassby != m_State.uLastPassbySt) {
    m_State.iLastCoil     = iCoil;
    m_State.uLastPassbySt = uPassby;

    GUI_COLOR cr = (uPassby == STATE_TRUE) ? MF_STAT_COLOR_ON : MF_STAT_COLOR_OFF;

    uint32_t uStrId = _CoilToStrId(iCoil);
    const char* pStr = (uStrId != 0) ? GetMultiLangString(uStrId) : "";
    if (nullptr == pStr) pStr = "";

    // Fill background, then draw text
    GUI_SetColor(MF_STAT_BG);
    GUI_FillRect(MF_STAT_X, MF_STAT_Y,
                 MF_STAT_X + MF_STAT_W - 1, MF_STAT_Y + MF_STAT_H - 1);

    GUI_RECT r;
    _MakeRect(&r, MF_STAT_X, MF_STAT_Y, MF_STAT_W, MF_STAT_H);
    GUI_SetFont(MF_STAT_FONT);
    GUI_SetColor(cr);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);
    GUI_DispStringInRect(pStr, &r, MF_STAT_ALIGN);
  }
}

//=============================================================================
// Drawing — Layer1: date / time
//   Over CSG image background → GUI_TEXTMODE_TRANS.
//   Left-aligned; trailing space in format string clears old rightmost chars.
//=============================================================================

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
      
    m_State.ucLastMinute = dt.Minutes;
      
    return true;
    }
#endif
    
  return false;
}

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

static void _UpdateAll(bool bForce)
{
  uint32_t uNow = GUI_GetTime();

  if (bForce || uNow - m_State.uLastClockTick >= UPDATE_CLOCK_MS) {
    m_State.uLastClockTick = uNow;
    _UpdateClock(bForce);
  }
  if (bForce || uNow - m_State.uLastMeasTick >= UPDATE_MEAS_MS) {
    m_State.uLastMeasTick = uNow;
    _UpdateMeasValues(bForce);
  }
  if (bForce || uNow - m_State.uLastStatTick >= UPDATE_STAT_MS) {
    m_State.uLastStatTick = uNow;
    _UpdateStatValues(bForce);
  }
  if (bForce || uNow - m_State.uLastGraphicTick >= UPDATE_GRAPHIC_MS) {
    m_State.uLastGraphicTick = uNow;
    _UpdateWires(bForce);
    _UpdateIcons(bForce);
  }
  if (bForce || uNow - m_State.uLastTextTick >= UPDATE_TEXT_MS) {
    m_State.uLastTextTick = uNow;
    _UpdateStatText(bForce);
  }
}

//=============================================================================
// Form lifecycle
//=============================================================================

static void _Init(const void* argument)
{
  (void)argument;
  memset(&m_State, 0, sizeof(m_State));
}

static void _Show(const void* argument)
{
  (void)argument;

  GUI_SetBkColor(GUI_BLACK);
  GUI_Clear();

  // Draw Layer0 once to screen
  _DrawLayer0();

  // Draw all Layer1 elements on top
  _UpdateAll(true);
}

static void _Close(const void* argument)
{
  (void)argument;
}

static void _OnTick(uint32_t uTick)
{
  (void)uTick;
  _UpdateAll(false);
}

static void _OnKeyUp(uint16_t uwKey)
{
  switch (uwKey) {
  case KEY_ENTER:  gfc::PushForm(WID_MenuForm, nullptr);    break;
  case KEY_ESCAPE: gfc::ReplaceForm(WID_SplashForm, nullptr); break;
  default: break;
  }
}

static void _OnTouch(uint16_t action, uint16_t x, uint16_t y)
{
  (void)x; (void)y;
  if (action == TOUCH_UP)
    gfc::PushForm(WID_MenuForm, nullptr);
}

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

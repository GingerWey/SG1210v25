//-----------------------------------------------------------------------------
/*
 File        : GConfigForm.cpp
 Version     : V2.00
 By          : Wey. Silver Grid

 Description : Config form - parameter configuration form with full dialog
               integration for register editing.

               Display: Shows configuration registers (Logic / Device / Serial /
               Ethernet) grouped by config type. Style aligned with LogListForm
               (outer/inner frames + right scrollbar + key throttle) plus a top
               Caption showing the current config type name + icon, a Menu button
               and an Edit button. Per SG1210v25 form design spec section VI.

               Editing (Phase 6 - Dialog Integration):
                 All 8 register types fully editable with dialog support:
                 - SIT_VAT_BIN: Direct toggle (no dialog)
                 - SIT_VAT_INT/HEX/REAL: GNumRegDialog (number keyboard)
                 - SIT_VAT_ENUM: GListbox (in-place enum selection)
                 - SIT_VAT_DATETIME: GDatetimeDialog (6-field editor)
                 - SIT_VAT_PASSWORD: Permission gate → GLoginDialog → GNumRegDialog
                 - IP Address: GIPAddressDialog (4-octet editor)

                 Dialog resources from GDialogRes (extern declarations):
                 - g_numRegDialogConfig, g_loginDialogConfig
                 - g_ipDialogConfig, g_datetimeDialogConfig
                 - g_listboxStyle
                 - All with professional icons from ImageRes DLGAtlas
                 - Array counts via NUM_Elements macro (type-safe, auto-calculated)

               Behaviour:
                 - Opened from GMenuForm config items (WID_ConfigForm); init
                   argument selects the initial config type.
                 - ESC / Menu-icon tap → PopForm back to GMenuForm.
                 - UP/DOWN navigate config rows (held key auto-repeats, throttled).
                 - LEFT/RIGHT (or swipe) cycle config types:
                   cgtLogic ↔ cgtDevice ↔ cgtSerial ↔ cgtEthernet.
                   LEFT/RIGHT respond to initial keydown only (no key-repeat).
                 - ENTER / Content-area tap → trigger edit dialog for focused row.
                 - Permission gate: if register requires password, show GLoginDialog
                   first; on success, re-trigger edit with permission granted.
                 - Dialog lifecycle: create → show → accept/cancel → write/abort →
                   destroy → return to form.
                 - Static display reloaded after successful register write.

 Date        : 2026.07.21 (V2.00 — Phase 6 dialog integration complete:
               GLoginDialog, GNumRegDialog, GIPAddressDialog, GDatetimeDialog,
               GListbox; all resources from GDialogRes with icons and NUM_Elements;
               permission gate with DevRegs API; datetime write via DevIntf_setDateTime;
               enum editing via RINF_getRegEnumList; touch-to-edit support)
              2026.07.14 (V1.10 — Yoda-style for variable comparisons by scope)
              2026.07.14 (V1.09 — use RAM_Malloc/RAM_Free uniformly)
              2026.07.14 (V1.08 — Yoda-style comparisons in conditionals)
              2026.07.14 (V1.07 - 使用 NUM_Elements 宏计算数组元素个数)
              2026.07.13 (V1.06 - Caption left edge: add 4px gap from frame
              border (CF_CAT_ICON_LEFT_GAP), aligned with GLogListForm)
              2026.07.13 (V1.05 - increase visible rows to 8 (row height
              23px); ENUM types also append dimension unit)
              2026.07.13 (V1.04 - increase visible rows from 5 to 6 (row
              height reduced to 31px); change REG_FN_COIL_VOLTAGE from ENUM_ROW
              to NUM_ROW since user changed it to SIT_VAT_INT in
              DevRegInfoList.cpp)
              2026.07.13 (V1.03 - drive value display type from
              SIT_GetVType(Property) per spec 6.7.2 (removed EnumMap): BIN ->
              switch image, ENUM -> option-list text indexed by value, else
              integer + dimension)
              2026.07.13 (V1.02 - hide scrollbar thumb when items fit on
              screen: clear the scrollbar lane on short categories so a stale
              thumb from a longer category is not left behind)
              2026.07.13 (V1.01 - refine per spec 6.6-6.8 + layout: derive
              display type from SIT_GetVType; enum option tables (cbxListBool/
              Volt/BaudRate/Parity); category icon 16x16 v-centered + 2px gap;
              BIN switch image centered in value cell; value cell 4px gap from
              selection frame; 1-space before dimension; taller rows to fill
              the list area)
              2026.07.13 (V1.00 - initial implementation, Display mode only)
*/
//-----------------------------------------------------------------------------
#include "GConfigForm.h"

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
#include "DevIntf.h"
#include "DevDebug.h"
#include "RamHeap.h"

// Phase 6: Dialog integration
#include "Dialog/GDialog.h"
#include "Dialog/GDialogRes.h"
#include "Dialog/GLoginDialog.h"
#include "Dialog/GNumRegDialog.h"
#include "Dialog/GIPAddressDialog.h"
#include "Dialog/GDatetimeDialog.h"
#include "Dialog/GListbox.h"

#include <cstdio>
#include <cstring>
#include <new>
#include <GUI_Type.h>
#include <GUIConf.h>

//=============================================================================
// Layout (aligned with LogListForm)
//=============================================================================
// Outer frame / panel. Encloses caption + row table + scrollbar.
#define CF_LIST_X          15
#define CF_LIST_Y          4
#define CF_LIST_W          290
#define CF_LIST_H          232
#define CF_LIST_X1         (CF_LIST_X + CF_LIST_W - 1)   // 304
#define CF_LIST_Y1         (CF_LIST_Y + CF_LIST_H - 1)   // 235

// Caption bar (config type name) at the top of the panel
#define CF_CAP_H           28
#define CF_CAP_Y0          CF_LIST_Y                     // 4
#define CF_CAP_Y1          (CF_CAP_Y0 + CF_CAP_H - 1)    // 31
#define CF_CAP_SEP         (CF_CAP_Y1 + 1)               // 32
#define CF_CAP_ICON_W      20

// Scrollbar lane (right side of panel)
#define CF_SCRL_W          8
#define CF_SCRL_GAP        4
#define CF_SCRL_X0         (CF_LIST_X1 - CF_SCRL_W + 1)  // 297
#define CF_SCRL_X1         CF_LIST_X1                     // 304

// Inner frame (row table). Below caption; right edge before scrollbar gap.
#define CF_FRAME_GAP       4
#define CF_INNER_X         (CF_LIST_X + CF_FRAME_GAP)               // 19
#define CF_INNER_Y         (CF_CAP_SEP + 1 + CF_FRAME_GAP)          // 37
#define CF_INNER_X1        (CF_SCRL_X0 - CF_SCRL_GAP - 1)           // 292
#define CF_INNER_Y1        (CF_LIST_Y1 - CF_FRAME_GAP)              // 231

// Category icon (left of caption text): 16x16, vertically centered,
// with a 2px gap to the caption text (improvement #1),
// with a 4px gap from the left frame edge.
#define CF_CAT_ICON_W      16
#define CF_CAT_ICON_H      16
#define CF_CAT_ICON_LEFT_GAP  4
#define CF_CAT_ICON_X0     (CF_LIST_X + CF_FRAME_GAP + CF_CAT_ICON_LEFT_GAP)  // 23
#define CF_CAT_ICON_Y0     (CF_CAP_Y0 + (CF_CAP_H - CF_CAT_ICON_H) / 2)  // v-center
#define CF_CAT_TEXT_GAP    2

// Caption icons: Menu at right, Edit left of Menu.
#define CF_CAP_ICON_X0     (CF_INNER_X1 - CF_CAP_ICON_W + 1)        // menu icon x0
#define CF_CAP_ICON_X1     CF_INNER_X1
#define CF_CAP_ICON_Y0     (CF_CAP_Y0 + (CF_CAP_H - 20) / 2)        // vertical center
#define CF_EDIT_GAP        8
#define CF_EDIT_ICON_X0    (CF_CAP_ICON_X0 - CF_EDIT_GAP - CF_CAP_ICON_W)  // edit icon x0
#define CF_EDIT_ICON_X1    (CF_EDIT_ICON_X0 + CF_CAP_ICON_W - 1)

// Row content - inset from inner frame
#define CF_CONTENT_PAD     3
#define CF_CONTENT_X0      (CF_INNER_X + CF_CONTENT_PAD)            // 22
#define CF_CONTENT_Y0      (CF_INNER_Y + CF_CONTENT_PAD)            // 40
#define CF_CONTENT_X1      (CF_INNER_X1 - CF_CONTENT_PAD)           // 289
#define CF_CONTENT_Y1      (CF_INNER_Y1 - CF_CONTENT_PAD)           // 228

// Row height: fill the content area as evenly as possible (improvement #5).
// Content height = 228-40+1 = 189; 8 rows -> 23px each fills it well.
#define CF_VISIBLE         8
#define CF_ROW_H           ((CF_CONTENT_Y1 - CF_CONTENT_Y0 + 1) / CF_VISIBLE)  // 23
#define CF_SWIPE_PX        30
#define CF_KEY_REPEAT_INIT_MS  300
#define CF_KEY_REPEAT_MS       120

// Value column geometry
#define CF_MARGIN          6
#define CF_VAL_W           110    // value column width
#define CF_VAL_GAP         4      // gap between value/frame right edge (improvement #3)
#define CF_SWITCH_W        32     // ON/OFF switch image width
#define CF_SWITCH_H        16     // ON/OFF switch image height

//=============================================================================
// Colors (aligned with LogListForm)
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
// Config-row value display model (spec 6.6-6.8)
//=============================================================================


// A config row = register number + (for ENUM registers) the option list to
// resolve its value against. The display TYPE is NOT stored here - it is read
// from the register property at runtime via SIT_GetVType (spec 6.7.2).
struct CfgRow {
  uint32_t        regNum;
//  const uint16_t* pEnum;      // enum option list (nullptr if register uses none)
//  uint8_t         enumCount;  // option count in pEnum
};

#define ENUM_ROW(reg, list)  { (reg), (list), (uint8_t)NUM_Elements(list) }
#define NUM_ROW(reg)         { (reg) }

//=============================================================================
// Config register groups (spec 6.5). One static table per config type.
// Display type per register is derived from SIT_GetVType(Property) at runtime
// (spec 6.7.2); ENUM registers resolve their value through the option list.
//=============================================================================
static const CfgRow s_rowsLogic[] = {
  NUM_ROW (REG_FN_AUTOCTRL_EN    ),
  NUM_ROW (REG_FN_AUTO_TURNOFF   ),
  NUM_ROW (REG_FN_AUTO_BREAKER_ON),
  NUM_ROW (REG_FN_PASSBY_EN      ),
  NUM_ROW (REG_FN_ACTION_VOLTAGE ),
  NUM_ROW (REG_FN_COIL_VOLTAGE   ),
  NUM_ROW (REG_FN_PWRON_TIME     ),
  NUM_ROW (REG_FN_PWROFF_TIME    ),
  NUM_ROW (REG_FN_SHUTDOWN_TIME  ),
};

static const CfgRow s_rowsDevice[] = {
  NUM_ROW (REG_FN_RELAY_DELAY),
  NUM_ROW (REG_FN_RELAY_TIME),
};

static const CfgRow s_rowsSerial[] = {
  NUM_ROW (REG_UART1_ADDR    ),
  NUM_ROW (REG_UART1_BAUDRATE),
  NUM_ROW (REG_UART1_PARITY  ),
};

// Ethernet: empty per spec 6.5.4
static const CfgRow* const s_rowsEthernet = nullptr;

//=============================================================================
// Config type descriptor - rows + caption string + caption icon
//=============================================================================
struct CfgTypeDesc {
  const CfgRow* pRows;
  uint16_t      uCount;
  uint32_t      uCapStrId;
  uint32_t      uCapIconId;
};

static const CfgTypeDesc kCfgTypes[] = {
  { s_rowsLogic,    NUM_Elements(s_rowsLogic),  idCfgGroup01, picIdxCF_Logic16x16Cyan    },
  { s_rowsDevice,   NUM_Elements(s_rowsDevice), idCfgGroup02, picIdxCF_Device16x16Cyan   },
  { s_rowsSerial,   NUM_Elements(s_rowsSerial), idCfgGroup03, picIdxCF_Serial16x16Cyan   },
  { s_rowsEthernet, 0,                          idCfgGroup04, picIdxCF_Ethernet16x16Cyan },
};
#define CF_TYPE_COUNT  NUM_Elements(kCfgTypes)

//=============================================================================
// Form state (heap-allocated in _Init, freed in _Close)
//=============================================================================

// Dialog type (Phase 6: Type dispatch)
enum TDialogType {
  dtNone = 0,        // No dialog active
  dtLogin = 1,       // GLoginDialog
  dtNumReg = 2,      // GNumRegDialog (INT/HEX/REAL)
  dtIPAddress = 3,   // GIPAddressDialog
  dtDatetime = 4,    // GDatetimeDialog
  dtListbox = 5      // GListbox (enum selection)
};

struct TConfigFormState {
  TConfigType eType;
  uint16_t    uCount;
  uint16_t    uTopItem;
  uint16_t    uCurItem;
  uint32_t    uLastKeyTick;
  uint16_t    uKeyRepeat;
#if GUI_SUPPORT_TOUCH
  int16_t     touchStartX;
  int16_t     touchStartY;
  int16_t     touchLastY;
  bool        touchActive;
#endif

  // Phase 6: Dialog integration
  TDialogType  dlgType;        // Current dialog type
  GDialogTodo  dlgTodo;        // Pending action after login (from GDialogDefs.h)
  GWidget*     pDialog;        // Active dialog instance (GDialog* or GListbox*)
  uint16_t     pendingRegNum;  // Register to edit after permission granted
  void*        pDlgData;       // Dialog-specific data (e.g., GDialogConfig)
};

static TConfigFormState* s_pState = nullptr;

//=============================================================================
// Forward declarations for Phase 6: Dialog Integration
//=============================================================================
static bool checkPermission(uint16_t regNum);
static void dispatchEdit(uint16_t regNum);
static void destroyDialog(void);
static void acceptEdit(void);
static void cancelEdit(void);

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
  int y = CF_CONTENT_Y0 + (int)(uIdx - s_pState->uTopItem) * CF_ROW_H;
  _MakeRect(pR, CF_CONTENT_X0, y, CF_CONTENT_X1 - CF_CONTENT_X0 + 1, CF_ROW_H);
}

/// Current config-type descriptor.
static const CfgTypeDesc* _GetDesc(void)
{
  uint8_t idx = (uint8_t)s_pState->eType;
  if (idx >= CF_TYPE_COUNT) {
    idx = 0;
  }
  return &kCfgTypes[idx];
}

/// Config row at absolute index (nullptr if out of range).
static const CfgRow* _GetRow(uint16_t uIdx)
{
  const CfgTypeDesc* pDesc = _GetDesc();
  if (nullptr == pDesc->pRows || uIdx >= pDesc->uCount) {
    return nullptr;
  }
  return &pDesc->pRows[uIdx];
}

static void _ChangeType(TConfigType newType)
{
  if (nullptr == s_pState) {
    return;
  }
  s_pState->eType    = newType;
  s_pState->uCount   = _GetDesc()->uCount;
  s_pState->uTopItem = 0;
  s_pState->uCurItem = 0;
  s_pState->uKeyRepeat   = 0;
  s_pState->uLastKeyTick = 0;
}

//=============================================================================
// Drawing
//=============================================================================
static void _DrawOuterFrame(void)
{
  GUI_SetColor(crOuterFrame);
  GUI_DrawRoundedRect(CF_LIST_X, CF_LIST_Y, CF_LIST_X1, CF_LIST_Y1, 3);
}

static void _DrawCaption(void)
{
  const CfgTypeDesc* pDesc = _GetDesc();

  // Category icon at the left of the caption bar - 16x16, vertically
  // centered (improvement #1).
  GUI_DrawPicture(CSG_WINATLAS, CF_CAT_ICON_X0, CF_CAT_ICON_Y0,
                  pDesc->uCapIconId, 100);

  // Caption text - 2px gap after the category icon (improvement #1),
  // leaving room for the Edit + Menu icons on the right.
  const char* pStr = GetMultiLangString(pDesc->uCapStrId);
  if (nullptr != pStr) {
    GUI_RECT r;
    int capTextX0 = CF_CAT_ICON_X0 + CF_CAT_ICON_W + CF_CAT_TEXT_GAP;
    int capTextW  = CF_EDIT_ICON_X0 - capTextX0;
    _MakeRect(&r, capTextX0, CF_CAP_Y0, capTextW, CF_CAP_H);
    GUI_SetFont(ftItem);
    GUI_SetColor(crCaption);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);
    GUI_DispStringInRect(pStr, &r, GUI_TA_LEFT | GUI_TA_VCENTER);
  }

#if GUI_SUPPORT_TOUCH
  // Edit button (drawn but inert in Display mode - spec 6.7)
  GUI_DrawPicture(CSG_WINATLAS, CF_EDIT_ICON_X0, CF_CAP_ICON_Y0,
                  picIdxCF_Edit20x20Cyan, 100);
  // Menu button at the right end of the caption bar
  GUI_DrawPicture(CSG_WINATLAS, CF_CAP_ICON_X0, CF_CAP_ICON_Y0,
                  picIdxMA_Menu20x20Cyan, 100);
#endif
  
  // Caption separator line
  GUI_SetColor(crSeparator);
  GUI_DrawHLine(CF_CAP_SEP, CF_LIST_X + CF_FRAME_GAP, CF_INNER_X1);
}

/// Redraw Caption area only (inset to avoid outer frame border)
static void _RedrawCaption(void)
{
  GUI_SetColor(crListPanel);
  int capX0 = CF_LIST_X + CF_FRAME_GAP;
  int capY0 = CF_LIST_Y + CF_FRAME_GAP;
  int capX1 = CF_INNER_X1;
  int capY1 = CF_CAP_SEP - 1;
  GUI_FillRect(capX0, capY0, capX1, capY1);
  _DrawCaption();
}

static void _DrawScrollbar(void)
{
  if (nullptr == s_pState) {
    return;
  }
  int trackY0 = CF_INNER_Y;
  int trackY1 = CF_INNER_Y1;
  // When all items fit on screen, hide the scrollbar: clear the lane back to
  // the panel color (removes a stale thumb left over from a longer category).
  if (CF_VISIBLE >= s_pState->uCount) {
    GUI_SetColor(crListPanel);
    GUI_FillRect(CF_SCRL_X0, trackY0, CF_SCRL_X1, trackY1);
    _DrawOuterFrame();
    return;
  }
  uint16_t maxTop = s_pState->uCount - CF_VISIBLE;
  int trackH  = trackY1 - trackY0 + 1;
  int thumbH  = trackH * CF_VISIBLE / s_pState->uCount;
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
  GUI_FillRect(CF_SCRL_X0, trackY0, CF_SCRL_X1, trackY1);
  GUI_SetColor(crScrollThumb);
  GUI_FillRoundedRect(CF_SCRL_X0 + 2, thumbY, CF_SCRL_X1 - 2, thumbY1, 2);
  _DrawOuterFrame();
}

static void _FlushForm(void)
{
  CSG_DrawPicture(IMAGE_BACKGROUND, 0, 0, 0, 100, nullptr);
  GUI_SetColor(crListPanel);
  GUI_FillRect(CF_LIST_X, CF_LIST_Y, CF_LIST_X1, CF_LIST_Y1);
  GUI_SetColor(crInnerFrame);
  GUI_DrawRoundedRect(CF_INNER_X, CF_INNER_Y, CF_INNER_X1, CF_INNER_Y1, 2);
  _DrawCaption();
  _DrawOuterFrame();
}

/// Format the enum/number value text for a config row into pBuf.
/// ENUM registers resolve value -> option-list index -> string (spec 6.6);
/// numeric registers show the value + 1-space + dimension (spec 6.7.2).
/// BIN registers produce no text - they draw a switch image in _DrawItem.
static void _FormatValue(const CfgRow* pRow, const TDevRegInfoItem* pInfo,
                         char* pBuf, int nLen)
{
  if (0 >= nLen) {
    return;
  }
  pBuf[0] = 0;
  uint32_t uVal   = DevReg_Read(pRow->regNum);
  uint32_t uVType = SIT_GetVType(pInfo->Property);

  if (SIT_VAT_ENUM == uVType) {
//    // Enum text: register value indexes the option list (spec 6.6)
//    if (nullptr == pRow->pEnum || 0 == pRow->enumCount) {
//      return;
//    }
//    uint8_t idx = (uint8_t)uVal;
//    if (pRow->enumCount <= idx) {
//      idx = (uint8_t)(pRow->enumCount - 1);
//    }
//    const char* pTxt = GetMultiLangString(pRow->pEnum[idx]);
    // Wey. 2026.7.21 
    // uses RINF_getRegEnumList
    const uint16_t* pEnum;
    const int enumCount = RINF_getRegEnumList(pRow->regNum, pEnum );
    if( nullptr == pEnum || 0 == enumCount ) {
      return ;
    }
    uint8_t idx = (uint8_t)uVal;
    if (enumCount <= idx) {
      idx = (uint8_t)(enumCount - 1);
    }
    const char* pTxt = GetMultiLangString(pEnum[idx]);
    
    if (nullptr != pTxt) {
      int pos = snprintf(pBuf, nLen, "%s", pTxt);
      // Append dimension if present (e.g., REG_UART1_BAUDRATE -> "9600 bps")
      const char* pDim = RINF_GetDIMNameEx(pInfo);
      if (nullptr != pDim && 0 < pos && nLen > pos) {
        snprintf(pBuf + pos, nLen - pos, " %s", pDim);
      }
    }
    return;
  }

  // Numeric value + 1-space + dimension (improvement #4).
  // Config registers are INT/HEX (no REAL group); note SIT_VAT_BIN and
  // SIT_VAT_REAL share the same encoding, and BIN is handled as a switch
  // image in _DrawItem before this runs.
  int pos = snprintf(pBuf, nLen, "%u", (unsigned)uVal);
  const char* pDim = RINF_GetDIMNameEx(pInfo);
  if (nullptr != pDim && 0 < pos && nLen > pos) {
    snprintf(pBuf + pos, nLen - pos, " %s", pDim);
  }
}

/// Draw one config row: name (left) + value/switch (right).
static void _DrawItem(uint16_t uIdx)
{
  
  if (nullptr == s_pState) {
    return;
  }
  
  if (uIdx >= s_pState->uCount ||
      uIdx < s_pState->uTopItem ||
      uIdx >= s_pState->uTopItem + CF_VISIBLE) {
    return;
  }
  
  GUI_RECT rRow;
  _RowRect(&rRow, uIdx);
  bool bSelected = (uIdx == s_pState->uCurItem);

  GUI_SetColor(crListPanel);
  GUI_FillRect(rRow.x0, rRow.y0, rRow.x1, rRow.y1);

  const CfgRow* pRow = _GetRow(uIdx);
  if (nullptr == pRow) {
    return;
  }
  
  const TDevRegInfoItem* pInfo = DevIntf_GetRegInfo(pRow->regNum);
  if (nullptr == pInfo) {
    return;
  }
  
  const char* pcRegName = GetMultiLangString(pInfo->NameStrId);
  if (nullptr == pcRegName) {
    pcRegName = pInfo->pName;
  }

  // Value cell spans CF_VAL_W and ends CF_VAL_GAP px before the selection
  // frame right edge (improvement #3).
  int cellX1 = rRow.x1 - CF_VAL_GAP;
  int cellX0 = cellX1 - CF_VAL_W + 1;

  // Register name (left) - fills up to the value cell
  GUI_RECT rName;
  _MakeRect(&rName, rRow.x0 + CF_MARGIN, rRow.y0,
            cellX0 - (rRow.x0 + CF_MARGIN), CF_ROW_H);
  GUI_SetFont(ftItem);
  GUI_SetColor(bSelected ? crAccent : crText);
  GUI_SetTextMode(GUI_TEXTMODE_TRANS);
  if (nullptr != pcRegName) {
    GUI_DispStringInRect(pcRegName, &rName, GUI_TA_LEFT | GUI_TA_VCENTER);
  }

  // Value (right) - display type from the register property (spec 6.7.2)
  uint32_t uVType = SIT_GetVType(pInfo->Property);
  if (SIT_VAT_BIN == uVType) {
    // ON/OFF switch image - horizontally + vertically centered in the value
    // cell (improvement #2).
    uint32_t uVal = DevReg_Read(pRow->regNum);
    uint32_t uImg = (0 != uVal) ? picIdxLV_SwitchON32x16Red
                                : picIdxLV_SwitchOFF32x16Cyan;
    int imgX0 = rRow.x1 - CF_SWITCH_W - 8; //cellX0 + (CF_VAL_W - CF_SWITCH_W) / 2;
    int imgY0 = rRow.y0 + (CF_ROW_H - CF_SWITCH_H) / 2;
    GUI_DrawPicture(CSG_WINATLAS, imgX0, imgY0, uImg, 100);
  } else {
    char szVal[32] = { 0 };
    _FormatValue(pRow, pInfo, szVal, sizeof(szVal));
    if (0 != szVal[0]) {
      GUI_RECT rVal;
      _MakeRect(&rVal, cellX0, rRow.y0, CF_VAL_W, CF_ROW_H);
      GUI_SetColor(bSelected ? crAccent : crText);
      GUI_DispStringInRect(szVal, &rVal, GUI_TA_RIGHT | GUI_TA_VCENTER);
    }
  }

  if (true == bSelected) {
    GUI_SetColor(crSelFrame);
    GUI_DrawRoundedFrame(rRow.x0, rRow.y0, rRow.x1, rRow.y1, 3, 1);
  }
}

static void _UpdateList(void)
{
  for (uint16_t i = 0; i < CF_VISIBLE; ++i) {
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

//=============================================================================
// Navigation
//=============================================================================
static void _EnsureCursorVisible(void)
{
  if (s_pState->uCurItem < s_pState->uTopItem) {
    s_pState->uTopItem = s_pState->uCurItem;
  } else if (s_pState->uCurItem >= s_pState->uTopItem + CF_VISIBLE) {
    s_pState->uTopItem = s_pState->uCurItem - CF_VISIBLE + 1;
  }
}

static void _ClampTop(void)
{
  
  if (CF_VISIBLE >= s_pState->uCount) {
    s_pState->uTopItem = 0;
    return;
  }
  
  uint16_t maxTop = s_pState->uCount - CF_VISIBLE;
  if (maxTop < s_pState->uTopItem) {
    s_pState->uTopItem = maxTop;
  }
}

/// Cycle to next config type (right), wrap around.
static void _CycleNext(void)
{
  
  uint8_t idx = (uint8_t)s_pState->eType;
  idx = (idx + 1) % CF_TYPE_COUNT;
  _ChangeType((TConfigType)idx);
  
  // Partial redraw: Caption + Row table + Scrollbar (no full screen flush)
  _RedrawCaption();
  _UpdateList();
  _DrawScrollbar();
}

/// Cycle to prev config type (left), wrap around.
static void _CyclePrev(void)
{
  
  uint8_t idx = (uint8_t)s_pState->eType;
  idx = (0 == idx) ? (CF_TYPE_COUNT - 1) : (idx - 1);
  _ChangeType((TConfigType)idx);
  
  // Partial redraw: Caption + Row table + Scrollbar (no full screen flush)
  _RedrawCaption();
  _UpdateList();
  _DrawScrollbar();
}

//=============================================================================
// Key handling (throttled auto-repeat, mirrors LogListForm)
//=============================================================================
static void _OnKey(uint16_t uwKey, bool bRepeat)
{

  if (nullptr == s_pState) {
    return;
  }

  if (true == bRepeat) {
    uint32_t now = GUI_GetTime();
    uint32_t threshold = (0 == s_pState->uKeyRepeat) ? CF_KEY_REPEAT_INIT_MS
                                                     : CF_KEY_REPEAT_MS;
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
    if (oldTop != s_pState->uTopItem) {
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
    } else if (CF_VISIBLE < s_pState->uCount &&
               s_pState->uCount - CF_VISIBLE > s_pState->uTopItem) {
      ++s_pState->uTopItem;
    } else {
      break;
    }
    
    if (oldTop != s_pState->uTopItem) {
      _UpdateList();
      _DrawScrollbar();
    } else {
      _DrawItem(old);
      _DrawItem(s_pState->uCurItem);
    }
    break;
  }
  
  case KEY_LEFT: {
    if (false == bRepeat) {
      _CyclePrev();
    }
    break;
  }

  case KEY_RIGHT: {
    if (false == bRepeat) {
      _CycleNext();
    }

    break;
  }

  // Phase 6: ENTER key triggers edit for current row
  case KEY_ENTER: {
    if (false == bRepeat && s_pState->uCurItem < s_pState->uCount) {
      const CfgRow* pRow = _GetRow(s_pState->uCurItem);
      if (nullptr != pRow) {
        uint16_t regNum = (uint16_t)pRow->regNum;
        // Check permission first
        if (checkPermission(regNum)) {
          // Permission granted, dispatch edit
          dispatchEdit(regNum);
        }
        // If permission denied, checkPermission already created login dialog
      }
    }
    break;
  }

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
    s_pState->touchActive = true;
    s_pState->touchStartX = x;
    s_pState->touchStartY = y;
    s_pState->touchLastY  = y;
    if (x >= CF_CONTENT_X0 && x <= CF_CONTENT_X1 &&
        y >= CF_CONTENT_Y0 &&
        y <  CF_CONTENT_Y0 + CF_VISIBLE * CF_ROW_H) {
      uint16_t idx = s_pState->uTopItem +
                     (uint16_t)((y - CF_CONTENT_Y0) / CF_ROW_H);
      if (idx < s_pState->uCount && idx != s_pState->uCurItem) {
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
      if (dy >= CF_ROW_H) {
        uint16_t shift = (uint16_t)(dy / CF_ROW_H);
        if (shift <= s_pState->uTopItem) {
          s_pState->uTopItem -= shift;
        } else {
          s_pState->uTopItem = 0;
        }
        s_pState->touchLastY = y;
        _ClampTop();
        _UpdateList();
        _DrawScrollbar();
      } else if (dy <= -CF_ROW_H) {
        uint16_t shift = (uint16_t)((-dy) / CF_ROW_H);
        if (s_pState->uCount > CF_VISIBLE) {
          uint16_t maxTop = s_pState->uCount - CF_VISIBLE;
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
      if (x >= CF_CAP_ICON_X0 && x <= CF_CAP_ICON_X1 &&
          y >= CF_CAP_ICON_Y0 && y <= CF_CAP_ICON_Y0 + 20 - 1) {
        s_pState->touchActive = false;
        gfc::PopForm();
        return;
      }
      int16_t dx = x - s_pState->touchStartX;
      int16_t dy = y - s_pState->touchStartY;
      int16_t adx = (0 <= dx) ? dx : (int16_t)(-dx);
      int16_t ady = (0 <= dy) ? dy : (int16_t)(-dy);
      if (CF_SWIPE_PX <= adx && ady < adx) {
        // Horizontal swipe - cycle config type
        if (0 < dx) {
          _CycleNext();
        } else {
          _CyclePrev();
        }
        if (nullptr == s_pState) {
          return;
        }
      } else if (adx < CF_SWIPE_PX && ady < CF_SWIPE_PX) {
        // Phase 6: Tap (not swipe) in content area - trigger edit like ENTER key
        if (x >= CF_CONTENT_X0 && x <= CF_CONTENT_X1 &&
            y >= CF_CONTENT_Y0 && y < CF_CONTENT_Y0 + CF_VISIBLE * CF_ROW_H) {
          uint16_t idx = s_pState->uTopItem +
                         (uint16_t)((y - CF_CONTENT_Y0) / CF_ROW_H);
          if (idx < s_pState->uCount) {
            const CfgRow* pRow = _GetRow(idx);
            if (nullptr != pRow) {
              uint16_t regNum = (uint16_t)pRow->regNum;
              // Check permission first
              if (checkPermission(regNum)) {
                // Permission granted, dispatch edit
                dispatchEdit(regNum);
              }
              // If permission denied, checkPermission already created login dialog
            }
          }
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
  // GConfigForm is static display - no automatic refresh.
  // Updates only on user interaction (key press, touch swipe, type switch).
}

//=============================================================================
// Phase 6: Dialog Integration Helpers
//=============================================================================

/// Destroy active dialog and free resources
static void destroyDialog(void)
{
  if (nullptr == s_pState || nullptr == s_pState->pDialog) {
    return;
  }

  // Close and destroy dialog
  s_pState->pDialog->onClose();
  s_pState->pDialog->~GWidget();  // Virtual destructor dispatches to GDialog/GListbox
  RAM_Free(s_pState->pDialog);
  s_pState->pDialog = nullptr;

  // Free dialog-specific data if any
  if (nullptr != s_pState->pDlgData) {
    RAM_Free(s_pState->pDlgData);
    s_pState->pDlgData = nullptr;
  }

  s_pState->dlgType = dtNone;
}

/// Check if user has permission to edit (Phase 6: Permission gate)
static bool checkPermission(uint16_t regNum)
{
  // Check if password is already validated (use Normal permission for now)
  // TODO: Determine which permission level (Password1Ok vs Password2Ok) based on register
  if (GetPassword1Ok) {
    return true;
  }

  // Permission denied - save pending action and show login dialog
  s_pState->pendingRegNum = regNum;
  s_pState->dlgTodo = dtdEdit;

  // Create login dialog based on user level
  // TODO: Determine login mode based on register or user level
  // For now, use lmLoginNormal (Password1)
  GLoginDialog::GLoginMode mode = GLoginDialog::lmLoginNormal;

  // Create login dialog with resource configuration
  GLoginDialog* pDlg = static_cast<GLoginDialog*>(RAM_Malloc(sizeof(GLoginDialog)));
  if (nullptr == pDlg) {
    return false;
  }

  ::new (pDlg) GLoginDialog(&g_loginDialogConfig, (void*)(uintptr_t)mode);
  pDlg->Init();
  pDlg->onShow();

  s_pState->pDialog = pDlg;
  s_pState->pDlgData = nullptr;
  s_pState->dlgType = dtLogin;

  return false;  // Permission not yet granted
}

/// Create number register dialog (INT/HEX/REAL)
static void createNumberDialog(uint16_t regNum)
{
  // Create number dialog with resource configuration
  GNumRegDialog* pDlg = static_cast<GNumRegDialog*>(RAM_Malloc(sizeof(GNumRegDialog)));
  if (nullptr == pDlg) {
    return;
  }

  ::new (pDlg) GNumRegDialog(&g_numRegDialogConfig, (void*)(uintptr_t)regNum);
  pDlg->Init();
  pDlg->onShow();

  s_pState->pDialog = pDlg;
  s_pState->pDlgData = nullptr;
  s_pState->dlgType = dtNumReg;
}

/// Create IP address dialog
static void createIPAddressDialog(uint16_t regNum)
{
  // Create IP address dialog with resource configuration
  GIPAddressDialog* pDlg = static_cast<GIPAddressDialog*>(RAM_Malloc(sizeof(GIPAddressDialog)));
  if (nullptr == pDlg) {
    return;
  }

  ::new (pDlg) GIPAddressDialog(&g_ipDialogConfig, (void*)(uintptr_t)regNum);
  pDlg->Init();
  pDlg->onShow();

  s_pState->pDialog = pDlg;
  s_pState->pDlgData = nullptr;
  s_pState->dlgType = dtIPAddress;
}

/// Create listbox for enum register
static void createListbox(uint16_t regNum)
{
  // Get enum option list via RINF_getRegEnumList
  const uint16_t* pStrIds = nullptr;
  int itemCount = RINF_getRegEnumList(regNum, pStrIds);

  if (itemCount <= 0 || nullptr == pStrIds) {
    // No enum list available, fallback to direct edit
    return;
  }

  // Get current value
  uint32_t curVal = DevReg_Read(regNum);
  if (curVal >= (uint32_t)itemCount) {
    curVal = 0;  // Clamp to valid range
  }

  // Allocate GListbox::GConfig
  GListbox::GConfig* pCfg = static_cast<GListbox::GConfig*>(RAM_Malloc(sizeof(GListbox::GConfig)));
  if (nullptr == pCfg) {
    return;
  }

  // Fill config (position will be set by GListbox based on current selection)
  pCfg->x = CF_CONTENT_X0;
  pCfg->y = CF_CONTENT_Y0;
  pCfg->w = CF_CONTENT_X1 - CF_CONTENT_X0 + 1;
  pCfg->h = CF_VISIBLE * CF_ROW_H;
  pCfg->margin = 2;
  pCfg->drawMode = GUI_DRAWMODE_NORMAL;
  pCfg->alignment = GUI_TA_LEFT | GUI_TA_VCENTER;
  pCfg->ItemCount = (uint8_t)itemCount;
  pCfg->pStrings = nullptr;
  pCfg->pStrIds = pStrIds;

  // Create GListbox with resource style
  GListbox* pListbox = static_cast<GListbox*>(RAM_Malloc(sizeof(GListbox)));
  if (nullptr == pListbox) {
    RAM_Free(pCfg);
    return;
  }

  ::new (pListbox) GListbox(&g_listboxStyle, pCfg, curVal);
  pListbox->onShow();

  s_pState->pDialog = pListbox;
  s_pState->pDlgData = pCfg;
  s_pState->dlgType = dtListbox;
  s_pState->pendingRegNum = regNum;  // Store regNum for later write
}

/// Create datetime dialog
static void createDatetimeDialog(uint16_t regNum)
{
  // Create datetime dialog with resource configuration
  GDatetimeDialog* pDlg = static_cast<GDatetimeDialog*>(RAM_Malloc(sizeof(GDatetimeDialog)));
  if (nullptr == pDlg) {
    return;
  }

  ::new (pDlg) GDatetimeDialog(&g_datetimeDialogConfig, (void*)(uintptr_t)regNum);
  pDlg->Init();
  pDlg->onShow();

  s_pState->pDialog = pDlg;
  s_pState->pDlgData = nullptr;
  s_pState->dlgType = dtDatetime;
}

/// Type dispatch - create appropriate dialog based on register type
static void dispatchEdit(uint16_t regNum)
{
  // Get register info
  const TDevRegInfoItem* pProp = DevIntf_GetRegInfo(regNum);
  if (nullptr == pProp) {
    return;
  }

  // Dispatch based on value type
  uint32_t vType = SIT_GetVType(pProp->Property);

  switch (vType) {
    case SIT_VAT_INT:
    case SIT_VAT_HEX:
    case SIT_VAT_REAL:
      createNumberDialog(regNum);
      break;

    case SIT_VAT_BIN:
      // Direct toggle - no dialog needed
      {
        uint32_t curVal = DevReg_Read(regNum);
        DevReg_Write(regNum, curVal ? 0 : 1);
        _UpdateList();  // Refresh display
      }
      break;

    case SIT_VAT_ENUM:
      // Create listbox for enum selection
      createListbox(regNum);
      break;

    case SIT_VAT_DATETIME:
      createDatetimeDialog(regNum);
      break;

    case SIT_VAT_PASSWORD:
      // Password register - show login dialog
      if (checkPermission(regNum)) {
        // Already have permission, proceed
        createNumberDialog(regNum);
      }
      break;

    // case SIT_VAT_IP:  // TODO: Define this constant if needed
    //   createIPAddressDialog(regNum);
    //   break;

    default:
      // Unsupported type or IP address (handle as numeric for now)
      createNumberDialog(regNum);
      break;
  }
}

/// Accept dialog result and write to register
static void acceptEdit(void)
{
  if (nullptr == s_pState || nullptr == s_pState->pDialog) {
    return;
  }

  // Special handling for login dialog
  if (dtLogin == s_pState->dlgType) {
    destroyDialog();

    // Check if password is now OK and we have pending edit
    // TODO: Check appropriate permission level based on login mode
    if (GetPassword1Ok && dtdEdit == s_pState->dlgTodo) {
      uint16_t regNum = s_pState->pendingRegNum;
      s_pState->dlgTodo = dtdNone;
      s_pState->pendingRegNum = 0;
      dispatchEdit(regNum);  // Retry the edit
    }
    return;
  }

  // For other dialogs, get result and write to register
  uint32_t result = 0;
  uint16_t regNum = 0;

  // Get register number and result based on dialog type
  switch (s_pState->dlgType) {
    case dtNumReg:
      regNum = static_cast<GNumRegDialog*>(s_pState->pDialog)->getRegNum();
      result = static_cast<GNumRegDialog*>(s_pState->pDialog)->getResult();
      DevReg_Write(regNum, result);
      break;

    case dtIPAddress:
      regNum = static_cast<GIPAddressDialog*>(s_pState->pDialog)->getRegNum();
      result = static_cast<GIPAddressDialog*>(s_pState->pDialog)->getResult();
      DevReg_Write(regNum, result);
      break;

    case dtDatetime:
      {
        // Datetime uses special API: DevIntf_setDateTime
        TDateTimeType dtResult = static_cast<GDatetimeDialog*>(s_pState->pDialog)->getResult();
        DevIntf_setDateTime(&dtResult, TOKEN_INTF_OPERATE);
      }
      break;

    case dtListbox:
      {
        // Listbox returns selected index
        regNum = s_pState->pendingRegNum;
        result = static_cast<GListbox*>(s_pState->pDialog)->getResult();
        DevReg_Write(regNum, result);
      }
      break;

    default:
      break;
  }

  // Cleanup and refresh
  destroyDialog();
  _UpdateList();
}

/// Cancel dialog edit
static void cancelEdit(void)
{
  if (nullptr == s_pState) {
    return;
  }

  destroyDialog();
  s_pState->dlgTodo = dtdNone;
  s_pState->pendingRegNum = 0;
}

//=============================================================================
// Form lifecycle
//=============================================================================
static void _Init(const void* argument)
{
  if (nullptr != s_pState) {
    RAM_Free(s_pState);
    s_pState = nullptr;
  }

  s_pState = static_cast<TConfigFormState*>(RAM_Malloc(sizeof(TConfigFormState)));
  DEV_ASSERT(nullptr == s_pState, GFC_OutOfMem);
  if (nullptr == s_pState) {
    return;
  }

  memset(s_pState, 0, sizeof(TConfigFormState));
  // Initial config type from the menu argument; default to cgtLogic.
  TConfigType eInit = (nullptr != argument)
                      ? (TConfigType)(uintptr_t)argument
                      : cgtLogic;
  if ((uint8_t)eInit >= CF_TYPE_COUNT) {
    eInit = cgtLogic;
  }

  _ChangeType(eInit);

  // Propare the device interface for config editing (spec 6.7.1).
  DevIntf_DevCfgEditPropare( TOKEN_INTF_OPERATE );

  // Phase 6: Clear password permissions on form entry
  ClrPassword1Ok;
  ClrPassword2Ok;
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
    // Phase 6: Destroy any active dialog before closing
    destroyDialog();

    RAM_Free(s_pState);
    s_pState = nullptr;
  }

  // Phase 6: Clear password permissions on form exit
  ClrPassword1Ok;
  ClrPassword2Ok;
}

static void _OnMessage(GM_MESSAGE* pMsg)
{

  if (nullptr == pMsg || nullptr == s_pState) {
    return;
  }

  // Phase 6: If dialog is active, delegate messages to it first
  if (nullptr != s_pState->pDialog) {
    switch (pMsg->MsgId) {
      case GM_DIALOG_ACCEPT:
        acceptEdit();
        pMsg->MsgId = 0;
        return;

      case GM_DIALOG_CANCEL:
        cancelEdit();
        pMsg->MsgId = 0;
        return;

      case GM_KEYDOWN:
      case GM_KEYPRESS:
      case GM_KEYUP:
      case GM_TOUCH:
        // Delegate to dialog
        s_pState->pDialog->onMessage(pMsg);
        pMsg->MsgId = 0;
        return;

      default:
        break;
    }
  }

  // Handle form's own messages
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
// Form descriptor + registration (WID_ConfigForm)
//=============================================================================
const GWinForm FConfigForm = {
  _Init,
  _Show,
  _Close,
  _OnMessage
};

static const gfc::FormRegistrar kRegConfig(WID_ConfigForm,
                                           &FConfigForm,
                                           "Config");
//-----------------------------------------------------------------------------

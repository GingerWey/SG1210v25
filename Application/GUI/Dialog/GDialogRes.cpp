//-----------------------------------------------------------------------------
/*
 File        : GDialogRes.cpp
 Version     : V1.00
 By          : Wey@SilverGrid

 Description : Dialog resource configurations for GConfigDialog
               - DigitalKeyboard key grid
               - GNumRegDialog (INT/HEX/REAL register editing)
               - GLoginDialog (password input)
               - GIPAddressDialog (IPv4 address editing)
               - GDatetimeDialog (date/time editing)

 Date        : 2026.07.21
*/
//-----------------------------------------------------------------------------
#include "GDialogRes.h"
#include "GDialog.h"
#include "GDialogDefs.h"
#include "GKeyboard.h"
#include "GLabel.h"
#include "GEditorPanel.h"

#include "DevIntf.h"
#include "DevRegs.h"
#include "Strings/TextStrs.h"

#include <GUI.h>
#include "FontSGRes.h"

//=============================================================================
// DigitalKeyboard Key Grid (§4.1.2)
//=============================================================================

// Key row as flat array (16 keys total: 4x4)
// Per spec §4.1.2 Digital Keyboard layout
static const GKey::GConfig s_digitalKeys[] = {
  // Row 1: 1, 2, 3, Delete
  {0, '1', 15, 5, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "1"},
  {0, '2', 70, 5, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "2"},
  {0, '3', 125, 5, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "3"},
  {0, KEY_BACKSPACE, 180, 5, 76, 30, 3, 3, 70, 24, GUI_DRAWMODE_TRANS, (GUI_TA_LEFT | GUI_TA_VCENTER), {nullptr, 0}, 0, "Delete"},

  // Row 2: 4, 5, 6, Enter
  {0, '4', 15, 40, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "4"},
  {0, '5', 70, 40, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "5"},
  {0, '6', 125, 40, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "6"},
  {0, KEY_ENTER, 180, 40, 76, 30, 3, 3, 70, 24, GUI_DRAWMODE_TRANS, (GUI_TA_LEFT | GUI_TA_VCENTER), {nullptr, 0}, 0, "Enter"},

  // Row 3: 7, 8, 9, Cancel
  {0, '7', 15, 75, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "7"},
  {0, '8', 70, 75, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "8"},
  {0, '9', 125, 75, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "9"},
  {0, KEY_ESCAPE, 180, 75, 76, 30, 3, 3, 70, 24, GUI_DRAWMODE_TRANS, (GUI_TA_LEFT | GUI_TA_VCENTER), {nullptr, 0}, 0, "Cancel"},

  // Row 4: ., 0, <, >
  {0, '.', 15, 110, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "."},
  {0, '0', 70, 110, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "0"},
  {0, KEY_LEFT, 125, 110, 50, 30, 3, 3, 44, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER), {nullptr, 0}, 0, "<"},
  {0, KEY_RIGHT, 180, 110, 76, 30, 3, 3, 70, 24, GUI_DRAWMODE_TRANS, (GUI_TA_LEFT | GUI_TA_VCENTER), {nullptr, 0}, 0, ">"}
};

// DigitalKeyboard key grid definition
static const GKeyboard::GKeyGrid s_digitalKeyGrid = {
  4,  // rowCount
  4,  // colCount
  s_digitalKeys
};

//=============================================================================
// Common Keyboard Style (§5.2.1, §6.2.1, §7.2.1, §8.2.1)
//=============================================================================

static const GKeyboard::GStyle s_commonKeyboardStyle = {
  0x031635,  // crKbdBackground - Keyboard background
  0x002A6C,  // crKbdFrame - Keyboard frame
  {          // keyStyle
    0x003366,  // crBackground - Key background
    0x002A6C,  // crFrame - Key frame (normal)
    0x00B5F2,  // crFrameFocus - Key frame (focused)
    0xC0C0C0,  // crLabel - Key text (normal)
    0x00B5F2,  // crLabelFocus - Key text (focused)
    GUI_FONT_16B_ASCII  // ftLabel - Key font
  }
};

//=============================================================================
// Common Editor Style
//=============================================================================

static const GEditor::GStyle s_commonEditorStyle = {
  0x005080,  // crBackground - Editor background
  0x002A6C,  // crFrame - Editor frame
  0x00B5F2,  // crFrameFocus - Editor frame when focused
  0x1000FF,  // crCursor - Cursor color
  0x40A0A0,  // crText - Editor text
  0x00B5F2,  // crTextFocus - Editor text when focused
  GUI_FONT_24LTH_CHN  // ftText - Editor font
};

static const GEditorPanel::GStyle s_commonEditorPanelStyle = {
  0x031635,  // crBackground - EditorPanel background
  0x002A6C,  // crFrame - EditorPanel frame
  s_commonEditorStyle  // editorStyle
};

//=============================================================================
// GNumRegDialog Resources (§5)
//=============================================================================

// Icon label for register info
static const GLabel::GConfig s_numRegIconLabel = {
  17, 17, 48, 48,  // x, y, w, h
  GLabel::drNone,
  GUI_DRAWMODE_TRANS,
  (GUI_TA_RIGHT | GUI_TA_VCENTER),
  0x031635,  // crBackground
  0xC0C0C0,  // crText
  nullptr,   // ftText
  {nullptr, 0},  // image - TODO: set icon
  0, nullptr
};

// Register name label
static const GLabel::GConfig s_numRegNameLabel = {
  71, 11, 170, 24,
  GLabel::drName,
  GUI_DRAWMODE_TRANS,
  (GUI_TA_RIGHT | GUI_TA_VCENTER),
  0x031635,
  0xC0C0C0,
  GUI_FONT_24LTH_CHN,
  {nullptr, 0},
  0, nullptr
};

// Register max value label
static const GLabel::GConfig s_numRegMaxLabel = {
  185, 39, 95, 16,
  GLabel::drMax,
  GUI_DRAWMODE_TRANS,
  (GUI_TA_RIGHT | GUI_TA_VCENTER),
  0x031635,
  0xC0C0C0,
  GUI_FONT_16_ASCII,
  {nullptr, 0},
  0, nullptr
};

// Register min value label
static const GLabel::GConfig s_numRegMinLabel = {
  71, 39, 95, 16,
  GLabel::drMin,
  GUI_DRAWMODE_TRANS,
  (GUI_TA_LEFT | GUI_TA_VCENTER),
  0x031635,
  0xC0C0C0,
  GUI_FONT_16_ASCII,
  {nullptr, 0},
  0, nullptr
};

// GNumRegDialog label array
static const GLabel::GConfig s_numRegLabels[] = {
  s_numRegIconLabel,
  s_numRegNameLabel,
  s_numRegMaxLabel,
  s_numRegMinLabel
};

// Single editor config for GNumRegDialog (dynamically configured)
static GEditor::GConfig s_numRegEditorConfig = {
  3, 2, 180, 28,  // xEdt, yEdt, wEdt, hEdt
  4, 2, 174, 24,  // xTxt, yTxt, wTxt, hTxt
  GUI_DRAWMODE_TRANS,
  (GUI_TA_RIGHT | GUI_TA_VCENTER)
};

// EditorPanel config for GNumRegDialog
static const GEditorPanel::GConfig s_numRegEditorPanelConfig = {
  71, 60, 210, 32,  // x, y, w, h
  1,  // Count
  &s_numRegEditorConfig
};

// GNumRegDialog main config
static const GDialog::GDialogConfig s_numRegDialogConfig = {
  4,  // labelCount
  s_numRegLabels,
  15, 150, 271, 145,  // kbdX, kbdY, kbdW, kbdH
  &s_commonKeyboardStyle,
  &s_digitalKeyGrid,
  &s_commonEditorPanelStyle,
  &s_numRegEditorPanelConfig
};

//=============================================================================
// GLoginDialog Resources (§6)
//=============================================================================

// Icon label for password
static const GLabel::GConfig s_loginIconLabel = {
  34, 17, 48, 48,
  GLabel::drNone,
  GUI_DRAWMODE_TRANS,
  (GUI_TA_RIGHT | GUI_TA_VCENTER),
  0x031635,
  0xC0C0C0,
  nullptr,
  {nullptr, 0},  // TODO: password icon
  0, nullptr
};

// Title label (dynamic text via GLabel::drTitle)
static const GLabel::GConfig s_loginTitleLabel = {
  93, 10, 180, 24,
  GLabel::drTitle,
  GUI_DRAWMODE_TRANS,
  (GUI_TA_RIGHT | GUI_TA_VCENTER),
  0x031635,
  0xC0C0C0,
  GUI_FONT_24LTH_CHN,
  {nullptr, 0},
  0, nullptr
};

// Hint label
static const GLabel::GConfig s_loginHintLabel = {
  93, 42, 180, 16,
  GLabel::drNone,
  GUI_DRAWMODE_TRANS,
  (GUI_TA_LEFT | GUI_TA_VCENTER),
  0x031635,
  0xC0C0C0,
  GUI_FONT_16_ASCII,
  {nullptr, 0},
  0,  // TODO: idPasswordHint - "Please input 4-digit password"
  nullptr
};

// GLoginDialog label array
static const GLabel::GConfig s_loginLabels[] = {
  s_loginIconLabel,
  s_loginTitleLabel,
  s_loginHintLabel
};

// 4 single-digit editors for password
static GEditor::GConfig s_loginEditors[4] = {
  {3, 2, 44, 28, 4, 2, 38, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {56, 2, 44, 28, 4, 2, 38, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {109, 2, 44, 28, 4, 2, 38, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {162, 2, 44, 28, 4, 2, 38, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)}
};

// EditorPanel config for GLoginDialog
static const GEditorPanel::GConfig s_loginEditorPanelConfig = {
  93, 60, 210, 32,  // x, y, w, h
  4,
  s_loginEditors
};

// GLoginDialog main config
static const GDialog::GDialogConfig s_loginDialogConfig = {
  3,
  s_loginLabels,
  15, 150, 271, 145,
  &s_commonKeyboardStyle,
  &s_digitalKeyGrid,
  &s_commonEditorPanelStyle,
  &s_loginEditorPanelConfig
};

//=============================================================================
// GIPAddressDialog Resources (§7) - Skeleton
//=============================================================================

static const GLabel::GConfig s_ipIconLabel = {
  17, 17, 48, 48, GLabel::drNone, GUI_DRAWMODE_TRANS,
  (GUI_TA_RIGHT | GUI_TA_VCENTER), 0x031635, 0xC0C0C0,
  nullptr, {nullptr, 0}, 0, nullptr
};

static const GLabel::GConfig s_ipTitleLabel = {
  71, 11, 170, 24, GLabel::drTitle, GUI_DRAWMODE_TRANS,
  (GUI_TA_RIGHT | GUI_TA_VCENTER), 0x031635, 0xC0C0C0,
  GUI_FONT_24LTH_CHN, {nullptr, 0}, 0, nullptr
};

static const GLabel::GConfig s_ipLabels[] = {
  s_ipIconLabel,
  s_ipTitleLabel
};

// 4 editors for IP octets (0-255 each)
static GEditor::GConfig s_ipEditors[4] = {
  {3, 2, 44, 28, 4, 2, 38, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {56, 2, 44, 28, 4, 2, 38, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {109, 2, 44, 28, 4, 2, 38, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {162, 2, 44, 28, 4, 2, 38, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)}
};

static const GEditorPanel::GConfig s_ipEditorPanelConfig = {
  71, 60, 210, 32,  // x, y, w, h
  4, s_ipEditors
};

static const GDialog::GDialogConfig s_ipDialogConfig = {
  2, s_ipLabels,
  15, 150, 271, 145,
  &s_commonKeyboardStyle, &s_digitalKeyGrid,
  &s_commonEditorPanelStyle, &s_ipEditorPanelConfig
};

//=============================================================================
// GDatetimeDialog Resources (§8) - Skeleton
//=============================================================================

static const GLabel::GConfig s_datetimeIconLabel = {
  17, 17, 48, 48, GLabel::drNone, GUI_DRAWMODE_TRANS,
  (GUI_TA_RIGHT | GUI_TA_VCENTER), 0x031635, 0xC0C0C0,
  nullptr, {nullptr, 0}, 0, nullptr
};

static const GLabel::GConfig s_datetimeTitleLabel = {
  71, 11, 170, 24, GLabel::drTitle, GUI_DRAWMODE_TRANS,
  (GUI_TA_RIGHT | GUI_TA_VCENTER), 0x031635, 0xC0C0C0,
  GUI_FONT_24LTH_CHN, {nullptr, 0}, 0, nullptr
};

static const GLabel::GConfig s_datetimeLabels[] = {
  s_datetimeIconLabel,
  s_datetimeTitleLabel
};

// 6 editors for datetime (year, month, day, hour, minute, second)
static GEditor::GConfig s_datetimeEditors[6] = {
  {3, 2, 56, 28, 4, 2, 50, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {68, 2, 32, 28, 4, 2, 26, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {109, 2, 32, 28, 4, 2, 26, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {150, 2, 32, 28, 4, 2, 26, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {191, 2, 32, 28, 4, 2, 26, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)},
  {232, 2, 32, 28, 4, 2, 26, 24, GUI_DRAWMODE_TRANS, (GUI_TA_HCENTER | GUI_TA_VCENTER)}
};

static const GEditorPanel::GConfig s_datetimeEditorPanelConfig = {
  71, 60, 270, 32,  // x, y, w, h
  6, s_datetimeEditors
};

static const GDialog::GDialogConfig s_datetimeDialogConfig = {
  2, s_datetimeLabels,
  15, 150, 271, 145,
  &s_commonKeyboardStyle, &s_digitalKeyGrid,
  &s_commonEditorPanelStyle, &s_datetimeEditorPanelConfig
};

//=============================================================================
// Public Accessor Functions
//=============================================================================

const GDialog::GDialogConfig* GetNumRegDialogConfig()
{
  return &s_numRegDialogConfig;
}

const GDialog::GDialogConfig* GetLoginDialogConfig()
{
  return &s_loginDialogConfig;
}

const GDialog::GDialogConfig* GetIPAddressDialogConfig()
{
  return &s_ipDialogConfig;
}

const GDialog::GDialogConfig* GetDatetimeDialogConfig()
{
  return &s_datetimeDialogConfig;
}
//-----------------------------------------------------------------------------

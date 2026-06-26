//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GPMainForm.cpp
 Version     : V1.11
 By          : Silver Grid Technology

 Description : Main operation form implementation.
               Primary data display and measurement screen.

 Date        : 2023.12.05 (V1.10 — original implementation)
              2026.06.25 (V1.11 — added GM_TOUCH touch screen handler)
*/
//-----------------------------------------------------------------------------//-----------------------------------------------------------------------------
#include "GPMainForm.h"

#include "GUI.h"
#include "GForm.h"
#include "GUICntr.h" // MCU: FGUIState, osWaitForever
#include "GUIConf.h"
#include "GUIMisc.h"
#include "GUIPicture.h"
#include "PictureRes.h"
#include "Strings/TextStrs.h"

#include "FontSGRes.h"  // Fonts resource

#include "DevFixed.h"
#include "DevRegs.h"

#ifndef __vmSIMULATOR__
  #include "BoardCtrl.h"
  #include "RamHeap.h"
  #include "RNGen.h"
  #include "gpio.h"
  #include "rtc.h"
  #include <arm_math.h>
#else
  #include <Windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include "GUIMessage.h"
#include "GWinTypes.h"
#include "GFormRegistrar.h"

//=============================================================================
// 
//-----------------------------------------------------------------------------
//
#define PAGES_IN_FORM       1
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION       24

#define WIDTH_EDGE          4

#define HiAAF               2
//-----------------------------------------------------------------------------
// Colors
#define crFormBkg           GUI_MAKE_COLOR(0x00F2F2F2)  
#define crFrameHigh         GUI_WHITE  
#define crFrameDrak         GUI_GRAY_3F

//--------------------------------
// Caption
#define Caption_x1          (0)
#define Caption_y1          (DESKTOP_HEIGHT - HIGHT_CAPTION)
#define Caption_x2          (DESKTOP_WIDTH  - 1)
#define Caption_y2          (DESKTOP_HEIGHT - 1)

#define CapLabel_x1         (Caption_x1 + WIDTH_EDGE)
#define CapLabel_y1         (Caption_y1 + WIDTH_EDGE)
#define CapLabel_x2         (Caption_x2 - WIDTH_EDGE)
#define CapLabel_y2         (Caption_y2 - 1)

#define crCaptionFont       GUI_GRAY_3F  
#define ftCaptionText       GUI_FONT_16LTH_CHN

#define crCaptionBkg        GUI_MAKE_COLOR(0x00D8D8D8)  
#define crCapFrameHi        GUI_GRAY_E7
#define crCapFrameDark      GUI_GRAY_3F

// Clock
#define rtClock_x1          (CapLabel_x1 + WIDTH_EDGE)
#define rtClock_y1          (CapLabel_y1)
#define rtClock_x2          (CapLabel_x1 + 160)
#define rtClock_y2          (CapLabel_y2)

#define crClockFont         crCaptionFont
#define ftClockFont         GUI_FONT_AA4_ASCII16B // ftCaptionText

// Addr
#define rtCommadr_x1        (CapLabel_x2 - 80)
#define rtCommadr_y1        (CapLabel_y1)
#define rtCommadr_x2        (CapLabel_x2 - WIDTH_EDGE)
#define rtCommadr_y2        (CapLabel_y2)

#define crCommAddrFont      crCaptionFont
#define ftCommAddrFont      ftCaptionText

//--------------------------------
// State Map
#define rtStateInfo_x1      (0)
#define rtStateInfo_y1      (0)
#define rtStateInfo_x2      (DESKTOP_WIDTH  - 1)
#define rtStateInfo_y2      (DESKTOP_HEIGHT - HIGHT_CAPTION)

#define rtStateMap_x1       (rtStateInfo_x1 + WIDTH_EDGE)
#define rtStateMap_y1       (rtStateInfo_y1 + WIDTH_EDGE)
#define rtStateMap_x2       (rtStateInfo_x2 - WIDTH_EDGE)
#define rtStateMap_y2       (rtStateInfo_y2 - WIDTH_EDGE)
#define rtStateMap_Width    (rtStateInfo_x2 - rtStateInfo_x1 + 1)
#define rtStateMap_Height   (rtStateInfo_y2 - rtStateInfo_y1 + 1)

#define crStateMapBkg       crFormBkg  
#define crStateMapHi        crFrameHigh
#define crStateMapDark      crFrameDrak

//--------------
// Pictures
// 
#define picACPowerOn        &picACPowerBlue60x72jpg
#define picACPowerOff       &picACPowerGray60x72jpg
#define WIDTH_ACPOWER       60
#define HEIGTH_ACPOWER      72
#define rtACPower_x1        (rtStateMap_x1 + WIDTH_EDGE)
#define rtACPower_y1        (rtStateMap_y1 + WIDTH_EDGE)
#define rtACPower_x2        (rtACPower_x1  + WIDTH_ACPOWER  - 1)
#define rtACPower_y2        (rtACPower_y1  + HEIGTH_ACPOWER - 1)

// 
#define picBreaker          &picBreaker76x80jpg
#define WIDTH_BREAKER       76
#define HEIGTH_BREAKER      80
#define rtBreaker_x1        (rtBreaker_x2  - WIDTH_BREAKER)
#define rtBreaker_y1        (rtStateMap_y1 + WIDTH_EDGE)
#define rtBreaker_x2        (rtStateMap_x2 - WIDTH_EDGE)
#define rtBreaker_y2        (rtBreaker_y1  + HEIGTH_BREAKER - 1)

// 
#define picInverter         &picInverter66x72jpg    //&picInverter80x72jpg
#define WIDTH_INVERTER      66
#define HEIGTH_INVERTER     72
#define rtInverter_x1       (rtACPower_x2  + 24)
#define rtInverter_y1       (rtInverter_y2 - HEIGTH_INVERTER)
#define rtInverter_x2       (rtInverter_x1 + WIDTH_INVERTER  - 1)
#define rtInverter_y2       (rtStateMap_y2 - HEIGHT_LABEL24 - (WIDTH_EDGE * 2))

// 
#define picBatteryNormal    &picBatteryNormal45x72jpg
#define picBatteryCharge    &picBatteryCharge45x72jpg
#define WIDTH_BATTERY       45
#define HEIGTH_BATTERY      72
#define rtBattery_x1        (rtBattery_x2  - WIDTH_BATTERY  + 1)
#define rtBattery_y1        (rtBattery_y2  - HEIGTH_BATTERY + 1)
#define rtBattery_x2        (rtStateMap_x2 - WIDTH_EDGE)
#define rtBattery_y2        (rtInverter_y2)

// 
#define picHeater           picHeater32x32jpg
#define picCooler           picCooler32x32jpg
#define WIDTH_TEMPCTRL      32
#define HEIGHT_TEMPCTRL     32
#define rtTempCtrl_x1       (labelInvTemp_x2 + WIDTH_EDGE)
#define rtTempCtrl_y1       (labelCurrBat_y2 + 1)
#define rtTempCtrl_x2       (rtTempCtrl_x1   + WIDTH_TEMPCTRL  - 1)
#define rtTempCtrl_y2       (rtTempCtrl_y1   + HEIGHT_TEMPCTRL - 1)

//--------------
// path
#define pathACPower_x1      (rtACPower_x2    + 1)
#define pathACPower_y1      (rtACPower_y1    + 20)
#define pathACPower_x2      (pathInverter_x1 - WIDTH_WIRE_HALF)
#define pathACPower_y2      (pathACPower_y1)

#define pathBreaker_x1      (pathInverter_x1 + 25)
#define pathBreaker_y1      (rtBreaker_y1    + 38)
#define pathBreaker_x2      (rtBreaker_x1)
#define pathBreaker_y2      (pathBreaker_y1)

#define pathInverter_x1     (rtInverter_x1  + 18)
#define pathInverter_y1     (rtInverter_y1  - 2) //(rtInverter_y1  + 4)
#define pathInverter_x2     (pathInverter_x1)
#define pathInverter_y2     (pathBreaker_y1 + 25)

#define pathBattery_x1      (rtInverter_x2 + 2)
#define pathBattery_y1      (pathChange_y2)
#define pathBattery_x2      (rtBattery_x1  - 2)
#define pathBattery_y2      (pathBattery_y1)

#define pathChange_x1       (rtACPower_x1 + 30)
#define pathChange_y1       (rtACPower_y2 + 2)
#define pathChange_x2       (pathChange_x1)
#define pathChange_y2       ((rtInverter_y1 + rtInverter_y2) / 2)
#define pathChange_x3       (rtInverter_x1)
#define pathChange_y3       (pathChange_y2)

//--------------
// Wire
#define WIDTH_WIRE          (8)
#define WIDTH_WIRE_HALF     (WIDTH_WIRE / 2)
#define WIDTH_WIRE_SLANT    (2)

// AC Power
//   1            2
//   --------------
//   4            3
#define wireACPower_x1       (pathACPower_x1)
#define wireACPower_y1       (pathACPower_y1 - WIDTH_WIRE_HALF)
#define wireACPower_x2       (pathACPower_x2)
#define wireACPower_y2       (wireACPower_y1)
#define wireACPower_x3       (pathACPower_x2)
#define wireACPower_y3       (wireACPower_y2 + WIDTH_WIRE - 1)
#define wireACPower_x4       (pathACPower_x1)
#define wireACPower_y4       (wireACPower_y3)

// passby state
//   1         2
//   ----------
//   8       7 \
//              \ 3        4
//               -----------
//              6          5
#define wirePassby_x1       (pathACPower_x1)
#define wirePassby_y1       (pathACPower_y1 - WIDTH_WIRE_HALF)
#define wirePassby_x2       (pathACPower_x2 + WIDTH_WIRE_SLANT)
#define wirePassby_y2       (wirePassby_y1)
#define wirePassby_x3       (pathBreaker_x1 + WIDTH_WIRE_SLANT)
#define wirePassby_y3       (pathBreaker_y1 - WIDTH_WIRE_HALF)
#define wirePassby_x4       (pathBreaker_x2)
#define wirePassby_y4       (wirePassby_y3)
#define wirePassby_x5       (wirePassby_x4)
#define wirePassby_y5       (wirePassby_y4  + WIDTH_WIRE - 1)
#define wirePassby_x6       (pathBreaker_x1 - WIDTH_WIRE_SLANT)
#define wirePassby_y6       (wirePassby_y3  + WIDTH_WIRE - 1)
#define wirePassby_x7       (pathACPower_x2 - WIDTH_WIRE_SLANT)
#define wirePassby_y7       (wirePassby_y2  + WIDTH_WIRE - 1)
#define wirePassby_x8       (wirePassby_x1)
#define wirePassby_y8       (wirePassby_y1  + WIDTH_WIRE - 1)

// invert output
//   2 | 3
//     | 
//   1 | 4
#define wireInvert0_x1       (pathInverter_x1 - WIDTH_WIRE_HALF)
#define wireInvert0_y1       (pathInverter_y1 ) //+ WIDTH_WIRE - 3)
#define wireInvert0_x2       (wireInvert0_x1)
#define wireInvert0_y2       (pathInverter_y2)
#define wireInvert0_x3       (wireInvert0_x1  + WIDTH_WIRE - 1)
#define wireInvert0_y3       (wireInvert0_y2)
#define wireInvert0_x4       (wireInvert0_x3)
#define wireInvert0_y4       (pathInverter_y1)

// invert drive
//        3              4
//         ---------------
//       / 6             5
//   2  /
//     | 7
//     | 
//   1 | 8
#define wireInvert1_x1       (pathInverter_x1 - WIDTH_WIRE_HALF)
#define wireInvert1_y1       (pathInverter_y1 ) //+ WIDTH_WIRE - 3)
#define wireInvert1_x2       (wireInvert1_x1)
#define wireInvert1_y2       (pathInverter_y2 - WIDTH_WIRE_SLANT)
#define wireInvert1_x3       (pathBreaker_x1  - WIDTH_WIRE_SLANT)
#define wireInvert1_y3       (pathBreaker_y1  - WIDTH_WIRE_HALF)
#define wireInvert1_x4       (pathBreaker_x2)
#define wireInvert1_y4       (wireInvert1_y3)
#define wireInvert1_x5       (wireInvert1_x4)
#define wireInvert1_y5       (wireInvert1_y4  + WIDTH_WIRE - 1)
#define wireInvert1_x6       (pathBreaker_x1  + WIDTH_WIRE_SLANT)
#define wireInvert1_y6       (wireInvert1_y3  + WIDTH_WIRE - 1)
#define wireInvert1_x7       (wireInvert1_x2  + WIDTH_WIRE - 1)
#define wireInvert1_y7       (pathInverter_y2 + WIDTH_WIRE_SLANT)
#define wireInvert1_x8       (wireInvert1_x7)
#define wireInvert1_y8       (pathInverter_y1)

// Switch frame
#define switchFrame_x1       (pathACPower_x2  - WIDTH_WIRE)
#define switchFrame_y1       (rtACPower_y1)
#define switchFrame_x2       (pathBreaker_x1)
#define switchFrame_y2       (pathInverter_y2 + WIDTH_WIRE)
#define switchFrame_r        (WIDTH_WIRE_HALF)
#define crSwitchFrame        GUI_MAKE_COLOR(0x00FFDEC6)
#define crSwitchBkg          GUI_MAKE_COLOR(0x00FEF4E9)

// battery
//   1            2
//   --------------
//   4            3
#define wireBattery_x1       (pathBattery_x1)
#define wireBattery_y1       (pathBattery_y1 - WIDTH_WIRE_HALF)
#define wireBattery_x2       (pathBattery_x2)
#define wireBattery_y2       (wireBattery_y1)
#define wireBattery_x3       (wireBattery_x2)
#define wireBattery_y3       (wireBattery_y2 + WIDTH_WIRE - 1)
#define wireBattery_x4       (wireBattery_x1)
#define wireBattery_y4       (wireBattery_y3)

// charging
//   1 | 6
//     |
//     | 5        4
//     ------------
//   2            3
#define wrieCharge_x1        (pathChange_x1 - WIDTH_WIRE_HALF)
#define wrieCharge_y1        (pathChange_y1)
#define wrieCharge_x2        (wrieCharge_x1)
#define wrieCharge_y2        (pathChange_y2 + WIDTH_WIRE_HALF - 1)
#define wrieCharge_x3        (pathChange_x3)
#define wrieCharge_y3        (wrieCharge_y2)
#define wrieCharge_x4        (wrieCharge_x3)
#define wrieCharge_y4        (pathChange_y3 - WIDTH_WIRE_HALF)
#define wrieCharge_x5        (pathChange_x2 + WIDTH_WIRE_HALF - 1)
#define wrieCharge_y5        (wrieCharge_y4)
#define wrieCharge_x6        (pathChange_x1 + WIDTH_WIRE_HALF - 1)
#define wrieCharge_y6        (wrieCharge_y1)

//--------------
// Label
#define ftLabelText24        GUI_FONT_24LTH_CHN
#define ftLabelText16        GUI_FONT_16LTH_CHN
#define crLabelText          GUI_MAKE_COLOR(0x00A03070)
#define crLabelTextAbn       GUI_RED
#define crLabelBkg           crStateMapBkg

#define crVoltTextNor        GUI_RED
#define crVoltTextAbn        GUI_MAKE_COLOR(0x00A03070)

#define HEIGHT_LABEL24         24
#define HEIGHT_LABEL16         16

#define labelCoilSt_x1       (pathBreaker_x1 + 4)                 //(pathBreaker_x1 + WIDTH_WIRE)
#define labelCoilSt_y1       (labelCoilSt_y2 - HEIGHT_LABEL24 + 1)  //(pathBreaker_y2 + WIDTH_WIRE)
#define labelCoilSt_x2       (pathBreaker_x2 - 1)                 //(pathBreaker_x2 - WIDTH_EDGE)
#define labelCoilSt_y2       (pathBreaker_y1 - WIDTH_WIRE)        //(labelCoilSt_y1 + HEIGHT_LABEL24 - 1)

#define labelVoltOut_x1      (pathInverter_x1 + WIDTH_WIRE)       //(pathBreaker_x1  + WIDTH_WIRE)
#define labelVoltOut_y1      (switchFrame_y2  + WIDTH_EDGE)       //(labelVoltOut_y2 - HEIGHT_LABEL24)
#define labelVoltOut_x2      (labelVoltOut_x1 + 70)               //(pathBreaker_x2  - WIDTH_EDGE)
#define labelVoltOut_y2      (labelVoltOut_y1 + HEIGHT_LABEL24 - 1) //(pathBreaker_y2  - WIDTH_WIRE + 1)

#define labelVoltIn_x1       (rtACPower_x1)
#define labelVoltIn_y1       (pathChange_y2  + WIDTH_WIRE)
#define labelVoltIn_x2       (pathChange_x3  - WIDTH_EDGE)
#define labelVoltIn_y2       (labelVoltIn_y1 + HEIGHT_LABEL24 - 1)

#define labelVoltBat_x1      (labelVoltBat_x2 - 66)
#define labelVoltBat_y1      (labelVoltBat_y2 - HEIGHT_LABEL24 + 1)
#define labelVoltBat_x2      (pathBattery_x2  - WIDTH_EDGE)
#define labelVoltBat_y2      (pathBattery_y2  - WIDTH_WIRE + 1)

#define labelCurrBat_x1      (labelCurrBat_x2 - 66)
#define labelCurrBat_y1      (pathBattery_y2  + WIDTH_WIRE - 1)
#define labelCurrBat_x2      (pathBattery_x2  - WIDTH_EDGE)
#define labelCurrBat_y2      (labelCurrBat_y1 + HEIGHT_LABEL24 - 1)

#define crLabelTempNor       GUI_MAKE_COLOR(0x0050B000)
#define crLabelTempAbn       GUI_RED

#define labelInvTemp_x1      (rtInverter_x1)    //(labelInvTemp_x2 - 80)
#define labelInvTemp_y1      (rtInverter_y2   + WIDTH_EDGE)
#define labelInvTemp_x2      (rtInverter_x2)
#define labelInvTemp_y2      (labelInvTemp_y1 + HEIGHT_LABEL16)

#define crLabelCapNor        GUI_MAKE_COLOR(0x0050B000)
#define crLabelCapLow        GUI_RED

#define labelCapBat_x1       (rtBattery_x1)     //(labelCapBat_x2  - 70)
#define labelCapBat_y1       (rtBattery_y2    + WIDTH_EDGE)
#define labelCapBat_x2       (rtBattery_x2)
#define labelCapBat_y2       (labelCapBat_y1  + HEIGHT_LABEL16)

#ifndef fabs
  #define fabs(a) ((a) >= 0 ? (a) : (-(a)))
#endif
    //=============================================================================
// Data
//-----------------------------------------------------------------------------
typedef struct tagStateImage
{
  const TGUIPicture* pImages[2];
  
  uint16_t           uwLeft, uwTop;
  
  uint32_t           uStateReg;
  
  int                iThreshold;
} TStateImage;
//-----------------------------------------------------------------------------
typedef struct tagStateLabel
{

  GUI_RECT           Rect;
  const GUI_FONT*    Font;
  const GUI_COLOR    crBackground, 
                     crSt0Text,            // when vakue > fThreshold
                     crSt1Text;
  
  uint32_t           uStateReg;
  uint32_t           uCoeff;
  float              fThreshold;
  
  uint32_t           uFormatId;
  int                uTextAlign;
} TStateLabel;
//=============================================================================
// Data
//-----------------------------------------------------------------------------
// State
const TStateImage StateImages[] =
{
  { { picACPowerOff, picACPowerOn },  // pImages
    rtACPower_x1, rtACPower_y1,       // Left/Top
    REG_RL_Uin,                       // uStateReg
    MIN_Volt                          // iTihreshold
  }
 ,{ { picBreaker, nullptr },          // pImages
    rtBreaker_x1, rtBreaker_y1,       // Left/Top
    0,                                // fuStateReg
    0                                 // iThreshold
  }
 ,{ { picInverter, nullptr },         // pImages
    rtInverter_x1, rtInverter_y1,     // Left/Top
    0,                                // uStateReg
    0                                 // iThreshold
  }
 ,{ { picBatteryNormal, picBatteryCharge }, // pImages
    rtBattery_x1, rtBattery_y1,       // Left/Top
    REG_RL_BCHRG_Ibus_Max,            // uStateReg
    5                                 // iThreshold --mA
  }
};
#define  IDX_BatteryState   3
#define  NUM_StateImages    NUM_Elements(StateImages)
//-----------------------------------------------------------------------------
// State
const TStateLabel StateLables[] =
{
  // Coil State
  { {labelCoilSt_x1, labelCoilSt_y1, labelCoilSt_x2, labelCoilSt_y2},
    ftLabelText24,                    // Font
    crLabelBkg,                       // crBackground
    crLabelText,                      // crSt0Text when > fThreshold
    crLabelTextAbn,                   // crSt1Text when < fThreshold
    0,                                // uStateReg
    0,                                // uCoeff
    0,                                // fThreshold
    0,                                // pFormatId
    GUI_TA_RIGHT | GUI_TA_VCENTER     // uTextAlign
  }
  // Output voltage
 ,{ {labelVoltOut_x1, labelVoltOut_y1, labelVoltOut_x2, labelVoltOut_y2},
    ftLabelText24,                    // Font
    crLabelBkg,                       // crBackground
    crVoltTextNor,                    // crSt0Text when > fThreshold
    crVoltTextAbn,                    // crSt1Text when < fThreshold
    REG_RL_Uout,                      // uStateReg
    1,                                // uCoeff
    150.0,                            // fThreshold
    idMainFmSt41,                     // uFormatId
    GUI_TA_LEFT | GUI_TA_VCENTER   // uTextAlign
  }
  // Input voltage
 ,{ {labelVoltIn_x1, labelVoltIn_y1, labelVoltIn_x2, labelVoltIn_y2},
    ftLabelText24,                    // Font
    crLabelBkg,                       // crBackground
    crVoltTextNor,                    // crSt0Text when > fThreshold
    crVoltTextAbn,                    // crSt1Text when < fThreshold
    REG_RL_Uin,                       // uStateReg
    1,                                // uCoeff
    150.0,                            // fThreshold
    idMainFmSt41,                     // uFormatId
    GUI_TA_RIGHT | GUI_TA_VCENTER     // uTextAlign
  }
  // Battery voltage
 ,{ {labelVoltBat_x1, labelVoltBat_y1, labelVoltBat_x2, labelVoltBat_y2},
    ftLabelText24,                    // Font
    crLabelBkg,                       // crBackground
    GUI_DARKGREEN,                    // crSt0Text when > fThreshold
    GUI_DARKRED,                      // crSt1Text when < fThreshold
    REG_RL_BTOUT_Ubus,                // uStateReg
    1,                                // uCoeff
    12.0,                             // fThreshold
    idMainFmSt41,                     // uFormatId
    GUI_TA_RIGHT | GUI_TA_VCENTER     // uTextAlign
  }
  // Battery release Current
 ,{ {labelCurrBat_x1, labelCurrBat_y1, labelCurrBat_x2, labelCurrBat_y1 + 15},
    GUI_FONT_AA4_ASCII16B,            // Font
    crLabelBkg,                       // crBackground
    GUI_RED,                          // crSt0Text when > fThreshold
    crVoltTextAbn,                    // crSt1Text when < fThreshold
    REG_RL_BTOUT_Ibus_Max,            // uStateReg
    1000,                             // uCoeff
    50,                               // fThreshold mA
    idMainFmSt42,                     // uFormatId
    GUI_TA_RIGHT | GUI_TA_VCENTER     // uTextAlign 
  }
  // Battery charge Current
 ,{ {labelCurrBat_x1, labelCurrBat_y1 + 15, labelCurrBat_x2, labelCurrBat_y1 + 30},
    GUI_FONT_AA4_ASCII16B,            // Font
    crLabelBkg,                       // crBackground
    GUI_DARKGREEN,                    // crSt0Text when > fThreshold
    crVoltTextAbn,                    // crSt1Text when < fThreshold
    REG_RL_BCHRG_Ibus_Max,            // uStateReg
    1000,                             // uCoeff
    5.0,                              // fThreshold  mA
    idMainFmSt42,                     // uFormatId
    GUI_TA_RIGHT | GUI_TA_VCENTER     // uTextAlign
  }
  // 
 ,{ {labelInvTemp_x1, labelInvTemp_y1, labelInvTemp_x2, labelInvTemp_y2},
    ftLabelText16,                    // Font
    crLabelBkg,                       // crBackground
    GUI_RED,                          // crSt0Text when > fThreshold
    GUI_DARKGREEN,                    // crSt1Text when < fThreshold
    REG_RL_BAT_TEMPERATRUE,           // uStateReg
    1,                                // uCoeff
    50.0,                             // fThreshold
    idMainFmSt43,                     // uFormatId
    GUI_TA_HCENTER | GUI_TA_VCENTER   // uTextAlign
  }
  // 
 ,{ {labelCapBat_x1, labelCapBat_y1, labelCapBat_x2, labelCapBat_y2},
    GUI_FONT_AA4_ASCII16B,            // Font
    crLabelBkg,                       // crBackground
    GUI_DARKGREEN,                    // crSt0Text when > fThreshold
    GUI_RED,                          // crSt1Text when < fThreshold
    REG_RL_BCHRG_Level,               // uStateReg
    1,                                // uCoeff
    20.0,                             // fThreshold
    idMainFmSt44,                     // uFormatId
    GUI_TA_HCENTER | GUI_TA_VCENTER   // uTextAlign
  }
};
#define  NUM_StateLabels    NUM_Elements(StateLables)
//-----------------------------------------------------------------------------
// 
const GUI_POINT ptWireACPower[] =
{
  { wireACPower_x1 * HiAAF, wireACPower_y1 * HiAAF }
 ,{ wireACPower_x2 * HiAAF, wireACPower_y2 * HiAAF }
 ,{ wireACPower_x3 * HiAAF, wireACPower_y3 * HiAAF }
 ,{ wireACPower_x4 * HiAAF, wireACPower_y4 * HiAAF }
};
#define NUM_ptWireACPower   NUM_Elements(ptWireACPower)

const GUI_POINT ptWirePassby[] =
{
  { wirePassby_x1 * HiAAF,  wirePassby_y1 * HiAAF }
 ,{ wirePassby_x2 * HiAAF,  wirePassby_y2 * HiAAF }
 ,{ wirePassby_x3 * HiAAF,  wirePassby_y3 * HiAAF }
 ,{ wirePassby_x4 * HiAAF,  wirePassby_y4 * HiAAF }
 ,{ wirePassby_x5 * HiAAF,  wirePassby_y5 * HiAAF }
 ,{ wirePassby_x6 * HiAAF,  wirePassby_y6 * HiAAF }
 ,{ wirePassby_x7 * HiAAF,  wirePassby_y7 * HiAAF }
 ,{ wirePassby_x8 * HiAAF,  wirePassby_y8 * HiAAF }
};
#define NUM_ptWirePassby    NUM_Elements(ptWirePassby)

const GUI_POINT ptWireInverterOut[] =
{
  {wireInvert0_x1 * HiAAF,  wireInvert0_y1 * HiAAF }
 ,{wireInvert0_x2 * HiAAF,  wireInvert0_y2 * HiAAF }
 ,{wireInvert0_x3 * HiAAF,  wireInvert0_y3 * HiAAF }
 ,{wireInvert0_x4 * HiAAF,  wireInvert0_y4 * HiAAF }
};
#define NUM_ptWireInverterOut    NUM_Elements(ptWireInverterOut)

const GUI_POINT ptWireInverterDrv[] =
{
  { wireInvert1_x1 * HiAAF,  wireInvert1_y1 * HiAAF }
 ,{ wireInvert1_x2 * HiAAF,  wireInvert1_y2 * HiAAF }
 ,{ wireInvert1_x3 * HiAAF,  wireInvert1_y3 * HiAAF }
 ,{ wireInvert1_x4 * HiAAF,  wireInvert1_y4 * HiAAF }
 ,{ wireInvert1_x5 * HiAAF,  wireInvert1_y5 * HiAAF }
 ,{ wireInvert1_x6 * HiAAF,  wireInvert1_y6 * HiAAF }
 ,{ wireInvert1_x7 * HiAAF,  wireInvert1_y7 * HiAAF }
 ,{ wireInvert1_x8 * HiAAF,  wireInvert1_y8 * HiAAF }
 };
#define NUM_ptWireInverterDrv    NUM_Elements(ptWireInverterDrv)
  
const GUI_POINT ptWireBattery[] =
{
  {wireBattery_x1 * HiAAF,  wireBattery_y1 * HiAAF }
 ,{wireBattery_x2 * HiAAF,  wireBattery_y2 * HiAAF }
 ,{wireBattery_x3 * HiAAF,  wireBattery_y3 * HiAAF }
 ,{wireBattery_x4 * HiAAF,  wireBattery_y4 * HiAAF } 
};
#define NUM_ptWireBattery   NUM_Elements(ptWireBattery)

const GUI_POINT ptWireAdapter[] =
{

  { wrieCharge_x1 * HiAAF,  wrieCharge_y1 * HiAAF }
 ,{ wrieCharge_x2 * HiAAF,  wrieCharge_y2 * HiAAF }
 ,{ wrieCharge_x3 * HiAAF,  wrieCharge_y3 * HiAAF }
 ,{ wrieCharge_x4 * HiAAF,  wrieCharge_y4 * HiAAF }
 ,{ wrieCharge_x5 * HiAAF,  wrieCharge_y5 * HiAAF }
 ,{ wrieCharge_x6 * HiAAF,  wrieCharge_y6 * HiAAF }
 };
#define NUM_ptWireAdapter    NUM_Elements(ptWireAdapter)
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// FormStateData
typedef struct tagFormState
{
  uint32_t     uNextTick,
               uMsgIdx;
  uint32_t     uPageIdx;

  struct 
  {
    const TGUIPicture* pImage;
    uint32_t           uTickCnt;
  } Images[NUM_StateImages + 2];

  struct 
  {
    union {
      int      iValue;
      float    fValue;
    }          Value;
    
    uint32_t   uTickCnt;
  } labels[NUM_StateLabels];

  struct
  {
    union {
      uint32_t uState;
      void*    pState;
    }          State;

    uint32_t   uTickCnt;
  } Wires[4];
  
} TMainFormState;
//-----------------------------------------------------------------------------
static TMainFormState  *FpFormState = nullptr;

#define INTEVAL_UPDATE   1000
//=============================================================================
// 
//-----------------------------------------------------------------------------
// Page 1
//-----------------------------------------------------------------------------
static void _updatePage1Caption( bool bAll )
{

  GUI_SetTextMode( GUI_TEXTMODE_NORMAL );
  GUI_SetBkColor ( crCaptionBkg );

  char szStr[32];
  if( true == bAll )
    {
    // Caption
    GUI_SetColor( crCaptionBkg );
    GUI_FillRect( Caption_x1, Caption_y1, Caption_x2, Caption_y2 );
    
    GUI_SetFont ( ftCommAddrFont );
    GUI_SetColor( crCommAddrFont );
    
    // 
    GUI_RECT aRect = { rtCommadr_x1 + WIDTH_EDGE, rtCommadr_y1, 
                       rtCommadr_x2,              rtCommadr_y2 };
    
    const char* szFmtStr = GetMultiLangString( idMainFmSt31 );
    if( nullptr == szFmtStr )
      szFmtStr = "Dev:%d";                 
    sprintf( szStr, szFmtStr, GET_UARTOPT(0)->Addr );
    GUI_DispStringInRect( szStr, &aRect, GUI_TA_CENTER | GUI_TA_VCENTER );
    }

  // --------------- 
#ifndef __vmSIMULATOR__
  TDateTimeType dtNow;
  RTC_GetTime( &dtNow );
  sprintf( szStr, "%04u-%02u-%02u %02u:%02u:%02u", 
           dtNow.Year, 
           dtNow.Month, 
           dtNow.Day, 
           dtNow.Hours, 
           dtNow.Minutes, 
           dtNow.Seconds );
#else
    SYSTEMTIME stLocal;

    // Get
    GetLocalTime(&stLocal);
    //// Get UTC 
    // GetSystemTime(&stUTC);

  sprintf(szStr, "%04u-%02u-%02u %02u:%02u:%02u ",
        stLocal.wYear,
        stLocal.wMonth,
        stLocal.wDay,
        stLocal.wHour,
        stLocal.wMinute,
        stLocal.wSecond );
#endif
  GUI_SetColor( crClockFont );
  GUI_SetFont ( ftClockFont );

  GUI_RECT cRect = { rtClock_x1, rtClock_y1, rtClock_x2, rtClock_y2 };
  GUI_DispStringInRect( szStr, &cRect, GUI_TA_LEFT | GUI_TA_VCENTER );
}
//-----------------------------------------------------------------------------
static void _updatePage1Wires( bool bAll )
{
  
  GUI_AA_EnableHiRes();
  GUI_AA_SetFactor( HiAAF );  // AAl

  GUI_SetBkColor ( crStateMapBkg );

  auto uVoltIn  = (uint32_t)_GetRealReg(REG_RL_Uin);
  auto uVoltOut = (uint32_t)_GetRealReg(REG_RL_Uout);
  GUI_COLOR crInverter;
  if( MIN_Volt < uVoltOut )
    crInverter = GUI_RED;
  else
    crInverter = GUI_GRAY_AA;

  //--------------------
#ifndef __vmSIMULATOR__
  // 
  auto iPassby  = BoardCtrl_GetPassby()? 0xAA00 : 0;
#else
  auto iPassby = 0xAA00;
#endif
  if( MIN_Volt < uVoltIn )
    iPassby |= 0x55;

  if( true == bAll || 
      iPassby != FpFormState->Wires[0].State.uState )
    {
    // Switch
#ifdef crSwitchBkg
    GUI_SetColor    ( crSwitchBkg );
    GUI_FillRoundedRect( switchFrame_x1, switchFrame_y1, 
                         switchFrame_x2, switchFrame_y2,
                         switchFrame_r  );
#endif
#ifdef crSwitchFrame
    GUI_SetColor    ( crSwitchFrame );
    GUI_SetPenSize  ( 1 * HiAAF );
    GUI_AA_DrawRoundedRect( switchFrame_x1 * HiAAF, switchFrame_y1 * HiAAF, 
                            switchFrame_x2 * HiAAF, switchFrame_y2 * HiAAF,
                            switchFrame_r  * HiAAF );
#endif

#ifndef crSwitchBkg
    GUI_SetColor ( crStateMapBkg );
#endif
    if( 0xAA00 == (iPassby & 0xFF00) )
      {
      // 
#ifndef crSwitchBkg
      // State
      GUI_AA_FillPolygon( ptWireInverterDrv, NUM_ptWireInverterDrv, 0, 0 );
#endif

      // Start
      GUI_SetColor ( crInverter );
      GUI_AA_FillPolygon( ptWireInverterOut, NUM_ptWireInverterOut, 0, 0 );
      FpFormState->Wires[1].State.uState = crInverter;

      // State
      if( MIN_Volt < uVoltIn )
        GUI_SetColor ( GUI_RED );
      else
        GUI_SetColor ( GUI_GRAY_AA );
      GUI_AA_FillPolygon( ptWirePassby, NUM_ptWirePassby, 0, 0 );
      }
    else
      {
#ifndef crSwitchBkg
      // State
      GUI_AA_FillPolygon( ptWirePassby, NUM_ptWirePassby, 0, 0 );
#endif

      // State
      if( MIN_Volt < uVoltIn )
        GUI_SetColor ( GUI_RED );
      else
        GUI_SetColor ( GUI_GRAY_AA );
      GUI_AA_FillPolygon( ptWireACPower, NUM_ptWireACPower, 0, 0 );

      // State
      GUI_SetColor ( crInverter );
      GUI_AA_FillPolygon( ptWireInverterDrv, NUM_ptWireInverterDrv, 0, 0 );
      
      FpFormState->Wires[1].State.uState = crInverter;
      }

    GUI_AA_FillCircle( pathBreaker_x1 * HiAAF,  pathBreaker_y1 * HiAAF,
                       WIDTH_WIRE * HiAAF );

    // State
    FpFormState->Wires[0].State.uState = iPassby;
    }

  //--------------------
  // 
  if( FpFormState->Wires[1].State.uState != crInverter )
    {
    GUI_SetColor ( crInverter );
    if( 0xAA55 == iPassby )
      {
      GUI_AA_FillPolygon( ptWireInverterOut, NUM_ptWireInverterOut, 0, 0 );
      }
    else
      {
      GUI_AA_FillPolygon( ptWireInverterDrv, NUM_ptWireInverterDrv, 0, 0 );

      GUI_AA_FillCircle( pathBreaker_x1 * HiAAF,  pathBreaker_y1 * HiAAF,
                         WIDTH_WIRE * HiAAF );
      }
      
    FpFormState->Wires[1].State.uState = crInverter;
    }
    
  //--------------------
  // 
  uint32_t uInAcValided = (MIN_Volt < uVoltIn)? 0x33AA : 0;
  if( true == bAll || 
      uInAcValided != FpFormState->Wires[2].State.uState )
    {
    // State
    if( 0 != uInAcValided )
      GUI_SetColor ( GUI_RED );
    else
      GUI_SetColor ( GUI_GRAY_AA );
    GUI_AA_FillPolygon( ptWireAdapter, NUM_ptWireAdapter, 0, 0 );
    
    // StateUpdateState
    if( 0xAA55 != iPassby )
      GUI_AA_FillPolygon( ptWireACPower, NUM_ptWireACPower, 0, 0 );

    // Updateowtate
    FpFormState->Wires[2].State.uState = uInAcValided;
    }
  
  //--------------------
  // 
  auto uBatDrvCurr = (uint32_t)_GetRealReg( REG_RL_BTOUT_Ibus_Max  );
  auto uBatLevel   = (uint32_t)_GetRealReg( REG_RL_BCHRG_Level );
  auto uBatVolt    = (uint32_t)_GetRealReg( REG_RL_BCHRG_Ubus  );
  uint32_t uBatSta = 0;
  if( 10 > uBatVolt )           // 
    uBatSta = 0xE0E0;
  else if( 100 < uBatDrvCurr )  // State > 100mA
    uBatSta = 0x33AA;
  else if( 98 <= uBatLevel)     // 
    uBatSta = 0x55CC;
  if( true == bAll || 
      uBatSta != FpFormState->Wires[3].State.uState )
    {
    // State
    switch( uBatSta )
      {
      case 0x33AA:  // State
        GUI_SetColor ( GUI_RED );
        break;
      case 0x55CC:  // State
        GUI_SetColor ( GUI_DARKGREEN );
        break;
      case 0xE0E0:  // None
        GUI_SetColor ( GUI_GRAY_AA );
        break;
      default:      // State
        GUI_SetColor ( GUI_DARKBLUE );
        break;
      }
    GUI_AA_FillPolygon( ptWireBattery, NUM_ptWireBattery, 0, 0 );

    // State
    FpFormState->Wires[3].State.uState = uBatSta;
    }

  GUI_AA_DisableHiRes();
}
//-----------------------------------------------------------------------------
// 
static bool _currentValue2Str( bool bForce, char* pBuf, float rValue, float rRecent )
{
  
  bool bRedraw = false;
  
  auto uDeltaValue = (uint32_t)(fabs(rRecent - rValue) * 10000 + 0.5f);
  if( 1000 < rValue )
    {
    if( true == bForce || 9999 < uDeltaValue )
      {
      bRedraw = true;
      sprintf( pBuf, "%0.3fA", rValue / 1000 );
      }
    }
  else if( 99.94f < rValue )
    {
    if( true == bForce || 999 < uDeltaValue )
      {
      bRedraw = true;
      sprintf( pBuf, "%0.1fmA", rValue );
      }
    }
  else if( 9.994f < rValue )
    {
    if( true == bForce || 99 < uDeltaValue )
      {
      bRedraw = true;
      sprintf( pBuf, "%0.2fmA", rValue );
      }
    }
  else if( 4.999f < rValue )  // Display
    {
    if( true == bForce || 9 < uDeltaValue )
      {
      bRedraw = true;
      sprintf( pBuf, "%0.3fmA", rValue );
      }
    }
  else
    {
    if( true == bForce || 9 < uDeltaValue )
      {
      bRedraw = true;
      sprintf( pBuf, "0.0A" );
      }
    }
    
  return bRedraw;
}
//-----------------------------------------------------------------------------
static void _updatePage1Labels( bool bAll )
{
  
  char szStr[16];
  
  GUI_SetTextMode( GUI_TM_TRANS );

  // DisplayStateData
  auto pState = FpFormState->labels;
  for( const auto& item : StateLables )
    {
    if( 0 == item.uStateReg )
      {
      pState++;
      continue;
      }
      
    auto  rValue = _GetRealReg( item.uStateReg );

    bool bRedraw = bAll;
    if( 1000 <= item.uCoeff )
      {
      // 
      bRedraw = _currentValue2Str( bAll, szStr, rValue, pState->Value.fValue );
      }
    else
      {
      if( 1 < item.uCoeff )
        rValue /= item.uCoeff;
    
      if( 0.1 < fabs(pState->Value.fValue - rValue) )
        bRedraw = true;
      }
    
    if( true == bRedraw )
      {
      pState->Value.fValue = rValue;

      GUI_SetColor  ( item.crBackground );
      GUI_FillRectEx( &(item.Rect) );

#ifdef __DEBUG
      // test only
      GUI_SetPenSize(1);
      GUI_SetColor  ( GUI_GRAY_E7 );
      GUI_DrawRectEx( &(item.Rect) );
#endif

      GUI_SetFont( item.Font );
      if( item.fThreshold < rValue )
        GUI_SetColor( item.crSt0Text );
      else
        GUI_SetColor( item.crSt1Text );
      
      if( 1000 <= item.uCoeff )  // 
        {
        GUI_DispStringInRect( szStr, 
                              (GUI_RECT*)&(item.Rect), 
                              item.uTextAlign );
        }
      else
        {
        const char* pcFmtStr = GetMultiLangString( item.uFormatId );
        if( nullptr != pcFmtStr )
          {
          sprintf( szStr, pcFmtStr, rValue );
          GUI_DispStringInRect( szStr, 
                                (GUI_RECT*)&(item.Rect), 
                                item.uTextAlign );
          }
        }
      }

    pState++;
    }
    
  // 
  auto iValue = GetCoilState();
  pState = &FpFormState->labels[0];
  if( true == bAll ||
      pState->Value.iValue != iValue )
    {
    uint32_t   uStrId;
    GUI_COLOR  crText;
    switch( iValue )
      {
      case COIL_Stop:
        uStrId = idMainFmSt21; //"Close";
        crText = GUI_GRAY_50;
        break;
      case COIL_Startup:
        uStrId = idMainFmSt22; //"Start"
        crText = GUI_DARKGREEN;
        break;
      case COIL_Monitor:
        uStrId = idMainFmSt23; //""
        crText = GUI_DARKGREEN;
        break;
      case COIL_KeppOn:
        uStrId = idMainFmSt24; //""
        crText = GUI_RED;
        break;
      case COIL_KeppOff:
        uStrId = idMainFmSt25; //"";
        crText = GUI_RED;
        break;
      case COIL_PassBy:
        uStrId = idMainFmSt26; //"";
        crText = GUI_RED;
        break;
      case COIL_ShutDownDelay:
        uStrId = idMainFmSt27; //"";
        crText = GUI_DARKBLUE;
        break;
      case COIL_ShutDown:
        uStrId = idMainFmSt28; //"";
        crText = GUI_GRAY_7C;
        break;
      case COIL_Fatal:
        uStrId = idMainFmSt29; //"";
        crText = GUI_RED;
        break;
      default:
        uStrId = 0;
        break;
      }
    
    const char* pcStr = GetMultiLangString( uStrId );
    if( nullptr != pcStr )
      {
      const auto& label = StateLables[0];
        
      GUI_SetFont( label.Font );
      GUI_SetBkColor( label.crBackground );
      GUI_SetColor  ( label.crBackground );
      GUI_FillRectEx( &(label.Rect) );

#ifdef __DEBUG
      // test only
      GUI_SetPenSize(1);
      GUI_SetColor  ( GUI_GRAY_E7 );
      GUI_DrawRectEx( &(label.Rect) );
#endif
      GUI_SetColor  ( crText );
      GUI_DispStringInRect( pcStr, 
                            (GUI_RECT*)&label.Rect, 
                            label.uTextAlign );
      }
      
    pState->Value.iValue = iValue;
    }
}
//-----------------------------------------------------------------------------
static void _updatePage1StateImage( bool bAll )
{

  // DisplayState
  auto pState = FpFormState->Images;
  for( const auto& item : StateImages )
    {
    const TGUIPicture* pImage = nullptr;
      
    if( 0 == item.uStateReg )
      {
      if( true == bAll )
        pImage = item.pImages[0];
      }
    else
      {
      auto iValue  = (int)_GetRealReg( item.uStateReg );
      if( iValue < item.iThreshold )
        pImage = item.pImages[0];
      else
        pImage = item.pImages[1];
      
      if( pState->pImage != pImage || true == bAll )
        {
        if( false == bAll && 5 > ++pState->uTickCnt )
          {
          pImage = nullptr;
          }
          
        if( nullptr != pImage )
          pState->pImage = pImage;
        }
      else
        pImage = nullptr;
      }

    if( nullptr != pImage )
      {
      GUI_DrawPicture( pImage, item.uwLeft, item.uwTop );
        
      pState->uTickCnt = 0;
      }
      
    pState++;
    }
    
  // 
  pState = FpFormState->Images + NUM_StateImages;
  if( RHF_ExADC1_ERR == GetHWFault( RHF_ExADC1_ERR ) ||
      RHF_ExADC2_ERR == GetHWFault( RHF_ExADC2_ERR ) )
    {
    const TGUIPicture* pImage = &picForbidMark32x32C565bmp;
      
    if( pImage != pState->pImage )
      {
      const TGUIBitmap* pBitmap = (const TGUIBitmap*)(pImage->pData);
      auto x = (rtBattery_x1 + rtBattery_x2) / 2 - pBitmap->XSize / 2,
           y = (rtBattery_y1 + rtBattery_y2) / 2 - pBitmap->YSize / 2;
      GUI_DrawPicture( pImage, x, y );

      pState->pImage = pImage;
      }
    }
  else if( nullptr != pState->pImage )
    {
    pState->pImage = nullptr;
      
    const auto& item = StateImages[IDX_BatteryState];

    // None
    const TGUIPicture* pImage;
    auto iValue  = (int)_GetRealReg( item.uStateReg );
    if( iValue < item.iThreshold )
      pImage = item.pImages[0];
    else
      pImage = item.pImages[1];
    
    if( true == bAll || pState->pImage != pImage )
      pState->pImage = pImage;
    else
      pImage = nullptr;

    if( nullptr != pImage )
      {
      GUI_DrawPicture( pImage, item.uwLeft, item.uwTop );
      }
    }

    // Process
  const TGUIPicture* pImage;
#ifndef __vmSIMULATOR__
    if (0 != BoardCtrl_GetHeater())
    pImage = &picHeater;
  else if( 0 != BoardCtrl_GetFanControl() )
    pImage = &picCooler;
  else
    pImage = nullptr;
#else
    pImage = &picHeater;
#endif
  
  pState = FpFormState->Images + NUM_StateImages + 1;
  if( pImage != pState->pImage )
    {
    if( nullptr == pImage )
      {
      GUI_SetColor( crStateMapBkg );
      GUI_FillRect( rtTempCtrl_x1, rtTempCtrl_y1, rtTempCtrl_x2, rtTempCtrl_y2);
      }
    else
      {
      GUI_DrawPicture( pImage, rtTempCtrl_x1, rtTempCtrl_y1 );
      }
    
    pState->pImage = pImage;
    }
}
//-----------------------------------------------------------------------------
static void _updatePage1Statemap( bool bAll )
{

#ifndef __vmSIMULATOR__
    osDelay(10);
#endif
  
  _updatePage1StateImage( bAll );
}
//-----------------------------------------------------------------------------
static void _updatePage1( bool bAll )
{

  // update Caption
  _updatePage1Caption( bAll );

  // update State map pictures
  _updatePage1Statemap( bAll );
  
  // udpate voltage display
  _updatePage1Labels( bAll );
  
  // Update Connections
  _updatePage1Wires( bAll );

  // Display
  _SetRealReg( REG_RL_BCHRG_Ibus_Max, 0 );
  _SetRealReg( REG_RL_BTOUT_Ibus_Max, 0 );
}
//-----------------------------------------------------------------------------
static void _showPage1()
{
  
  _updatePage1( true );
}
////-----------------------------------------------------------------------------
////-----------------------------------------------------------------------------
//// Page 2
////-----------------------------------------------------------------------------
//static void _showPage2()
//{

//  GUI_SetColor( crFrameHigh );
//  GUI_RECT Rect = { CapLabel_x1, CapLabel_y1, CapLabel_x2, CapLabel_y2 };
//  GUI_DrawRectEx( &Rect );

//  GUI_SetColor( crCaptionFont );
//  GUI_SetFont( GUI_FONT_24LTH_CHN );
//  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
//  GUI_DispStringInRect( "", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
//  
//  // Browser
//  GUI_SetColor( crFrameHigh );
//  GUI_DrawRect( rtBrowser_x1,  rtBrowser_y1, rtBrowser_x2,  rtBrowser_y2 );
//}
////-----------------------------------------------------------------------------
//static void _updatePage2( bool bAll )
//{
//}
////-----------------------------------------------------------------------------
//static void _UpdateClock()
//{
//  
//  osMutexWait( FGUIState.mutexGUI, osWaitForever );

//  GUI_SetBkColor( crFormBkg );
//  GUI_SetColor( crClockFont );
//  GUI_SetFont( GUI_FONT_6X8_ASCII );
//  GUI_SetTextMode( GUI_TEXTMODE_NORMAL );

//  osMutexRelease( FGUIState.mutexGUI ); 
//}
//-----------------------------------------------------------------------------
static void _updateForm()
{

//  _UpdateClock();
  
#if PAGES_IN_FORM < 2
      _updatePage1( false );
#else  
  switch( FpFormState->uPageIdx )
    {
    case 0:
      {
      _updatePage1( false );
      break;
      }

//    case 1:
//      {
//      _updatePage2( false );
//      break;
//      }
    }
#endif
}
//-----------------------------------------------------------------------------
static void _showForm()
{

  GUI_SetBkColor( crFormBkg );
  GUI_Clear();

#if PAGES_IN_FORM < 2
      _showPage1();
#else  
  switch( FpFormState->uPageIdx )
    {
    case 0:
      {
      _showPage1();

      break;
      }
      
//    case 1:
//      {
//      _showPage2();
//        
//      _updatePage2( true );

//      break;
//      }
    }
#endif
}
//=============================================================================
// Global methods
//-----------------------------------------------------------------------------
static void _Init(const void* argument)
{
  
  if (nullptr == FpFormState)
#ifndef __vmSIMULATOR__
      FpFormState = (TMainFormState*)RAM_Malloc(sizeof(TMainFormState));
#else
      FpFormState = new TMainFormState;
#endif

#ifdef USE_DEV_ASSERT
   DEV_ASSERT( nullptr == FpFormState, GFC_OutOfMem );
#endif

  memset( FpFormState, 0, sizeof(TMainFormState) );
  
  FpFormState->uNextTick = GUI_GetTime() + INTEVAL_UPDATE;
  
  FpFormState->uMsgIdx  = 0;

  // Display
  _SetRealReg( REG_RL_BCHRG_Ibus_Max, 0 );
  _SetRealReg( REG_RL_BTOUT_Ibus_Max, 0 );
}
//-----------------------------------------------------------------------------
static void _Show(const void* argument)
{

  _showForm();
}
//-----------------------------------------------------------------------------
static void _Close(const void* argument)
{
  
#ifndef __vmSIMULATOR__
    RAM_Free(FpFormState);
#else
    delete FpFormState;
#endif

  FpFormState = nullptr;
}
//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{

  if( FpFormState->uNextTick <= uTick )
    {
    FpFormState->uNextTick = GUI_GetTime() + INTEVAL_UPDATE;
      
    _updateForm();
    }
}
//-----------------------------------------------------------------------------
static void _OnKeyUp(uint16_t uwKey)
{

    switch (uwKey) {
    case KEY_RIGHT: {
#if PAGES_IN_FORM > 1
        if (PAGES_IN_FORM - 1 > FpFormState->uPageIdx) {
            FpFormState->uPageIdx++;
            _showForm();
        }
#else
        gform::PushForm(WID_MenuForm, nullptr);
#endif
        break;
      }

    case KEY_LEFT: {
#if PAGES_IN_FORM > 1
        if (0 < FpFormState->uPageIdx) {
            FpFormState->uPageIdx--;

            _showForm();
        }
#endif
        break;
      }

    case KEY_UP: {
        gform::PushForm(WID_DevInfoForm, nullptr);

        break;
      }

    case KEY_DOWN: {
          gform::PushForm(WID_SplashForm, nullptr);

        break;
      }

    case KEY_ENTER: {
          gform::PushForm(WID_MenuForm, nullptr);
        break;
      }

    case KEY_ESCAPE: {
        gform::PopForm();
        break;
      }
    }
}
//-----------------------------------------------------------------------------
static void _OnMessage(GM_MESSAGE* pMsg)
{
  
  if( 0 == pMsg )
    return ;
  
  switch( pMsg->MsgId )
    {
    case GM_TIMER_TICK:
      { 
      _OnTick( pMsg->Data.v );
        
      break;
      }
      
    //case GM_KEYDOWN:
    //  {
    //  if( pMsg->Param )
    //    {
    //    _OnKeyDown( pMsg->Param );
    //      
    //    pMsg->MsgId = 0;
    //    }
    //    
    //  break;
    //  }

    case GM_KEYUP: {
        if (pMsg->Param) {
            _OnKeyUp(pMsg->Param);

            pMsg->MsgId = 0;
        }

        break;
    }

#if GUI_SUPPORT_TOUCH
    case GM_TOUCH: {
        // Only act on touch up (avoid accidental navigation on drag)
        if (pMsg->Param == TOUCH_UP) {
            uint16_t x = static_cast<uint16_t>((pMsg->Data.v >> 16) & 0xFFFF);
            uint16_t y = static_cast<uint16_t>(pMsg->Data.v & 0xFFFF);
            (void)y;  // y unused for MainForm zone layout

            // Touch zone layout (320x240):
            //   Left  1/3 (x < 106):  device info
            //   Right 1/3 (x > 213):  menu
            //   Center:                device info
            if (x > 213) {
                gform::PushForm(WID_MenuForm, nullptr);
            } else {
                gform::PushForm(WID_DevInfoForm, nullptr);
            }
        }
        pMsg->MsgId = 0;
        break;
    }
#endif

    //case GM_TIMERECV:
    //  {
    //  if( FGUIState.pCurForm == &FMainForm )
    //    _UpdateClock();
    //  
    //  break;
    //  }
      
    //case GM_DATARECV:
    //  {
    //  
    //  break;
    //  }
    }
}
//=============================================================================
// Form
//-----------------------------------------------------------------------------
const GWinForm FMainForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};

// Auto-register with new GForm system
static const gform::FormRegistrar kRegMain(WID_MainForm, &FMainForm, "Main");
//-----------------------------------------------------------------------------

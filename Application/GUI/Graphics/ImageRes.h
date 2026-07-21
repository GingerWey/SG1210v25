// ------------------------------------------------------------
/*
   ImageRes.h
   CSG image data

   Encoder    name: CSG Toolkits
   Encoder version: 1.7.2627

   By Wey.   Silver Grid 2026
   Time: 2026-07-13 08:11:19
*/
// ------------------------------------------------------------
#ifndef ImageRes_H
#define ImageRes_H

#include "GUIPicture.h"

// ============================================================
#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------------------
// CSG Image: picbkg320x240csg
extern const TGUIPicture picbkg320x240csg;

#define IMAGE_BACKGROUND  &picbkg320x240csg
//===============================================================
/*
CSG atlas: picWINAtlascsg
   No. Image                      resolution     size  CRM       CRN  CAS          Size   ratio  
 ------------------------------------------------------------------------------------------------
   1.  MA_Logo78x18Green.png         78x18       2134  RGB565     8  None           560   26.24%
   2.  MA_Logo80x20Cyan.png          90x20       2189  RGB565     8  None           708   32.34%
   3.  MA_ACPow32x59Cyan.png         32x59       4510  RGB565     8  MiniLZ77       276    6.12%
   4.  MA_CtrlG80x62Cyan.png         80x62       7635  RGB565    16  DEFLATE       1132   14.83%
   5.  MA_CtrlLED13x13Red.png        13x13        280  RGB565     8  None            96   34.29%
   6.  MA_Brkr56x60Cyan.png          56x60       8300  RGB565    16  DEFLATE       1564   18.84%
   7.  MA_Battery32x20C1.png         32x20       1132  RGB565    16  RLE            208   18.37%
   8.  MA_Battery32x20C2.png         32x20       1171  RGB565    16  RLE            220   18.79%
   9.  MA_Battery32x20C3.png         32x20       1178  RGB565    16  RLE            232   19.69%
  10.  MA_Battery32x20C4.png         32x20       1179  RGB565    16  RLE            228   19.34%
  11.  MA_Battery32x20C5.png         32x20       1086  RGB565    16  RLE            184   16.94%
  12.  MA_BatCharge11X13.png         11x13        309  RGB565     8  RLE             84   27.18%
  13.  MA_Menu20x20Cyan.png          20x20        701  RGB565     4  MiniLZ77       112   15.98%
  14.  MA_Fan16x16Cyan.png           16x16        662  RGB565     4  None            88   13.29%
  15.  MA_Fire16x16.png              16x16        672  RGB565    16  RLE            160   23.81%
  16.  MU_Home24x22.png              24x22        868  RGB565     4  None           156   17.97%
  17.  MU_Item32x32_01.png           32x32       1887  RGB565     4  None           280   14.84%
  18.  MU_Item32x32_02.png           32x32       1364  RGB565     4  MiniLZ77       232   17.01%
  19.  MU_Item32x32_03.png           32x32       1592  RGB565     4  MiniLZ77       248   15.58%
  20.  MU_Item32x32_04.png           32x32       1533  RGB565     4  None           280   18.26%
  21.  MU_Item32x32_05.png           32x32       1235  RGB565     4  MiniLZ77       200   16.19%
  22.  MU_Item32x32_06.png           32x32       1755  RGB565     4  None           280   15.95%
  23.  MU_Item32x32_07.png           32x32       2025  RGB565     4  None           280   13.83%
  24.  MU_Item32x32_08.png           32x32       1935  RGB565     4  None           280   14.47%
  25.  MU_Item32x32_09.png           32x32       1299  RGB565     4  MiniLZ77       184   14.16%
  26.  MU_Item32x32_10.png           32x32       1903  RGB565     4  None           280   14.71%
  27.  DL_GroupA20x16Cyan.png        20x16        846  RGB565     4  None           104   12.29%
  28.  DL_GroupB20x16Cyan.png        20x16        459  RGB565     4  None           104   22.66%
  29.  DL_GroupC20x16Cyan.png        20x16        869  RGB565     4  None           104   11.97%
  30.  DL_GroupD20x16Cyan.png        20x16        731  RGB565     4  None           104   14.23%
  31.  DL_GroupE20x16Cyan.png        20x16        386  RGB565     4  MiniLZ77        88   22.80%
  32.  LV_CheckMark16x16Red.png      16x16        494  RGB565     8  RLE            116   23.48%
  33.  LV_CrossMark16x16Green.png    16x16        980  RGB565     8  None           128   13.06%
  34.  LV_SwitchON32x16Red.png       32x14        724  RGB565     4  None           136   18.78%
  35.  LV_SwitchOFF32x16Cyan.png     32x14       1050  RGB565     4  None           136   12.95%
  36.  LV_LogA20x20Cyan.png          20x20        922  RGB565     4  None           124   13.45%
  37.  LV_LogB20x20Cyan.png          20x20        712  RGB565     4  None           124   17.42%
  38.  LV_LogC20x20Cyan.png          20x20        873  RGB565     4  None           124   14.20%
  39.  CF_Edit20x20Cyan.png          20x20       1141  RGB565     4  None           124   10.87%
  40.  CF_Logic16x16Cyan.png         16x16       1056  RGB565     4  None            88    8.33%
  41.  CF_Device16x16Cyan.png        16x16       1163  RGB565     4  None            88    7.57%
  42.  CF_Serial16x16Cyan.png        16x16       1138  RGB565     4  None            88    7.73%
  43.  CF_Ethernet16x16Cyan.png      16x16        618  RGB565     4  None            88   14.24%
 ------------------------------------------------------------------------------------------------
   Total                                        64696                             10420   16.11%
*/
// CSG Atlas: WINAtlas object
extern const TGUIPicture picWINAtlascsg;
#define CSG_WINATLAS   (&picWINAtlascsg)

// CSG Atlas: WINAtlas picture index constants
constexpr int picIdxMA_Logo78x18Green      = 0;
constexpr int picIdxMA_Logo80x20Cyan       = 1;
constexpr int picIdxMA_ACPow32x59Cyan      = 2;
constexpr int picIdxMA_CtrlG80x62Cyan      = 3;
constexpr int picIdxMA_CtrlLED13x13Red     = 4;
constexpr int picIdxMA_Brkr56x60Cyan       = 5;
constexpr int picIdxMA_Battery32x20C1      = 6;
constexpr int picIdxMA_Battery32x20C2      = 7;
constexpr int picIdxMA_Battery32x20C3      = 8;
constexpr int picIdxMA_Battery32x20C4      = 9;
constexpr int picIdxMA_Battery32x20C5      = 10;
constexpr int picIdxMA_BatCharge11X13      = 11;
constexpr int picIdxMA_Menu20x20Cyan       = 12;
constexpr int picIdxMA_Fan16x16Cyan        = 13;
constexpr int picIdxMA_Fire16x16           = 14;
constexpr int picIdxMU_Home24x22           = 15;
constexpr int picIdxMU_Item32x32_01        = 16;
constexpr int picIdxMU_Item32x32_02        = 17;
constexpr int picIdxMU_Item32x32_03        = 18;
constexpr int picIdxMU_Item32x32_04        = 19;
constexpr int picIdxMU_Item32x32_05        = 20;
constexpr int picIdxMU_Item32x32_06        = 21;
constexpr int picIdxMU_Item32x32_07        = 22;
constexpr int picIdxMU_Item32x32_08        = 23;
constexpr int picIdxMU_Item32x32_09        = 24;
constexpr int picIdxMU_Item32x32_10        = 25;
constexpr int picIdxDL_GroupA20x16Cyan     = 26;
constexpr int picIdxDL_GroupB20x16Cyan     = 27;
constexpr int picIdxDL_GroupC20x16Cyan     = 28;
constexpr int picIdxDL_GroupD20x16Cyan     = 29;
constexpr int picIdxDL_GroupE20x16Cyan     = 30;
constexpr int picIdxLV_CheckMark16x16Red   = 31;
constexpr int picIdxLV_CrossMark16x16Green = 32;
constexpr int picIdxLV_SwitchON32x16Red    = 33;
constexpr int picIdxLV_SwitchOFF32x16Cyan  = 34;
constexpr int picIdxLV_LogA20x20Cyan       = 35;
constexpr int picIdxLV_LogB20x20Cyan       = 36;
constexpr int picIdxLV_LogC20x20Cyan       = 37;
constexpr int picIdxCF_Edit20x20Cyan       = 38;
constexpr int picIdxCF_Logic16x16Cyan      = 39;
constexpr int picIdxCF_Device16x16Cyan     = 40;
constexpr int picIdxCF_Serial16x16Cyan     = 41;
constexpr int picIdxCF_Ethernet16x16Cyan   = 42;
//===============================================================
/*
CSG atlas: picDLGAtlascsg
   No. Image                     resolution     size  CRM       CRN  CAS          Size   ratio  
 -----------------------------------------------------------------------------------------------
   1.  DN_Datetime48x48Cyan.png     48x48       1814  RGB565     4  MiniLZ77       444   24.48%
   2.  DN_IPAddress48x48Cyan.png    48x48       3102  RGB565     4  MiniLZ77       232    7.48%
   3.  DN_Number48x48Cyan.png       48x48        674  RGB565     4  MiniLZ77       164   24.33%
   4.  DN_Password48x48Cyan.png     48x48       1174  RGB565     4  RLE            328   27.94%
 -----------------------------------------------------------------------------------------------
   Total                                        6764                              1168   17.27%
*/
// CSG Atlas: DLGAtlas object
extern const TGUIPicture picDLGAtlascsg;
#define CSG_DLGATLAS   (&picDLGAtlascsg)

// CSG Atlas: DLGAtlas picture index constants
constexpr int picIdxDN_Datetime48x48Cyan  = 0;
constexpr int picIdxDN_IPAddress48x48Cyan = 1;
constexpr int picIdxDN_Number48x48Cyan    = 2;
constexpr int picIdxDN_Password48x48Cyan  = 3;
//===============================================================
#ifdef __cplusplus
}
#endif
// ============================================================
#endif  // ImageRes_H

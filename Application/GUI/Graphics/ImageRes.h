// ------------------------------------------------------------
/*
   ImageRes.h
   CSG image data

   Encoder    name: CSG Toolkits
   Encoder version: 1.7.2627

   By Wey.   Silver Grid 2026
   Time: 2026-07-04 15:47:33
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
CSG atlas: picMAUAtlascsg
   No. Image                      resolution     size  CRM       CRN  CAS          Size   ratio  
 ------------------------------------------------------------------------------------------------
   1.  MA_Logo78x18Green.png         78x18       2134  RGB565     8  DEFLATE        648   30.37%
   2.  MA_Logo80x20Cyan.png          90x20       2189  RGB565     8  DEFLATE        772   35.27%
   3.  MA_ACPow32x59Cyan.png         32x59       4510  RGB565     8  MiniLZ77       276    6.12%
   4.  MA_CtrlG80x62Cyan.png         80x62       7635  RGB565    16  DEFLATE       1132   14.83%
   5.  MA_CtrlLED13x13Red.png        13x13        280  RGB565     8  RLE            100   35.71%
   6.  MA_Brkr56x60Cyan.png          56x60       8300  RGB565    16  DEFLATE       1564   18.84%
   7.  MA_Battery32x20C1.png         32x20       1132  RGB565    16  RLE            208   18.37%
   8.  MA_Battery32x20C2.png         32x20       1171  RGB565    16  RLE            220   18.79%
   9.  MA_Battery32x20C3.png         32x20       1178  RGB565    16  RLE            232   19.69%
  10.  MA_Battery32x20C4.png         32x20       1179  RGB565    16  RLE            228   19.34%
  11.  MA_Battery32x20C5.png         32x20       1086  RGB565    16  RLE            184   16.94%
  12.  MA_BatCharge11X13.png         11x13        309  RGB565     8  RLE             84   27.18%
  13.  MA_Menu20x20Cyan.png          20x20        701  RGB565     4  MiniLZ77       112   15.98%
  14.  MA_Fan16x16Cyan.png           16x16        662  RGB565     4  MiniLZ77       116   17.52%
  15.  MA_Fire16x16.png              16x16        672  RGB565    16  RLE            160   23.81%
  16.  MU_Home24x22.png              24x22        868  RGB565     4  MiniLZ77       156   17.97%
  17.  MU_Item32x32_01.png           32x32       1887  RGB565     4  MiniLZ77       308   16.32%
  18.  MU_Item32x32_02.png           32x32       1364  RGB565     4  MiniLZ77       232   17.01%
  19.  MU_Item32x32_03.png           32x32       1592  RGB565     4  MiniLZ77       248   15.58%
  20.  MU_Item32x32_04.png           32x32       1533  RGB565     4  MiniLZ77       296   19.31%
  21.  MU_Item32x32_05.png           32x32       1235  RGB565     4  MiniLZ77       200   16.19%
  22.  MU_Item32x32_06.png           32x32       1755  RGB565     4  MiniLZ77       268   15.27%
  23.  MU_Item32x32_07.png           32x32       2025  RGB565     4  MiniLZ77       296   14.62%
  24.  MU_Item32x32_08.png           32x32       1935  RGB565     4  MiniLZ77       288   14.88%
  25.  MU_Item32x32_09.png           32x32       1299  RGB565     4  MiniLZ77       184   14.16%
  26.  MU_Item32x32_10.png           32x32       1903  RGB565     4  MiniLZ77       284   14.92%
  27.  LV_CheckMark16x16Red.png      16x16        494  RGB565     8  MiniLZ77       132   26.72%
  28.  LV_CrossMark16x16Green.png    16x16        980  RGB565     8  MiniLZ77       140   14.29%
 ------------------------------------------------------------------------------------------------
   Total                                        52008                              9068   17.44%
*/
// CSG Atlas: MAUAtlas object
extern const TGUIPicture picMAUAtlascsg;
#define CSG_MAUATLAS   (&picMAUAtlascsg)

// CSG Atlas: MAUAtlas picture index constants
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
constexpr int picIdxLV_CheckMark16x16Red   = 26;
constexpr int picIdxLV_CrossMark16x16Green = 27;
//===============================================================
#ifdef __cplusplus
}
#endif
// ============================================================
#endif  // ImageRes_H

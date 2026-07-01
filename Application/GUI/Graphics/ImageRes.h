// ------------------------------------------------------------
/*
   ImageRes.h
   CSG image data

   Encoder    name: CSG Toolkits
   Encoder version: 1.5.2633

   By Wey.   Silver Grid 2026
   Time: 2026-06-26 15:57:30
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
// CSG Image: picbkg320x240Rcsg RLE
extern const TGUIPicture picbkg320x240Rcsg;

// ------------------------------------------------------------
// CSG Image: picbkg320x240csg LZ77
extern const TGUIPicture picbkg320x240Lcsg;

//===============================================================
// ------------------------------------------------------------
/*
   CSG atlas: picMAUAtlascsg
      No. Image                 resolution     size  CRM       CRN  CAS          Size  ratio  
    -------------------------------------------------------------------------------------------
      1.  MA_Logo78x18.png         78x18       2134  RGB565    16  MiniLZ77       924   43.30%
      2.  MA_ACPow32x59Cyan.png    32x59       4510  RGB565     8  MiniLZ77       276    6.12%
      3.  MA_CtrlF80x62Cyan.png    80x62       7979  RGB565    16  DEFLATE       1116   13.99%
      4.  MA_CtrlN80x62Cyan.png    80x62       8099  RGB565    16  DEFLATE       1004   12.40%
      5.  MA_CtrlP80x62Cyan.png    80x62       8469  RGB565    16  DEFLATE       1092   12.89%
      6.  MA_Brkr56x60Cyan.png     56x60       8300  RGB565    16  DEFLATE       1564   18.84%
      7.  MA_Battey44x24C1.png     44x24        794  RGB565     8  MiniLZ77       140   17.63%
      8.  MA_Battey44x24C2.png     44x24        880  RGB565     8  MiniLZ77       152   17.27%
      9.  MA_Battey44x24C3.png     44x24        962  RGB565     8  MiniLZ77       148   15.38%
     10.  MA_Battey44x24C4.png     44x24       1050  RGB565     8  MiniLZ77       168   16.00%
     11.  MA_Battey44x24C5.png     44x24       1100  RGB565     8  MiniLZ77       160   14.55%
     12.  MA_Fan16x16Cyan.png      16x16        662  RGB565     4  RLE            108   16.31%
     13.  MA_Fire16x16.png         16x16        672  RGB565    16  RLE            160   23.81%
     14.  MU_Home24x22.png         24x22        868  RGB565     4  MiniLZ77       156   17.97%
     15.  MU_Item32x32_01.png      32x32       1887  RGB565     4  MiniLZ77       308   16.32%
     16.  MU_Item32x32_02.png      32x32       1364  RGB565     4  MiniLZ77       232   17.01%
     17.  MU_Item32x32_03.png      32x32       1592  RGB565     4  MiniLZ77       248   15.58%
     18.  MU_Item32x32_04.png      32x32       1533  RGB565     4  MiniLZ77       296   19.31%
     19.  MU_Item32x32_05.png      32x32       1235  RGB565     4  MiniLZ77       200   16.19%
     20.  MU_Item32x32_06.png      32x32       1755  RGB565     4  MiniLZ77       268   15.27%
     21.  MU_Item32x32_07.png      32x32       2025  RGB565     4  MiniLZ77       296   14.62%
     22.  MU_Item32x32_08.png      32x32       1935  RGB565     4  MiniLZ77       288   14.88%
     23.  MU_Item32x32_09.png      32x32       1299  RGB565     4  MiniLZ77       184   14.16%
     24.  MU_Item32x32_10.png      32x32       1903  RGB565     4  MiniLZ77       284   14.92%
    -------------------------------------------------------------------------------------------
      Total                                   63007                              9772   15.51%
*/
extern const TGUIPicture picMAUAtlascsg;

constexpr int picIdxMA_Logo78x18      = 0;
constexpr int picIdxMA_ACPow32x59Cyan = 1;
constexpr int picIdxMA_CtrlF80x62Cyan = 2;
constexpr int picIdxMA_CtrlN80x62Cyan = 3;
constexpr int picIdxMA_CtrlP80x62Cyan = 4;
constexpr int picIdxMA_Brkr56x60Cyan  = 5;
constexpr int picIdxMA_Battey44x24C1  = 6;
constexpr int picIdxMA_Battey44x24C2  = 7;
constexpr int picIdxMA_Battey44x24C3  = 8;
constexpr int picIdxMA_Battey44x24C4  = 9;
constexpr int picIdxMA_Battey44x24C5  = 10;
constexpr int picIdxMA_Fan16x16Cyan   = 11;
constexpr int picIdxMA_Fire16x16      = 12;
constexpr int picIdxMU_Home24x22      = 13;
constexpr int picIdxMU_Item32x32_01   = 14;
constexpr int picIdxMU_Item32x32_02   = 15;
constexpr int picIdxMU_Item32x32_03   = 16;
constexpr int picIdxMU_Item32x32_04   = 17;
constexpr int picIdxMU_Item32x32_05   = 18;
constexpr int picIdxMU_Item32x32_06   = 19;
constexpr int picIdxMU_Item32x32_07   = 20;
constexpr int picIdxMU_Item32x32_08   = 21;
constexpr int picIdxMU_Item32x32_09   = 22;
constexpr int picIdxMU_Item32x32_10   = 23;
//===============================================================

#ifdef __cplusplus
}
#endif
// ============================================================
#endif  // ImageRes_H

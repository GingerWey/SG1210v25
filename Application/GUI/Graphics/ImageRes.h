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
      3.  MA_Ctrl78x61Cyan.png     78x61       4028  RGB565     8  DEFLATE        700   17.38%
      4.  MA_Brkr56x60Cyan.png     56x60       8300  RGB565    16  DEFLATE       1564   18.84%
      5.  MA_Battey44x24C1.png     44x24        794  RGB565     8  MiniLZ77       140   17.63%
      6.  MA_Battey44x24C2.png     44x24        880  RGB565     8  MiniLZ77       152   17.27%
      7.  MA_Battey44x24C3.png     44x24        962  RGB565     8  MiniLZ77       148   15.38%
      8.  MA_Battey44x24C4.png     44x24       1050  RGB565     8  MiniLZ77       168   16.00%
      9.  MA_Battey44x24C5.png     44x24       1100  RGB565     8  MiniLZ77       160   14.55%
     10.  MA_Fan16x16Cyan.png      16x16        662  RGB565     4  RLE            108   16.31%
     11.  MA_Fire16x16.png         16x16        672  RGB565    16  RLE            160   23.81%
     12.  MU_Home24x22.png         24x22        868  RGB565     4  MiniLZ77       156   17.97%
     13.  MU_Item32x32_01.png      32x32       1887  RGB565     4  MiniLZ77       308   16.32%
     14.  MU_Item32x32_02.png      32x32       1364  RGB565     4  MiniLZ77       232   17.01%
     15.  MU_Item32x32_03.png      32x32       1592  RGB565     4  MiniLZ77       248   15.58%
     16.  MU_Item32x32_04.png      32x32       1533  RGB565     4  MiniLZ77       296   19.31%
     17.  MU_Item32x32_05.png      32x32       1235  RGB565     4  MiniLZ77       200   16.19%
     18.  MU_Item32x32_06.png      32x32       1755  RGB565     4  MiniLZ77       268   15.27%
     19.  MU_Item32x32_07.png      32x32       2025  RGB565     4  MiniLZ77       296   14.62%
     20.  MU_Item32x32_08.png      32x32       1935  RGB565     4  MiniLZ77       288   14.88%
     21.  MU_Item32x32_09.png      32x32       1299  RGB565     4  MiniLZ77       184   14.16%
     22.  MU_Item32x32_10.png      32x32       1903  RGB565     4  MiniLZ77       284   14.92%
    -------------------------------------------------------------------------------------------
      Total                                   42488                              7260   17.09%
*/
extern const TGUIPicture picMAUAtlascsg;

constexpr int picIdxMA_Logo78x18      = 0;
constexpr int picIdxMA_ACPow32x59Cyan = 1;
constexpr int picIdxMA_Ctrl78x61Cyan  = 2;
constexpr int picIdxMA_Brkr56x60Cyan  = 3;
constexpr int picIdxMA_Battey44x24C1  = 4;
constexpr int picIdxMA_Battey44x24C2  = 5;
constexpr int picIdxMA_Battey44x24C3  = 6;
constexpr int picIdxMA_Battey44x24C4  = 7;
constexpr int picIdxMA_Battey44x24C5  = 8;
constexpr int picIdxMA_Fan16x16Cyan   = 9;
constexpr int picIdxMA_Fire16x16      = 10;
constexpr int picIdxMU_Home24x22      = 11;
constexpr int picIdxMU_Item32x32_01   = 12;
constexpr int picIdxMU_Item32x32_02   = 13;
constexpr int picIdxMU_Item32x32_03   = 14;
constexpr int picIdxMU_Item32x32_04   = 15;
constexpr int picIdxMU_Item32x32_05   = 16;
constexpr int picIdxMU_Item32x32_06   = 17;
constexpr int picIdxMU_Item32x32_07   = 18;
constexpr int picIdxMU_Item32x32_08   = 19;
constexpr int picIdxMU_Item32x32_09   = 20;
constexpr int picIdxMU_Item32x32_10   = 21;
//===============================================================

#ifdef __cplusplus
}
#endif
// ============================================================
#endif  // ImageRes_H

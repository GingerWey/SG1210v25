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
   CSG atlas: picImageRescsg
      No. Image                 resolution     size  CRM       CRN  CAS          Size  ratio
    -------------------------------------------------------------------------------------------
      1.  MA_Logo78x18.png         78x19       2139  RGB565     4  MiniLZ77       488   22.81%
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
    -------------------------------------------------------------------------------------------
      Total                                   25097                              4064   16.19%
*/
extern const TGUIPicture picMAImageRescsg;

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
//===============================================================

#ifdef __cplusplus
}
#endif
// ============================================================
#endif  // ImageRes_H

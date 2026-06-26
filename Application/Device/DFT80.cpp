//-----------------------------------------------------------------------------
/*
 File        : DFT80.c
 Version     : V1.10
 By          : 银网科技

 Description :实现80点DFT运算方法
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "DFT80.h"

//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 余弦表
#if DFT_USES_FLOAT > 0
  #define  DFT80P0       1
  #define  DFT80P1       0.99691733373312797619777340874204f
  #define  DFT80P2       0.98768834059513772619004024769344f
  #define  DFT80P3       0.9723699203976766018336458341188f
  #define  DFT80P4       0.95105651629515357211643933337938f
  #define  DFT80P5       0.92387953251128675612818318939679f
  #define  DFT80P6       0.89100652418836786235970957141363f
  #define  DFT80P7       0.85264016435409222151938345813041f
  #define  DFT80P8       0.80901699437494742410229341718282f
  #define  DFT80P9       0.7604059656000309381745943648449f
  #define  DFT80P10      0.70710678118654752440084436210485f
  #define  DFT80P11      0.64944804833018365572632077089376f
  #define  DFT80P12      0.58778525229247312916870595463907f
  #define  DFT80P13      0.52249856471594886498789788017829f
  #define  DFT80P14      0.45399049973954679156040836635787f
  #define  DFT80P15      0.3826834323650897717284599840304f
  #define  DFT80P16      0.30901699437494742410229341718282f
  #define  DFT80P17      0.23344536385590541176774443020287f
  #define  DFT80P18      0.15643446504023086901010531946717f
  #define  DFT80P19      0.07845909572784494503296024599346f
#else
  #include "SFCMathProc.h"
  
  // 余弦表倍率
  #define  COSMULP       32768u
  
  #define  DFT80P0       ((TDFTResult)(1 * COSMULP))
  #define  DFT80P1       ((TDFTResult)(0.9969173337331279640f * COSMULP))
  #define  DFT80P2       ((TDFTResult)(0.9876883405951377700f * COSMULP))
  #define  DFT80P3       ((TDFTResult)(0.9723699203976765570f * COSMULP))
  #define  DFT80P4       ((TDFTResult)(0.9510565162951535310f * COSMULP))
  #define  DFT80P5       ((TDFTResult)(0.9238795325112867380f * COSMULP))
  #define  DFT80P6       ((TDFTResult)(0.8910065241883678990f * COSMULP))
  #define  DFT80P7       ((TDFTResult)(0.8526401643540921780f * COSMULP))
  #define  DFT80P8       ((TDFTResult)(0.8090169943749474510f * COSMULP))
  #define  DFT80P9       ((TDFTResult)(0.7604059656000309310f * COSMULP))
  #define  DFT80P10      ((TDFTResult)(0.7071067811865475730f * COSMULP))
  #define  DFT80P11      ((TDFTResult)(0.6494480483301836580f * COSMULP))
  #define  DFT80P12      ((TDFTResult)(0.5877852522924731370f * COSMULP))
  #define  DFT80P13      ((TDFTResult)(0.5224985647159489100f * COSMULP))
  #define  DFT80P14      ((TDFTResult)(0.4539904997395468040f * COSMULP))
  #define  DFT80P15      ((TDFTResult)(0.3826834323650898370f * COSMULP))
  #define  DFT80P16      ((TDFTResult)(0.3090169943749474510f * COSMULP))
  #define  DFT80P17      ((TDFTResult)(0.2334453638559054740f * COSMULP))
  #define  DFT80P18      ((TDFTResult)(0.1564344650402309240f * COSMULP))
  #define  DFT80P19      ((TDFTResult)(0.0784590957278449991f * COSMULP))
#endif
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 基波   6963ns@168MHz
// 输入： pwSamples - 80个点的实部系列
// 输出： pfResult  - 保存结果的缓冲区，一个复数，实部、虚部  
void DFT80_H1(const int16_t *pwSamples, TDFTResult *pfResult)
{

  const int16_t *s = pwSamples;
  TDFTResult fReal, fImage;
   
  fReal = 
    DFT80P0  * ( (int) s[0]  - s[40] ) +
    DFT80P1  * ( (int) s[1]  - s[39] - s[41] + s[79] ) +
    DFT80P2  * ( (int) s[2]  - s[38] - s[42] + s[78] ) +
    DFT80P3  * ( (int) s[3]  - s[37] - s[43] + s[77] ) +
    DFT80P4  * ( (int) s[4]  - s[36] - s[44] + s[76] ) +
    DFT80P5  * ( (int) s[5]  - s[35] - s[45] + s[75] ) +
    DFT80P6  * ( (int) s[6]  - s[34] - s[46] + s[74] ) +
    DFT80P7  * ( (int) s[7]  - s[33] - s[47] + s[73] ) +
    DFT80P8  * ( (int) s[8]  - s[32] - s[48] + s[72] ) +
    DFT80P9  * ( (int) s[9]  - s[31] - s[49] + s[71] ) +
    DFT80P10 * ( (int) s[10] - s[30] - s[50] + s[70] ) +
    DFT80P11 * ( (int) s[11] - s[29] - s[51] + s[69] ) +
    DFT80P12 * ( (int) s[12] - s[28] - s[52] + s[68] ) +
    DFT80P13 * ( (int) s[13] - s[27] - s[53] + s[67] ) +
    DFT80P14 * ( (int) s[14] - s[26] - s[54] + s[66] ) +
    DFT80P15 * ( (int) s[15] - s[25] - s[55] + s[65] ) +
    DFT80P16 * ( (int) s[16] - s[24] - s[56] + s[64] ) +
    DFT80P17 * ( (int) s[17] - s[23] - s[57] + s[63] ) +
    DFT80P18 * ( (int) s[18] - s[22] - s[58] + s[62] ) +
    DFT80P19 * ( (int) s[19] - s[21] - s[59] + s[61] );
//------------------------
  fImage = 
    DFT80P19 * ( (int)-s[1]  - s[39] + s[41] + s[79] ) +
    DFT80P18 * ( (int)-s[2]  - s[38] + s[42] + s[78] ) +
    DFT80P17 * ( (int)-s[3]  - s[37] + s[43] + s[77] ) +
    DFT80P16 * ( (int)-s[4]  - s[36] + s[44] + s[76] ) +
    DFT80P15 * ( (int)-s[5]  - s[35] + s[45] + s[75] ) +
    DFT80P14 * ( (int)-s[6]  - s[34] + s[46] + s[74] ) +
    DFT80P13 * ( (int)-s[7]  - s[33] + s[47] + s[73] ) +
    DFT80P12 * ( (int)-s[8]  - s[32] + s[48] + s[72] ) +
    DFT80P11 * ( (int)-s[9]  - s[31] + s[49] + s[71] ) +
    DFT80P10 * ( (int)-s[10] - s[30] + s[50] + s[70] ) +
    DFT80P9  * ( (int)-s[11] - s[29] + s[51] + s[69] ) +
    DFT80P8  * ( (int)-s[12] - s[28] + s[52] + s[68] ) +
    DFT80P7  * ( (int)-s[13] - s[27] + s[53] + s[67] ) +
    DFT80P6  * ( (int)-s[14] - s[26] + s[54] + s[66] ) +
    DFT80P5  * ( (int)-s[15] - s[25] + s[55] + s[65] ) +
    DFT80P4  * ( (int)-s[16] - s[24] + s[56] + s[64] ) +
    DFT80P3  * ( (int)-s[17] - s[23] + s[57] + s[63] ) +
    DFT80P2  * ( (int)-s[18] - s[22] + s[58] + s[62] ) +
    DFT80P1  * ( (int)-s[19] - s[21] + s[59] + s[61] ) +
    DFT80P0  * ( (int)-s[20]                 + s[60] );
   
#if DFT_USES_FLOAT > 0
  pfResult[0] = fReal;
  pfResult[1] = fImage;
#else
  pfResult[0] = fReal / COSMULP;
  pfResult[1] = fImage / COSMULP;
#endif    
}
//-----------------------------------------------------------------------------
// 二次谐波  6832ns@84MHz
// 输入： pwSamples - 80个点的实部系列
// 输出： pfResult  - 保存结果的缓冲区，一个复数，实部、虚部  
void DFT80_H2(const int16_t *pwSamples, TDFTResult *pfResult)
{

  const int16_t *s = pwSamples;
  TDFTResult fReal, fImage;
  
//------------------------
  fReal = 
    DFT80P0  * ( (int) s[0]  - s[20] + s[40] - s[60] ) +
    DFT80P2  * ( (int) s[1]  - s[19] - s[21] + s[39] + s[41] - s[59] - s[61] + s[79] ) +
    DFT80P4  * ( (int) s[2]  - s[18] - s[22] + s[38] + s[42] - s[58] - s[62] + s[78] ) +
    DFT80P6  * ( (int) s[3]  - s[17] - s[23] + s[37] + s[43] - s[57] - s[63] + s[77] ) +
    DFT80P8  * ( (int) s[4]  - s[16] - s[24] + s[36] + s[44] - s[56] - s[64] + s[76] ) +
    DFT80P10 * ( (int) s[5]  - s[15] - s[25] + s[35] + s[45] - s[55] - s[65] + s[75] ) +
    DFT80P12 * ( (int) s[6]  - s[14] - s[26] + s[34] + s[46] - s[54] - s[66] + s[74] ) +
    DFT80P14 * ( (int) s[7]  - s[13] - s[27] + s[33] + s[47] - s[53] - s[67] + s[73] ) +
    DFT80P16 * ( (int) s[8]  - s[12] - s[28] + s[32] + s[48] - s[52] - s[68] + s[72] ) +
    DFT80P18 * ( (int) s[9]  - s[11] - s[29] + s[31] + s[49] - s[51] - s[69] + s[71] );
//------------------------
  fImage = 
    DFT80P18 * ( (int)-s[1]  - s[19] + s[21] + s[39] - s[41] - s[59] + s[61] + s[79] ) +
    DFT80P16 * ( (int)-s[2]  - s[18] + s[22] + s[38] - s[42] - s[58] + s[62] + s[78] ) +
    DFT80P14 * ( (int)-s[3]  - s[17] + s[23] + s[37] - s[43] - s[57] + s[63] + s[77] ) +
    DFT80P12 * ( (int)-s[4]  - s[16] + s[24] + s[36] - s[44] - s[56] + s[64] + s[76] ) +
    DFT80P10 * ( (int)-s[5]  - s[15] + s[25] + s[35] - s[45] - s[55] + s[65] + s[75] ) +
    DFT80P8  * ( (int)-s[6]  - s[14] + s[26] + s[34] - s[46] - s[54] + s[66] + s[74] ) +
    DFT80P6  * ( (int)-s[7]  - s[13] + s[27] + s[33] - s[47] - s[53] + s[67] + s[73] ) +
    DFT80P4  * ( (int)-s[8]  - s[12] + s[28] + s[32] - s[48] - s[52] + s[68] + s[72] ) +
    DFT80P2  * ( (int)-s[9]  - s[11] + s[29] + s[31] - s[49] - s[51] + s[69] + s[71] ) +
    DFT80P0  * ( (int)-s[10] + s[30] - s[50] + s[70] );
      
#if DFT_USES_FLOAT > 0
  pfResult[0] = fReal;
  pfResult[1] = fImage;
#else
  pfResult[0] = fReal / COSMULP;
  pfResult[1] = fImage / COSMULP;
#endif    
}
//-----------------------------------------------------------------------------


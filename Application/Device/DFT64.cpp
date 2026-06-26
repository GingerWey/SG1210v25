//-----------------------------------------------------------------------------
/*
 File      	: DFT64.c
 Version   	: V1.01
 By        	: 西安银网科技智能电气有限公司

 Description: 实现64点DFT运算方法
				
 Date       : 2017.9.9
*/
//-----------------------------------------------------------------------------
#include "DFT64.h"

//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 余弦表
#if DFT_USES_FLOAT > 0
  #define  DFT64P0       1
  #define  DFT64P1       0.995184719562531f
  #define  DFT64P2       0.980785250663757f
  #define  DFT64P3       0.956940352916718f
  #define  DFT64P4       0.923879504203796f
  #define  DFT64P5       0.881921291351318f
  #define  DFT64P6       0.831469595432281f
  #define  DFT64P7       0.773010492324829f
  #define  DFT64P8       0.70710676908493f
  #define  DFT64P9       0.634393274784088f
  #define  DFT64P10      0.555570244789124f
  #define  DFT64P11      0.471396774053574f
  #define  DFT64P12      0.382683426141739f
  #define  DFT64P13      0.290284633636475f
  #define  DFT64P14      0.195090353488922f
  #define  DFT64P15      0.0980171337723732f
#else
  #include "SFCMathProc.h"
  
  // 余弦表倍率
  #define  COSMULP       32768u
  
  #define  DFT64P0       1
  #define  DFT64P1       ((TDFTResult)(0.995184719562531f * COSMULP))
  #define  DFT64P2       ((TDFTResult)(0.980785250663757f * COSMULP))
  #define  DFT64P3       ((TDFTResult)(0.956940352916718f * COSMULP))
  #define  DFT64P4       ((TDFTResult)(0.923879504203796f * COSMULP))
  #define  DFT64P5       ((TDFTResult)(0.881921291351318f * COSMULP))
  #define  DFT64P6       ((TDFTResult)(0.831469595432281f * COSMULP))
  #define  DFT64P7       ((TDFTResult)(0.773010492324829f * COSMULP))
  #define  DFT64P8       ((TDFTResult)(0.70710676908493f  * COSMULP))
  #define  DFT64P9       ((TDFTResult)(0.634393274784088f * COSMULP))
  #define  DFT64P10      ((TDFTResult)(0.555570244789124f * COSMULP))
  #define  DFT64P11      ((TDFTResult)(0.471396774053574f * COSMULP))
  #define  DFT64P12      ((TDFTResult)(0.382683426141739f * COSMULP))
  #define  DFT64P13      ((TDFTResult)(0.290284633636475f * COSMULP))
  #define  DFT64P14      ((TDFTResult)(0.195090353488922f * COSMULP))
  #define  DFT64P15      ((TDFTResult)(0.0980171337723732f * COSMULP))
#endif
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 基波   6963ns@168MHz
// 输入： pwSamples - 64个点的实部系列
// 输出： pfResult  - 保存结果的缓冲区，一个复数，实部、虚部  
void DFT64_H1(const int16_t *pwSamples, TDFTResult *pfResult)
{

  const int16_t *s = pwSamples;
  TDFTResult fReal, fImage;
   
//------------------------
  fReal = 
    DFT64P0  * ( (int) s[0]  - s[32] ) +
    DFT64P1  * ( (int) s[1]  - s[31] - s[33] + s[63] ) +
    DFT64P2  * ( (int) s[2]  - s[30] - s[34] + s[62] ) +
    DFT64P3  * ( (int) s[3]  - s[29] - s[35] + s[61] ) +
    DFT64P4  * ( (int) s[4]  - s[28] - s[36] + s[60] ) +
    DFT64P5  * ( (int) s[5]  - s[27] - s[37] + s[59] ) +
    DFT64P6  * ( (int) s[6]  - s[26] - s[38] + s[58] ) +
    DFT64P7  * ( (int) s[7]  - s[25] - s[39] + s[57] ) +
    DFT64P8  * ( (int) s[8]  - s[24] - s[40] + s[56] ) +
    DFT64P9  * ( (int) s[9]  - s[23] - s[41] + s[55] ) +
    DFT64P10 * ( (int) s[10] - s[22] - s[42] + s[54] ) +
    DFT64P11 * ( (int) s[11] - s[21] - s[43] + s[53] ) +
    DFT64P12 * ( (int) s[12] - s[20] - s[44] + s[52] ) +
    DFT64P13 * ( (int) s[13] - s[19] - s[45] + s[51] ) +
    DFT64P14 * ( (int) s[14] - s[18] - s[46] + s[50] ) +
    DFT64P15 * ( (int) s[15] - s[17] - s[47] + s[49] );
//------------------------
  fImage = 
    DFT64P15 * ( (int)-s[1]  - s[31] + s[33] + s[63] ) +
    DFT64P14 * ( (int)-s[2]  - s[30] + s[34] + s[62] ) +
    DFT64P13 * ( (int)-s[3]  - s[29] + s[35] + s[61] ) +
    DFT64P12 * ( (int)-s[4]  - s[28] + s[36] + s[60] ) +
    DFT64P11 * ( (int)-s[5]  - s[27] + s[37] + s[59] ) +
    DFT64P10 * ( (int)-s[6]  - s[26] + s[38] + s[58] ) +
    DFT64P9  * ( (int)-s[7]  - s[25] + s[39] + s[57] ) +
    DFT64P8  * ( (int)-s[8]  - s[24] + s[40] + s[56] ) +
    DFT64P7  * ( (int)-s[9]  - s[23] + s[41] + s[55] ) +
    DFT64P6  * ( (int)-s[10] - s[22] + s[42] + s[54] ) +
    DFT64P5  * ( (int)-s[11] - s[21] + s[43] + s[53] ) +
    DFT64P4  * ( (int)-s[12] - s[20] + s[44] + s[52] ) +
    DFT64P3  * ( (int)-s[13] - s[19] + s[45] + s[51] ) +
    DFT64P2  * ( (int)-s[14] - s[18] + s[46] + s[50] ) +
    DFT64P1  * ( (int)-s[15] - s[17] + s[47] + s[49] ) +
    DFT64P0  * ( (int)-s[16] + s[48] );
   
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
// 输入： pwSamples - 64个点的实部系列
// 输出： pfResult  - 保存结果的缓冲区，一个复数，实部、虚部  
void DFT64_H2(const int16_t *pwSamples, TDFTResult *pfResult)
{

  const int16_t *s = pwSamples;
  TDFTResult fReal, fImage;
   
//------------------------
  fReal = 
    DFT64P0  * ( (int) s[0]  - s[16] + s[32] - s[48] ) +
    DFT64P2  * ( (int) s[1]  - s[15] - s[17] + s[31] + s[33] - s[47] - s[49] + s[63] ) +
    DFT64P4  * ( (int) s[2]  - s[14] - s[18] + s[30] + s[34] - s[46] - s[50] + s[62] ) +
    DFT64P6  * ( (int) s[3]  - s[13] - s[19] + s[29] + s[35] - s[45] - s[51] + s[61] ) +
    DFT64P8  * ( (int) s[4]  - s[12] - s[20] + s[28] + s[36] - s[44] - s[52] + s[60] ) +
    DFT64P10 * ( (int) s[5]  - s[11] - s[21] + s[27] + s[37] - s[43] - s[53] + s[59] ) +
    DFT64P12 * ( (int) s[6]  - s[10] - s[22] + s[26] + s[38] - s[42] - s[54] + s[58] ) +
    DFT64P14 * ( (int) s[7]  - s[9]  - s[23] + s[25] + s[39] - s[41] - s[55] + s[57] );
//------------------------
  fImage = 
    DFT64P14 * ( (int)-s[1]  - s[15] + s[17] + s[31] - s[33] - s[47] + s[49] + s[63] ) +
    DFT64P12 * ( (int)-s[2]  - s[14] + s[18] + s[30] - s[34] - s[46] + s[50] + s[62] ) +
    DFT64P10 * ( (int)-s[3]  - s[13] + s[19] + s[29] - s[35] - s[45] + s[51] + s[61] ) +
    DFT64P8  * ( (int)-s[4]  - s[12] + s[20] + s[28] - s[36] - s[44] + s[52] + s[60] ) +
    DFT64P6  * ( (int)-s[5]  - s[11] + s[21] + s[27] - s[37] - s[43] + s[53] + s[59] ) +
    DFT64P4  * ( (int)-s[6]  - s[10] + s[22] + s[26] - s[38] - s[42] + s[54] + s[58] ) +
    DFT64P2  * ( (int)-s[7]  - s[9]  + s[23] + s[25] - s[39] - s[41] + s[55] + s[57] ) +
    DFT64P0  * ( (int)-s[8]  + s[24] - s[40] + s[56] );
   
#if DFT_USES_FLOAT > 0
  pfResult[0] = fReal;
  pfResult[1] = fImage;
#else
  pfResult[0] = fReal / COSMULP;
  pfResult[1] = fImage / COSMULP;
#endif    
}
//-----------------------------------------------------------------------------


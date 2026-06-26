//-----------------------------------------------------------------------------
/*
 File        : RNGen.h
 Version     : V1.10
 By          : 银网科技

 For         : Stm32f40x
 Mode        : Thumb2
 Toolchain   : 
                 RealView Microcontroller Development Kit (MDK)
                 Keil uVision
 Description :定义随机数发生器（Random Number Generator）
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef __RNGEN_H_
#define __RNGEN_H_

#include <stdint.h>
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 公用宏
//-----------------------------------------------------------------------------

//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------
/**
 * @brief Seed pseudo-random number generator
 * @param[in] seed An integer value to be used as seed by the pseudo-random number generator
 * @return Error code
 **/
int RNG_Init(uint32_t seed);
//-----------------------------------------------------------------------------
/**
 * @brief Get a random value
 * @return Error code
 **/
uint32_t RNG_GetRand(void);
//-----------------------------------------------------------------------------
/**
 * @brief Get a random value in the specified range
 * @param[in] min Lower bound
 * @param[in] max Upper bound
 * @return Random value in the specified range
 **/
int32_t RNG_GetRandRange(int32_t min, int32_t max);
//=============================================================================
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif //__RNGEN_H_

//-----------------------------------------------------------------------------
/*
 File        : RNGen.c
 Version     : V1.10
 By          : 银网科技

 For         : Stm32f40x
 Mode        : Thumb2
 Toolchain   : 
                 RealView Microcontroller Development Kit (MDK)
                 Keil uVision
 Description :实现随机数发生器（Random Number Generator）
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "RNGen.h"

//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define USES_4xxRNG       1

#if (USES_4xxRNG > 0)
  #include "stm32f4xx_hal.h"
#endif

//Pseudo-random number generator state
static uint32_t RNG_State = 0;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------

//=============================================================================
// 局部宏
//-----------------------------------------------------------------------------

//=============================================================================
// 局部数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 局部数据
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
/**
 * @brief Seed pseudo-random number generator
 * @param[in] seed An integer value to be used as seed by the pseudo-random number generator
 * @return Error code
 **/
int RNG_Init(uint32_t seed)
{

// 采用硬件RNG
// Wey. 2016.2.24
#if (USES_4xxRNG > 0)
  uint16_t uCntr = 200;

  if( 0 == RNG_State )
    {
    //Enable RNG peripheral clock
     __HAL_RCC_RNG_CLK_ENABLE();
     //Enable RNG
     //RNG_Handle.Instance = RNG;
     //__HAL_RNG_ENABLE(&RNG_Handle);
     RNG->CR |=  RNG_CR_RNGEN;  
    
     while( --uCntr > 0 )
       {
       if( RNG_FLAG_DRDY == (RNG->SR & RNG_FLAG_DRDY) )
         break;       
       }
    }
#endif  

  //Seed the pseudo-random number generator
   RNG_State += seed;
      
#if (USES_4xxRNG > 0)
   //Successful processing
    return uCntr? 0 : 1;  
#else
   //Successful processing
   return 0;  
#endif
}
//-----------------------------------------------------------------------------
/**
 * @brief Get a random value
 * @return Error code
 **/
uint32_t RNG_GetRand(void)
{
   uint32_t result;

#if (USES_4xxRNG > 0)
  //Wait for the RNG to contain a valid data
   uint16_t uCntr = 200;
   while( --uCntr > 0 ) 
     if( RNG_FLAG_DRDY == (RNG->SR & RNG_FLAG_DRDY) )
       break;

  if( uCntr > 0 )
    //Get 32-bit random value
    result = RNG->DR; 
  else
#endif
    {
    //Use a linear congruential generator (LCG) to update the state of the PRNG
     RNG_State *= 1103515245;
     RNG_State += 12345;
     result = (RNG_State >> 16) & 0x07FF;

     RNG_State *= 1103515245;
     RNG_State += 12345;
     result <<= 10;
     result |= (RNG_State >> 16) & 0x03FF;

     RNG_State *= 1103515245;
     RNG_State += 12345;
     result <<= 10;
     result |= (RNG_State >> 16) & 0x03FF;
     }
   //Return the resulting value
   return result;
}
//-----------------------------------------------------------------------------
/**
 * @brief Get a random value in the specified range
 * @param[in] min Lower bound
 * @param[in] max Upper bound
 * @return Random value in the specified range
 **/
int32_t RNG_GetRandRange(int32_t min, int32_t max)
{
   uint32_t result;

#if (USES_4xxRNG > 0)
   uint16_t uCntr = 200;
  //Wait for the RNG to contain a valid data
   while( --uCntr > 0 ) 
     if( RNG_FLAG_DRDY == (RNG->SR & RNG_FLAG_DRDY) )
       break;

  if( uCntr > 0 )
    //Return a random value in the given range
    result = min + RNG->DR % (max - min + 1);
  else
#endif
    //Return a random value in the given range
    result = min + RNG_GetRand() % (max - min + 1);

   return result;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/*
 File       : DevRegInfo.h
 Version    : V1.01
 By         : 银网科技

 Description :实现用于解析寄存器描述的方法

   卫荣平
   2017.9.12
*/
//-----------------------------------------------------------------------------
#include "DevRegInfo.h"

#include "DevIntf.h"

#include "DevRegs.h"

#include <string.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
#define DIM_Name_CT_1A     "/1"
#define DIM_Name_CT_5A     "/5"
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 本地常量
//-----------------------------------------------------------------------------
// 量纲名称列表
// 表中的顺序要与 SIT_DIM_xx宏对应，即要等于SIT_GetDimVal(x)的值
static const char *plistDIMName[] =
{
  "V ",
  "A ",
  "W ",      
  "Var",    
  "VA",     
  "Wh",     
  "Varh",   
  "VAh",    
  "Hz",     
  "` ",    
  "`C",
  "#",      // Ω 
  "% ",
  "S ",
  "ms",  
  "/100",
  "/220",
  "/380"
};
#define NUM_DIMNames          (NUM_Elements(plistDIMName))
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
const char* getDynaDIMName( const TDevRegInfoItem* pInfo )
{

  const char* pcDIMName;
  switch( SIT_GetDim(pInfo->Property) )
    {
  // 按CT变比类型选择
  case SIT_DIM_CT:
    {
      if( !GetDevOption( REG_CT_1A ) )
    pcDIMName = DIM_Name_CT_5A;
   else
    pcDIMName = DIM_Name_CT_1A;
    break;
      }
      
    // 按4~20mA输入类型选择显示的单位
    case SIT_DIM_4_20TW:
      {
      if( REG_DEVCONFIG == REG_TYPE(pInfo->RefReg) )
        {
        if( 0 != _GetDevCfgReg( pInfo->RefReg ) )
       pcDIMName = "%";
        else
       pcDIMName = "`C";
        }
      else
        pcDIMName = "`C";
      break;
      }
    default:
      {
      pcDIMName = NULL;
      break;
      }      
    }
    
  return pcDIMName;
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 获取量纲名称
//-----------------------------------------------------------------------------
const char* RINF_GetDIMNameEx( const TDevRegInfoItem* pInfo)
{

  if( NULL == pInfo ) 
    {
    return NULL;
    }
  
  // 取量纲序号
  uint32_t uDIMShift = SIT_GetDimVal( pInfo->Property );
  if( SIT_DIM_None == uDIMShift )
    {
    return NULL;
    }
  else if( uDIMShift >= NUM_DIMNames )
    {
    return getDynaDIMName( pInfo );
    }
  
  // 取量纲名称
  const char* pcDIMName = plistDIMName[ uDIMShift - 1 ];
  
  // 量纲缓冲区
  char* pcDIMNamePtr = nullptr;
  
  // 根据需要加倍率前缀
  if( SIT_MUL_Kilo == SIT_GetMulti(pInfo->Property) )
    {
    if( RDM_TEST != GetDevMode )
      {
      pcDIMNamePtr = DevCache.DIMBuffer;
      *pcDIMNamePtr++ = 'k';
      }
    }
 else if( SIT_MUL_Mill == SIT_GetMulti(pInfo->Property) )
    {
    pcDIMNamePtr = DevCache.DIMBuffer;
   *pcDIMNamePtr++ = 'm';
    }
 else if( SIT_MUL_Mega == SIT_GetMulti(pInfo->Property) )
    {
    if( RDM_TEST != GetDevMode )
      {
      pcDIMNamePtr = DevCache.DIMBuffer;
      *pcDIMNamePtr++ = 'M';
      }      
    }

  // 量纲已加前缀? 
  if( NULL != pcDIMNamePtr )
    {
    // 生成量纲名称
    strncpy( pcDIMNamePtr, pcDIMName, SIZE_DIM_CACHE );
      
    pcDIMName = DevCache.DIMBuffer;
    }
  
  return pcDIMName;
}
//-----------------------------------------------------------------------------
// 获取量纲的名称 
// 输入：uRegNum = 寄存器地址
// 返回：0 = 寄存器不存在或寄存器无量纲  != 指向名称字符串的指针
const char* RINF_GetDIMName( uint32_t uRegNum )
{

  // 取寄存器描述
  const TDevRegInfoItem* pInfo = DevIntf_GetRegInfo( uRegNum );
  return RINF_GetDIMNameEx( pInfo );
}
//-----------------------------------------------------------------------------

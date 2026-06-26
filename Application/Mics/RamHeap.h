//-----------------------------------------------------------------------------
/*
 File        : RamHeap.h
 Version     : V1.10
 By          : 银网科技

 Description :管理RAM Heap内存分配的C访问接口
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef __RAMHEAP_H
#define __RAMHEAP_H

#include <stddef.h>
#include <stdint.h>

#ifndef __vmSIMULATOR__
#include <cmsis_os.h>
#endif
//=============================================================================
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据 
//-----------------------------------------------------------------------------

//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------
// 初始化

#ifndef __vmSIMULATOR__
size_t RAM_GetFreeHeapSize( void );
size_t RAM_MinimumEverFreeHeapSize( void );
void   RAM_GetHeapStats ( HeapStats_t *pxHeapStats );
  
/*
 * Called automatically to setup the required heap structures the first time
 * SDRAM_Malloc() is called.
 */
void RAM_Init(void);

// 分配内存
void *RAM_Malloc( size_t stWantedSize);
// 释放内存块
void RAM_Free( void *pv );
#else
#define   RAM_Init()                       {}

#define   RAM_GetFreeHeapSize()             0
#define   RAM_MinimumEverFreeHeapSize()     0

#define   RAM_Malloc(x)        (new (uint8_t*)[x])
#define   RAM_Free(x)          (delete[] (uint8_t*)(x))

#endif
//=============================================================================
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // __RAMHEAP_H

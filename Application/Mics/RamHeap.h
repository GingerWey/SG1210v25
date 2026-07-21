//-----------------------------------------------------------------------------
/*
 File        : RamHeap.h
 Version     : V1.22
 By          : 银网科技

 Description :管理RAM Heap内存分配的C访问接口
        
 Date       : 2023.12.05

        v1.2.0
                升级到10.3.1算法
			  v1.2.2
                脱离RTOS，去掉os依赖; 确保在Simulator下也能编译通过
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
// RAM Heap 内存分配统计信息
typedef struct ramHeapStats
{
	size_t xAvailableHeapSpaceInBytes;		  // The total heap size currently available - this is the sum of all the free blocks, not the largest block that can be allocated.
	size_t xSizeOfLargestFreeBlockInBytes; 	// The maximum size, in bytes, of all the free blocks within the heap at the time vPortGetHeapStats() is called.
	size_t xSizeOfSmallestFreeBlockInBytes; // The minimum size, in bytes, of all the free blocks within the heap at the time vPortGetHeapStats() is called.
	size_t xNumberOfFreeBlocks;				      // The number of free memory blocks within the heap at the time vPortGetHeapStats() is called.
	size_t xMinimumEverFreeBytesRemaining;	// The minimum amount of total free memory (sum of all free blocks) there has been in the heap since the system booted.
	size_t xNumberOfSuccessfulAllocations;	// The number of calls to pvPortMalloc() that have returned a valid memory block.
	size_t xNumberOfSuccessfulFrees;		    // The number of calls to vPortFree() that has successfully freed a block of memory.
} ramHeapStats_t;
//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------
// 初始化
// Called automatically to setup the required heap structures the first time
// SDRAM_Malloc() is called.
void  RAM_Init(void);

// 分配内存
void* RAM_Malloc( size_t stWantedSize);
// 释放内存块
void  RAM_Free( void *pv );

// Get the total free heap size (in bytes) currently available - t
// his is the sum of all the free blocks, not the largest block that can be allocated.
size_t RAM_GetFreeHeapSize(void);
// Get the minimum amount of total free memory (sum of all free blocks) 
// there has been in the heap since the system booted.
size_t RAM_MinimumEverFreeHeapSize(void);

// Get a structure containing information on the heap state.
void  RAM_GetHeapStats(ramHeapStats_t* pxHeapStats);
//=============================================================================
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // __RAMHEAP_H

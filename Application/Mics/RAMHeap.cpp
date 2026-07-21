//-----------------------------------------------------------------------------
/*
 File        : RamHeap.cpp
 Version     : V1.10
 By          : 银网科技

 Description :管理RAM-Heap的对象

 Date       : 2023.12.26

 v2.0
   升级到10.3.1算法
*/
//-----------------------------------------------------------------------------
#include "RamHeap.h"

#include "DevDebug.h"

#include <stdlib.h>
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------
// 0x4000 余  0x2470
#define  TOTAL_RAMHEAP_SIZE   0x02000     // 12KB

#define  ramBYTE_ALIGNMENT    8
//-----------------------------------------------------------------------------
/* Block sizes must not get too small. */
#define RAM_MINIMUM_BLOCK_SIZE  ( (size_t)( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define RAM_BITS_PER_BYTE    ( (size_t)8 )
#if ramBYTE_ALIGNMENT == 32
 #define ramBYTE_ALIGNMENT_MASK (0x001f)
#endif

#if ramBYTE_ALIGNMENT == 16
 #define ramBYTE_ALIGNMENT_MASK (0x000f)
#endif

#if ramBYTE_ALIGNMENT == 8
 #define ramBYTE_ALIGNMENT_MASK (0x0007)
#endif

#if ramBYTE_ALIGNMENT == 4
 #define ramBYTE_ALIGNMENT_MASK (0x0003)
#endif

#if ramBYTE_ALIGNMENT == 2
 #define ramBYTE_ALIGNMENT_MASK (0x0001)
#endif

#if ramBYTE_ALIGNMENT == 1
 #define ramBYTE_ALIGNMENT_MASK (0x0000)
#endif

#ifndef mtCOVERAGE_TEST_MARKER
 #define mtCOVERAGE_TEST_MARKER()
#endif

#ifndef ramheapASSERT
 #define ramheapASSERT(x)              \
    {                                 \
        if ((x) == 0)                 \
            DEV_FAULT(GFC_SystemErr); \
    }
#endif

#ifndef traceMALLOC
 #define traceMALLOC(pvAddress, uiSize)
#endif

#ifndef traceFREE
 #define traceFREE(pvAddress, uiSize)
#endif

#ifdef __vmSIMULATOR__
 #define ramTaskSuspendAll() 
 #define ramTaskResumeAll()

 #define ramEXIT_CRITICAL()
 #define ramENTER_CRITICAL()
    
 #define ramMAX_DELAY         0xffffffffUL          
#else
 #define ramTaskResumeAll()   (void)xTaskResumeAll()
 #define ramTaskSuspendAll()  vTaskSuspendAll()

 #define ramENTER_CRITICAL()  taskENTER_CRITICAL()
 #define ramEXIT_CRITICAL()   taskEXIT_CRITICAL()
 
 #define ramMAX_DELAY         portMAX_DELAY
#endif

//=============================================================================
// 局部数据结构 
//-----------------------------------------------------------------------------
/* Define the linked list structure.  This is used to link free blocks in order
  of their memory address. */
typedef struct A_BLOCK_LINK
{
  struct A_BLOCK_LINK *pxNextFreeBlock;  /*<< The next free block in the list. */
  size_t xBlockSize;            /*<< The size of the free block. */
} RAMBlockLink_t;

//=============================================================================
// 局布数据 
//-----------------------------------------------------------------------------

/* Allocate the memory for the heap. */
static uint8_t ucRAMHeap[ TOTAL_RAMHEAP_SIZE ];

/* The size of the structure placed at the beginning of each allocated memory
  block must by correctly byte aligned. */
static const size_t xHeapStructSize  = 
           ( sizeof(RAMBlockLink_t) + ( (size_t)(ramBYTE_ALIGNMENT - 1) ) ) & 
           ~( (size_t)ramBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static RAMBlockLink_t xStart, *RAM_pblEnd = nullptr;

/* Keeps track of the number of calls to allocate and free memory as well as the
  number of free bytes remaining, but says nothing about fragmentation. */
static size_t xRAMFreeBytesRemaining = 0U;
static size_t xRAMMinimumEverFreeBytesRemaining = 0U;
static size_t xRAMNumberOfSuccessfulAllocations = 0;
static size_t xRAMNumberOfSuccessfulFrees = 0;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
  member of an RAMBlockLink_t structure is set then the block belongs to the
  application.  When the bit is free the block is still part of the free heap
  space. */
static size_t xRAMBlockAllocatedBit = 0;

//=============================================================================
// 内部方法申明
//-----------------------------------------------------------------------------
/*
   Inserts a block of memory that is being freed into the correct position in
   the list of free memory blocks.  The block being freed will be merged with
   the block in front it and/or the block behind it if the memory blocks are
   adjacent to each other.
*/
static void RAM_InsertBlockIntoFreeList(RAMBlockLink_t* pxBlockToInsert);

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
void *RAM_Malloc ( size_t xWantedSize )
{
  RAMBlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
  void *pvReturn = nullptr;

  DEV_ASSERT( (0 == xWantedSize), GFC_ErrParam );

  ramTaskSuspendAll();
    {
    /* If this is the first call to malloc then the heap will require
      initialisation to setup the list of free blocks. */
    if( RAM_pblEnd == nullptr )
      {
      RAM_Init();
      }
    else
      {
      mtCOVERAGE_TEST_MARKER();
      }

    /* Check the requested block size is not so large that the top bit is
      set.  The top bit of the block size member of the RAMBlockLink_t structure
      is used to determine who owns the block - the application or the
      kernel, so it must be free. */
    if( ( xWantedSize & xRAMBlockAllocatedBit ) == 0 )
      {
      /* The wanted size is increased so it can contain a RAMBlockLink_t
        structure in addition to the requested amount of bytes. */
      if( xWantedSize > 0 )
        {
        xWantedSize += xHeapStructSize;

        /* Ensure that blocks are always aligned to the required number
          of bytes. */
        if( ( xWantedSize & ramBYTE_ALIGNMENT_MASK ) != 0x00 )
          {
          /* Byte alignment required. */
          xWantedSize += ( ramBYTE_ALIGNMENT - (xWantedSize & ramBYTE_ALIGNMENT_MASK) );
          ramheapASSERT( ( xWantedSize & ramBYTE_ALIGNMENT_MASK ) == 0 );
          }
        else
          {
          mtCOVERAGE_TEST_MARKER();
          }
        }
      else
        {
        mtCOVERAGE_TEST_MARKER();
        }

      if( (xWantedSize > 0) && (xWantedSize <= xRAMFreeBytesRemaining)  )
        {
        /* Traverse the list from the start  (lowest address) block until
          one  of adequate size is found. */
        pxPreviousBlock = &xStart;
        pxBlock = xStart.pxNextFreeBlock;
        while ( ( pxBlock->xBlockSize < xWantedSize ) &&
                ( pxBlock->pxNextFreeBlock != nullptr ) )
          {
          pxPreviousBlock = pxBlock;
          pxBlock = pxBlock->pxNextFreeBlock;
          }

        /* If the end marker was reached then a block of adequate size
          was  not found. */
        if( pxBlock != RAM_pblEnd )
          {
          /* Return the memory space pointed to - jumping over the
            RAMBlockLink_t structure at its start. */
          pvReturn = (void*)( ( (uint8_t*)pxPreviousBlock->pxNextFreeBlock ) +
                              xHeapStructSize );

          /* This block is being returned for use so must be taken out
            of the list of free blocks. */
          pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

          /* If the block is larger than required it can be split into
            two. */
          if( ( pxBlock->xBlockSize - xWantedSize ) > RAM_MINIMUM_BLOCK_SIZE )
            {
            /* This block is to be split into two.  Create a new
              block following the number of bytes requested. The void
              cast is used to prevent byte alignment warnings from the
              compiler. */
            pxNewBlockLink = (RAMBlockLink_t*)( ( (uint8_t*) pxBlock ) + xWantedSize );
            ramheapASSERT( (((size_t)pxNewBlockLink) & ramBYTE_ALIGNMENT_MASK) == 0 );

            /* Calculate the sizes of two blocks split from the single block. */
            pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
            pxBlock->xBlockSize = xWantedSize;

            /* Insert the new block into the list of free blocks. */
            RAM_InsertBlockIntoFreeList ( pxNewBlockLink );
            }
          else
            {
            mtCOVERAGE_TEST_MARKER();
            }

          xRAMFreeBytesRemaining -= pxBlock->xBlockSize;

          if( xRAMFreeBytesRemaining < xRAMMinimumEverFreeBytesRemaining )
            {
            xRAMMinimumEverFreeBytesRemaining = xRAMFreeBytesRemaining;
            }
          else
            {
            mtCOVERAGE_TEST_MARKER();
            }

          /* The block is being returned - it is allocated and owned
            by the application and has no "next" block. */
          pxBlock->xBlockSize |= xRAMBlockAllocatedBit;
          pxBlock->pxNextFreeBlock = nullptr;
          xRAMNumberOfSuccessfulAllocations++;
          }
        else
          {
          mtCOVERAGE_TEST_MARKER();
          }
        }
      else
        {
        mtCOVERAGE_TEST_MARKER();
        }
      }
    else
      {
      mtCOVERAGE_TEST_MARKER();
      }

    traceMALLOC ( pvReturn, xWantedSize );
    }
  ramTaskResumeAll();

#if( configUSE_MALLOC_FAILED_HOOK == 1 )
    {
    if( pvReturn == nullptr )
      {
      extern void vApplicationMallocFailedHook(void);
      vApplicationMallocFailedHook();
      }
    else
      {
      mtCOVERAGE_TEST_MARKER();
      }
    }
#endif

  DEV_ASSERT( (nullptr == pvReturn), GFC_OutOfMem );

  ramheapASSERT ((((size_t)pvReturn ) & (size_t)ramBYTE_ALIGNMENT_MASK ) == 0 );

  return pvReturn;
}
//-----------------------------------------------------------------------------

void RAM_Free ( void *pv )
{
  
  uint8_t *puc = (uint8_t*) pv;
  RAMBlockLink_t *pxLink;

  if( pv != nullptr )
    {
    /* The memory being freed will have an RAMBlockLink_t structure immediately
      before it. */
    puc -= xHeapStructSize;

    /* This casting is to keep the compiler from issuing warnings. */
    pxLink = (RAMBlockLink_t*)puc;

    /* Check the block is actually allocated. */
    ramheapASSERT ( ( pxLink->xBlockSize & xRAMBlockAllocatedBit ) != 0 );
    ramheapASSERT ( pxLink->pxNextFreeBlock == nullptr );

    if( ( pxLink->xBlockSize & xRAMBlockAllocatedBit ) != 0 )
      {
      if( pxLink->pxNextFreeBlock == nullptr )
        {
        /* The block is being returned to the heap - it is no longer allocated. */
        pxLink->xBlockSize &= ~xRAMBlockAllocatedBit;

        ramTaskSuspendAll();
          {
          /* Add this block to the list of free blocks. */
          xRAMFreeBytesRemaining += pxLink->xBlockSize;
          traceFREE ( pv, pxLink->xBlockSize );
          RAM_InsertBlockIntoFreeList ( ( (RAMBlockLink_t*)pxLink ) );
          xRAMNumberOfSuccessfulFrees++;
          }
          ramTaskResumeAll();
        }
      else
        {
        mtCOVERAGE_TEST_MARKER();
        }
      }
    else
      {
      mtCOVERAGE_TEST_MARKER();
      }
    }
  else
    DEV_FAULT( GFC_OutOfMem );
}
//=============================================================================
// 内部方法
//-----------------------------------------------------------------------------
size_t RAM_GetFreeHeapSize ( void )
{
  return xRAMFreeBytesRemaining;
}
//-----------------------------------------------------------------------------

size_t RAM_MinimumEverFreeHeapSize ( void )
{
  return xRAMMinimumEverFreeBytesRemaining;
}
//-----------------------------------------------------------------------------

static void _InitialiseBlocks ( void )
{
  /* This just exists to keep the linker quiet. */
}
//-----------------------------------------------------------------------------

void RAM_Init ( void )
{
  RAMBlockLink_t *pxFirstFreeBlock;
  uint8_t *pucAlignedHeap;
  size_t uxAddress;
  size_t xTotalHeapSize = TOTAL_RAMHEAP_SIZE;

  /* Ensure the heap starts on a correctly aligned boundary. */
  uxAddress = (size_t)ucRAMHeap;

  if( ( uxAddress & ramBYTE_ALIGNMENT_MASK ) != 0 )
    {
    uxAddress +=  ( ramBYTE_ALIGNMENT - 1 );
    uxAddress &= ~( (size_t)ramBYTE_ALIGNMENT_MASK );
    xTotalHeapSize -= uxAddress - (size_t)ucRAMHeap;
    }

  pucAlignedHeap = (uint8_t*) uxAddress;

  /* xStart is used to hold a pointer to the first item in the list of free
    blocks.  The void cast is used to prevent compiler warnings. */
  xStart.pxNextFreeBlock = (struct A_BLOCK_LINK*)pucAlignedHeap;
  xStart.xBlockSize = (size_t)0;

  /* RAM_pblEnd is used to mark the end of the list of free blocks and is inserted
    at the end of the heap space. */
  uxAddress  = ( (size_t)pucAlignedHeap ) + xTotalHeapSize;
  uxAddress -= xHeapStructSize;
  uxAddress &= ~( (size_t)ramBYTE_ALIGNMENT_MASK );
  RAM_pblEnd = (RAMBlockLink_t*)uxAddress;
  RAM_pblEnd->xBlockSize = 0;
  RAM_pblEnd->pxNextFreeBlock = nullptr;

  /* To start with there is a single free block that is sized to take up the
    entire heap space, minus the space taken by RAM_pblEnd. */
  pxFirstFreeBlock = (RAMBlockLink_t*)pucAlignedHeap;
  pxFirstFreeBlock->xBlockSize = uxAddress - (size_t)pxFirstFreeBlock;
  pxFirstFreeBlock->pxNextFreeBlock = RAM_pblEnd;

  /* Only one block exists - and it covers the entire usable heap space. */
  xRAMMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
  xRAMFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

  /* Work out the position of the top bit in a size_t variable. */
  xRAMBlockAllocatedBit = ((size_t)1) << ((sizeof(size_t) * RAM_BITS_PER_BYTE) - 1);
}
//-----------------------------------------------------------------------------

static void RAM_InsertBlockIntoFreeList ( RAMBlockLink_t *pxBlockToInsert )
{
  RAMBlockLink_t *pxIterator;
  uint8_t *puc;

  /* Iterate through the list until a block is found that has a higher address
    than the block being inserted. */
  for ( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert;
        pxIterator = pxIterator->pxNextFreeBlock )
    {
    /* Nothing to do here, just iterate to the right position. */
    }

  /* Do the block being inserted, and the block it is being inserted after
    make a contiguous block of memory? */
  puc = (uint8_t*) pxIterator;
  if( ( puc + pxIterator->xBlockSize ) == (uint8_t*) pxBlockToInsert )
    {
    pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
    pxBlockToInsert = pxIterator;
    }
  else
    {
    mtCOVERAGE_TEST_MARKER();
    }

  /* Do the block being inserted, and the block it is being inserted before
    make a contiguous block of memory? */
  puc = (uint8_t*) pxBlockToInsert;
  if( (puc + pxBlockToInsert->xBlockSize) == (uint8_t*)pxIterator->pxNextFreeBlock )
    {
    if( pxIterator->pxNextFreeBlock != RAM_pblEnd )
      {
      /* Form one big block from the two blocks. */
      pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
      pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
      }
    else
      {
      pxBlockToInsert->pxNextFreeBlock = RAM_pblEnd;
      }
    }
  else
    {
    pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

  /* If the block being inserted plugged a gab, so was merged with the block
    before and the block after, then it's pxNextFreeBlock pointer will have
    already been set, and should not be set here as that would make it point
    to itself. */
  if( pxIterator != pxBlockToInsert )
    {
    pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
  else
    {
    mtCOVERAGE_TEST_MARKER();
    }
}
//-----------------------------------------------------------------------------
void RAM_GetHeapStats(ramHeapStats_t* pxHeapStats)
{
  
  if( RAM_pblEnd == nullptr )
    {
    RAM_Init();
    }
  
  RAMBlockLink_t *pxBlock;
  /* ramMAX_DELAY used as a portable way of getting the maximum value. */
  size_t xBlocks = 0, xMaxSize = 0, xMinSize = ramMAX_DELAY;

  ramTaskSuspendAll();
    {
    pxBlock = xStart.pxNextFreeBlock;

    /* pxBlock will be nullptr if the heap has not been initialised.  The heap
      is initialised automatically when the first allocation is made. */
    if( pxBlock != nullptr )
      {
      do
        {
        /* Increment the number of blocks and record the largest block seen so far. */
        xBlocks++;

        if( pxBlock->xBlockSize > xMaxSize )
          {
          xMaxSize = pxBlock->xBlockSize;
          }

        if( pxBlock->xBlockSize < xMinSize )
          {
          xMinSize = pxBlock->xBlockSize;
          }

        /* Move to the next block in the chain until the last block is
          reached. */
        pxBlock = pxBlock->pxNextFreeBlock;
        }
      while ( pxBlock != RAM_pblEnd );
      }
    }
  ramTaskResumeAll();

  pxHeapStats->xSizeOfLargestFreeBlockInBytes  = xMaxSize;
  pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
  pxHeapStats->xNumberOfFreeBlocks             = xBlocks;

  ramENTER_CRITICAL();
    {
    pxHeapStats->xAvailableHeapSpaceInBytes     = xRAMFreeBytesRemaining;
    pxHeapStats->xNumberOfSuccessfulAllocations = xRAMNumberOfSuccessfulAllocations;
    pxHeapStats->xNumberOfSuccessfulFrees       = xRAMNumberOfSuccessfulFrees;
    pxHeapStats->xMinimumEverFreeBytesRemaining = xRAMMinimumEverFreeBytesRemaining;
    }
  ramEXIT_CRITICAL();
}
//-----------------------------------------------------------------------------

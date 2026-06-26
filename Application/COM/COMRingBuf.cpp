//-----------------------------------------------------------------------------
/*
  File        : COMRingBuf.h
  Version     : V1.01
  By          : 银网科技

  Description : 实现COM的环形缓冲区
               
  Date        : 2010.3.16

  2024.01.15 v2.0
    代码规范化
*/
//-----------------------------------------------------------------------------
#include "COMRingBuf.h"

#include <string.h>
//=============================================================================
//                       TCOMRingBuffer实现
//-----------------------------------------------------------------------------
// 构造
// 输入: pvBuf: 缓冲区指针
//       uSize: 缓冲区尺寸
TCOMRingBuffer::TCOMRingBuffer ( void *pvBuf, uint32_t uSize )
  : TCOMBuffer ( pvBuf, uSize )
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 保存字符到缓冲区
// 输入:
//      ucData: 需要保存的字符
// 输出:
//    返回: 存贮字符的指针位置
uint32_t TCOMRingBuffer::PutChar ( uint8_t ucData )
{

  // 存放字符到缓冲区
  m_pucCache[m_uWriteIndex] = ucData;

  // 移动写指针, 超界时卷绕
  if ( m_uWriteIndex < m_uCacheSize - 1 )
    m_uWriteIndex++;
  else
    m_uWriteIndex = 0;

  // 超过读指针时,放弃当前字节
  if ( m_uWriteIndex == m_uReadIndex )
    {
    if ( m_uReadIndex < m_uCacheSize - 1 )
      m_uReadIndex++;
    else
      m_uReadIndex = 0;
    }

  // 返回字符存放的位置
  return m_uWriteIndex;
}
//-----------------------------------------------------------------------------
// 保存一个字符串到缓冲区
// 输入:  pucBuf: 需要的字串所在指针
//    uNumBytes: 数据长度
// 输出:  返回: 当前存贮指针的位置
uint32_t TCOMRingBuffer::Write ( const void *pvBuf, uint32_t uNumBytes )
{

#ifdef DEBUG_VER
  if ( nullptr != pvBuf )
#endif
    {
    const uint8_t *pucBuf = (const uint8_t*)pvBuf;

    if ( uNumBytes > 1 )
      {
      uint32_t uWriteIndex = m_uWriteIndex;
      uint32_t uReadIndex  = m_uReadIndex;
      if ( uNumBytes > m_uCacheSize )
        {
        pucBuf += uNumBytes - m_uCacheSize;
        uNumBytes  = m_uCacheSize;
        }

      if ( uWriteIndex >= uReadIndex )                // 缓冲区呈直线
        {
        if ( m_uCacheSize - uWriteIndex <= uNumBytes )
          {
          memcpy ( m_pucCache + uWriteIndex, 
                   pucBuf, 
                   m_uCacheSize - uWriteIndex );
          uNumBytes -= m_uCacheSize - uWriteIndex;
          pucBuf    += m_uCacheSize - uWriteIndex;

          if ( 0 < uNumBytes )
            {
            memcpy ( m_pucCache, pucBuf, uNumBytes );
            uWriteIndex = uNumBytes;
            }
          else        // 刚好填满
            {
            uWriteIndex = 0;
            }
          }
        else
          {
          // uWriteIndex到顶的空间足够
          memcpy ( m_pucCache + uWriteIndex, pucBuf, uNumBytes );
          uWriteIndex += uNumBytes;
          }
        }
      else
        {
        // uWriteIndex < uReadIndex, 缓冲区已卷回
        if ( m_uCacheSize - uWriteIndex <= uNumBytes )
          {
          memcpy ( m_pucCache + uWriteIndex, 
                   pucBuf, 
                   m_uCacheSize - uWriteIndex );

          uNumBytes -= m_uCacheSize - uWriteIndex;
          pucBuf    += m_uCacheSize - uWriteIndex;

          if ( uNumBytes )
            {
            memcpy ( m_pucCache, pucBuf, uNumBytes );

            uWriteIndex = uNumBytes;
            uReadIndex  = uWriteIndex + 1;
            }
          else
            {
            // 刚好填满
            uWriteIndex = 0;
            uReadIndex  = 1;
            }
          }
        else
          {
          // uWriteIndex到顶的空间足够
          memcpy ( m_pucCache + uWriteIndex, pucBuf, uNumBytes );
          uWriteIndex += uNumBytes;
          }
        }

      if ( uReadIndex >= m_uCacheSize )
        {
        uReadIndex -= m_uCacheSize;
        }

      if ( uWriteIndex >= m_uCacheSize )
        {
        uWriteIndex -= m_uCacheSize;
        }

      m_uWriteIndex = uWriteIndex;
      m_uReadIndex  = uReadIndex;
      }
    else
      {
      // uNumBytes == 1
      PutChar ( *pucBuf );
      }
    }
  return m_uReadIndex;
}
//-----------------------------------------------------------------------------
// 取缓冲区相对取指针的字符
// 输入:
//   uOffset: 从uReadIndex后第uOffset个字符
// 输出:
//   字符
uint8_t TCOMRingBuffer::GetChar ( uint32_t uOffset )
{

  // 由相对位置计算绝对位置
  uint32_t uPosition = m_uReadIndex + uOffset;

  // 超界后卷回
  if ( uPosition >= m_uCacheSize )
    uPosition -= m_uCacheSize;

  // 返回字符
  return m_pucCache[uPosition];
}
//-----------------------------------------------------------------------------
// 从缓冲区中读数据块
// 输入:
//     pucBuf:
//   uOffset: 从第index个位置开始
// 输出:
//    返回: 实际读的字节数
uint32_t TCOMRingBuffer::Read ( void     *pvBuf,
                                uint32_t  uNumBytes,
                                uint32_t  uOffset )
{

#ifdef DEBUG_VER
  if ( nullptr == pvBuf || 0 == uNumBytes )
    return 0;
#endif

  uint32_t uReadIndex = m_uReadIndex + uOffset;
  if ( uReadIndex >= m_uCacheSize )
    uReadIndex = uReadIndex - m_uCacheSize;

  uint32_t uResult;
  if ( uReadIndex < m_uWriteIndex )
    {
    if ( m_uWriteIndex - uReadIndex < uNumBytes )
      uResult = m_uWriteIndex - m_uReadIndex;
    else
      uResult = uNumBytes;

    if ( 0 < uResult )
      memcpy ( pvBuf, m_pucCache + uReadIndex, uResult );
    }
  else if ( uReadIndex > m_uWriteIndex ) // >>>>>>>> Needs to test.
    {
    if ( m_uCacheSize - uReadIndex + m_uWriteIndex < uNumBytes )
      uResult = m_uCacheSize - uReadIndex + m_uWriteIndex;
    else
      uResult = uNumBytes;

    uint8_t *pucBuf = (uint8_t*)pvBuf;
    if ( m_uCacheSize - uReadIndex >= uResult )
      {
      memcpy ( pucBuf + m_uCacheSize - uReadIndex, 
               m_pucCache, 
               uResult );
      }
    else
      {
      memcpy ( pucBuf, 
               m_pucCache + uReadIndex, 
               m_uCacheSize - uReadIndex );
      memcpy ( pucBuf + m_uCacheSize - uReadIndex,
               m_pucCache,
               uResult - ( m_uCacheSize - uReadIndex ) );
      }
    }
  else
    uResult = 0;

  return uResult;
}
//-----------------------------------------------------------------------------
// 将当前缓冲区中的内容复制到目标缓冲区中
// 输入:
//     pDestBuf: 目标缓冲区
//    uNumBytes: 要读的字节数  0:=全部
//      uOffset: 从GetPtr后从第uOffset个位置开始
// 输出:
//    返回: 复制的字节数
uint32_t TCOMRingBuffer::CopyTo ( TCOMBuffer  *pDestBuf,
                                    uint32_t   uNumBytes,
                                    uint32_t   uOffset )
{

#ifdef DEBUG_VER
  if ( nullptr == pDestBuf )
    return 0;
#endif

  if ( 0 == uNumBytes )
    uNumBytes = Used();

  uint32_t uReadIndex = m_uReadIndex + uOffset;
  if ( uReadIndex >= m_uCacheSize )
    uReadIndex = uReadIndex - m_uCacheSize;

  uint32_t uResult;
  if ( uReadIndex < m_uWriteIndex )
    {
    if ( m_uWriteIndex - uReadIndex < uNumBytes )
      uResult = m_uWriteIndex - m_uReadIndex;
    else
      uResult = uNumBytes;

    if ( uResult )
      pDestBuf->Write ( m_pucCache + uReadIndex, uResult );
    }
  else if ( uReadIndex > m_uWriteIndex )
    {
    if ( m_uCacheSize - uReadIndex + m_uWriteIndex < uNumBytes )
      uResult = m_uCacheSize - uReadIndex + m_uWriteIndex;
    else
      uResult = uNumBytes;

    if ( m_uCacheSize > uReadIndex )
      pDestBuf->Write ( m_pucCache + uReadIndex, m_uCacheSize - uReadIndex );

    uNumBytes -= m_uCacheSize - uReadIndex;
    if ( uNumBytes )
      pDestBuf->Write ( m_pucCache, uNumBytes );
    }
  else
    uResult = 0;

  return uResult;
}
//-----------------------------------------------------------------------------
// 前向移动移动取字符指针
// 输入:
//   uNumBytes: 移动的距离
// 输出:
//    返回:  当前位置
uint32_t TCOMRingBuffer::AdvanceReadIndex ( uint32_t uNumBytes )
{

  // 由相对位置计算绝对位置
  uint32_t uReadIndex = m_uReadIndex + uNumBytes;

  // 超界后卷回
  while ( uReadIndex >= m_uCacheSize )
    uReadIndex -= m_uCacheSize;

  // 设置取数据指针
  m_uReadIndex = uReadIndex;

  // 返回当前位置
  return uReadIndex;
}
//-----------------------------------------------------------------------------
// 在缓冲区中搜索字符
//   在缓冲中内搜索指定报文的字符,并计算出该字符距当前读位置的距离
// 输入:  ucData: 字符
// 输出:  
// 返回:  目标字符距当前读指针的距离; 若为"-1"，则表明搜索失败
uint32_t TCOMRingBuffer::Search ( uint8_t ucData )
{

  
  uint32_t uNumBytes = 0, uDistance = 0;

  // 复制uuReadIndex
  uint32_t uReadIndex = m_uReadIndex;

  while ( 1 )
    {
    if ( uReadIndex < m_uWriteIndex )
      {
      // 缓冲区未卷回
      // 向后搜索
      while ( uReadIndex < m_uWriteIndex )
        {
        if ( m_pucCache[ uReadIndex ] == ucData )
          {
          uNumBytes = m_uWriteIndex - uReadIndex;
          break;
          }
        uReadIndex++;
        uDistance++;
        }
      break;
      }
    else
      {
      if ( uReadIndex > m_uWriteIndex )
        {
        // 缓冲区已卷回
        // 在缓冲区后半部分搜索
        while ( uReadIndex < m_uCacheSize )
          {
          if ( m_pucCache[ uReadIndex ] == ucData )
            {
            uNumBytes = m_uCacheSize - uReadIndex + m_uWriteIndex;
            break;
            }
          uReadIndex++;
          uDistance++;
          }
        if ( uNumBytes )
          {
          // 若搜索完成,则跳出循环
          break;
          }

        if ( uReadIndex >= m_uCacheSize )
          {
          // 搜索失败
          // 卷回到缓冲区头
          uReadIndex = 0;
          if ( m_uWriteIndex > 0 )
            {
            // 准备从缓冲区头开始搜索
            // 继续从缓冲区头开始搜索
            continue;
            }
          else
            {
            // 搜索失败  结束搜索
            break;
            }
          }
        }
      else
        {
        // 缓冲区为空
        // 搜索失败 结束搜索
        break;
        }
      }
    }
  return ( uNumBytes ? uDistance : ( uint32_t ) -1 ); // 返回距离
}
//-----------------------------------------------------------------------------
// 缓冲区数据处理
//-----------------------------------------------------------------------------
// 对缓冲区信息执行指定运算
//   用给定的方法处理缓冲区中的信息
// 输入:     pFunc: 用于处理数据的方法
//       uNumBytes: 计算长度
//         uOffset: 开始位置距当前“读位置”的偏移(bytes)
//           uSeed: 初值，CRC为0xFFFF   Checksum: 0
// 输出:
// 返回:  校验结果
uint32_t TCOMRingBuffer::Execute ( PBufOperator pFunc,
                                       uint32_t uNumBytes,
                                       uint32_t uOffset,
                                       uint32_t uSeed )
{

  uint32_t  uResult;

  if ( nullptr == pFunc || Used() < uOffset )
    uResult = uSeed;
  else if ( m_uWriteIndex < m_uReadIndex ) // 未卷回，使用TCOMBuffer计算
    uResult = TCOMBuffer::Execute ( pFunc, uNumBytes, uOffset, uSeed );
  else  // 已卷回
    {
    if ( Used() < uOffset + uNumBytes )
      uNumBytes = Used() - uOffset;

    // uReadIndex偏移uOfs之后剩余空间是否够 uLen长？
    if ( m_uCacheSize - ( m_uReadIndex + uOffset ) >= uNumBytes )
      {
      // 够长
      uResult = pFunc ( m_pucCache + m_uReadIndex + uOffset, uNumBytes, uSeed );
      }
    else
      {
      // 不够长，要卷回去算

      // 先算后关截
      uResult = pFunc ( m_pucCache + m_uReadIndex + uOffset,
                        m_uCacheSize - ( m_uReadIndex + uOffset ),
                        uSeed );

      // 卷回去的长度
      uNumBytes -= m_uCacheSize - ( m_uReadIndex + uOffset );

      // 算卷回去的部分
      uResult = pFunc ( m_pucCache,  uNumBytes, uResult );
      }
    }

  return uResult;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 空间查询
//-----------------------------------------------------------------------------
// 缓冲区中剩余字符数量
// 输入: 无
// 输出: 返回: 剩余字符的数量
uint32_t TCOMRingBuffer::Used()
{

  // Copy the Read/Write indices for calculation.
  uint32_t uWriteIndex = m_uWriteIndex;
  uint32_t uReadIndex  = m_uReadIndex;

  // Return the number of bytes contained in the ring buffer.
  return ( ( uWriteIndex >= uReadIndex ) ?
           ( uWriteIndex -  uReadIndex ) :
           ( m_uCacheSize - ( uReadIndex - uWriteIndex ) ) );
}
//-----------------------------------------------------------------------------
// 缓冲区中可用容量
// 输入:
//   无
// 输出:
//    返回: 剩余字符的数量
uint32_t TCOMRingBuffer::Free()
{

  // Return the number of bytes available in the ring buffer.
  return ( ( m_uCacheSize - 1 ) - Used() );
}
//-----------------------------------------------------------------------------

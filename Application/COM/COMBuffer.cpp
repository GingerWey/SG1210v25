//-----------------------------------------------------------------------------
/*
  File        : COMBuffer.h
  Version     : V1.01
  By          : 银网科技

  Description : 实现COM的缓冲区
               
  Date        : 2010.3.16

  2024.01.15 v2.0
    代码规范化
*/
//-----------------------------------------------------------------------------
#include "COMBuffer.h"

#include <string.h>
//=============================================================================
//                       TCOMBuffer实现
//-----------------------------------------------------------------------------
// 默认构造
TCOMBuffer::TCOMBuffer()
{

  SetBuffer ( nullptr, 0 );
}
//-----------------------------------------------------------------------------
// 构造
// 输入:  pucBuffer: 缓冲区指针
//            uSize: 缓冲区尺寸
TCOMBuffer::TCOMBuffer ( void *pvBuffer,  uint32_t uSize )
{

  //
  // Initialize the buffer object.
  SetBuffer ( pvBuffer, uSize );
}
//-----------------------------------------------------------------------------
// 构造
// 输入:  pucBuffer: 缓冲区指针
//            uSize: 缓冲区尺寸
void TCOMBuffer::SetBuffer ( void *pvBuffer,  uint32_t uSize )
{

  m_uCacheSize  = uSize;
  m_pucCache    = (uint8_t*)pvBuffer;
  m_uWriteIndex = m_uReadIndex = 0;
}
//-----------------------------------------------------------------------------
// 清空缓冲区
// 输入:
//   无
// 输出:
//   返回：缓冲区容量
uint32_t TCOMBuffer::Reset()
{

  m_uWriteIndex = m_uReadIndex = 0;

  return m_uCacheSize;
}
//-----------------------------------------------------------------------------
// 保存字符到缓冲区
// 输入:
//      ucData: 需要保存的字符
// 输出:
//    返回: 存贮字符的指针位置
uint32_t TCOMBuffer::PutChar ( uint8_t ucData )
{

  // 有空间存放？
  if ( m_uWriteIndex < m_uCacheSize )
    {
    // 存放字符到缓冲区
    m_pucCache[m_uWriteIndex] = ucData;

    // 移动写指针
    m_uWriteIndex++;
    }

  // 返回字符存放的位置
  return m_uWriteIndex;
}
//-----------------------------------------------------------------------------
// 保存一个字符串到缓冲区
// 输入:
//    pucBuf: 需要的字串所在指针
//    uNumBytes: 数据长度
// 输出:
//    返回: 当前存贮指针的位置
uint32_t TCOMBuffer::Write ( const void *pvBuf, uint32_t uNumBytes )
{

  uint32_t uWriteIndex = m_uWriteIndex;
  if ( nullptr != pvBuf && 0 < uNumBytes )
    {
    if ( uNumBytes > m_uCacheSize - m_uWriteIndex )
      {
      uNumBytes  = m_uCacheSize - m_uWriteIndex;
      }

    if ( 0 < uNumBytes )
      {
      memcpy ( m_pucCache + uWriteIndex, pvBuf, uNumBytes );

      uWriteIndex += uNumBytes;

      m_uWriteIndex = uWriteIndex;
      }
    }

  return m_uWriteIndex;
}
//-----------------------------------------------------------------------------
// 保存一个字到缓冲区, 低字节在前，高字节在后
// 输入:
//    uwData: 需要保存的数据
// 输出:
//    返回: 当前存贮指针的位置
uint32_t TCOMBuffer::PutWordLH ( uint16_t uwData )
{

  PutChar ( (uint8_t)uwData & 0xff );

  return PutChar ( (uint8_t)( ( uwData >> 8 ) & 0xff ) );
}
//-----------------------------------------------------------------------------
// 保存一个字到缓冲区，高字节在后, 低字节在前
// 输入:
//    uwData: 需要保存的数据
// 输出:
//    返回: 当前存贮指针的位置
uint32_t TCOMBuffer::PutWordHL ( uint16_t uwData )
{

  PutChar ( (uint8_t)( ( uwData >> 8 ) & 0xff ) );

  return PutChar ( (uint8_t)uwData & 0xff );
}
//-----------------------------------------------------------------------------
// 取缓冲区相对取指针的字符
// 输入:
//   uOffset: 从uReadIndex后第uOffset个字符
// 输出:
//   字符
uint8_t TCOMBuffer::GetChar ( uint32_t uOffset )
{

  // 由相对位置计算绝对位置
  uint32_t uPosition = m_uReadIndex + uOffset;

  // 超界后返回
  if ( uPosition >= m_uCacheSize )
    return 0;

  // 返回字符
  return m_pucCache[uPosition];
}
//-----------------------------------------------------------------------------
// 取缓冲区的双字节数据, 低在前,高在后
// 输入:
//   uOffset: 从uReadIndex后从第uOffset个位置开始
// 输出:
//    返回: 双字节整数
uint32_t TCOMBuffer::GetWordLH ( uint32_t uOffset )
{

  // 取缓冲区中相对uReadIndex距离为index的一个字的高低字节组成字
  return ( uint32_t ) GetChar ( uOffset ) +
         ( uint32_t ) GetChar ( uOffset + 1 ) * 0x100;
}
//-----------------------------------------------------------------------------
// 取缓冲区的双字节数据, 高在前,低在后
// 输入:
//   uOffset: 从uReadIndex后从第uOffset个位置开始
// 输出:
//    返回: 双字节整数
uint32_t TCOMBuffer::GetWordHL ( uint32_t uOffset )
{

  // 取缓冲区中相对uReadIndex距离为index的一个字的高低字节组成字
  return ( uint32_t ) GetChar ( uOffset + 1 ) +
         ( uint32_t ) GetChar ( uOffset ) * 0x100;
}
//-----------------------------------------------------------------------------
// 取缓冲区的双字节数据
// 等价于 GetWordLH
// 输入:
//   uOffset: 从第uOffset个位置开始
// 输出:
//    返回: 双字节整数
uint32_t TCOMBuffer::GetWord ( uint32_t uOffset )
{

  return ( uint32_t ) GetChar ( uOffset ) +
         ( uint32_t ) GetChar ( uOffset + 1 ) * 0x100;
}
//-----------------------------------------------------------------------------
// 从缓冲区中读数据块
// 输入:
//     pucBuf: 保存
//  uNumBytes:
//    uOffset: 从第index个位置开始
// 输出:
//    返回: 实际读的字节数
uint32_t TCOMBuffer::Read ( void    *pvBuf,
                            uint32_t uNumBytes,
                            uint32_t uOffset )
{

#ifdef DEBUG_VER
  if ( !pucBuf || !uNumBytes )
    return 0;
#endif

  uint32_t uBytesRead;
  if ( m_uWriteIndex < m_uReadIndex + uOffset + uNumBytes )
    uBytesRead = m_uWriteIndex - ( m_uReadIndex + uOffset );
  else
    uBytesRead = uNumBytes;

  if ( 0 < uBytesRead )
    memcpy ( pvBuf, m_pucCache + m_uReadIndex + uOffset, uBytesRead );

  return uBytesRead;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 将当前缓冲区中的内容复制到目标缓冲区中
// 输入:
//     DestBuf: 目标缓冲区
//   uNumBytes: 要读的字节数  0:=全部
//     uOffset: 从GetPtr后从第uOffset个位置开始
// 输出:
//    返回: 复制的字节数
uint32_t TCOMBuffer::CopyTo ( TCOMBuffer *pDestBuf,
                                uint32_t  uNumBytes,
                                uint32_t  uOffset )
{

#ifdef DEBUG_VER
  if ( nullptr == pDestBuf )
    return 0;
#endif

   if ( 0 == uNumBytes )
    uNumBytes = Used();

  uint32_t uBytes2Write;
  if ( m_uWriteIndex < m_uReadIndex + uOffset + uNumBytes )
    uBytes2Write = m_uWriteIndex - ( m_uReadIndex + uOffset );
  else
    uBytes2Write = uNumBytes;

  if ( 0 < uBytes2Write )
    pDestBuf->Write ( m_pucCache + m_uReadIndex + uOffset, uBytes2Write );

  return uBytes2Write;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 前向移动移动取字符指针
// 输入:
//   uNumBytes: 移动的距离
// 输出:
//    返回:  当前位置
uint32_t TCOMBuffer::AdvanceReadIndex ( uint32_t uNumBytes )
{

  // 由相对位置计算绝对位置
  uint32_t uReadIndex = m_uReadIndex + uNumBytes;

  // 不许超越 m_uWriteIndex
  if ( uReadIndex > m_uWriteIndex )
    uReadIndex = m_uWriteIndex;

  // 设置取数据指针
  m_uReadIndex = uReadIndex;

  // 返回当前位置
  return uReadIndex;
}
//-----------------------------------------------------------------------------
// 前向移动移动写字符指针
// 输入:
//   uNumBytes: 移动的距离
// 输出:
//    返回:  当前位置
uint32_t TCOMBuffer::AdvanceWriteIndex ( uint32_t uNumBytes )
{

  // 由相对位置计算绝对位置
  uint32_t uWriteIndex = m_uWriteIndex + uNumBytes;

  // 不允许越界
  if ( uWriteIndex > m_uCacheSize )
    uWriteIndex = m_uCacheSize;

  // 设置取数据指针
  m_uWriteIndex = uWriteIndex;

  // 返回当前位置
  return uWriteIndex;
}
//-----------------------------------------------------------------------------
// 清理已用数据区
// 输入:
//   uOffset: 从Read指针开始额外要抛弃的字节长度
// 输出:
//   返回：缓冲区容量
uint32_t TCOMBuffer::DiscardReadMessage(uint32_t uOffset)
{

  // 由相对位置计算绝对位置
  uint32_t uReadIndex  = m_uReadIndex + uOffset;
  uint32_t uWriteIndex = m_uWriteIndex;

  if( uReadIndex < uWriteIndex )
    {
    if( 0 < uReadIndex )
      {
      memcpy( m_pucCache, m_pucCache + uReadIndex, uWriteIndex - uReadIndex );
      
      uWriteIndex -= uReadIndex;
      uReadIndex   = 0;
        
      m_uReadIndex  = uReadIndex;
      m_uWriteIndex = uWriteIndex;
      }
    }
  else
    Reset();
  
  return uWriteIndex;
}
//-----------------------------------------------------------------------------
// 在缓冲区中搜索报文首字符
//   在缓冲中内搜索指定报文的首字符,
//     若搜索成功:将uReadIndex指针移到报文起始位置,
//     若搜索失败:清空缓冲区
// 输入:  ucData: 字符
// 输出:  返回:  缓冲区中剩余的字符
uint32_t TCOMBuffer::Locate ( uint8_t ucData )
{

  uint32_t uOffset = Search ( ucData );
  if ( ( uint32_t ) -1 != uOffset )
    {
    // Wey. 2024.1.17
    uint32_t uReadIndex = m_uReadIndex + uOffset;
    if ( uReadIndex >= m_uCacheSize )
      {
      uReadIndex -= m_uCacheSize;
      }

    m_uReadIndex = uReadIndex;
    }
  else
    {
    m_uReadIndex = m_uWriteIndex;
    }
  //  return Free();
  return Used();      // 返回剩余长度.2011.02.12 LEE.ZJ改
}
//-----------------------------------------------------------------------------
// 在缓冲区中搜索字符
//   在缓冲中内搜索指定报文的字符,并计算出该字符距当前读位置的距离
// 输入:
//       ucData: 字符
// 输出:
//    返回:  目标字符距当前读指标的距离; 若为"-1"，则表明搜索失败
uint32_t TCOMBuffer::Search ( uint8_t ucData )
{

  uint32_t uReadIndex = m_uReadIndex;
  for ( ; uReadIndex < m_uWriteIndex; uReadIndex++ )
    {
    if ( m_pucCache[uReadIndex] == ucData )
      break;
    }

  return ( uReadIndex >= m_uWriteIndex ) ? ( uint32_t ) -1 :
         ( uReadIndex - m_uReadIndex );
}
//-----------------------------------------------------------------------------
// 缓冲区数据处理
//-----------------------------------------------------------------------------
// 对缓冲区信息执行指定运算
//   用给定的方法处理缓冲区中的信息
// 输入:
//     pFunc: 用于处理数据的方法
// uNumBytes: 计算长度
//   uOffset: 开始位置距当前“读位置”的偏移(bytes)
//     uSeed: 初值，CRC为0xFFFF   Checksum: 0
// 输出:
//    返回:  校验结果
uint32_t TCOMBuffer::Execute ( PBufOperator pFunc,
                                   uint32_t uNumBytes,
                                   uint32_t uOffset,
                                   uint32_t uSeed )
{

  uint32_t  uResult;
  if ( nullptr == pFunc || m_uWriteIndex <= m_uReadIndex + uOffset )
    uResult = uSeed;
  else
    {
    if ( m_uWriteIndex < m_uReadIndex + uOffset + uNumBytes )
      uNumBytes = m_uWriteIndex - m_uReadIndex - uOffset;

    uResult = pFunc ( m_pucCache + m_uReadIndex + uOffset, uNumBytes, uSeed );
    }

  return uResult;
}
//-----------------------------------------------------------------------------
// 空间查询
//-----------------------------------------------------------------------------
// 缓冲区中剩余字符数量
// 输入:
//   无
// 输出:
//    返回: 剩余字符的数量
uint32_t TCOMBuffer::Used()
{

  // Copy the Read/Write indices for calculation.
  uint32_t uWriteIndex = m_uWriteIndex;
  uint32_t uReadIndex  = m_uReadIndex;

  // Return the number of bytes contained in the buffer.
  return uWriteIndex - uReadIndex;
}
//-----------------------------------------------------------------------------
// 缓冲区中可用容量
// 输入:
//   无
// 输出:
//    返回: 剩余字符的数量
uint32_t TCOMBuffer::Free()
{

  // Copy the Read/Write indices for calculation.
  uint32_t uWriteIndex = m_uWriteIndex;

  // Return the number of bytes available in the buffer.
  return ( m_uCacheSize - uWriteIndex );
}
//-----------------------------------------------------------------------------

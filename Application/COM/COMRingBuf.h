//-----------------------------------------------------------------------------
/*
  File        : COMRingBuf.h
  Version     : V1.01
  By          : 银网科技

  Description : 定义COM的环形缓冲区
               
  Date        : 2010.3.16

  2024.01.15 v2.0
    代码规范化
*/
//-----------------------------------------------------------------------------
#ifndef COM_RingBuf_H_
#define COM_RingBuf_H_

#include "COMBuffer.h"
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 类定义
//-----------------------------------------------------------------------------
class TCOMRingBuffer : public TCOMBuffer
{
public:
  // 构造
  // 输入:
  //    pvBuf: 缓冲区指针
  //    uSize: 缓冲区尺寸
  TCOMRingBuffer(void *pvBuf, uint32_t uSize);
  
  // 保存字符到缓冲区
  // 输入:
  //      ucData: 需要保存的字符
  // 输出:
  //    返回: 存贮字符的指针位置
  virtual uint32_t PutChar(uint8_t ucData);
  // 保存一个字符串到缓冲区
  // 输入:
  //    pucBuf: 需要的字串所在指针
  //    uNumBytes: 数据长度
  // 输出:
  //    返回: 当前存贮指针的位置
  virtual uint32_t Write(const void *pvBuf, uint32_t uNumBytes);

  // 取缓冲区相对取指针的字符
  // 输入:
  //   uOffset: 从GetPtr后第uOffset个字符
  // 输出:
  //   字符
  virtual uint8_t GetChar(uint32_t uOffset);

  // 从缓冲区中读数据块
  // 输入:
  //     pucBuf: 存贮读出数据的内存空间
  // uNumBytes: 要读的字节数
  //   uOffset: 从第uOffset个位置开始
  // 输出:
  //    返回: 实际读的字节数
  virtual uint32_t Read(void* pvBuf, uint32_t uNumBytes, uint32_t uOffset);

  // 将当前缓冲区中的内容复制到目标缓冲区中
  // 输入:
  //       pBuf: 目标缓冲区
  // uNumBytes: 要读的字节数  0:=全部
  //   uOffset: 从GetPtr后从第uOffset个位置开始
  // 输出:
  //    返回: 复制的字节数
  virtual uint32_t CopyTo( TCOMBuffer *pBuf, 
                             uint32_t  uNumBytes = 0, 
                             uint32_t  uOffset = 0 );

  // 前向移动移动取字符指针
  // 输入:
  //   uNumBytes: 移动的距离
  // 输出:
  //    返回:  当前位置
  virtual uint32_t AdvanceReadIndex(uint32_t uNumBytes);

  // 在缓冲区中搜索字符
  //   在缓冲中内搜索指定报文的字符,并计算出该字符距当前读位置的距离
  // 输入:
  //       ucData: 字符
  // 输出:
  //    返回:  目标字符距当前读指标的距离; 若为"-1"，则表明搜索失败
  virtual uint32_t Search(uint8_t ucData);

  // 对缓冲区信息执行指定运算
  //   用给定的方法处理缓冲区中的信息
  // 输入:
  //      pFunc: 用于处理数据的方法，要求pFunc的输出可作为下一段buf处理的Seed
  //  uNumBytes: 计算长度
  //    uOffset: 开始位置距当前“读位置”的偏移(bytes)
  //      uSeed: 初值，CRC为0xFFFF   Checksum: 0
  // 输出:
  //    返回:  校验结果
  virtual uint32_t Execute(PBufOperator pFunc, 
                               uint32_t uNumBytes, 
                               uint32_t uOffset = 0, 
                               uint32_t uSeed  = 0xFFFFFFFF);

  // 缓冲区中剩余字符数量
  // 输入:
  //   无
  // 输出:
  //    返回: 剩余字符的数量
  virtual uint32_t Used(void);
  // 缓冲区中可用容量
  // 输入:
  //   无
  // 输出:
  //    返回: 可用的容量
  virtual uint32_t Free(void);
};
//-----------------------------------------------------------------------------
#endif // __COMRingBuf_H

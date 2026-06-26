//-----------------------------------------------------------------------------
/*
  File        : COMBuffer.h
  Version     : V1.01
  By          : 银网科技

  Description : 定义COM的缓冲区
               
  Date        : 2010.3.16

  2024.01.15 v2.0
    代码规范化
*/
//-----------------------------------------------------------------------------
#ifndef COM_Buffer_H_
#define COM_Buffer_H_

#include <cstdint>
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------

//=============================================================================
// 全局数据结构
//-----------------------------------------------------------------------------
// 用于处理缓冲区数据的函数指针
// 输入：
//      pucBuf：报文缓冲区指针
//  uNumBytes: 要处理的长度
//      uSeed: 初始值
typedef uint32_t (*PBufOperator)(const void *pvBuffer,  
                                   uint32_t  uNumBytes, 
                                   uint32_t  uSeed);
//=============================================================================
// 类定义
//-----------------------------------------------------------------------------
class TCOMBuffer
{
protected:
    // The buffer size.
    uint32_t            m_uCacheSize;

    // The buffer read offset.
    volatile uint32_t   m_uWriteIndex;

    // The buffer read offset.
    volatile uint32_t   m_uReadIndex;

    // The buffer.
    uint8_t            *m_pucCache;

public:
  // 默认构造
  TCOMBuffer(void);

  // 构造
  // 输入:
  //    pBuf: 缓冲区指针
  //   aSize: 缓冲区尺寸
  TCOMBuffer(void *pucBuf, uint32_t aSize);

  // 构造
  // 输入:
  //    pBuf: 缓冲区指针
  //   aSize: 缓冲区尺寸
  // 输出:
  //   无
  void SetBuffer(void *pucBuf, uint32_t aSize);

  // 清空缓冲区
  // 输入:
  //   无
  // 输出:
  //   返回：缓冲区容量
  uint32_t Reset(void);

  // 保存字符到缓冲区
  // 输入:
  //      ucData: 需要保存的字符
  // 输出:
  //    返回: 存贮字符的指针位置
  virtual uint32_t PutChar(uint8_t ucData);

  // 保存一个字符串到缓冲区
  // 输入:
  //    buf: 需要写入的字串所在指针
  //    uNumBytes: 数据长度
  // 输出:
  //    返回: 当前存贮指针的位置
  virtual uint32_t Write(const void *pvBuf, uint32_t uNumBytes);

  // 保存一个字到缓冲区, 低字节在前，高字节在后
  // 输入:
  //    uwData: 需要保存的数据
  // 输出:
  //    返回: 当前存贮指针的位置
  uint32_t PutWordLH(uint16_t uwData);

  // 保存一个字到缓冲区，高字节在后, 低字节在前
  // 输入:
  //    uwData: 需要保存的数据
  // 输出:
  //    返回: 当前存贮指针的位置
  uint32_t PutWordHL(uint16_t uwData);

  // 取缓冲区相对取指针的字符
  // 输入:
  //   uOffset: 从GetPtr后第uOffset个字符
  // 输出:
  //   字符
  virtual uint8_t GetChar(uint32_t uOffset);

  // 取缓冲区的双字节数据, 低在前,高在后
  // 输入:
  //   uOffset: 从GetPtr后从第uOffset个位置开始
  // 输出:
  //    返回: 双字节整数
  uint32_t GetWordLH(uint32_t uOffset);

  // 取缓冲区的双字节数据, 高在前,低在后
  // 输入:
  //   uOffset: 从GetPtr后从第index个位置开始
  // 输出:
  //    返回: 双字节整数
  uint32_t GetWordHL(uint32_t uOffset);

  // 取缓冲区的双字节数据
  // 等价于 GetWordLH
  // 输入:
  //   uOffset: 从第uOffset个位置开始
  // 输出:
  //    返回: 双字节整数
  uint32_t GetWord(uint32_t uOffset);

  // 从缓冲区中读数据块
  // 输入:
  //     pucBuf: 存贮读出数据的内存空间
  // uNumBytes: 要读的字节数
  //   uOffset: 从第uOffset个位置开始
  // 输出:
  //    返回: 实际读的字节数
  virtual uint32_t Read(void *pvBuf, uint32_t uNumBytes, uint32_t uOffset = 0);

  // 将当前缓冲区中的内容复制到目标缓冲区中
  // 输入:
  // uNumBytes: 要读的字节数  0:=全部
  //   uOffset: 从GetPtr后从第index个位置开始
  // 输出:
  //    返回: 复制的字节数
  virtual uint32_t CopyTo( TCOMBuffer *pDestBuf, 
                             uint32_t  uNumBytes = 0, 
                             uint32_t  uOffset = 0 );

  // 前向移动移动取字符指针
  // 输入:
  //   uNumBytes: 移动的距离
  // 输出:
  //    返回:  当前位置
  virtual uint32_t AdvanceReadIndex(uint32_t uNumBytes);

  // 前向移动移动写字符指针
  // 输入:
  //   uNumBytes: 移动的距离
  // 输出:
  //    返回:  当前位置
  virtual uint32_t AdvanceWriteIndex(uint32_t uNumBytes);

  // 清理已用数据区
  // 输入:
  //   uOffset: 从Read指针开始额外要抛弃的字节长度
  // 输出:
  //   返回：缓冲区容量
  uint32_t DiscardReadMessage(uint32_t uOffset = 0);

  // 在缓冲区中定位字符
  //   在缓冲中内查找指定的字符, 并舍弃指定字符之前的内容
  //     若搜索成功:将读指针移到报文起始位置, 
  //     若搜索失败:清空缓冲区
  // 输入:
  //       ucData: 字符
  // 输出:
  //    返回:  缓冲区中剩余的字符
  virtual uint32_t Locate(uint8_t ucData);

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
  // uNumBytes: 计算长度
  //   uOffset: 开始位置距当前“读位置”的偏移(bytes)
  //     uSeed: 初值，CRC为0xFFFF   Checksum: 0
  // 输出:
  //    返回:  校验结果
  virtual uint32_t Execute( PBufOperator pFunc, 
                                uint32_t uNumBytes, 
                                uint32_t uOffset = 0, 
                                uint32_t uSeed = 0xFFFFFFFF);

  // 缓冲区尺寸
  // 输入:
  //   无
  // 输出:
  //    返回: 缓冲区尺寸
  uint32_t Size(void) { return m_uCacheSize; };

  // 缓冲区中已有字符数量
  // 输入:
  //   无
  // 输出:
  //    返回: 已存字符的数量
  virtual uint32_t Used(void);
  // 缓冲区中可用容量
  // 输入:
  //   无
  // 输出:
  //    返回: 可用的容量
  virtual uint32_t Free(void);
  
  // 缓冲区指针
  // 输入:
  //   无
  // 输出:
  //    返回: 缓冲区指针
  uint8_t* getBuffer(void) { return m_pucCache; }

  // 报文指针
  // 输入:
  //   无
  // 输出:
  //    返回: 缓冲区从读指针开始的缓冲区
  const uint8_t* MessageBuffer(void) { return m_pucCache + m_uReadIndex; }

  // 返回当前写指针开始的缓冲区位置
  // 输入:
  //   无
  // 输出:
  //    返回: 可写入的缓冲区指针
  uint8_t* ValidBufPtr(void) { return m_pucCache + m_uWriteIndex; }
};
//-----------------------------------------------------------------------------

#endif // __COMBuffer_H

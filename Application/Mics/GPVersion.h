//-----------------------------------------------------------------------------
/*
   软件各模块的版本信息
   
   银网科技

   Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef __GPVersion_H_
#define __GPVersion_H_

#include <stdint.h>
//=============================================================================
// 公用宏
//-----------------------------------------------------------------------------
// 装置类型的宏
// 产品系列
#define Dev_Series(x)   ((x) & 0xFC00)  // 6 0~63
#define Dev_SG1210      (60 << 10)

// MCU类型
#define Dev_MCU(x)      ((x) & 0x0380)  // 3 0~7
#define Dev_MCU_M0      (0 << 7)
#define Dev_MCU_M3      (1 << 7)
#define Dev_MCU_M4      (2 << 7)
#define Dev_MCU_M7      (3 << 7)
#define Dev_MCU_H7      (4 << 7)
// 液晶规格型号
#define Dev_LCD(x)      ((x) & 0x070) // 3: 0~7
#define Dev_LCD_None    (0 << 4)
#define Dev_LCD_BW      (1 << 4)
#define Dev_LCD_TFT3224 (2 << 4)
#define Dev_LCD_TFT4824 (3 << 4)
#define Dev_LCD_BW1616  (4 << 4)
#define Dev_LCD_EXT     (7 << 4)
// 功能板件类型
#define Dev_SubBrd(x)   ((x) & 0x000E) // 3: 0~7
#define Dev_Host        (0 << 1)
#define Dev_Route       (1 << 1)
#define Dev_AI          (2 << 1)
#define Dev_DI          (3 << 1)
#define Dev_D0          (4 << 1)
// 当前固件类型
#define Dev_Firmware(x) ((x) & 0x0001) // 1: 0/1
#define Dev_FWIAP       (0 << 0)
#define Dev_FWMain      (1 << 0)
//-----------------------------------------------------------------------------
// 装置型号
#define Dev_Modual    ((uint16_t)( Dev_XDSG    +     \
                                   Dev_MCU_M4  +     \
                                   Dev_LCD_TFT3224 + \
                                   Dev_Host    +     \
                                   Dev_FWMain ))

// 装置硬件的版本
// 指装置硬件档案号
#define Hardware_Ver    2002430u

// 装置固件的版本
// 每次功能升级后调整
#define Firmware_Ver    233

// 装置固件的类型演化
// 每个不同功能的演化，要更新这个号
#define Firmware_Evo    1

// 每次功能修改后调整
#define Firmware_Impr   2532

// 根据装配支持的通讯配置填写
// 用一个Word表示装置配置的通讯端口
// 各位意义如下：
//   x000：以太网的数量 0~15
//   0x00：CAN网的数量  0~15
//   00x0：串口的数量   0-15
#define CommMida_Mount  0x0010
  #define Num_Comm_NIC  ((CommMida_Mount & 0xF000) >> 12)
  #define Num_Comm_CAN  ((CommMida_Mount & 0x0F00) >> 8)
  #define Num_Comm_SER  ((CommMida_Mount & 0x00F0) >> 4)
//-----------------------------------------------------------------------------

//=============================================================================
// 公用方法
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"  {
#endif
//-----------------------------------------------------------------------------
// 取版本信息
// 输入：
//   pcBuf：指向缓存的
// 返回：
//   版本信息填写在 pcBuf 指向的空间
uint32_t GetDevVersion(const uint8_t* pcBuf);
//-----------------------------------------------------------------------------
// 取设备标识
// 输入：
//   pcBuf：指向缓存的
// 返回：
//   版本信息填写在 pcBuf 指向的空间
uint32_t GetDevIdent(uint8_t* pucBuf);
//-----------------------------------------------------------------------------
// 取设备固件校验和
// 输入：无
// 返回：固件校验和   
uint32_t GetFrimwareIdent(void);
//-----------------------------------------------------------------------------
// 取设备标识检验
// 输入：
// 返回：设备标识检验
uint32_t GetDevIdentCRC(void);
//=============================================================================
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // __GPLVersion_H_

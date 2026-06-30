//----------------------------------------------------------------------------
/*
 File        : NvRAM.cpp
 Version     : V1.10
 By          : 银网科技
 Description :管理外部存贮器
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "NvRAM.h"


#ifdef __vmSIMULATOR__
  #define  NvRamFileName  "SG1210.nvram"
  #define  FlashFileName  "SG1210.flash"

  #define  FeRAM_SIZE        2048
  #define  FLASH_SIZE        0x200000
  #define  FLASH_SIZE_SECTOR 4096

  #include <string.h>
  #include <Windows.h>
  #include <strsafe.h> // 用于 StringCchCopyA / StringCchCatA

#else
  #include "MB85RSxx.h"
  #include "SPIFlash.h"

  #include "GPVersion.h"
  
  #include "cmsis_os.h"
#endif
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------

//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------

//=============================================================================
// 全局方法引用
//-----------------------------------------------------------------------------

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
#ifdef __vmSIMULATOR__
// 生成 NVRAM 文件名：将 EXE 所在目录与 pfName 拼接到 dest 中
// dest 应至少能容纳 MAX_PATH 字符
static bool genNVFileName(const char* pfName, char* dest, size_t destSize)
{
    if (!pfName || !dest || destSize == 0) {
        if (dest) dest[0] = '\0';
        return false;
    }

    // 1. 获取执行文件完整路径
    char exePath[MAX_PATH] = { 0 };
    DWORD iLength = GetModuleFileNameA(nullptr, exePath, sizeof(exePath));
    if (iLength == 0 || iLength >= sizeof(exePath)) {
        dest[0] = '\0';
        return false;
    }

    // 2. 去掉文件名，保留目录（含末尾分隔符）
    char* pLastSep = strrchr(exePath, '\\');
    if (!pLastSep) pLastSep = strrchr(exePath, '/');
    if (!pLastSep) {
        dest[0] = '\0';
        return false;
    }
    pLastSep[1] = '\0';  // 在分隔符后截断，保留 "D:\path\"

    // 3. 安全拼接：目录 + 文件名
    if (FAILED(StringCchCopyA(dest, destSize, exePath)))
        return false;
    if (FAILED(StringCchCatA(dest, destSize, pfName)))
        return false;

    return true;
}
//-----------------------------------------------------------------------------
// 检查fileName是否存在，不存在则创建并写入初始数据（0xFFFFFFFF），
// 总字节数为 totalBytes
static bool createNvFile(const char* fullPath, const int fileSize)
{

    if (!fullPath || sizeof(DWORD) > fileSize)
        return false;

    // 检查文件是否存在；若不存在则尝试创建
    WIN32_FILE_ATTRIBUTE_DATA fileInfo = { 0 };
    if( 0 != GetFileAttributesExA( fullPath, GetFileExInfoStandard, &fileInfo ) )
      {
        // 文件存在，检查大小是否匹配；如果匹配则认为创建成功，无需覆盖
        if (INVALID_FILE_ATTRIBUTES != fileInfo.dwFileAttributes && 
             fileSize == fileInfo.nFileSizeLow ) 
            return true;
      }

    // 创建或覆盖文件
    HANDLE hFile = CreateFileA(fullPath,
        GENERIC_WRITE,
        0, // 不共享
        nullptr,
        CREATE_ALWAYS, // 覆盖或创建
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
      // CreateFileA 返回 INVALID_HANDLE_VALUE 表示打开文件失败，
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    bool bResult = true;

    // 将文件内容初始化为 0xFFFFFFFF，写入 totalBytes 字节
    for (int nSize = 0; nSize < fileSize && true == bResult; nSize += sizeof(DWORD)) {
        DWORD written = 0;
        DWORD data = 0xFFFFFFFF;
        // WriteFile 返回 0 表示写入失败，可能是由于文件访问权限、磁盘空间不足等；或者写入的字节数不匹配；
        if (0 == WriteFile(hFile, &data, sizeof(DWORD), &written, nullptr) || sizeof(DWORD) != written)
            bResult = false;
    }

    CloseHandle(hFile);
    return bResult;
}
//-----------------------------------------------------------------------------
// 读取非易失性数据内容到 buffer 中，确保读取的字节数与 size 匹配
static bool readNvFile(const char* fileName, const uint32_t uOffset, 
                             void* pBuffer,  const size_t totalBytes, 
                       const size_t uMaxSize)
{
    // 生成文件名
    // 判断文件是否存在，若不存在，则创建并写入初始数据
    // 读取文件内容到 buffer 中，确保读取的字节数与 size 匹配
    if (!fileName || !pBuffer || totalBytes == 0 || uOffset + totalBytes > uMaxSize)
        return false;

    // 生成文件名
    char fullPath[MAX_PATH] = { 0 };
    if (false == genNVFileName(fileName, fullPath, sizeof(fullPath)))
        return false;

    // 检查文件是否存在；若不存在则尝试创建
    if (false == createNvFile(fullPath, uMaxSize))
        return false;

    //	创建文件句柄以读取内容
    HANDLE hFile = CreateFileA(fullPath,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    // CreateFileA 返回 INVALID_HANDLE_VALUE 表示打开文件失败，
    // 可能是由于文件访问权限、路径错误等；需要返回 false
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    // 设置文件指针到指定偏移位置，FILE_BEGIN 表示从文件开头计算偏移
    DWORD dwPtrLow = SetFilePointer(hFile, (LONG)uOffset, nullptr, FILE_BEGIN);
    // SetFilePointer 返回 INVALID_SET_FILE_POINTER 表示设置文件指针失败，
    if (dwPtrLow == INVALID_SET_FILE_POINTER) {
        CloseHandle(hFile);
        return false;
    }

    // 读取文件内容到 buffer 中，确保读取的字节数与 size 匹配
    DWORD toRead = (DWORD)totalBytes;
    DWORD totalRead = 0;
    BYTE* dst = (BYTE*)pBuffer;

    // 可能需要多次调用 ReadFile 以确保读取完整内容，直到达到 size 或文件末尾
    while (totalRead < toRead) {
        DWORD chunk = toRead - totalRead;
        DWORD read = 0;
        // ReadFile 返回 0 表示读取失败，可能是由于文件访问权限、硬件问题等；
        // 需要关闭句柄并返回 false
        if (!ReadFile(hFile, dst + totalRead, chunk, &read, nullptr)) {
            CloseHandle(hFile);
            return false;
        }
        if (read == 0) {
            // 到达文件末尾，填充剩余为 0
            break;
        }

        // 读取成功，更新总读取字节数；如果读取的字节小于请求，
        totalRead += read;
    }

    CloseHandle(hFile);

    // 返回读取结果，只有当 totalRead 与 totalBytes 完全匹配时才返回 true
    return (totalRead == totalBytes);
}
//-----------------------------------------------------------------------------
// 将 buffer 的内容写入非易失性存储，确保写入的字节数与 size 匹配
static bool writeNvFile(const char* fileName, const uint32_t uOffset, 
                        const void* pBuffer,  const size_t   totalBytes, 
                        const size_t uMaxSize)
{
    // 生成 Flash 文件名
    // 判断文件是否存在，若不存在，则创建并写入初始数据
    // 将buffer的内容写入文件中，确保写入的字节数与 size 匹配
    if (!fileName || !pBuffer || totalBytes == 0 || uOffset + totalBytes > uMaxSize)
        return false;

    // 生成 NvRam 文件名
    char fullPath[MAX_PATH] = { 0 };
    if (false == genNVFileName(fileName, fullPath, sizeof(fullPath)))
        return false;

    // 检查文件是否存在；若不存在则尝试创建
    if (false == createNvFile(fullPath, uMaxSize))
        return false;

    //  创建文件句柄以读取内容
    HANDLE hFile = CreateFileA(fullPath,
        GENERIC_WRITE,
        0, // 不共享
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    // CreateFileA 返回 INVALID_HANDLE_VALUE 表示打开文件失败，
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    // 设置文件指针到指定偏移位置，FILE_BEGIN 表示从文件开头计算偏移
    DWORD dwPtrLow = SetFilePointer(hFile, (LONG)uOffset, nullptr, FILE_BEGIN);
    // SetFilePointer 返回 INVALID_SET_FILE_POINTER 表示设置文件指针失败，
    if (dwPtrLow == INVALID_SET_FILE_POINTER) {
        CloseHandle(hFile);
        return false;
    }

    // 读取文件内容到 buffer 中，确保读取的字节数与 size 匹配
    DWORD toWrite = (DWORD)totalBytes;
    DWORD totalWrite = 0;
    BYTE* dest = (BYTE*)pBuffer;

    //  可能需要多次调用 writeFile 以确保读取完整内容，直到达到 size 或文件末尾
    while (totalWrite < toWrite) {
        DWORD chunk = toWrite - totalWrite;
        DWORD writen = 0;
        // ReadFile 返回 0 表示写入失败，可能是由于文件访问权限、硬件问题等；
        // 需要关闭句柄并返回 false
        if (0 == WriteFile(hFile, dest + totalWrite, chunk, &writen, nullptr)) {
            CloseHandle(hFile);
            return false;
        }
        if (writen == 0) {
            // 到达文件末尾，填充剩余为 0
            break;
        }
        // 写入成功，更新总写入字节数；如果写入的字节小于请求，
        // 继续循环会返回 0 或更多直到达到 EOF
        totalWrite += writen;
    }

    // 
    CloseHandle(hFile);

    // 返回写入结果，只有当 totalWrite 与 totalBytes 完全匹配时才返回 true
    return (totalWrite == totalBytes);
}

//-----------------------------------------------------------------------------
// 模拟器中读取 NvRAM 文件内容到 buffer 中，确保读取的字节数与 size 匹配
static bool readNvRAMFile(const uint32_t uOffset, void* pBuffer, const size_t size)
{

  return readNvFile(NvRamFileName, uOffset, pBuffer, size, FeRAM_SIZE);
}
//-----------------------------------------------------------------------------
// 模拟器中将 buffer 的内容写入 NvRAM 文件，确保写入的字节数与 size 匹配
static bool writeNvRamFile(const uint32_t uOffset, const void* pBuffer, const size_t size)
{

  return writeNvFile(NvRamFileName, uOffset, pBuffer, size, FeRAM_SIZE);
}
//-----------------------------------------------------------------------------
// 模拟器中读取 Flash 文件内容到 buffer 中，确保读取的字节数与 size 匹配
static bool readFlashFile(const uint32_t uOffset, void* pBuffer, const size_t size)
{

  return readNvFile(FlashFileName, uOffset, pBuffer, size, FLASH_SIZE);
}
//-----------------------------------------------------------------------------
// 模拟器中将 buffer 的内容写入 Flash 文件，确保写入的字节数与 size 匹配
static bool writeFlashFile(const uint32_t uOffset, const void* pBuffer, const size_t size)
{

    return writeNvFile(FlashFileName, uOffset, pBuffer, size, FLASH_SIZE);
}
//-----------------------------------------------------------------------------
static bool earseFlashFile(const uint32_t uAddress, const size_t size)
{

    // 参数检查：地址和大小必须在 Flash 范围内，且大小不能为0
    if (uAddress + size > FLASH_SIZE || 0 == size)
        return false;

    // 生成 Flash 文件名
    char fullPath[MAX_PATH] = { 0 };
    if (false == genNVFileName(FlashFileName, fullPath, sizeof(fullPath)))
        return false;

    // 检查文件是否存在；若不存在则尝试创建
    if (false == createNvFile(fullPath, FLASH_SIZE))
        return false;

    //  创建文件句柄以读取内容
    HANDLE hFile = CreateFileA(fullPath,
        GENERIC_WRITE,
        0, // 不共享
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    // CreateFileA 返回 INVALID_HANDLE_VALUE 表示打开文件失败，
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    // 计算起始地址，并向下取整到当前扇区边界
    uint32_t uSectorStart = (uAddress / FLASH_SIZE_SECTOR) * FLASH_SIZE_SECTOR;
    // 计算结束地址，并向上取整到下一个扇区边界
    uint32_t uSectorEnd = ((uAddress + size + FLASH_SIZE_SECTOR - 1) / FLASH_SIZE_SECTOR) * FLASH_SIZE_SECTOR;

    // 设置文件指针到指定偏移位置，FILE_BEGIN 表示从文件开头计算偏移
    DWORD dwPtrLow = SetFilePointer(hFile, (LONG)uSectorStart, nullptr, FILE_BEGIN);
    // SetFilePointer 返回 INVALID_SET_FILE_POINTER 表示设置文件指针失败，
    if (dwPtrLow == INVALID_SET_FILE_POINTER) {
        CloseHandle(hFile);
        return false;
    }

    // 计算需要擦除的扇区数量
    uint32_t uSectors = (uSectorEnd - uSectorStart) / FLASH_SIZE_SECTOR;
    // 计算实际需要写入的字节数，可能会超过 size，因为擦除是以扇区为单位的
    DWORD toWrite = (DWORD)uSectors * FLASH_SIZE_SECTOR;

    bool bResult = true;

    // 将文件内容初始化为 0xFFFFFFFF，写入 totalBytes 字节
    for (DWORD nSize = 0; nSize < toWrite; nSize += sizeof(DWORD)) {
        DWORD written = 0;
        DWORD data = 0xFFFFFFFF;
        // WriteFile 返回 0 表示写入失败，可能是由于文件访问权限、磁盘空间不足等；或者写入的字节数不匹配；
        if (0 == WriteFile(hFile, &data, sizeof(DWORD), &written, nullptr) || sizeof(DWORD) != written) {
            bResult = false;
            break;
        }    
    }

    return bResult;
}
#endif

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 初始化
void NvRAM_Init(void)
{
  
#ifndef __vmSIMULATOR__
  MB85RSxx_Init();
#endif
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                 FeRAM访问
//-----------------------------------------------------------------------------
// FeRAM尺寸
uint32_t NvRAM_GetFeRamSize(void)
{
  
  return FeRAM_SIZE;
}
//-----------------------------------------------------------------------------
// 读FeRAM
// 返回：0=成功
uint32_t NvRAM_Read( const uint32_t uAddr, 
                           void*    pvBuff, 
                           uint32_t uBytesToRead )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pvBuff || 0 == uBytesToRead, GFC_ErrParam );
#endif
    
  uint32_t uTryCntr = 3;
  while( uTryCntr > 0 )
    {
#ifdef __vmSIMULATOR__
      // 模拟器中直接从文件读取，成功则返回0，否则重试3次后返回2
      if (true == readNvRAMFile(uAddr, pvBuff, uBytesToRead))
          break;
#else
      // 直接从 FeRAM 读取，成功则返回0，否则重试3次后返回2
    if( MB85RSxx_Read( uAddr, pvBuff, uBytesToRead ) == uBytesToRead )
      break;
#endif

    uTryCntr--;
    }
    
  return (uTryCntr > 0)? 0 : 2; 
}  
//-----------------------------------------------------------------------------
// 写FeRAM
uint32_t NvRAM_Write( const uint32_t uToken,
                      const uint32_t uAddr, 
                      const void*    pvBuff, 
                            uint32_t uBytesToWrite )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_NvRAM_ACCESS != uToken, GFC_ErrToken );
  
  DEV_ASSERT( nullptr == pvBuff || 0 == uBytesToWrite, GFC_ErrParam );
#endif

#ifdef __vmSIMULATOR__
  // 模拟器中直接写入文件，成功则返回0，否则返回3
  if (true == writeNvRamFile(uAddr, (void*) pvBuff, uBytesToWrite))
      return 0;
  else
      return 3;
#else
  // 允许写入
  MB85RSxx_EnabledWrite( TOKEN_WriteFeRAM_Enabled );

  // 尝试3次
  uint32_t uTryCntr = 3;
  while( uTryCntr > 0 )
    {
      // 直接写入 FeRAM，成功则返回0，否则重试3次后返回3
    if( MB85RSxx_Write( uAddr, pvBuff, uBytesToWrite ) == uBytesToWrite )
      break;

    uTryCntr--;
    }

  // 禁止写入
  MB85RSxx_EnabledWrite( 0 );

  return (uTryCntr > 0)? 0 : 3;
#endif
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                 Flash访问
//-----------------------------------------------------------------------------
// 初始化Flash
void NvRAM_InitFlash(void)
{

#if !defined(__vmSIMULATOR__) && defined(DEV_SPIFLASH_H)
  SPIFLASH_Init();
  
  // 识别Flash 芯片
  uint32_t uID = SPIFLASH_GetID();
  if( IS_VALID_JID(uID) )
    {
#ifdef REG_HW_FAULT
    // 清标识
    ClrHWFault(RHF_ExFLASH_ERR);
#endif
    
//    // debug only
//    DevChk_FlashTest(); 
    }
  else
    {
#ifdef REG_HW_FAULT
    // 发送事件  
    if( 0 == GetHWFault(RHF_ExFLASH_ERR) )
      {
#if NUM_EVENTLOGS > 0
      EVTMGR_AppendEvent( REG_EH_ExFLASH_FAULT, STATE_TRUE );
#endif

      // 置标识
      SetHWFault(RHF_ExFLASH_ERR);
      }
#endif
    }
#endif

#ifdef __vmSIMULATOR__
    createNvFile(FlashFileName, FLASH_SIZE);
#endif // __vmSIMULATOR__
}
//-----------------------------------------------------------------------------
// Flash尺寸
uint32_t NvRAM_GetFlashSize(void)
{

#ifndef  __vmSIMULATOR__
  #if defined(DEV_SPIFLASH_H)
    TFlashParameter FlashParam;
    SPIFLASH_GetFlashParam( &FlashParam );
    return FlashParam.Capacity;
  #else
    return 0;
  #endif
#else
  return FLASH_SIZE;
#endif
}
//-----------------------------------------------------------------------------
// 获取Flash扇区尺寸
uint32_t NvRAM_FlashSectorSize(void)
{
  
#ifdef __vmSIMULATOR__
  return FLASH_SIZE_SECTOR;
#else
  #if defined(DEV_SPIFLASH_H)
    TFlashParameter FlashParam;
    SPIFLASH_GetFlashParam( &FlashParam );
    return FlashParam.SectorSize;
  #else
    return 0;
  #endif
#endif
}
//-----------------------------------------------------------------------------
// 从Flash中读
//-----------------------------------------------------------------------------
// 从Flash中读
uint32_t NvRAM_FlashRead ( uint32_t  uAddress, 
                               void *pvBuf,
                           uint32_t  uBytesToRead )
{
  
#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pvBuf || 0 == uBytesToRead, GFC_ErrParam );
#endif

  uint32_t uRes;
#ifdef __vmSIMULATOR__
  if (true == readFlashFile(uAddress, pvBuf, uBytesToRead))
      uRes = uBytesToRead;
  else {
      memset(pvBuf, 0, uBytesToRead);

      uRes = 0;
  }
#else
  #if defined(DEV_SPIFLASH_H)
    uRes = SPIFLASH_Read( uAddress, pvBuf, uBytesToRead );
  #else
    uRes = 0;
  #endif
#endif

  return uRes;
}
//-----------------------------------------------------------------------------
// 写使能
void NvRAM_FlashWriteEnable( const uint32_t uToken )
{
  
#if !defined(__vmSIMULATOR__) && defined(DEV_SPIFLASH_H)
  if( TOKEN_NvRAM_ACCESS == uToken ) 
    SPIFLASH_EnabledWrite(TOKEN_WriteFLASH_Enabled);
  else
    SPIFLASH_EnabledWrite(0);
#endif  
}  
//-----------------------------------------------------------------------------
// 擦除Flash
uint32_t NvRAM_FlashErase( const uint32_t uToken,
                           const uint32_t uAddress,
                                 uint32_t uBytesToErase )
{

#ifndef __vmSIMULATOR__
 #if defined(DEV_SPIFLASH_H)
    TFlashParameter FlashParam;
    SPIFLASH_GetFlashParam( &FlashParam );

    if( 0 == FlashParam.Capacity || 0 == FlashParam.SectorSize )
      return 5;

  #ifdef USE_DEV_ASSERT
    DEV_ASSERT( FlashParam.Capacity - FlashParam.SectorSize < uAddress ||
                FlashParam.Capacity < uAddress + uBytesToErase, GFC_ErrParam );
  #endif
 #else
    return 4;
 #endif
#else
 #ifdef USE_DEV_ASSERT
  DEV_ASSERT( FLASH_SIZE - FLASH_SIZE_SECTOR < uAddress ||
              FLASH_SIZE < uAddress + uBytesToErase, GFC_ErrParam  );
 #endif
#endif

 #ifdef USE_DEV_ASSERT
  DEV_ASSERT( TOKEN_NvRAM_ACCESS != uToken, GFC_ErrToken );
 #else
  if( TOKEN_NvRAM_ACCESS != uToken )
    return 3;
 #endif

#ifndef __vmSIMULATOR__
  // 计算起始扇区和扇区数
  uint32_t uSecNo  = uAddress / FlashParam.SectorSize,
           uSecNum = (uBytesToErase + (FlashParam.SectorSize - 1)) /
                     FlashParam.SectorSize,
           uRes;
  
  SPIFLASH_EnabledWrite(TOKEN_WriteFLASH_Enabled);

  uRes = SPIFLASH_EraseSectors( uSecNo, uSecNum );

  SPIFLASH_EnabledWrite(0);

  return (uRes == uSecNum)? 0 : 2;
 #else
  return 0;
 #endif
}
//-----------------------------------------------------------------------------
// 向Flash中写
uint32_t NvRAM_FlashWrite( uint32_t uToken,
                           uint32_t uAddress, 
                        const void* pvBuf, 
                           uint32_t uBytesToWrite)
{
  
 #ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pvBuf || 0 == uBytesToWrite, GFC_ErrParam );

  DEV_ASSERT( TOKEN_NvRAM_ACCESS != uToken, GFC_ErrToken );
#endif

#ifdef __vmSIMULATOR__
  // 模拟器中直接写入文件，成功则返回写入字节数，否则返回0
  if (true == writeFlashFile(uAddress, pvBuf, uBytesToWrite))
      return uBytesToWrite;
  else
      return 0;
#else
  SPIFLASH_EnabledWrite(TOKEN_WriteFLASH_Enabled);

  uint32_t uRes;
  // 直接写入 Flash，成功则返回写入字节数，否则返回0
  if( uBytesToWrite == SPIFLASH_Write( uAddress, pvBuf, uBytesToWrite ) )
    uRes = uBytesToWrite;
  else
    uRes = 0;

  SPIFLASH_EnabledWrite(0);

  return uRes;
 #endif 
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 向Flash中写
// 自动决定是否要擦，bEarse参数用于强制擦除
// 返回：实际写入的字节数
uint32_t NvRAM_WriteToFlash( const uint32_t  uToken,
                                   uint32_t  uAddress, 
                                 const void *pvBuf, 
                                   uint32_t  uBytesToWrite,
                                       bool  bEarse,
                                  NvRCbkNon  pProc )
{

#ifdef USE_DEV_ASSERT
  DEV_ASSERT( nullptr == pvBuf || 0 == uBytesToWrite, GFC_ErrParam  );

  // check Token
  DEV_ASSERT( TOKEN_NvRAM_ACCESS != uToken, GFC_ErrToken );
#endif

  // 逐扇区写入
  uint32_t uBytesWritten = 0;
#ifdef __vmSIMULATOR__
  // 如果要求擦除，则直接覆盖整个 Flash 文件以模拟擦除效果
  if (true == bEarse && false == earseFlashFile(uAddress, uBytesToWrite))
     return false;

  // 模拟器中直接写入文件，成功则返回写入字节数，否则返回0
  if (true == writeFlashFile(uAddress, pvBuf, uBytesToWrite))
      uBytesWritten = uBytesToWrite;
  else
      return 0;
#else
  // Flash每个扇区的尺寸
  uint32_t uSectorSize  = NvRAM_FlashSectorSize();
  if( 1024 > uSectorSize )
    return 0;

  // 判断是否要强制擦
  if( 0 == (uAddress % uSectorSize) )
    bEarse = true;

  // 缓冲区
  uint8_t *pucBuf = (uint8_t*)pvBuf;

  // 逐扇区写入
  while( uBytesWritten < uBytesToWrite )
    {
    uint32_t uSecBegAddr = (uAddress / uSectorSize) * uSectorSize;
    // 计算本次要写入的数据量
    uint32_t uSize;
    if( (uAddress - uSecBegAddr) + (uBytesToWrite - uBytesWritten) > uSectorSize )
      uSize = uSectorSize - (uAddress - uSecBegAddr);
    else
      uSize = uBytesToWrite - uBytesWritten;

    // 擦除扇区
    if( true == bEarse )
      {
      if( 0 != NvRAM_FlashErase( TOKEN_NvRAM_ACCESS, uAddress, 1 ) )
        return uBytesWritten;
      }

    // 允许写入数据
    NvRAM_FlashWriteEnable( TOKEN_NvRAM_ACCESS );
    // 写入数据
    if( uSize != NvRAM_FlashWrite( TOKEN_NvRAM_ACCESS, 
                                   uAddress, 
                                   pucBuf, 
                                   uSize) )
      return uBytesWritten;

    // 关闭写入
    NvRAM_FlashWriteEnable( 0 );

    // 准备下次写入
    uAddress      += uSize;
    pucBuf        += uSize;
    uBytesWritten += uSize;
    
    bEarse = true; // 下次写一定是在另一个扇区

    if( nullptr != pProc )
      pProc();    
    }
#endif

  return uBytesWritten;
}
//-----------------------------------------------------------------------------

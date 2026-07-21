# SG1210 银网科技 — 固件项目

SG1210 智能电力仪表固件，基于 STM32F407 + FreeRTOS + emWin。

## 技术栈

### 嵌入式目标 (MCU)
- **MCU**: STM32F407VGTx (Cortex-M4F, 168MHz, 1MB Flash, 192KB SRAM)
- **RTOS**: FreeRTOS Kernel V10.3.1 (via CMSIS-RTOS v2 wrapper)
- **HAL**: STM32F4xx HAL Driver (STMicroelectronics)
- **GUI**: emWin V6.56 (SEGGER)
- **编译器**: ARM Compiler V6.22 (ARMCLANG)
- **IDE**: Keil MDK (uVision)
- **编程语言**: C++17 / C11

### 模拟器 (Windows)
- **IDE**: Visual Studio 2022 (v145 toolset)
- **平台**: Win32 (Debug/Release)
- **编程语言**: C++20 / C17
- **GUI模拟**: emWin Simulation Library (GUISim.lib)

## 项目结构

```
SG1210v21/
├── Application/          # 应用层 — 入口、任务、GUI、通信、设备逻辑
│   ├── main.cpp          # 入口：HAL_Init → Clock → osKernel → Tasks_Init → osKernelStart
│   ├── COM/              # 通信栈：UART Socket、Modbus RTU从站、协议层、环形缓冲
│   ├── Config/           # 设备配置宏、FreeRTOSConfig.h、OS端口配置
│   ├── Device/           # 设备逻辑：线圈控制、DFT(64/80点)、数值处理、采样、录波
│   ├── GUI/              # GUI 交互系统
│   │   ├── Dialog/       # 各种Form: 主界面、菜单、启动画面、参数设置、事件浏览器等
│   │   ├── Form/         # 对话框相关源代码
│   │   └── Graphics/     # 图像资质
│   ├── Mics/             # 杂项：AES128、CRC16/32、中值平均滤波、RAM堆分配、随机数
│   ├── System/           # 核心系统：设备类型、寄存器、缓冲区、时钟、调试、事件管理、多语言字符串
│   └── Tasks/            # RTOS 任务：AppTask、CtrlTask、HMITask、UARTTask
├── Drivers/              # 驱动层
│   ├── CMSIS/            # CMSIS 5：core_cm4、STM32F4xx 设备头文件、启动代码
│   ├── Cube/             # STM32CubeMX 生成代码：ADC/CRC/DMA/FSMC/GPIO/IWDG/RTC/SPI/TIM/USART
│   ├── HAL_Driver/       # STM32F4 HAL 库头文件(Inc/)和源文件(Src_/)
│   └── Peripherals/      # 外设驱动：ADC管理器、板控、DIO、Flash接口、I2C、INA226、LED、
│                          键盘、MB85RS FRAM、MCP401x、NvRAM、SD3077 RTC、SPI通道、UART通道
├── Middlewares/          # 中间件
│   ├── FreeRTOS/         # FreeRTOS 内核 + CMSIS-RTOS v2 适配层 + CM4F 移植
│   └── emWin/            # emWin GUI 库 + CSG图像编解码器 + 字体 + 用户配置
│       ├── csgimage/     # CSG编解码：BitPacker、CSGCodec、CSGDecoder、CSGCommon
│       │   └── Compress/ # 压缩算法：RLE、Huffman、MiniLZ77
│       ├── fonts/        # 字体：ASCII(6/16/24)、中文(16/24)、Digital(32/40/44)、Monaco16
│       └── user/         # emWin集成：GUIConf、LCDConf、ST7789S驱动、位图资源
├── Project/              # Keil MDK 工程
│   ├── SG1210_H20v21.uvprojx   # Keil 工程文件
│   ├── SG1210_H20v21.uvoptx    # Keil 选项
│   ├── DebugConfig/            # 调试配置
│   └── Objects/                # 编译产物 (.o, .d, .map, .axf, .sct)
├── Sim/                  # Visual Studio 模拟器工程
│   ├── SG1210Sim.sln           # VS 解决方案
│   ├── SimulationTrial.vcxproj # VS 项目 (Win32, C++20)
│   ├── Simulation/             # 模拟器入口：WinMain.cpp、GUIMain.cpp、GUISim.lib
│   ├── Config/                 # 模拟器 GUIConf、LCDConf、SIMConf
│   ├── GUI/Include/            # 模拟器版 emWin 头文件
│   └── Tool/                   # UI设计 PSD 源文件
└── Doc/                  # 设计文档
    └── SG1210v21技术参考.md    # 完整目录树
```

## 常用命令

### MCU 编译
- 在 Keil MDK 中打开 `Project/SG1210_H20v21.uvprojx`
- Target 选择 `APP`，点击 Build (F7)
- 输出：`Project/Objects/SG1210_H21v{version}.axf`

### 模拟器编译

- Visual Studio 2026 安装在 `d:/apps/vs2026`
- MSBuild 路径：`d:/apps/vs2026/MSBuild/Current/Bin/MSBuild.exe`

**命令行编译 (Git Bash)**

```bash
# 完整流程：杀旧进程 → 编译 → 启动
taskkill //F //IM SG1210Sim.exe 2>/dev/null; sleep 1
cd Sim
"d:/apps/vs2026/MSBuild/Current/Bin/MSBuild.exe" SG1210Sim.sln \
  -p:Configuration=Debug -p:Platform=Win32 -m:1 -v:minimal
start "" "D:/Works/SilverGrid/SG1210/Firmware/SG1210v21/Sim/Build/SG1210Sim.exe"
```

> **Git Bash 注意**：必须使用 `-p:` (短横线) 而非 `/p:` (斜杠)，否则 bash 会错误解析为 Unix 路径导致 MSB1008 错误。

> **编译前必须杀进程**：SG1210Sim.exe 运行时会锁定 .exe 文件，LNK1168 错误。用 `taskkill //F //IM SG1210Sim.exe` 强制结束。

> **`/m:1` 单线程编译**：多核并行编译 (`/m`) 时多个 `cl.exe` 进程争用同一个 `.pdb` 文件，报 `C1041`。长期方案是添加 `/FS` 编译选项。

- 配置选择 Debug | Win32 或 Release | Win32
- 输出：`Sim/Build/SG1210Sim.exe`

**常见问题**

1. **新增 .cpp 在 Sim 下编译失败** → 检查是否依赖 FreeRTOS / STM32 HAL / rtc.h 等 MCU 专有头文件。如果是，应通过 `#ifndef __vmSIMULATOR__` 条件编译排除，不要加入 vcxproj。
2. **链接时缺少外部符号** → 确认对应的 .cpp 已加入 vcxproj 的 `<ClCompile>` 列表，且头文件 include 路径正确。
3. **PDB 锁定 (C1041)** → 使用 `-m:1` 单线程编译，或考虑添加 `/FS` 编译选项到 vcxproj。

## 编码规范

### 命名风格
- **类型/结构体/枚举**: PascalCase — `TUARTConfig`、`TGUIState`、`GWinForm`
- **公共函数**: PascalCase — `Tasks_Init()`、`BoardCtrl_Init()`、`CreateHMITask()`
- **文件内部函数**: camelCase 前缀 `_` — `_updatePage1()`、`_showForm()`、`_OnTick()`
- **变量**: 匈牙利前缀 — `uTick` (uint32_t)、`uwKey` (uint16_t)、`ucFlag` (uint8_t)、`pBuf` (pointer)、`rValue` (float)
- **宏/常量**: MACRO_CASE 或 PascalCase — `DEV_TYPES_H`、`STATE_TRUE`、`NUM_ADC_CHANNELS`

### 头文件保护
```c
#ifndef DEV_TYPES_H
#define DEV_TYPES_H
// ...
#endif
```

### 文件头注释

**新建文件模板**:
```cpp
//-----------------------------------------------------------------------------
/*
 File        : NewFile.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : 功能描述

 Date       : 2026.06.25
*/
//-----------------------------------------------------------------------------
```

**修订规则** — 每次修改源文件必须同步更新头部:
1. **Version** — 递增次版本号 (V1.00→V1.01→V1.02 ...)
2. **Date** — 追加新行 `Date: YYYY.MM.DD (版本 — 变更说明)`，旧行保留不删
3. **By** — 修订者在 Date 行注明即可，By 行保持不变（原始作者）

**修订示例** (GUICntr.cpp):
```cpp
/*
 File        : GUICntr.cpp
 Version     : V2.01
 By          : Wey. Silver Grid

 Description : GUI controller — adapter layer.

 Date       : 2026.06.25 (V2.01 — added touch polling support)
              2026.06.24 (V2.0  — adapter for gform)
              2023.12.05 (V1.10 — original implementation)
*/
```

**修订原因用简洁的英文短句**，描述"做了什么"而非"为什么"（为什么在 git log 中）:
- `added TouchEvent API for touch screen`
- `added GM_TOUCH touch screen handler`
- `added picIndex & saturation params`
- `saturation helpers, atlas sub-picture, picIndex param`

### 注释语言
- **注释语言**：英文
- **新建文件**: 统一使用 UTF-8 with BOM
- **旧有文件**: 大部分可转 UTF-8，以下两类**必须保持 GB2312**（详见下方中文显示原理）：
  - 中文字符串 (`Application/System/Strings/StrsCHS.h`、`TextStrs.cpp`)
  - 中文字库文件（文件名含 `FontCHS` 标识，如 `FontCHS24LTH.cpp`、`FontCHS16LTH.cpp`）

## 核心架构

### RTOS 任务 (4个任务)
| 任务 | 入口函数 | 优先级 | 堆栈 | 职责 |
|------|----------|--------|------|------|
| appTask | TaskApp | Normal | 2048B | 主调度：启动设备、创建其他任务、周期计算、LED/WDT |
| HMITask | TaskHMI | Normal | 8192B | GUI：初始化 emWin、运行消息循环 |
| CTRLTask | TaskCtrl | High | 8192B | 控制：线圈逻辑、继电器扫描、板卡控制 |
| UARTTask | TaskUart | — | — | 串口通信处理 |

### 看门狗
协作式窗口看门狗：各任务在每个循环置位 `RSTSrc` 对应位，TIM9 1ms 中断检查所有任务位在时间窗口内是否被置位后才喂狗。

### GUI 架构 (emWin + GForm)
- 自研表单系统 `GWinForm`：每个界面=4个函数指针(Init/Show/Close/MsgProc)
- `gform::` namespace API 统一管理：Init/Tick/PushForm/ReplaceForm/PopForm/SendMsg/PostMsg
- `GUICntr.cpp/h` 适配层：GUIStart/GUICenter/GUIFormOpen/GUIFormClose → gform 转发
  - Sim 编译：4 个表单通过 `s_formTableSim[]` 注册
  - MCU 编译：4+11=15 个表单通过 `s_formTableSim[]` + `s_formTableMcu[]` 注册
  - `TGUIState/FGUIState` 全局状态供 MCU 背光超时/键盘轮询
  - **不可删除** — 18+ 文件依赖，含 MCU 专有 `KEYDB_OnChanged`/`IRKBD_Received`/`GUIShowFatalMessage`
- `FormManager.cpp/h` — 已删除 (Phase 5)，原为半成品单例，已被 GForm 完全替代
- WID_ 常量双重定义：
  - `GUICntr.h`: `#define` 宏（兼容旧代码，`#include` 顺序先于 GForm.h）
  - `GForm.h`: `constexpr uint16_t`（新代码推荐，`#ifndef WID_FORMBEGIN` 防护）
  - **修改 WID_ 时必须两处同步更新**

#### 中文显示原理

SG1210 的中文显示依赖 GB2312 编码的直接字节索引，**转码为 UTF-8 会彻底破坏字符查找**。

**数据流**:
```
应用层字符串 (StrsCHS.h, GB2312)
  │  例: "电压" → GB2312: \xB5\xE7 \xD1\xB9
  ▼
emWin 文本绘制 (GUI_DispString)
  ▼
GUIPROP_UC_DispChar(U16P ch)        ← GUI_UC_Font.c
  │  ch = (\xB5<<8) | \xE7 = 0xB5E7  (双字节 GB2312 码点)
  ▼
GUI_UC_FindFontItem(pFontList, ch)  ← 二分查找
  │  遍历 TGUIFontItem[].Char (2字节 GB2312 编码)
  ▼
FontCHS24LTH.cpp / FontCHS16LTH.cpp
  │  static const TGUIFontItem fiCharDotmx[] = {
  │    {"一", ...},   ← GB2312 编码，2 字节
  │    {"乙", ...},
  │    ...
  │  }
  ▼
返回点阵数据 DotMix[] → LCD_DrawBitmap 绘制
```

**关键数据结构** (`GUI_UC_Font.h`):
```c
typedef struct {
    const uint8_t *Char;    // GB2312 编码的汉字，固定 2 字节
    const uint8_t *DotMix;  // 点阵数据 (如 24×24=72 bytes)
} TGUIFontItem;

typedef struct {
    uint32_t Count;
    TGUIFontType Type;      // Width, Height, BytesPerLine
    TGUIFontItem Items[];   // 按 GB2312 码点排序，支持二分查找
} TGUIFontList;
```

**为何不能转 UTF-8**:
1. `TGUIFontItem.Char` 固定 2 字节存储 GB2312 编码。若将 `FontCHS*.cpp` 转为 UTF-8，`"一"` 从 `\xD2\xBB`(2B) 变为 `\xE4\xB8\x80`(3B)，`FindFontItem` 的 `ucCode[0]<<8 | ucCode[1]` 将读到错误码点
2. Form 文件中的中文字符串与字库文件使用同一套 GB2312 编码，转码导致 UI 显示乱码
3. `StrsCHS.h` 中所有多语言字符串均为 GB2312，通过 `GUI_DispString` 直接送入上述管线

### 设备寄存器模型
- 所有设备状态存储在编号"寄存器"中（类似 Modbus 寄存器模型）
- `TDevRegInfoItem` 描述每个寄存器的元数据（名称、范围、比例、单位、事件、属性）
- `TDevRegListClass` 枚举寄存器分组（保护、测量、事件、配置等）
- 访问接口：`_GetRealReg()`、`_SetRealReg()`、`GetHWFault()`、`SetRSTSrc()` 等

### 通信栈
- `TCOMSocket` 抽象基类 → `SocketUART` 实现
- `TCOMProtocol` → `proModbusRTUSlave` / `EchoSvr` / `InspSvr`
- `COMRingBuf` / `COMBuffer` 环形缓冲队列
- `COMDevIntf` 桥接通信层与设备寄存器系统

### 平台适配
- MCU 与模拟器通过 `#ifdef __vmSIMULATOR__` 分支
- 模拟器：`<windows.h>` + Win32 API
- MCU：STM32 HAL 外设头文件 (`gpio.h`、`rtc.h` 等)

#### Sim 不可编译的 MCU 文件类型

以下类型的文件依赖 MCU 专有基础设施，**禁止加入 vcxproj**：
- FreeRTOS API (`osMutexWait`/`osMutexRelease`/`osWaitForever`/`portMAX_DELAY`)
- STM32 HAL 头文件 (`stm32f4xx_hal.h`、`rtc.h`、`gpio.h`、`cmsis_os.h`)
- MCU 字体 (`FontCHS12x12HT.h`、`GUIFontCHS12x12.h`)
- MCU 外设 (`IndLED.h`、`KeyBoard.h`、`RNGen.h`)
- `TGUIState::mutexGUI` 字段仅在 `#ifndef __vmSIMULATOR__` 下存在

正确做法是在 `GUICntr.cpp` 中用 `#ifndef __vmSIMULATOR__` 条件编译隔离，而非试图让它们通过 Sim 编译。

#### GForm API 速查

```cpp
// 生命周期
gform::Init();           // 初始化导航栈和消息队列（不清空注册表）
gform::Tick();           // 10ms Tick — 排空延迟消息 + 投递 GM_TIMER_TICK
gform::Run();            // 返回当前栈深度

// 注册
gform::RegisterForm(id, &form, "name");
gform::FindForm(id);     // → const FormRecord*
gform::UnregisterForm(id);

// 导航
gform::PushForm(id, para);
gform::ReplaceForm(id, para);
gform::PopForm();
gform::GetCurrentFormId();

// 消息 （SendMsg/PostMsg 避免 <Windows.h> SendMessage/PostMessage 宏冲突）
gform::SendMsg(msgId, param, value);
gform::SendMsgPtr(msgId, param, data);
gform::PostMsg(msgId, param, value);     // 延迟到下次 Tick 投递
gform::BroadcastMsg(msgId, param, value);
gform::KeyEvent(key, pressedCnt);

// 容量限制
kMaxForms = 32;    // 注册表容量
kMaxStack = 16;    // 导航栈深度
```

## 注意事项

**文件与权限**
- 允许在 `D:\Works\SilverGrid\SG1210\Firmware\SG1210v21` 及其子目录中读写文件
- 允许在 `D:\Works\SilverGrid\SG1210\Firmware\SG1210v21` 及其子目录中执行 Bash 命令

**编译目标**
- MCU 固件面向 ARM Compiler V6 编译
- 模拟器面向 MSVC (Visual Studio 2022) 编译
- 模拟器是单线程的 — `gform::platform::Lock` 在 Sim 下为空操作

**编码**
- 中文注释主要在 Application 和 Drivers/Peripherals 层
- CSG 图像编解码器代码遵循 Google C++ 规范（英文注释）
- **编码转换规则**:
  - ❌ Form 文件、`System/Strings/`、`FontCHS*` 字库 → **禁止转码**（中文显示管线依赖 GB2312 字节索引）
  - ✅ 其余所有旧文件 → 可安全转为 UTF-8 with BOM
- 中文显示原理见"核心架构 → GUI 架构 → 中文显示原理"；参考文件: `GUI_UC_Font.c`、`FontCHS24LTH.cpp`

**GUI 表单**
- 新增 WID_ 常量必须在 `GUICntr.h`（`#define`）和 `GForm.h`（`constexpr`）**两处同步**
- Sim 编译不通过的文件不应强行加入 vcxproj，应通过 `#ifndef __vmSIMULATOR__` 隔离
- `gform::SendMsg`/`PostMsg` 命名是为了避免 `<Windows.h>` 的 `SendMessageA/W`、`PostMessageA/W` 宏冲突

**Git**
- `Sim/` 目录曾有独立 `.git` 导致变更无法跟踪，已通过 `rm -rf Sim/.git` 修复
- 不要在 Sim 下重建独立的 `.git` 目录

**MSBuild**
- Git Bash 中使用 `-p:` 而非 `/p:`
- 使用 `-m:1` 避免 PDB 文件锁定 (C1041)
- 编译前确认 vcxproj 的 `<ClCompile>` 列表与实际文件一致

**ARMCLANG 命令行**
- ARM Compiler 6 路径：`D:/Apps/Keil/ARM/ARMCLANG/bin/armclang.exe`
- 语法检查最小命令模板：
```bash
ARMCLANG="D:/Apps/Keil/ARM/ARMCLANG/bin/armclang.exe"
"$ARMCLANG" \
  --target=arm-arm-none-eabi \
  -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
  -std=c++17 -xc++ -fsyntax-only -Wc++11-narrowing \
  -DUSE_HAL_DRIVER -DSTM32F407xx -D__TARGET_FPU_VFP -D__FPU_PRESENT=1U \
  -I ...include paths... \
  file.cpp
```
- FreeRTOS include 路径注意：实际路径为 `Middlewares/FreeRTOS/include/` 和 `Middlewares/FreeRTOS/CMSIS_RTOS_V2/`，不是 `Source/include` 和 `Source/CMSIS_RTOS_V2`
- `portmacro.h` 在 `Middlewares/FreeRTOS/portable/RVDS/ARM_CM4F/`，需额外添加
- `GUI_RECT` 字段类型为 `I16`(short)，`LCD_GetXSize()` 返回 `int`，花括号初始化会触发 `-Wc++11-narrowing` 错误。改为逐字段赋值 + `static_cast<I16>()`

---

## MCU 性能优化经验

### CSGDraw.cpp MCU 路径优化

**三级优化（2026.06.30）**：

| 优化 | 代码 | 收益 |
|------|------|------|
| 指针直转 | `const uint16_t* puwPixels = (uint16_t*)outBuf` 替代逐像素 `CrmToRgb565()` | 消除函数调用（仅适用 RGB565 CRM，bpc=2） |
| FSMC 寄存器直写 | `LCD_DATADDR = uwColor` 替代 `*(volatile uint16_t*)0x60000000 = v` | 减少 volatile 解引用中间步骤 |
| 游标批处理 | 仅透明→不透明切换时设一次 `LCD_SetCursor`；连续不透明像素间 ST7789 自动递增 | 大幅减少 SPI 命令开销 |

**游标批处理模式**：
```cpp
U32 bNeedCursor = 1;
for (int i = 0; i < n; ++i) {
    if (uwColor == transpRgb565) { bNeedCursor = 1; continue; }  // 透明：标记下次需设游标
    if (bNeedCursor) { bNeedCursor = 0; LCD_SetCursor(x0 + i, y0 + row); }
    LCD_DATADDR = uwColor;  // ST7789 列地址自动+1
}
```
**Form中的内存分配**
Form中的内存分配采用RAMHeap中的方法

### GPMainForm 时钟更新优化

| 优化 | 旧 | 新 |
|------|-----|-----|
| 状态存储 | `char szClock[32]` + `strcmp` | `uint8_t ucLastMinute` + 单字节比较 |
| 检测逻辑 | `_UpdateClock` 做 strcmp → 调 `_GetClockStr` | `_GetClockStr(bForce, ...)` 内部比较分钟并返回 bool |

节省 32B 静态内存，时钟更新从字符串比较降为单字节比较。

---

## GUI 绘制注意事项

### emWin ClipRect 与 MCU 直写

- **Sim**：`GUI_SetClipRect` 在 emWin 驱动层生效，所有绘制自动裁剪
- **MCU**：`CSG_DrawPicture` 通过 `LCD_DATADDR` 直写 FSMC，**完全绕过 emWin**，`GUI_SetClipRect` 无效
- **解决**：CSGDraw V1.05 在 MCU 路径手动读取 `GUI_GetClipRect()` 并软件裁剪行列范围

```cpp
const GUI_RECT* pClip = GUI_GetClipRect();
// 全屏时 bHasClip=false → 零开销快速路径
// 有裁剪时：行级跳过（Y 范围）、列级裁剪（iClipStart/iClipEnd）、像素级跳过（per-pixel 路径）
```

### 栈溢出风险

- `CSGDecoderState` 含 `window[8192]`（~8.3KB），绝不能放栈上
- HMITask 栈 8KB，放栈上必然 HardFault
- 固定用法：`static CSGDecoderState state;` → `.bss` 段
- 代价：函数不可重入（GUI 单线程无影响）

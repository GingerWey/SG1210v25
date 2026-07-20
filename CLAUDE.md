# SG1210v25 项目编码规范

## Yoda-style 比较表达式规范

在所有比较表达式中，必须遵循以下顺序规则：

### 1. 常量与变量比较
**规则：常量在左，变量在右**

```cpp
// 正确
if (DL_CONTENT_X0 <= x)
if (LL_VISIBLE >= count)
if (0 < value)

// 错误
if (x >= DL_CONTENT_X0)
if (count <= LL_VISIBLE)
if (value > 0)
```

### 2. 表达式与变量比较
**规则：表达式在左，变量在右**

```cpp
// 正确
if (maxTop < m_State.uTopItem)
if (s_rowCount - DL_VISIBLE > m_State.uTopItem)
if (newTop > maxTop)

// 错误
if (m_State.uTopItem > maxTop)
if (m_State.uTopItem < s_rowCount - DL_VISIBLE)
if (maxTop < newTop)
```

### 3. 变量与变量比较
**规则：作用域小的在左，作用域大的在右**

作用域大小排序：
- 局部变量（最小）
- 函数参数
- 结构体成员变量
- 全局变量（最大）

```cpp
// 正确
if (oldTop != m_State.uTopItem)        // 局部变量 vs 结构体成员
if (idx != s_pState->uCurItem)         // 局部变量 vs 结构体成员
if (shift <= s_pState->uTopItem)       // 局部变量 vs 结构体成员

// 错误
if (m_State.uTopItem != oldTop)
if (s_pState->uCurItem != idx)
if (s_pState->uTopItem >= shift)
```

### 4. 特殊情况
- 当两个变量作用域相同时，优先将被传入的参数或先定义的变量放在左边
- 表达式中包含的变量按表达式整体作用域判断

## 内存管理规范

### 统一使用 RAM_Malloc/RAM_Free
在所有平台（包括模拟器）上统一使用项目的内存管理函数，不使用 C++ 的 new/delete 操作符。

```cpp
// 正确
s_pState = (TFormState*)RAM_Malloc(sizeof(TFormState));
RAM_Free(s_pState);

// 错误 - 不要使用条件编译
#ifndef __vmSIMULATOR__
  RAM_Free(s_pState);
#else
  delete s_pState;
#endif
```

## 空指针检查

在访问指针前必须进行 nullptr 检查：

```cpp
// 正确
if (nullptr == s_pState) {
  return;
}
// ... 使用 s_pState
```

## 版本管理

每次修改文件后需要：
1. 递增版本号（V1.xx）
2. 添加修改日期和简要说明

```cpp
 Version     : V1.14
 Date        : 2026.07.14 (V1.14 — Yoda-style for variable comparisons by scope)
              2026.07.14 (V1.13 — complete Yoda-style for expressions vs variables)
```

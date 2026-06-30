//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GPMenuForm.cpp
 Version     : V2.01
 By          : Wey. Silver Grid

 Description : 主菜单表单 -- 2×5 图标网格，键盘/触屏导航。
               按 SG1210v21表单设计.md §二 实现。

   Layer0: 背景图 + Caption 背板/分割线 + Caption 标题
   Layer1: Home 按钮 + 提示文字 + 10 个菜单项（图标+标签）+ 选取框

   键盘操作:
     - 上/下/左/右 切换当前项 (2×5 网格中移动)
     - Enter 激活菜单项动作 (切入对应子 Form)
     - ESC  返回 GMainForm
     - 当前项图标向左上移位(-1,-1)，按下 Enter 向右下移位(1,1)

   触屏操作:
     - 在菜单图标区域内单击 → 激活对应菜单项
     - 点击 Home 图标 → 返回 GMainForm

   选中态还原:
     - 切换时用 clip + CSG 背景重绘擦除旧选取框区域
     - 选取框填充使用 GUI_AA_FillRoundedRect + GUI_SetAlpha 半透明
     - 需 GUI_AA_EnableHiRes() 全局启用（GUICntr.cpp GUIStart）

 Date        : 2026.06.29 (V2.02 -- clip-redraw 替代 MemDev；AA 半透明选取框)
              2026.06.29 (V2.01 -- MemDev 快照还原替代黑色填充，修复透明图标背景)
              2026.06.29 (V2.00 -- 按表单设计文档重写，2×5 网格替代旧列表)
              2026.06.25 (V1.11 -- added GM_TOUCH)
              2023.12.05 (V1.10 -- original vertical-list implementation)
*/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include "GPMenuForm.h"

#include "GUI.h"
#include "GFormCentra.h"
#include "GUIMessage.h"
#include "GWinTypes.h"
#include "GFormCentraRegistrar.h"

#include "FontSGRes.h"
#include "GUIPicture.h"
#include "PictureRes.h"
#include "Graphics/ImageRes.h"
#include "Strings/TextStrs.h"
#include "CSGDraw.h"

#include <string.h>

//=============================================================================
// 布局常量 -- 按表单设计文档 §二.2.3
// 坐标格式 (x, y, w, h)
//=============================================================================

// --- Layer0: 背景图 ---
#define MU_BKG_X          0
#define MU_BKG_Y          0

// --- Layer0: Caption 背板 (40px 高) ---
#define MU_CAP_X          0
#define MU_CAP_Y          0
#define MU_CAP_W          320
#define MU_CAP_H          40
#define MU_CAP_FILL       0x041736     // 填充色
#define MU_CAP_ALPHA      51           // 不透明度 20% = 255*0.20

// --- Layer0: Caption 分割线 ---
#define MU_SEP_X          0
#define MU_SEP_Y          40
#define MU_SEP_W          320
#define MU_SEP_H          1
#define MU_SEP_COLOR      0x2F5CA6
#define MU_SEP_ALPHA      38           // 不透明度 15% = 255*0.15

// --- Layer0: Caption 标题 ---
#define MU_TITLE_X        60
#define MU_TITLE_Y        7
#define MU_TITLE_W        221
#define MU_TITLE_H        27
#define MU_TITLE_FONT     GUI_FONT_24LTH_CHN
#define MU_TITLE_COLOR    0x00B5F2
#define MU_TITLE_ALIGN    (GUI_TA_HCENTER | GUI_TA_VCENTER)

// --- Layer1: Home 按钮 ---
#define MU_HOME_X         8
#define MU_HOME_Y         8
#define MU_HOME_PICIDX    picIdxMU_Home24x22

// --- Layer1: 提示文字 ---
#define MU_HINT_X         14
#define MU_HINT_Y         221
#define MU_HINT_W         200
#define MU_HINT_H         16
#define MU_HINT_FONT      GUI_FONT_16LTH_CHN
#define MU_HINT_COLOR     0x00B5F2
#define MU_HINT_BG        0x070D28
#define MU_HINT_ALIGN     (GUI_TA_LEFT | GUI_TA_VCENTER)

// --- Layer1: 选取框（空心圆角矩形边框，相对菜单项图标左上角）---
#define MU_SEL_DX         -9
#define MU_SEL_DY         -5
#define MU_SEL_W          50
#define MU_SEL_H          60
#define MU_SEL_RADIUS     4
#define MU_SEL_COLOR      0x4DC9FC

// --- 菜单项: 图标尺寸 ---
#define MU_ICON_W         32
#define MU_ICON_H         32

// --- 网格布局 ---
#define MU_GRID_COLS      5
#define MU_GRID_ROWS      2
#define MU_ITEM_COUNT     10

//=============================================================================
// 菜单项描述表 -- 图标坐标、子图索引、标签区域、目标 Form、说明文字
//=============================================================================
typedef struct tagMenuItem {
  int        xIcon, yIcon;        // 图标左上角坐标
  int        picIdx;              // 图集子图索引
  int        xLbl, yLbl, wLbl, hLbl;  // 标签区域
  uint32_t   uNameId;             // 多语言字符串 ID（标签文字）
  uint32_t   uDespId;             // 多语言字符串 ID（提示文字）
  uint16_t   uTargetWID;          // 激活后切入的目标 Form WID
} TMenuItem;

static const TMenuItem kMenuItems[MU_ITEM_COUNT] =
{
  // Row 1
  {  24, 72,  picIdxMU_Item32x32_01,  24, 107, 33, 17, idMenuName1,  idMenuDesp1,  WID_DataListForm    },
  {  86, 72,  picIdxMU_Item32x32_02,  86, 107, 32, 17, idMenuName2,  idMenuDesp2,  WID_FatalForm       },
  { 148, 72,  picIdxMU_Item32x32_03, 148, 107, 33, 17, idMenuName3,  idMenuDesp3,  WID_SystemLogForm   },
  { 210, 72,  picIdxMU_Item32x32_04, 210, 107, 33, 17, idMenuName4,  idMenuDesp4,  WID_WLGListForm     },
  { 272, 72,  picIdxMU_Item32x32_05, 272, 107, 33, 17, idMenuName5,  idMenuDesp5,  WID_DeviceTestForm  },
  // Row 2
  {  24, 145, picIdxMU_Item32x32_06,  24, 180, 33, 17, idMenuName6,  idMenuDesp6,  WID_CTRLConfigForm  },
  {  86, 145, picIdxMU_Item32x32_07,  86, 180, 32, 17, idMenuName7,  idMenuDesp7,  WID_ConfigForm      },
  { 148, 145, picIdxMU_Item32x32_08, 148, 180, 33, 17, idMenuName8,  idMenuDesp8,  WID_UARTConfigForm  },
  { 210, 145, picIdxMU_Item32x32_09, 210, 180, 33, 17, idMenuName9,  idMenuDesp9,  WID_EthernetConfigForm },
  { 272, 145, picIdxMU_Item32x32_10, 272, 180, 33, 17, idMenuName10, idMenuDesp10, WID_AboutForm        },
};

//=============================================================================
// Form 状态
//=============================================================================
typedef struct tagMenuState {
  int  iCurItem;       // 当前选中项索引 (0..9)
  bool bEnterDown;     // Enter 按下状态（控制移位绘制）
} TMenuState;

static TMenuState m_State;

//=============================================================================
// 辅助: (x,y,w,h) → emWin GUI_RECT
//=============================================================================
static void _MakeRect(GUI_RECT* pR, int x, int y, int w, int h)
{
  pR->x0 = x;
  pR->y0 = y;
  pR->x1 = x + w - 1;
  pR->y1 = y + h - 1;
}

//-----------------------------------------------------------------------------
// 根据索引计算网格行列
//-----------------------------------------------------------------------------
static int _ItemRow(int idx) { return idx / MU_GRID_COLS; }
static int _ItemCol(int idx) { return idx % MU_GRID_COLS; }

//-----------------------------------------------------------------------------
// 根据行列计算索引，越界返回 -1
//-----------------------------------------------------------------------------
static int _RowColToIdx(int row, int col)
{
  if (row < 0 || row >= MU_GRID_ROWS || col < 0 || col >= MU_GRID_COLS)
    return -1;
  return row * MU_GRID_COLS + col;
}

//=============================================================================
// 选取框区域擦除 — clip + 背景 CSG 重绘，保证图标透明区域正确
//=============================================================================

//-----------------------------------------------------------------------------
// 擦除菜单项 idx 的选取框区域：裁剪到选取框范围，重绘背景 CSG。
// 然后调用方重绘图标即可恢复未选中态。
//-----------------------------------------------------------------------------
static void _EraseSelArea(int idx)
{
  const TMenuItem* pItem = &kMenuItems[idx];
  GUI_RECT rClip;
  _MakeRect(&rClip,
            pItem->xIcon + MU_SEL_DX - 1,       // 覆盖边框最外侧
            pItem->yIcon + MU_SEL_DY - 1,
            MU_SEL_W + 2, MU_SEL_H + 2);         // 向外扩展 1px Margin

  GUI_SetClipRect(&rClip);
  CSG_DrawPicture(&picbkg320x240Lcsg, 0, 0, 0, 100);   // 重绘背景图层
  GUI_SetClipRect(nullptr);                              // 解除裁剪
}

//=============================================================================
// 绘制
//=============================================================================

//-----------------------------------------------------------------------------
// 模拟半透明填充：emWin 不直接支持 alpha，这里用纯色填充近似。
// 颜色值已包含所需的不透明度视觉效果。
//-----------------------------------------------------------------------------
static void _DrawCaptionBar(void)
{
  GUI_RECT r;

  // 背板 — 20% 不透明度叠加在 CSG 背景图上
  GUI_EnableAlpha(1);
  GUI_SetAlpha(MU_CAP_ALPHA);
  _MakeRect(&r, MU_CAP_X, MU_CAP_Y, MU_CAP_W, MU_CAP_H);
  GUI_SetColor(MU_CAP_FILL);
  GUI_FillRectEx(&r);

  // 分割线 — 15% 不透明度
  GUI_SetAlpha(MU_SEP_ALPHA);
  _MakeRect(&r, MU_SEP_X, MU_SEP_Y, MU_SEP_W, MU_SEP_H);
  GUI_SetColor(MU_SEP_COLOR);
  GUI_FillRectEx(&r);
  GUI_EnableAlpha(0);
}

//-----------------------------------------------------------------------------
// 绘制 Caption 标题
//-----------------------------------------------------------------------------
static void _DrawCaptionTitle(void)
{
  const char* pStr = GetMultiLangString(idMenuCaption);
  if (nullptr == pStr) return;

  GUI_RECT r;
  _MakeRect(&r, MU_TITLE_X, MU_TITLE_Y, MU_TITLE_W, MU_TITLE_H);
  GUI_SetFont(MU_TITLE_FONT);
  GUI_SetColor(MU_TITLE_COLOR);
  GUI_SetTextMode(GUI_TEXTMODE_TRANS);
  GUI_DispStringInRect(pStr, &r, MU_TITLE_ALIGN);
}

//-----------------------------------------------------------------------------
// 绘制 Home 按钮图标
//-----------------------------------------------------------------------------
static void _DrawHome(void)
{
  CSG_DrawPicture(&picMAUAtlascsg, MU_HOME_X, MU_HOME_Y, MU_HOME_PICIDX, 100);
}

//-----------------------------------------------------------------------------
// 绘制提示文字
//-----------------------------------------------------------------------------
static void _DrawHint(void)
{
  const char* pStr = GetMultiLangString(kMenuItems[m_State.iCurItem].uDespId);
  if (nullptr == pStr) return;

  // Fill background
  GUI_SetColor(MU_HINT_BG);
  GUI_FillRect(MU_HINT_X, MU_HINT_Y,
               MU_HINT_X + MU_HINT_W - 1, MU_HINT_Y + MU_HINT_H - 1);

  GUI_RECT r;
  _MakeRect(&r, MU_HINT_X, MU_HINT_Y, MU_HINT_W, MU_HINT_H);
  GUI_SetFont(MU_HINT_FONT);
  GUI_SetColor(MU_HINT_COLOR);
  GUI_SetTextMode(GUI_TEXTMODE_TRANS);
  GUI_DispStringInRect(pStr, &r, MU_HINT_ALIGN);
}

//-----------------------------------------------------------------------------
// 绘制单个菜单项（图标 + 标签）
// idx:     菜单项索引
// bSelect: 是否为当前选中项（影响移位和选取框绘制）
//-----------------------------------------------------------------------------
static void _DrawMenuItem(int idx, bool bSelect)
{
  const TMenuItem* pItem = &kMenuItems[idx];

  // --- 计算移位偏移 ---
  int dx = 0, dy = 0;
  //if (bSelect)
  //{
  //  if (m_State.bEnterDown)
  //  {
  //    dx = 1; dy = 1;   // Enter 按下：右下移位
  //  }
  //  else
  //  {
  //    dx = -1; dy = -1;  // 选中态：左上移位
  //  }
  //}

  // 不再用黑色填充擦除旧选取框。选中态切换时由调用方通过
  // _CaptureItemBg/_RestoreItemBg 管理 MemDev 快照来精确还原背景。
  // _DrawMenuItem(idx, false) 仅在 _DrawAllItems 初始绘制时使用，
  // 此时屏幕刚清空，不需要擦除。

  int ix = pItem->xIcon + dx;
  int iy = pItem->yIcon + dy;

  // --- 绘制选取框（空心圆角矩形边框）---
  if (bSelect)
  {
    int sx = pItem->xIcon + MU_SEL_DX + dx;
    int sy = pItem->yIcon + MU_SEL_DY + dy;

    GUI_SetColor(MU_SEL_COLOR);
    GUI_SetPenSize(1);
    GUI_DrawRoundedRect(sx, sy, sx + MU_SEL_W - 1, sy + MU_SEL_H - 1,
                        MU_SEL_RADIUS);
  }

  // --- 绘制图标 ---
  CSG_DrawPicture(&picMAUAtlascsg, ix, iy, pItem->picIdx, 100);

  // --- 绘制标签 ---
  const char* pStr = GetMultiLangString(pItem->uNameId);
  if (nullptr != pStr)
  {
    GUI_RECT r;
    _MakeRect(&r, pItem->xLbl + dx, pItem->yLbl + dy,
              pItem->wLbl, pItem->hLbl);
    GUI_SetFont(MU_HINT_FONT);   // GUI_FONT_16LTH_CHN
    GUI_SetColor(MU_HINT_COLOR);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);
    GUI_DispStringInRect(pStr, &r, GUI_TA_HCENTER | GUI_TA_VCENTER);
  }
}

//-----------------------------------------------------------------------------
// 绘制全部菜单项：先画非选中项，最后画选中项（使其在最上层）
//-----------------------------------------------------------------------------
static void _DrawAllItems(void)
{
  for (int i = 0; i < MU_ITEM_COUNT; i++)
  {
    if (i == m_State.iCurItem) continue;
    _DrawMenuItem(i, false);
  }
  // 选中项只画选取框+移位图标（不画非选中态，避免重影）
  _DrawMenuItem(m_State.iCurItem, true);
}

//-----------------------------------------------------------------------------
// 全屏重绘
//-----------------------------------------------------------------------------
static void _Redraw(void)
{
  GUI_SetBkColor(GUI_BLACK);
  GUI_Clear();

  // Layer0
  CSG_DrawPicture(&picbkg320x240Lcsg, MU_BKG_X, MU_BKG_Y, 0, 100);
  _DrawCaptionBar();
  _DrawCaptionTitle();

  // Layer1
  _DrawHome();
  _DrawHint();
  _DrawAllItems();
}

//-----------------------------------------------------------------------------
// 局部刷新：擦除旧项选取框区域（clip + 背景 CSG），重绘旧项图标，
//           再对新项画选取框 + 移位图标。
//-----------------------------------------------------------------------------
static void _RedrawItems(int iOld, int iNew)
{
  // 1. 擦除旧项选取框区域，重绘旧项图标于正常位置
  _EraseSelArea(iOld);
  _DrawMenuItem(iOld, false);

  // 2. 新项只画选取框+移位图标（不画非选中态，避免重影）
  _DrawMenuItem(iNew, true);

  // 3. 更新提示文字
  _DrawHint();
}

//=============================================================================
// 导航与动作
//=============================================================================

//-----------------------------------------------------------------------------
// 移动选中项到指定行列。若行列有效则局部刷新，无效则无操作。
//-----------------------------------------------------------------------------
static void _MoveTo(int row, int col)
{
  int iNew = _RowColToIdx(row, col);
  if (iNew < 0 || iNew == m_State.iCurItem) return;

  int iOld = m_State.iCurItem;
  m_State.iCurItem = iNew;
  m_State.bEnterDown = false;
  _RedrawItems(iOld, iNew);
}

//-----------------------------------------------------------------------------
// 激活当前菜单项：切入目标 Form
//-----------------------------------------------------------------------------
static void _Activate(void)
{
  uint16_t uWID = kMenuItems[m_State.iCurItem].uTargetWID;
  gfc::PushForm(uWID, nullptr);
}

//=============================================================================
// 触屏命中检测
//=============================================================================

//-----------------------------------------------------------------------------
// 检测点 (x,y) 是否在 Home 按钮区域内
//-----------------------------------------------------------------------------
static bool _HitHome(int x, int y)
{
  return (x >= MU_HOME_X && x < MU_HOME_X + 24 &&
          y >= MU_HOME_Y && y < MU_HOME_Y + 22);
}

//-----------------------------------------------------------------------------
// 检测点 (x,y) 落在哪个菜单项图标区域内（返回索引，-1 表示未命中）
//-----------------------------------------------------------------------------
static int _HitItem(int x, int y)
{
  for (int i = 0; i < MU_ITEM_COUNT; i++)
  {
    const TMenuItem* pItem = &kMenuItems[i];
    if (x >= pItem->xIcon && x < pItem->xIcon + MU_ICON_W &&
        y >= pItem->yIcon && y < pItem->yIcon + MU_ICON_H)
    {
      return i;
    }
  }
  return -1;
}

//=============================================================================
// Form 生命周期回调
//=============================================================================

//-----------------------------------------------------------------------------
static void _Init(const void* argument)
{
  (void)argument;
  memset(&m_State, 0, sizeof(m_State));
}

//-----------------------------------------------------------------------------
static void _Show(const void* argument)
{
  (void)argument;
  m_State.iCurItem   = 0;
  m_State.bEnterDown  = false;
  _Redraw();
}

//-----------------------------------------------------------------------------
static void _Close(const void* argument)
{
  (void)argument;
}

//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{
  (void)uTick;
  // 菜单不主动刷新（§二.2.2）
}

//-----------------------------------------------------------------------------
// 键盘按下：方向键切换 + Enter 按下移位
//-----------------------------------------------------------------------------
static void _OnKeyDown(uint16_t uwKey)
{
  int row = _ItemRow(m_State.iCurItem);
  int col = _ItemCol(m_State.iCurItem);

  switch (uwKey)
  {

  case KEY_UP:
    _MoveTo(row - 1, col);
    break;

  case KEY_DOWN:
    _MoveTo(row + 1, col);
    break;

  case KEY_LEFT:
    if (col > 0)
      _MoveTo(row, col - 1);
    else
      _MoveTo(row, MU_GRID_COLS - 1);  // 行内循环
    break;

  case KEY_RIGHT:
    if (col < MU_GRID_COLS - 1)
      _MoveTo(row, col + 1);
    else
      _MoveTo(row, 0);                  // 行内循环
    break;

  case KEY_ENTER:
    // Enter 按下 → 图标右下移位
    if (!m_State.bEnterDown)
    {
      m_State.bEnterDown = true;
      _DrawMenuItem(m_State.iCurItem, true);
    }
    break;

  default:
    break;
  }
}

//-----------------------------------------------------------------------------
// 键盘释放：ESC 返回，Enter 激活菜单项
//-----------------------------------------------------------------------------
static void _OnKeyUp(uint16_t uwKey)
{
  switch (uwKey)
  {

  case KEY_ESCAPE:
    gfc::PopForm();           // 返回 GMainForm
    break;

  case KEY_ENTER:
    if (m_State.bEnterDown)
    {
      m_State.bEnterDown = false;
      _DrawMenuItem(m_State.iCurItem, true);  // 恢复选中态移位
      _Activate();
    }
    break;

  default:
    break;
  }
}

//-----------------------------------------------------------------------------
// 触屏处理
//-----------------------------------------------------------------------------
static void _OnTouch(uint16_t action, uint16_t x, uint16_t y)
{
  switch (action)
  {

  case TOUCH_DOWN:
  case TOUCH_MOVE:
    {
      // 移动到触摸的菜单项
      int iHit = _HitItem(x, y);
      if (iHit >= 0 && iHit != m_State.iCurItem)
      {
        int iOld = m_State.iCurItem;
        m_State.iCurItem = iHit;
        m_State.bEnterDown = false;
        _RedrawItems(iOld, iHit);
      }
    }
    break;

  case TOUCH_UP:
    {
      // Home 按钮 → 返回
      if (_HitHome(x, y))
      {
        gfc::PopForm();
        break;
      }
      // 菜单项 → 先更新选中态视觉，再激活
      int iHit = _HitItem(x, y);
      if (iHit >= 0)
      {
        int iOld = m_State.iCurItem;
        m_State.iCurItem = iHit;
        m_State.bEnterDown = false;
        _RedrawItems(iOld, iHit);
        _Activate();
      }
    }
    break;

  default:
    break;
  }
}

//-----------------------------------------------------------------------------
// 消息分发
//-----------------------------------------------------------------------------
static void _OnMessage(GM_MESSAGE* pMsg)
{
  if (nullptr == pMsg) return;

  switch (pMsg->MsgId)
  {

  case GM_TIMER_TICK:
    _OnTick(static_cast<uint32_t>(pMsg->Data.v));
    break;

  case GM_KEYDOWN:
    if (pMsg->Param)
    {
      _OnKeyDown(pMsg->Param);
      pMsg->MsgId = 0;
    }
    break;

  case GM_KEYUP:
    if (pMsg->Param)
    {
      _OnKeyUp(pMsg->Param);
      pMsg->MsgId = 0;
    }
    break;

#if GUI_SUPPORT_TOUCH
  case GM_TOUCH:
    {
      uint16_t x = static_cast<uint16_t>((pMsg->Data.v >> 16) & 0xFFFF);
      uint16_t y = static_cast<uint16_t>(pMsg->Data.v & 0xFFFF);
      _OnTouch(pMsg->Param, x, y);
      pMsg->MsgId = 0;
    }
    break;
#endif

  default:
    break;
  }
}

//=============================================================================
// Form 描述符
//=============================================================================
const GWinForm FMainMenuForm =
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};

// 自动注册
static const gfc::FormRegistrar kRegMenu(WID_MenuForm, &FMainMenuForm, "Menu");
//-----------------------------------------------------------------------------

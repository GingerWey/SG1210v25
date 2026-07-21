//-----------------------------------------------------------------------------
/*
 File        : GWidget.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GWidget - base class of all Dialog framework GUI elements.
               GM_MESSAGE dispatch to virtual handlers.
               Per SG1210v25 dialog spec section 3.1.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#include "GWidget.h"
#include <GUIConf.h>
#include <GUIMessage.h>
#include <cstdint>

//=============================================================================
// GWidget::onMessage - dispatch GM_MESSAGE to virtual handlers
//-----------------------------------------------------------------------------
void GWidget::onMessage(GM_MESSAGE* pMsg)
{
  if( nullptr == pMsg ) {
    return;
  }

  switch( pMsg->MsgId ) {
  case GM_TIMER_TICK:
    onTick(static_cast<uint32_t>(pMsg->Data.v));
    break;

  case GM_KEYDOWN:
    onKeyDown(pMsg->Param, 0);
    pMsg->MsgId = 0;
    break;

  case GM_KEYPRESS:
    onKeyDown(pMsg->Param, static_cast<uint32_t>(pMsg->Data.v));
    pMsg->MsgId = 0;
    break;

  case GM_KEYUP:
    onKeyUp(pMsg->Param);
    pMsg->MsgId = 0;
    break;

#if GUI_SUPPORT_TOUCH
  case GM_TOUCH: {
    int32_t packed = pMsg->Data.v;
    int16_t x = (int16_t)((packed >> 16) & 0xFFFF);
    int16_t y = (int16_t)(packed & 0xFFFF);
    switch( pMsg->Param ) {
      case TOUCH_DOWN: {
        onTouchDown(x, y);
        break;
      }
      case TOUCH_MOVE: {
        if( true == m_bTouchActive ) {
          int dx = x - m_iTouchStartX,
              dy = y - m_iTouchStartY;
          onTouchMove(dx, dy);
        }
        break;
      }
      case TOUCH_UP: {
        if( true == m_bTouchActive ) {
          int dx = x - m_iTouchStartX,
              dy = y - m_iTouchStartY;
          onTouchUp(dx, dy);
        }
        break;
      }
    }
    pMsg->MsgId = 0;
    break;
  }
#endif

  default:
    break;
  }
}

//=============================================================================
#if GUI_SUPPORT_TOUCH
//-----------------------------------------------------------------------------
void GWidget::onTouchDown(int x, int y)
{
  m_bTouchActive = true;
  m_iTouchStartX = x;
  m_iTouchStartY = y;
}
#endif
//-----------------------------------------------------------------------------

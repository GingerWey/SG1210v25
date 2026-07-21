//-----------------------------------------------------------------------------
/*
 File        : GLabel.cpp
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : GLabel - static text / image label implementation.
               Per SG1210v25 dialog spec section 3.2.

 Date        : 2026.07.21 (V1.00 - initial implementation from dialog spec)
*/
//-----------------------------------------------------------------------------
#include "GDialog.h"
#include "GLabel.h"

#include "CSGDraw.h"
#include "DevDebug.h"
#include "DevIntf.h"
#include "DevRegInfo.h"
#include "DevTypes.h"
#include "GUI.h"
#include "Strings/TextStrs.h"

#include "GWidget.h"
#include <GUI_Type.h>
#include <cstdint>
#include <cstdio>
//-----------------------------------------------------------------------------
GLabel::GLabel(GDialog* pOwner, const GConfig* pConfig, int dlgX0, int dlgY0)
  : GWidget(reinterpret_cast<GWidget*>(pOwner))
{
  m_pConfig = pConfig;
  if( nullptr != pConfig ) {
    m_Rect.x = pConfig->x + dlgX0;
    m_Rect.y = pConfig->y + dlgY0;
    m_Rect.w = pConfig->w;
    m_Rect.h = pConfig->h;
  }
}

//=============================================================================
// GLabel::onShow - draw image / text / register info per config
//-----------------------------------------------------------------------------
void GLabel::onShow()
{
  DEV_ASSERT( nullptr == m_pConfig, GFC_EmptyPtr );

  GUI_RECT rtInfo;
  rtInfo.x0 = m_Rect.x;
  rtInfo.y0 = m_Rect.y;
  rtInfo.x1 = m_Rect.x + m_Rect.w - 1;
  rtInfo.y1 = m_Rect.y + m_Rect.h - 1;

  // Background (only in NORMAL draw mode)
  if( GUI_DRAWMODE_TRANS != m_pConfig->drawMode ) {
    GUI_SetColor(m_pConfig->crBackground);
    GUI_FillRect(rtInfo.x0, rtInfo.y0, rtInfo.x1, rtInfo.y1);
  }

  bool bHasImage = (nullptr != m_pConfig->image.pAtlas);

  // Resolve text: pText > uTextId > register info
  const char* pStr = nullptr;
  if( nullptr != m_pConfig->pText ) {
    pStr = m_pConfig->pText;
  } else if( 0 != m_pConfig->uTextId ) {
    pStr = GetMultiLangString(m_pConfig->uTextId);
  } else if( false == bHasImage ) {
    // Draw register info via owner Dialog
    GDialog* pDlg = reinterpret_cast<GDialog*>(m_pParent);
    if( nullptr != pDlg ) {
      static char szBuf[40];
      uint32_t uRegNum = pDlg->getRegNum();
      const TDevRegInfoItem* pProp = DevIntf_GetRegInfo(uRegNum);

      switch( m_pConfig->regDraw ) {
      case drName:
        if( nullptr != pProp ) {
          pStr = GetMultiLangString(pProp->NameStrId);
        }
        break;
      case drTitle:
        pStr = pDlg->getTitle();
        break;
      case drMax:
        if( nullptr != pProp ) {
          const char* pDim = RINF_GetDIMNameEx(pProp);
          snprintf(szBuf, sizeof(szBuf), "Max=%u%s",
                   (unsigned)pProp->Max, (nullptr != pDim) ? pDim : "");
          pStr = szBuf;
        }
        break;
      case drMin:
        if( nullptr != pProp ) {
          const char* pDim = RINF_GetDIMNameEx(pProp);
          snprintf(szBuf, sizeof(szBuf), "Min=%d%s",
                   (int)pProp->Min, (nullptr != pDim) ? pDim : "");
          pStr = szBuf;
        }
        break;
      default:
        break;
      }
    }
  }

  // Draw image (left) + text (right) per available content
  int iImgW = 0;
  if( true == bHasImage ) {
    CSG_DrawPicture(m_pConfig->image.pAtlas, rtInfo.x0, rtInfo.y0,
                    m_pConfig->image.uIndex, 100, nullptr);
    iImgW = m_Rect.h;   // assume square icon of label height
  }

  if( nullptr != pStr ) {
    if( nullptr != m_pConfig->ftText ) {
      GUI_SetFont(m_pConfig->ftText);
    }
    GUI_SetColor(m_pConfig->crText);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);

    GUI_RECT rText = rtInfo;
    if( true == bHasImage ) {
      rText.x0 = rtInfo.x0 + iImgW;
    }
    GUI_DispStringInRect(pStr, &rText, m_pConfig->alignment);
  }
}
//-----------------------------------------------------------------------------

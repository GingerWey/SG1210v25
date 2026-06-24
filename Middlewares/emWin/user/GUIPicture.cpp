//---------------------------------------------------------------------------
/*
  GUI Picture

  Display GUI picture

  Silver Grid Technology
  Date       : 2023.12.28
*/
//---------------------------------------------------------------------------
#include "GUIPicture.h"

#include "GUIBitMap.h"

#include "GUI.h"
#include "../../../Application/GUI/CSGDraw.h"
//===========================================================================
// Global methods
//---------------------------------------------------------------------------
// Draw Picture
void GUI_DrawPicture(const TGUIPicture *pPic, int x0, int y0)
{
  
  if( nullptr == pPic || nullptr == pPic->pData)
    return ;
    
  switch( pPic->Type )
    {
    case ID_BITMAP:
      {
      const TGUIBitmap *pBmp = (const TGUIBitmap*)(pPic->pData);
      
      LCDX_Bitmap_Draw( pBmp, x0, y0 );
      
      break;
      }

//    case ID_PNG:
//      {
//      GUI_PNG_Draw( pPic->pData, pPic->Size, x0, y0 );
//      break;
//      }


    case ID_CSG:
      {
      CSG_DrawPicture(pPic, x0, y0);
      break;
      }

    case ID_JPG:
      {
      GUI_JPEG_Draw( pPic->pData, pPic->Size, x0, y0 );
      break;
      }
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
/*
  GUI位图

  定义GUI位图

  银网科技
  Date       : 2023.12.05
*/
//---------------------------------------------------------------------------
#include "GUIBitmap.h"

#include "GUI.h"
#include "GUI_Private.h"

#include "dev_cfg.h"

#ifndef __vmSIMULATOR__
  #include "ST7789S.h"
#endif

#include <Global.h>
#include <LCD.h>
#include <stdint.h>
#include <LCD_Protected.h>
#include <GUI_Type.h>
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#define LCD_SetCursor(x, y)   LCD_ST7789S_SetCursor(x, y)
//-----------------------------------------------------------------------------
#ifndef LCD_nCS_LOW
 #define LCD_nCS_LOW
#endif

#ifndef LCD_nCS_HIGH
 #define LCD_nCS_HIGH
#endif
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// 绘制图案
// 输入:
//   plpDesp：输出图案的描述，调用者须自己定义并填写TGUIPartten结构
// 输出:(无)
// 返回:(无)
void LCDX_Draw4GrayBitmap(const TGUIBitmap* pBM, int x0, int y0)
{
#ifndef __vmSIMULATOR__
  U8 pucColors[3];
  pucColors[0] = GUI_pContext->Color >> 16;
  pucColors[1] = GUI_pContext->Color >> 8;
  pucColors[2] = GUI_pContext->Color;

  const U8 *pucPtr, *pucRow;
  pucRow = (const U8*)pBM->pData;
  U32 bNeedCursor = 1;
  for( U32 uY = 0; uY < pBM->YSize; uY++ )
    {
    pucPtr = pucRow;

    for( U32 uX = 0; uX < pBM->XSize; uX++ )
      {
      U32 uPix = (uX & 1)? pucPtr[0] & 0xF : pucPtr[0] >> 4;

      if( pBM->TranColor != uPix )
        {
        U16 uwColor = (((pucColors[2] * uPix) >> 4) << 11) +
                      (((pucColors[1] * uPix) >> 4) << 5)  +
                       ((pucColors[0] * uPix) >> 4);

        if( 0 != bNeedCursor )
          {
          //LCD_nCS_Set(1);
          LCD_SetCursor( uX + x0, uY + y0 );
          bNeedCursor = 0;
      
          //LCD_WRAM_Perpare();
          }

        //LCD_WRAM_Perpare();
        LCD_WriteData( uwColor );
        }
      else
        bNeedCursor = 1;

      if( (uX & 1) == 1 )
        pucPtr++;
      }

    pucRow += pBM->BytesPerLine;
    bNeedCursor = 1;  // 换行了
    }

//  LCD_nCS_Set(1);
#else
    GUI_DRAWMODE DrawMode = GUI_pContext->TextMode;
    GUI_DRAWMODE OldDrawMode = LCD_SetDrawMode(DrawMode);

    LCD_DrawBitmap(x0, y0,
        pBM->XSize, pBM->YSize,
        0, 0,
        1, /* Bits per Pixel */
        pBM->BytesPerLine,
        pBM->pData,
        &LCD_BKCOLORINDEX);

    LCD_SetDrawMode(OldDrawMode); /* Restore draw mode */
#endif
}
//----------------------------------------------------------------------------
// 绘制图案
// 输入:
//   plpDesp：输出图案的描述，调用者须自己定义并填写TGUIPartten结构
// 输出:(无)
// 返回:(无)
void LCDX_Draw8GrayBitmap( const TGUIBitmap *pBM, int x0, int y0 )
{

  U32 uMax = 0;
  const U8 *pucbmpData = (const U8*)pBM->pData;
  for( int iIdx = pBM->XSize * pBM->YSize - 1; iIdx >= 0; iIdx-- )
    if( uMax < pucbmpData[iIdx] )
      uMax = pucbmpData[iIdx];
    
  if( uMax < 0x10 )
    return ;
  
  U8 pucColors[3];
  pucColors[0] = GUI_pContext->Color >> 16;
  pucColors[1] = GUI_pContext->Color >> 8;
  pucColors[2] = GUI_pContext->Color;

  const U8 *pucPtr, *pucRow;
  pucRow = (const U8*)pBM->pData;
  U32 bNeedCursor = 1;
  for( U32 uY = 0; uY < pBM->YSize; uY++ )
    {
    pucPtr = pucRow;
  
    for( U32 uX = 0; uX < pBM->XSize; uX++ )
      {
      U32 uPix = *pucPtr * 0xFF / uMax;
      
      if( pBM->TranColor != uPix )
        {
#ifndef __vmSIMULATOR__
          U16 uwColor = ((((pucColors[2] * uPix) >> 3) & 0x1F) << 11) +
                        ((((pucColors[1] * uPix) >> 2) & 0x3F) << 5)  +
                         (((pucColors[0] * uPix) >> 3) & 0x1F);

        if( bNeedCursor )
          {
          LCD_SetCursor( uX + x0, uY + y0 );
          bNeedCursor = 0;
      
          LCD_WRAM_Perpare();
          }

        LCD_WriteData( uwColor );
#else
        U32 uColor888 = (((pucColors[0] * uPix) >> 8) & 0xFF) << 16 |
                        (((pucColors[1] * uPix) >> 8) & 0xFF) << 8 | 
                        (((pucColors[2] * uPix) >> 8) & 0xFF);
        GUI_SetColor(uColor888);
        GUI_DrawPixel(uX + x0, uY + y0);
#endif
        }
      else
        bNeedCursor = 1;
        
      pucPtr++;
      }

    pucRow += pBM->BytesPerLine;
    bNeedCursor = 1;  // 换行了
    }
  
//  LCD_nCS_Set(1);
}
//----------------------------------------------------------------------------
// 绘制图案
// 输入:
//   plpDesp：输出图案的描述，调用者须自己定义并填写TGUIPartten结构
// 输出:(无)
// 返回:(无)
void LCDX_Draw16BitsBitmap( const TGUIBitmap *pBM, int x0, int y0 )
{

  int  xSize = LCD_GetXSize(), 
       ySize = LCD_GetYSize();
  if( y0 + pBM->YSize >= ySize )
    ySize = ySize - y0;
  else
    ySize = pBM->YSize;

  if( x0 + pBM->XSize >= xSize )
    xSize = xSize - x0;
  else
    xSize = pBM->XSize;
  
  const U8 *pucRow;
  const U8 *pucbmpData = (const U8*)pBM->pData;
  if( y0 < 0 )
    {
    ySize += y0;
    pucRow = pucbmpData + (-y0 * pBM->BytesPerLine);
    }
  else
    pucRow = pucbmpData;

  if( x0 < 0 )
    xSize += x0;

  U32 bNeedCursor = 1;
  for( int uY = 0; uY < ySize; uY++ )
    {
    const U16 *puwPtr = (const U16*)pucRow;
  
    if( x0 < 0 )
      puwPtr += -x0;
    
    for( int uX = 0; uX < xSize; uX++ )
      {
      U16 uwColor = *puwPtr;
      if( pBM->TranColor != uwColor )
        {
#ifndef __vmSIMULATOR__
          if (0 != bNeedCursor)
          {
          LCD_SetCursor( uX + x0, uY + y0 );
          bNeedCursor = 0;
          }

        LCD_DATADDR = uwColor; //LCD_WriteData( uwColor );
#else

        uint32_t uColor888 = (((uwColor & 0xF800) >> 11) * 255 / 31) << 16 | 
                             (((uwColor & 0x07E0) >> 5) * 255 / 63) << 8 | 
                             ((uwColor & 0x001F) * 255 / 31);
        GUI_SetColor(uColor888);
        GUI_DrawPixel(uX + x0, uY + y0);
#endif
        }
      else
        bNeedCursor = 1;
        
      puwPtr++;
      }

    pucRow += pBM->BytesPerLine;
    bNeedCursor = 1;  // 换行了
    }
  
    //  LCD_nCS_Set(1);
}
//===========================================================================
// 全局方法
//---------------------------------------------------------------------------
// 绘制XBitmap
void LCDX_Bitmap_Draw(const TGUIBitmap *pBM, int x0, int y0)
{

  if( !pBM )
    return ;
  
  if( 4 == pBM->BitsPerPixel )
    LCDX_Draw4GrayBitmap( pBM, x0, y0 );
  else if( 8 == pBM->BitsPerPixel )
    LCDX_Draw8GrayBitmap( pBM, x0, y0 );
  else if( 16 == pBM->BitsPerPixel )
    LCDX_Draw16BitsBitmap( pBM, x0, y0 );
}
//---------------------------------------------------------------------------

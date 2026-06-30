//---------------------------------------------------------------------------
/*
  GUI Picture

  ����GUIͼƬ

  Silver Grid Technology
  Date       : 2023.12.05 (V1.10)
  Updated    : 2026.06.25 (V1.11 — added picIndex parameter for CSG atlas)
*/
//---------------------------------------------------------------------------
#ifndef GUI_PICTURE_H
#define GUI_PICTURE_H

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"  {
#endif
//===========================================================================
// Global macros
//---------------------------------------------------------------------------
// Bitmap type flags
#define ID_BITMAP       0x424D
#define ID_CSG          0x5650
#define ID_PNG          0x5089
#define ID_JPG          0xD8FF
//===========================================================================
// Global data structures
//---------------------------------------------------------------------------
// Picture info
typedef struct tagGUIPICTURE
{
  unsigned short      Type;
  unsigned short      version;
  unsigned int        Size;
  const void         *pData;
} TGUIPicture;
//===========================================================================
// Global methods
//---------------------------------------------------------------------------
// Draw Picture
// picIndex: sub-picture index within an atlas (0-based, default 0)
void GUI_DrawPicture(const TGUIPicture *pPic, int x0, int y0,
                     int picIndex = 0, int saturation = 100);
//---------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//---------------------------------------------------------------------------
#endif

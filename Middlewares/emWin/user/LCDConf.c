/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2020  SEGGER Microcontroller GmbH                *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V6.16 - Graphical user interface for embedded applications **
All  Intellectual Property rights  in the Software belongs to  SEGGER.
emWin is protected by  international copyright laws.  Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with the following terms:

The software has been licensed to  ARM LIMITED whose registered office
is situated at  110 Fulbourn Road,  Cambridge CB1 9NJ,  England solely
for  the  purposes  of  creating  libraries  for  ARM7, ARM9, Cortex-M
series,  and   Cortex-R4   processor-based  devices,  sublicensed  and
distributed as part of the  MDK-ARM  Professional  under the terms and
conditions  of  the   End  User  License  supplied  with  the  MDK-ARM
Professional. 
Full source code is available at: www.segger.com

We appreciate your understanding and fairness.
----------------------------------------------------------------------
Licensing information
Licensor:                 SEGGER Software GmbH
Licensed to:              ARM Ltd, 110 Fulbourn Road, CB1 9NJ Cambridge, UK
Licensed SEGGER software: emWin
License number:           GUI-00181
License model:            LES-SLA-20007, Agreement, effective since October 1st 2011 
Licensed product:         MDK-ARM Professional
Licensed platform:        ARM7/9, Cortex-M/R4
Licensed number of seats: -
----------------------------------------------------------------------
File        : LCDConf.c
Purpose     : Display controller configuration (single layer)
---------------------------END-OF-HEADER------------------------------
*/
//-----------------------------------------------------------------------------
/*
   File        : LCDConf.c
   Version     : V1.2
   By          : ÒøÍø¿Æ¼¼

   Description : emWin 6.16 ÒÆÖ²
          
   Date       : 2023.12.25
*/
//-----------------------------------------------------------------------------
#include <stddef.h>

#include "ST7789S.h"

#include "GUIDRV_FlexColor.h"

/*********************************************************************
*
*       Layer configuration
*
**********************************************************************
*/
//
// Physical display size
//
#define XSIZE_PHYS          320
#define YSIZE_PHYS          240

//
// Color conversion
//
#define COLOR_CONVERSION    GUICC_565   //GUICC_M565

//
// Display driver
//
#define DISPLAY_DRIVER      GUIDRV_FLEXCOLOR

//
// Orientation
//
//#define DISPLAY_ORIENTATION   (0)
//#define DISPLAY_ORIENTATION   (GUI_MIRROR_X)
//#define DISPLAY_ORIENTATION   (GUI_MIRROR_Y)
//#define DISPLAY_ORIENTATION   (GUI_MIRROR_X | GUI_MIRROR_Y)
//#define DISPLAY_ORIENTATION   (GUI_SWAP_XY)
//#define DISPLAY_ORIENTATION   (GUI_MIRROR_X | GUI_SWAP_XY)
#define DISPLAY_ORIENTATION   (GUI_MIRROR_Y | GUI_SWAP_XY)
//#define DISPLAY_ORIENTATION   (GUI_MIRROR_X | GUI_MIRROR_Y | GUI_SWAP_XY)

/*********************************************************************
*
*       Configuration checking
*
**********************************************************************
*/
#ifndef   VXSIZE_PHYS
  #define VXSIZE_PHYS XSIZE_PHYS
#endif
#ifndef   VYSIZE_PHYS
  #define VYSIZE_PHYS YSIZE_PHYS
#endif
#ifndef   XSIZE_PHYS
  #error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
  #error Physical Y size of display is not defined!
#endif
#ifndef   COLOR_CONVERSION
  #error Color conversion not defined!
#endif
#ifndef   DISPLAY_DRIVER
  #error No display driver defined!
#endif
#ifndef   DISPLAY_ORIENTATION
  #define DISPLAY_ORIENTATION 0
#endif

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
//-----------------------------------------------------------------------------
/********************************************************************
*
*       LcdWriteReg16
*
* Function description:
*   Sets display register
*/
static void LcdWriteReg16(U16 uwData) 
{

  __NOP();
  
  LCD_REGADDR = uwData;
}
//-----------------------------------------------------------------------------
/********************************************************************
*
*       LcdReadReg16
*
* Function description:
*   Reada value from display register
*/
static U16 LcdReadReg16()
{

  for( int iIdx = 0; iIdx < 160 * 5; iIdx++ )
    __NOP();

  U16 uwData = LCD_REGADDR;
  
  return uwData;
}
//-----------------------------------------------------------------------------
/********************************************************************
*
*       LcdWriteData16
*
* Function description:
*   Writes a value to a display buffer
*/
static void LcdWriteData16(U16 uwData) 
{

  __NOP();

  LCD_DATADDR = uwData;
}
//-----------------------------------------------------------------------------
/********************************************************************
*
*       LcdReadData16
*
* Function description:
*   Read a value from a display buffer
*/
uint16_t LcdReadData16 (void)
{
  
  uint16_t uwData = LCD_DATADDR;
  
  return uwData;
}
//-----------------------------------------------------------------------------
/********************************************************************
*
*       LcdWriteReg16Multiple
*
* Function description:
*   Writes multiple values to a display register.
*/
static void LcdWriteReg16Multiple(U16 * pData, int NumItems) 
{
  
  while (NumItems--) {
    __NOP();
    LCD_REGADDR = *pData++;
  } 
}
//-----------------------------------------------------------------------------
/********************************************************************
*
*       LcdReadReg16Multiple
*
* Function description:
*   Reads multiple values from a display register.
*/
static void LcdReadReg16Multiple(U16 * pData, int NumItems) 
{
  
  while (NumItems--)
    *pData++ = LcdReadReg16();
}
//-----------------------------------------------------------------------------
/********************************************************************
*
*       LcdWriteData16Multiple
*
* Function description:
*   Writes multiple values to display buffer.
*/
static void LcdWriteData16Multiple(U16 * pData, int NumItems) 
{
  
  while (NumItems--) {
    __NOP();
    LCD_DATADDR = *pData++;
  } 
}
/********************************************************************
*
*       LcdReadData16Multiple
*
* Function description:
*   Reads multiple values from display buffer.
*/
static void LcdReadData16Multiple(U16 * pData, int NumItems) 
{
  
  while (NumItems--)
    *pData++ = LCD_DATADDR;
}
/*********************************************************************
*
*       _InitController
*
* Purpose:
*   Initializes the display controller
*/
/*********************************************************************
*
*       _ReadPixel
*/
static U16 _ReadPixel(int LayerIndex) {
  #ifndef WIN32
  U16 aData[3];
  U16 Index;

  GUI_USE_PARA(LayerIndex);
  //
  // Switch to read mode
  //
  //LCD_X_8080_16_Write00(0x2E);
  LCD_REGADDR = 0x2E;
  //
  // Dummy- and data read
  //
  //LCD_X_8080_16_ReadM01(aData, GUI_COUNTOF(aData));
  LcdReadData16Multiple(aData, GUI_COUNTOF(aData));
  
  //
  // Convert to index
  //
  Index = (aData[2] >> 11) | ((aData[1] & 0xfc) << 3) | (aData[1] & 0xf800);
  #else
  U16 Index;

  GUI_USE_PARA(LayerIndex);
  Index = 0;
  #endif
  return Index;
}

/*********************************************************************
*
*       _ReadMPixels
*/
static void _ReadMPixels(int LayerIndex, U16 * pBuffer, U32 NumPixels) {
  #ifndef WIN32
  U16 Data;
  U16 Index;
  int State;

  GUI_USE_PARA(LayerIndex);
  //
  // Switch to read mode
  //
  //LCD_X_8080_16_Write00(0x2E);
  LCD_REGADDR = 0x2E;
  //
  // Dummy- read
  //
  //LCD_X_8080_16_Read01();
  Data = LCD_DATADDR;
  for (State = 0; NumPixels; NumPixels--, State ^= 1) 
  {
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    
    switch (State) {
    case 0:
      Data = LCD_DATADDR;  //LCD_X_8080_16_Read01();
      Index  =  Data & 0xf800;        // blue
      Index |= (Data & 0x00fc) <<  3; // green
      Data = LCD_DATADDR;  //LCD_X_8080_16_Read01();
      Index |= (Data & 0xf800) >> 11; // red
      break;
    case 1:
      Index  = (Data & 0x00f8) <<  8; // blue
      Data = LCD_DATADDR;  //LCD_X_8080_16_Read01();
      Index |= (Data & 0xfc00) >>  5; // green
      Index |= (Data & 0x00f8) >>  3; // red
      break;
    }
    *pBuffer++ = Index;
  }
  #else
  GUI_USE_PARA(LayerIndex);
  GUI_USE_PARA(pBuffer);
  GUI_USE_PARA(NumPixels);
  #endif
  return;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       LCD_X_Config
*
* Purpose:
*   Called during the initialization process in order to set up the
*   display driver configuration.
*
*/
void LCD_X_Config(void) {
  GUI_DEVICE * pDevice;
  GUI_PORT_API PortAPI = {0};
  CONFIG_FLEXCOLOR Config = {0};

  //
  // Set display driver and color conversion for 1st layer
  //
  pDevice = GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER, COLOR_CONVERSION, 0, 0);
  //
  // Display driver configuration
  //
  if (DISPLAY_ORIENTATION & GUI_SWAP_XY) {
    LCD_SetSizeEx (0, YSIZE_PHYS,   XSIZE_PHYS);
    LCD_SetVSizeEx(0, VYSIZE_PHYS,  VXSIZE_PHYS);
  } else {
    LCD_SetSizeEx (0, XSIZE_PHYS,   YSIZE_PHYS);
    LCD_SetVSizeEx(0, VXSIZE_PHYS,  VYSIZE_PHYS);
  }
  //
  // Function selection, hardware routines (PortAPI) and operation mode (bus, bpp and cache)
  //
  #ifndef WIN32
  PortAPI.pfWrite16_A0  = LcdWriteReg16;          // LCD_X_8080_16_Write00;
  PortAPI.pfWrite16_A1  = LcdWriteData16;         // LCD_X_8080_16_Write01;
  PortAPI.pfWriteM16_A0 = LcdWriteReg16Multiple;  // LCD_X_8080_16_WriteM00;
  PortAPI.pfWriteM16_A1 = LcdWriteData16Multiple; // LCD_X_8080_16_WriteM01;
  PortAPI.pfRead16_A0   = LcdReadReg16;           // LCD_X_8080_16_Read00;
  PortAPI.pfRead16_A1   = LcdReadData16;          // LCD_X_8080_16_Read01;
  PortAPI.pfReadM16_A0  = LcdReadReg16Multiple;   // LCD_X_8080_16_ReadM00;
  PortAPI.pfReadM16_A1  = LcdReadData16Multiple;  // LCD_X_8080_16_ReadM01;
  #endif
  GUIDRV_FlexColor_SetFunc( pDevice, 
                           &PortAPI, 
                            GUIDRV_FLEXCOLOR_F66709, 
                            GUIDRV_FLEXCOLOR_M16C0B16);
  //
  // Orientation
  //
  Config.Orientation = DISPLAY_ORIENTATION;
  GUIDRV_FlexColor_Config(pDevice, &Config);
  //
  // Set custom reading function(s)
  //
  LCD_SetDevFunc(0, LCD_DEVFUNC_READPIXEL,   (void(*)(void))_ReadPixel);
  LCD_SetDevFunc(0, LCD_DEVFUNC_READMPIXELS, (void(*)(void))_ReadMPixels);
}

/*********************************************************************
*
*       LCD_X_DisplayDriver
*
* Purpose:
*   This function is called by the display driver for several purposes.
*   To support the according task the routine needs to be adapted to
*   the display controller. Please note that the commands marked with
*   'optional' are not cogently required and should only be adapted if
*   the display controller supports these features.
*
* Parameter:
*   LayerIndex - Index of layer to be configured
*   Cmd        - Please refer to the details in the switch statement below
*   pData      - Pointer to a LCD_X_DATA structure
*/
int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData) {
  int r;

  GUI_USE_PARA(LayerIndex);
  GUI_USE_PARA(pData);
  switch (Cmd) {
  //
  // Required
  //
  case LCD_X_INITCONTROLLER: {
    //
    // Called during the initialization process in order to set up the
    // display controller and put it into operation. If the display
    // controller is not initialized by any external routine this needs
    // to be adapted by the customer...
    //
    LCD_ST7789S_Init();
    return 0;
  }
  default:
    r = -1;
  }
  return r;
}

/*************************** End of file ****************************/

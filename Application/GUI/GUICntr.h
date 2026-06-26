//-----------------------------------------------------------------------------
/*
 File        : GUICntr.h
 Version     : V1.10
 By          : Silver Grid Technology
 Description : GUI controller
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GUICNTR_H
#define GUI_GUICNTR_H

#include "GUI.h"
#include "GWinTypes.h"
#include "Dev_Cfg.h"

#ifndef __vmSIMULATOR__ 
 #include "cmsis_os.h"
#endif

#include <stdint.h>
//-----------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {     // Make sure we have C-declarations in C++ programs
#endif
//=============================================================================
// Global macros
//-----------------------------------------------------------------------------
// GUI����
#define  GUI_OK        0xA5
// GUI Mode
#define  GMD_NORMAL    0
#define  GMD_IAP       0x1A
//=============================================================================
// Global data structures
//-----------------------------------------------------------------------------
typedef struct tagGUIState
{
  uint8_t   ucGUIOk;

  uint8_t   ucMode;
  uint8_t   ucCurIRKey;
  
  //const GWinForm *pCurForm;
  //      uint32_t  uCurFormID;

        uint32_t  uKeyIdle;

        uint32_t  uStartupTime;

#ifndef __vmSIMULATOR__
        osMutexId  mutexGUI;
#endif
}TGUIState;
//-----------------------------------------------------------------------------
// class: vxWindowsManager
// Window manager
// Features:
//   1.��WM_HWINΪ��ʶ���������д���
//   2.�������ڣ����ٴ��ڣ��л�����
//   3.Provide inter-window communication interface
//   4.Provide global message interface
//   5.Provide global state interface
//   6.Provide forced initialization interface
//   7.�ṩ����������ʾ�ӿ�
//   8.���д��ڰ�Stack��ʽ�������л�����ʱ�����ٵ�ǰ���ڣ������´���
//=============================================================================
// Global data
//-----------------------------------------------------------------------------
// ��ǰ����ָ��
extern volatile TGUIState  FGUIState;
//=============================================================================
// Global methods
//-----------------------------------------------------------------------------
// Initialize
void GUIStart(void);
//-----------------------------------------------------------------------------
// ���� GUI, �ӹ�����
void GUICenter(void);
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// ��ָ������ 
bool GUIFormOpen( uint32_t uFormId, const void* para );
//-----------------------------------------------------------------------------
// �رմ���
bool GUIFormClose(const void* para=nullptr);
//-----------------------------------------------------------------------------
//// �л���ָ������
//bool GUIFormSwitch(uint32_t uFormId, const void* para);
//-----------------------------------------------------------------------------
// GUI mode changed
void GUIModeChanged(void);
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef __vmSIMULATOR__
// Key event
void GUIKeyEnevt(uint32_t uKey, uint32_t uPressedCnt);
#endif
//-----------------------------------------------------------------------------
// �������
//-----------------------------------------------------------------------------
// Window Identifiers
#define  WID_CONSOLE         1

#define  WID_FORMBEGIN       100
#define  WID_SplashForm      (WID_FORMBEGIN + 0)
#define  WID_MenuForm        (WID_FORMBEGIN + 1)
#define  WID_MainForm        (WID_FORMBEGIN + 2)
#define  WID_EventListForm   (WID_FORMBEGIN + 3)
#define  WID_CTRLConfigForm  (WID_FORMBEGIN + 4)
#define  WID_UARTConfigForm  (WID_FORMBEGIN + 5)
#define  WID_DevInfoForm     (WID_FORMBEGIN + 6)
#define  WID_MessageForm     (WID_FORMBEGIN + 7)
#define  WID_ConfigForm      (WID_FORMBEGIN + 8)
#define  WID_GuageForm       (WID_FORMBEGIN + 9)
#define  WID_FatalForm       (WID_FORMBEGIN + 10)
#define  WID_LoginDialog     (WID_FORMBEGIN + 11)
#define  WID_TimeDialog      (WID_FORMBEGIN + 12)
#define  WID_WLGListForm     (WID_FORMBEGIN + 13)
#define  WID_WLGViewForm     (WID_FORMBEGIN + 14)
//-----------------------------------------------------------------------------
// �������ڷ���Ϣ
void GUISendMessage( uint32_t uMsgIdent, uint32_t uParam, uint32_t uData );
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// ����������Ϣ
//-----------------------------------------------------------------------------
// ϵͳ������������ʱ����ʾ������Ϣ
void GUIShowFatalMessage(const char* pcMsg);
//-----------------------------------------------------------------------------
#if defined(__cplusplus)
}
#endif
//-----------------------------------------------------------------------------
#endif // GUI_GUICNTR_H

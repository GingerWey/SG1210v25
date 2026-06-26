//-----------------------------------------------------------------------------
/*
  GUI Message System

  Defines GUI message structure and message IDs.

  Silver Grid Technology
  Date       : 2023.12.05
  Updated    : 2026.06.24 (GM_FORM_ACTIVATED / GM_FORM_DEACTIVATED added)
               2026.06.25 (TOUCH_UP/DOWN/MOVE sub-type constants for GM_TOUCH)
*/
//-----------------------------------------------------------------------------
#ifndef GUI_MESSAGE_H
#define GUI_MESSAGE_H

#include <cstdint>
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//-----------------------------------------------------------------------------
// Key / paint messages
#define GM_PAINT                      1     // Repaint window (because content is (partially) invalid 
#define GM_CREATE                     2     // The first message received, right after client has actually been created 
#define GM_CLOSE                      3     // Close top window. 
#define GM_SHOWAIT                    4     // Show wait window. 
#define GM_CANCELWAIT                 5     // Cancel wait window. 
#define GM_MAINMENU                   6     // Create and show mainmenu 
#define GM_MOVE                       7     // window has been moved 
#define GM_SHOW                       8     // windows has just received the show command 
#define GM_HIDE                       9     // windows has just received the hide command 
#define GM_FGND                      10     // window has been made top of window stack 
#define GM_BGND                      11     // window has just been put to bottom of stack 
#define GM_TOUCH                     12     // touch screen message

// Touch event sub-types (in Param field of GM_TOUCH)
#define TOUCH_UP                     0      // Pen/finger released
#define TOUCH_DOWN                   1      // Pen/finger pressed
#define TOUCH_MOVE                   2      // Pen/finger moved while pressed
// Coordinate encoding in Data.v: (uint16_t x << 16) | (uint16_t y)
#define GM_KEYDOWN                   13     // Key has been pressed 
#define GM_KEYPRESS                  14     // Key has been pressed and hold
#define GM_KEYUP                     15     // Key has been released 
#define GM_TIMER_TICK                16     // Timer tick 
#define GM_ISEDITING                 20     // In Editting 
#define GM_CANCELEDIT                21     // Cancel editting 
#define GM_IDLE_EXPIRE               22     // Idle expire query. if Form want prevents closing when idle expire, 
                                            // then it should fill Message.Data.v with number greater than zero. 
#define GM_FORM_ACTIVATED            18     // Form has been activated (became top of stack)
#define GM_FORM_DEACTIVATED          19     // Form has been deactivated (pushed down or closed)

#define GM_SHOW_MSGDLG               23     // Show Message dialog, Param= Icon, Data.v = String
#define GM_SHOW_INFDLG               24     // Show Message dialog, Param= Type, Data.v = HWND

#define GM_SET_TICK                  25     // Set Window close Tick, Msg.Param = Tick 
#define GM_SET_TEXT                  26     // Set Text for Component, eg. TGUIMemoLabel 
#define GM_SET_IMAGE_INDEX           28     // Set Image index for TGUIImageList, Msg.Param = index 

#define GM_HAS_FATALEVENT            30     // SFC has fatal event 
#define GM_HAS_ALARMEVENT            31     // SFC has alarm event 

#define GM_LISTITEMS_CHANGED         40     // List items changed 

#define GM_NEXT_PAGE                 50     // Next page
#define GM_PERIOR_PAGE               51     // Previous page

#define GM_GET_CLIENT_RECT          100     // get client rectangle in window coordinates*/
#define GM_GET_INSIDE_RECT          102     // get inside rectangle: client rectangle minus pixels lost to effect
#define GM_GET_ORGIN                104
#define GM_GET_ID                   105     // Get id of widget
#define GM_GET_CLIENT_WINDOW        106     // Get window handle of client window. Default is the same as window
#define GM_CAPTURE_RELEASED         107     // Let window know that mouse capture is over

#define GM_INIT_DIALOG              109     // Inform dialog that it is ready for init
#define GM_SET_FOCUS                110     // Inform window that it has gotten or lost the focus
#define GM_GET_FOCUS                111     // query which window has gotten the focus
#define GM_GET_ACCEPT_FOCUS         112     // Find out if window can accept the focus
#define GM_GET_FOCUSSED_CHILD       113     // Which child currently has the focus    
#define GM_GET_HAS_FOCUS            114     // Does this window have the focus ?      
#define GM_GET_BKCOLOR              115     // Return back ground color (only frame window and similar)
#define GM_SET_ENABLE               116     // Enable or disable widget
#define GM_GET_SCROLL               117     // Get scroll status ... only effective for scrollbars
#define GM_SET_SCROLL               118     // Set scroll status ... only effective for scrollbars

#define GM_SET_EDITSTATE            119     // Set edit state       
#define GM_EDITOR_ACCEPT            120     // Editor are accept.   
#define GM_EDITOR_CANCEL            121     // Editor are cancelled.

#define GM_NOTIFY_CHILD_HAS_FOCUS   125
#define GM_NOTIFY_PARENT            126     // Notify to parent

#define GM_CLOSE_QUERY              130     // Query before form will be closed

#define GM_SETTING_ENABLE           200     // Enable  setting 
#define GM_SETTING_DISABLE          201     // Disable setting 
#define GM_DEVICE_COMFIRM           202     // Device set Comfirm setting 
#define GM_DEVICE_CANCEL            203     // Device set Cancel  setting 
#define GM_SETTING_COMFIRM          204     // Comfirm setting 
#define GM_SETTING_CANCEL           205     // Cancel  setting 

#define GM_TIMERECV                 220
#define GM_DATARECV                 221

#define GM_USER                   10000     // User Message beyond this
//-----------------------------------------------------------------------------
// Message structure (C++ class wrapper)�
#ifdef __cplusplus
class TGUIMessage
{
public:
  uint16_t    MsgId;          // Identify of message
  uint16_t    Param;          // param of message
  union
    {
    void*     p;              // Some messages need more info
    int32_t   v;
  } Data;
};
#endif
//-----------------------------------------------------------------------------
// C-style struct
typedef struct _GM_MESSAGE
{
  uint16_t    MsgId;          // Identify of message
  uint16_t    Param;          // param of message
  union
    {
    void*     p;              // Some messages need more info
    int32_t   v;
  } Data;
} GM_MESSAGE;
//-----------------------------------------------------------------------------
// Key state info
typedef struct _GM_KEY_INFO 
{
  uint16_t    Key;            // Key
  uint16_t    PressedCnt;     // Pressed count
} GM_KEY_INFO;
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif // GUI_MESSAGE_H

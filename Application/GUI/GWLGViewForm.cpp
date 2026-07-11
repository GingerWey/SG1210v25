//-----------------------------------------------------------------------------
/*
 File        : GPWLGViewForm.c
 Version     : V1.10
 By          : 银网科技
 Description: 录波波形窗体
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "GWLGViewForm.h"

#include "GUI.h"
#include "GUICntr.h"
#include "GUIConf.h"
#include "GUIFontCHS12x12.h"
#include "FontCHS24x24LTH.h"

#include "GUIBitMap.h"
#include "PictureRes.h"
#include "ST7796S.h"

#include "CRC1632.h"
#include "DevRegs.h"
#include "GPVersion.h"

#include "WaveLogger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stm32f4xx_hal.h>
//-----------------------------------------------------------------------------
#if WAVELOGGER_EN > 0
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
// 样本时间间隔us
#define INTERVAL_SMAPLE  (int)(20000 / NUM_SAMPLOG_PER_PEROID)
  
#define getVisSamples    (ChartView_WIDTH * m_FormState.FucScale)
#define FSCALE_MAX       ((int)(SIZE_SAMPLOG_ADCCHL / ChartView_WIDTH) + 2)
#define FSCALE_STEP      2
//-----------------------------------------------------------------------------
// 窗体部件
//-----------------------------------------------------------------------------
#define HIGHT_CAPTION     30

#define WIDTH_EDGE         4
//-----------------------------------------------------------------------------
#define Caption_x1         0
#define Caption_y1         0
#define Caption_x2        (DESKTOP_WIDTH - 1)
#define Caption_y2        (HIGHT_CAPTION - 1)
                          
#define CapLogTime_x1     (Caption_x1 +  WIDTH_EDGE)
#define CapLogTime_y1     (Caption_y1 +  WIDTH_EDGE / 2)
#define CapLogTime_x2     (CapLogTime_x1 + DESKTOP_WIDTH / 2 - 1)
#define CapLogTime_y2     (Caption_y2 -  WIDTH_EDGE / 2)

#define CapScale_x1       (CapLogTime_x2 + WIDTH_EDGE)
#define CapScale_y1       (CapLogTime_y1)
#define CapScale_x2       (CapScale_x1 + 80 - 1)
#define CapScale_y2       (CapLogTime_y2)

#define CapOffset_x1      (CapScale_x2 + WIDTH_EDGE)
#define CapOffset_y1      (CapLogTime_y1)
#define CapOffset_x2      (Caption_x2  - WIDTH_EDGE)
#define CapOffset_y2      (CapLogTime_y2)

// Wave List              
#define ChartPanel_x1      (0 + WIDTH_EDGE / 2)
#define ChartPanel_y1      (Caption_y2)
#define ChartPanel_x2      (DESKTOP_WIDTH  - WIDTH_EDGE / 2 - 1)
#define ChartPanel_y2      (DESKTOP_HEIGHT - WIDTH_EDGE / 2 - 1)
                          
#define ChartView_x1       (ChartPanel_x1 +  WIDTH_EDGE / 2)
#define ChartView_y1       (ChartPanel_y1 +  WIDTH_EDGE / 2)
#define ChartView_x2       (ChartPanel_x2 -  WIDTH_EDGE / 2)
#define ChartView_y2       (ChartPanel_y2 -  WIDTH_EDGE / 2)

#define ChartView_WIDTH    (ChartView_x2 - ChartView_x1 + 1)  // 472
#define ChartView_HEIGHT   (ChartView_y2 - ChartView_y1 + 1)  // 285
                          
// Colors                 
#define crFormBkg          0x001F1F1F  
#define crFrameHigh        0x002F2F2F  
#define crFrameDrak        0x000F0F0F
                          
#define crCaptionFont      GUI_WHITE  
#define crCaptionBkg       0x00101010  
#define ftCaption         &GUI_CHSLTHFont24
                          
#define crChartBkg         0 
#define crChartLine        GUI_GRAY_3F
#define crCursor           0x00FFFFFF

#define crChartText        0x0055551F
#define ftChartText       &GUI_CHSLTHFont24

#define crCurve            0x004040FF

#define crGuageBar         0x0040FF40
#define crGuageBarBkg      0x00004000
//-----------------------------------------------------------------------------
// 通道高度
#define HEIGHT_ADC_CHL      70
#define HEIGHT_DIG_CHL      24

// 通道描述
#define WIDTH_CHLTEXT       48
#define CurveText_x1       (ChartView_x1 + WIDTH_EDGE) 
#define CurveText_x2       (CurveText_x1 + WIDTH_CHLTEXT - 1) 

// 进度条
#define GuageBar_x1        (ChartView_x1 + 1)
#define GuageBar_y1        (ChartView_y1 + 1)
#define GuageBar_x2        (ChartView_x2 - 1)
#define GuageBar_y2        (GuageBar_y1 + WIDTH_EDGE  - 1)
#define GuageBar_WIDTH     (GuageBar_x2 - GuageBar_x1 + 1)

// 各通道所在位置
#define Channel1_y1        (GuageBar_y2 + 1)
#define Channel1_y2        (Channel1_y1 + HEIGHT_ADC_CHL - 1)
#define Curve1_y1          (Channel1_y1 + WIDTH_EDGE)
#define Curve1_y2          (Channel1_y2 - WIDTH_EDGE - 2)
#define Curve1_VCenter     ((Curve1_y1 + Curve1_y2) / 2)

#define Channel2_y1        (Curve1_y2 - WIDTH_EDGE + 2)
#define Channel2_y2        (Channel2_y1 + HEIGHT_ADC_CHL - 1)
#define Curve2_y1          (Channel2_y1 + WIDTH_EDGE - 2)
#define Curve2_y2          (Channel2_y2 - WIDTH_EDGE)
#define Curve2_VCenter     ((Curve2_y1 + Curve2_y2) / 2)

#define Channel3_y1        (Channel2_y2 + 1)
#define Channel3_y2        (Channel3_y1 + HEIGHT_DIG_CHL - 1)
#define Curve3_y1          (Channel3_y1 + WIDTH_EDGE)
#define Curve3_y2          (Channel3_y2 - WIDTH_EDGE)
#define Curve3_VCenter     ((Curve3_y1 + Curve3_y2) / 2)

#define Channel4_y1        (Channel3_y2 + 1)
#define Channel4_y2        (Channel4_y1 + HEIGHT_DIG_CHL - 1)
#define Curve4_y1          (Channel4_y1 + WIDTH_EDGE)
#define Curve4_y2          (Channel4_y2 - WIDTH_EDGE)
#define Curve4_VCenter     ((Curve4_y1 + Curve4_y2) / 2)

#define Channel5_y1        (Channel4_y2 + 1)
#define Channel5_y2        (Channel5_y1 + HEIGHT_DIG_CHL - 1)
#define Curve5_y1          (Channel5_y1 + WIDTH_EDGE)
#define Curve5_y2          (Channel5_y2 - WIDTH_EDGE)
#define Curve5_VCenter     ((Curve5_y1 + Curve5_y2) / 2)

#define Channel6_y1        (Channel5_y2 + 1)
#define Channel6_y2        (Channel6_y1 + HEIGHT_DIG_CHL - 1)
#define Curve6_y1          (Channel6_y1 + WIDTH_EDGE)
#define Curve6_y2          (Channel6_y2 - WIDTH_EDGE)
#define Curve6_VCenter     ((Curve6_y1 + Curve6_y2) / 2)

#define Channel7_y1        (Channel6_y2 + 1)
#define Channel7_y2        (Channel7_y1 + HEIGHT_DIG_CHL - 1)
#define Curve7_y1          (Channel7_y1 + WIDTH_EDGE)
#define Curve7_y2          (Channel7_y2 - WIDTH_EDGE)
#define Curve7_VCenter     ((Curve7_y1 + Curve7_y2) / 2)

#define Channel8_y1        (Channel7_y2 + 1)
#define Channel8_y2        (Channel8_y1 + HEIGHT_DIG_CHL - 1)
#define Curve8_y1          (Channel8_y1 + WIDTH_EDGE)
#define Curve8_y2          (Channel8_y2 - WIDTH_EDGE)
#define Curve8_VCenter     ((Curve8_y1 + Curve8_y2) / 2)

//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
// 列表状态数据
typedef struct __GWavelogView
{

  uint32_t          FuTick;
  uint32_t          FuNextTick;
                    
  int32_t           FuBegin;
  int32_t           FuCursor;
  uint8_t           FucCursorNum;  // 允许光标
  uint8_t           FucScale;
                    
  uint16_t          FuwLogIndex;
  TEventLogSummary  FLogSummary;
  TWaveLogItem     *FRecorder;

  const GWinForm   *FParent;
} GWavelogView;
//-----------------------------------------------------------------------------
typedef struct __GWaveCurve
{
  uint8_t     uChlType;         // 0=ADC 1=DIG
  uint8_t     uChlIndex;        // 通道序号，模拟和数字分别排序

  const char *Caption;
  
  GUI_RECT    rtCurve;
  GUI_RECT    rtText;
  
  int16_t     iVCenter;
  int16_t     iHeight;
} GWaveCurve;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
// 列表状态
static GWavelogView m_FormState = {0};
//-----------------------------------------------------------------------------
// 录波曲线定义
static const GWaveCurve WavelogCurves[] = 
{
  {0, 0, "电压", {ChartView_x1, Curve1_y1, ChartView_x2, Curve1_y2}, {CurveText_x1, Channel1_y1, CurveText_x2, Channel1_y2}, Curve1_VCenter, (Curve1_y2 - Curve1_y1 + 1) / 2}
 ,{0, 1, "电流", {ChartView_x1, Curve2_y1, ChartView_x2, Curve2_y2}, {CurveText_x1, Channel2_y1, CurveText_x2, Channel2_y2}, Curve2_VCenter, (Curve2_y2 - Curve2_y1 + 1) / 2}
 ,{1, 0, "引弧", {ChartView_x1, Curve3_y1, ChartView_x2, Curve3_y2}, {CurveText_x1, Channel3_y1, CurveText_x2, Channel3_y2}, Curve3_VCenter, (Curve3_y2 - Curve3_y1 + 1) }
 ,{1, 1, "吹弧", {ChartView_x1, Curve4_y1, ChartView_x2, Curve4_y2}, {CurveText_x1, Channel4_y1, CurveText_x2, Channel4_y2}, Curve4_VCenter, (Curve4_y2 - Curve4_y1 + 1) }
 ,{1, 2, "合位", {ChartView_x1, Curve5_y1, ChartView_x2, Curve5_y2}, {CurveText_x1, Channel5_y1, CurveText_x2, Channel5_y2}, Curve5_VCenter, (Curve5_y2 - Curve5_y1 + 1) }
 ,{1, 3, "分位", {ChartView_x1, Curve6_y1, ChartView_x2, Curve6_y2}, {CurveText_x1, Channel6_y1, CurveText_x2, Channel6_y2}, Curve6_VCenter, (Curve6_y2 - Curve6_y1 + 1) }
 ,{1, 4, "合闸", {ChartView_x1, Curve7_y1, ChartView_x2, Curve7_y2}, {CurveText_x1, Channel7_y1, CurveText_x2, Channel7_y2}, Curve7_VCenter, (Curve7_y2 - Curve7_y1 + 1) }
 ,{1, 5, "跳闸", {ChartView_x1, Curve8_y1, ChartView_x2, Curve8_y2}, {CurveText_x1, Channel8_y1, CurveText_x2, Channel8_y2}, Curve8_VCenter, (Curve8_y2 - Curve8_y1 + 1) }
};
#define NUM_WavelogCurves (sizeof(WavelogCurves) / sizeof(WavelogCurves[0]))
//=============================================================================
// 引用数据区
//-----------------------------------------------------------------------------
//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// 读录波曲线
//-----------------------------------------------------------------------------
static void _FlushForm()
{
  
  osMutexWait( FGUIState.FLCDMutex, osWaitForever );
  
  GUI_SetBkColor( crFormBkg );
  GUI_Clear();
  
#if HIGHT_CAPTION > 0
  GUI_SetColor( crFrameHigh );
  GUI_DrawRect( CapLogTime_x1 - 2, CapLogTime_y1 - 2, CapLogTime_x2 + 2, CapLogTime_y2 + 2);
  GUI_DrawRect( CapScale_x1 - 2,   CapScale_y1 - 2,   CapScale_x2 + 2,   CapScale_y2 + 2);
  GUI_DrawRect( CapOffset_x1 - 2,  CapOffset_y1 - 2,  CapOffset_x2 + 2,  CapOffset_y2 + 2);

  GUI_SetColor( crCaptionBkg );
  GUI_RECT Rect1 = { CapLogTime_x1, CapLogTime_y1, CapLogTime_x2, CapLogTime_y2 };
  GUI_FillRectEx( &Rect1 );

  GUI_SetColor( crCaptionFont );
  GUI_SetFont( ftCaption ); 
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
  
  char szText[64];
  sprintf( szText,
           "%02u-%02u-%02u %02u:%02u:%02u:%03u",
           m_FormState.FLogSummary.Time.Year % 100,
           m_FormState.FLogSummary.Time.Month,
           m_FormState.FLogSummary.Time.Day,
           m_FormState.FLogSummary.Time.Hours,
           m_FormState.FLogSummary.Time.Minutes,
           m_FormState.FLogSummary.Time.Seconds,
           m_FormState.FLogSummary.Time.milSecs );
  
  Rect1.x0 += WIDTH_EDGE;
  GUI_DispStringInRect( szText, &Rect1, GUI_TA_LEFT | GUI_TA_VCENTER );
#endif

  GUI_SetColor( crFrameHigh );
  GUI_DrawRect( ChartPanel_x1, ChartPanel_y1, ChartPanel_x2, ChartPanel_y2 );

  osMutexRelease( FGUIState.FLCDMutex ); 
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _UpdateSacleRate()
{

  char szText[32];
  if( m_FormState.FucScale == 1 )
    sprintf( szText, "1X" );
  else
    sprintf( szText, "1/%uX", m_FormState.FucScale );
    
  // Earse
  GUI_RECT Rect = { CapScale_x1,  CapScale_y1,  CapScale_x2,  CapScale_y2 };
  GUI_SetColor( crCaptionBkg );
  GUI_FillRectEx( &Rect );
  
  GUI_SetColor( crCaptionFont );
  GUI_SetFont( ftCaption ); 
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
  GUI_DispStringInRect( szText, &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
}
//-----------------------------------------------------------------------------
static void _UpdateTimeOffset()
{

  int iZeroTime = m_FormState.FLogSummary.State.Type * NUM_SAMPLOG_PER_PEROID,
      iCurPos   = m_FormState.FuCursor;
  // 当前点相对零时间的偏移
  int iOffsetUSec = (int)(iCurPos - iZeroTime) * INTERVAL_SMAPLE;

  char szText[32];
  sprintf( szText, "%0.2fms", iOffsetUSec / 1000.0f );
  
  // Earse
  GUI_RECT Rect = { CapOffset_x1, CapOffset_y1, CapOffset_x2, CapOffset_y2 };
  GUI_SetColor( crCaptionBkg );
  GUI_FillRectEx( &Rect );
  
  Rect.x1 -= WIDTH_EDGE;
  GUI_SetColor( crCaptionFont );
  GUI_SetFont( ftCaption ); 
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
  GUI_DispStringInRect( szText, &Rect, GUI_TA_RIGHT | GUI_TA_VCENTER );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void _DrawCurves()
{
  
  if( 0 == m_FormState.FRecorder )
    return ;

  GUI_SetColor( crCurve );
  GUI_SetPenSize( 1 );
  GUI_SetLineStyle( GUI_LS_SOLID );  
  
  // 模拟量通道
  for( uint32_t uIdx = 0; uIdx < 2; uIdx++ )
    {
    const GWaveCurve *pCurve = &WavelogCurves[uIdx];
    uint16_t *puwSamples = &(m_FormState.FRecorder->Measure[uIdx][m_FormState.FuBegin]);
    GUI_POINT p0;
    for( uint32_t uPos = 0; uPos < ChartView_WIDTH; uPos++ )
      {
      uint32_t uSampleIndex = uPos * m_FormState.FucScale;
      uint32_t uY = pCurve->iVCenter;
      if( SIZE_SAMPLOG_ADCCHL > uSampleIndex )
        uY -= ((int16_t)puwSamples[uSampleIndex] - 2048) * pCurve->iHeight / 2048;

      // 合并同值的点为长线
      if( 0 == uPos )
        {
        p0.x = pCurve->rtCurve.x0 + uPos;
        p0.y = uY;
        }
      else if( uY != p0.y )
        {
        uint32_t uX = pCurve->rtCurve.x0 + uPos;
        if( uX - p0.x > 1 )
          GUI_DrawLine( p0.x, p0.y, uX - 1, p0.y );
        GUI_DrawLine( uX - 1, p0.y, uX,     uY );

        p0.x = uX;
        p0.y = uY;
        }
      else if( ChartView_WIDTH - 1 == uPos )
        {
        GUI_DrawLine( p0.x, p0.y, pCurve->rtCurve.x0 + uPos, uY );
        }
      }
    }

  // 二值通道
  for( uint32_t uIdx = 0; uIdx < 6; uIdx++ )
    {
    const GWaveCurve *pCurve = &WavelogCurves[2 + uIdx];

    uint8_t *pucBuff = m_FormState.FRecorder->Binary[pCurve->uChlIndex];
    GUI_POINT p0;
    for( uint32_t uPos = 0; uPos < ChartView_WIDTH; uPos++ )
      {
      uint32_t uSampleIndex = m_FormState.FuBegin + uPos * m_FormState.FucScale;
      uint32_t uY;
      if( SIZE_SAMPLOG_ADCCHL > uSampleIndex )
        {
        uint32_t uByteIndex = uSampleIndex / 8;
        uint8_t  ucSample   = pucBuff[uByteIndex],
                 ucMask     = 1 << (uSampleIndex & 0x7);

         if( 0 == (ucSample & ucMask) )
          uY = pCurve->rtCurve.y1;
        else
          uY = pCurve->rtCurve.y0;
        }
      else
        {
        //uY = pCurve->rtCurve.y1;  
        GUI_DrawLine( p0.x, p0.y, pCurve->rtCurve.x0 + uPos, p0.y );
        break;
        }

      // Draw
      // 合并同值的点为长线
      if( 0 == uPos )
        {
        p0.x = pCurve->rtCurve.x0 + uPos;
        p0.y = uY;
        }
      else if( uY != p0.y )
        {
        uint32_t uX = pCurve->rtCurve.x0 + uPos;
        if( uX - p0.x > 1 )
          GUI_DrawLine( p0.x, p0.y, uX - 1, p0.y );
        GUI_DrawLine( uX - 1, p0.y, uX,     uY );

        p0.x = uX;
        p0.y = uY;
        }
      else if( ChartView_WIDTH - 1 == uPos )
        {
        GUI_DrawLine( p0.x, p0.y, pCurve->rtCurve.x0 + uPos, uY );
        }
      }
    }
}
//-----------------------------------------------------------------------------
static void _DrawCursor()
{
  
  if( 0 == m_FormState.FucCursorNum )
    return ;
  
  // 
  GUI_SetColor( crCursor );
  GUI_SetPenSize( 1 );
  GUI_SetLineStyle( GUI_LS_SOLID );
  GUI_SetDrawMode( GUI_DRAWMODE_XOR );

  uint32_t uCursorPos = ChartView_x1 + 
                        (m_FormState.FuCursor - m_FormState.FuBegin) /
                        m_FormState.FucScale;
  GUI_DrawLine( uCursorPos, Channel1_y1,
                uCursorPos, ChartView_y2 );

  GUI_SetDrawMode( GUI_DRAWMODE_NORMAL );
}
//-----------------------------------------------------------------------------
static void _DrawGuageBar()
{

  //   
  GUI_SetColor( crGuageBarBkg );
  GUI_FillRect( GuageBar_x1,  GuageBar_y1,  GuageBar_x2,  GuageBar_y2 );
  
  uint32_t uBarBegin = GuageBar_x1 + 
                       m_FormState.FuBegin * GuageBar_WIDTH / SIZE_SAMPLOG_ADCCHL,
           uBarEnd   = GuageBar_x1 + 
                       (m_FormState.FuBegin + getVisSamples - 1) * 
                       GuageBar_WIDTH / SIZE_SAMPLOG_ADCCHL;
  if( uBarEnd > GuageBar_x2 )
    uBarEnd = GuageBar_x2;

  // Slid Bar
  GUI_SetColor( crGuageBar );
  GUI_FillRect( uBarBegin, GuageBar_y1, uBarEnd, GuageBar_y2 );
}
//-----------------------------------------------------------------------------
static void _DrawChart()
{

  if( 0 == m_FormState.FRecorder )
    {
    GUI_SetColor( crCaptionFont );
    GUI_SetFont( ftCaption ); 
    GUI_SetTextMode( GUI_TEXTMODE_TRANS );
      
    GUI_RECT Rect = {ChartView_x1, ChartView_y1, ChartView_x2, ChartView_y2};
    GUI_DispStringInRect( "录波数据读失败！", &Rect, GUI_TA_CENTER | GUI_TA_VCENTER );
    
    return ;
    }
  
  // Chart
  GUI_SetColor( crChartBkg );
  GUI_FillRect( ChartView_x1, ChartView_y1, ChartView_x2, ChartView_y2 );
  
  GUI_SetPenSize( 1 );
  
  GUI_SetFont ( ftChartText ); 
  GUI_SetTextMode( GUI_TEXTMODE_TRANS );
  for( uint32_t uIdx = 0; uIdx < NUM_WavelogCurves; uIdx++ )
    {
    const GWaveCurve *pCurve = &WavelogCurves[uIdx];
    
    GUI_SetColor( crChartLine );
    if( 0 == pCurve->uChlType )
      {
      GUI_SetLineStyle( GUI_LS_DASH );
      GUI_DrawLine( ChartView_x1, pCurve->rtCurve.y0, 
                    ChartView_x2, pCurve->rtCurve.y0 );
      GUI_DrawLine( ChartView_x1, pCurve->rtCurve.y1, 
                    ChartView_x2, pCurve->rtCurve.y1 );
      GUI_SetLineStyle( GUI_LS_DOT );
      GUI_DrawLine( ChartView_x1, pCurve->iVCenter, 
                    ChartView_x2, pCurve->iVCenter );
      }
    else
      {
      GUI_SetLineStyle( GUI_LS_DOT );
      GUI_DrawLine( ChartView_x1, pCurve->rtCurve.y0, 
                    ChartView_x2, pCurve->rtCurve.y0 );
      GUI_DrawLine( ChartView_x1, pCurve->rtCurve.y1, 
                    ChartView_x2, pCurve->rtCurve.y1 );
      }
        
    GUI_SetColor( crChartText );
    GUI_DispStringInRect( pCurve->Caption, &(pCurve->rtText), 
                          GUI_TA_LEFT | GUI_TA_VCENTER );
    }
  
  GUI_SetLineStyle( GUI_LS_SOLID );

  // Time Offset    
  _UpdateTimeOffset();
    
  // 
  _DrawGuageBar();
    
  // Draw curve
  _DrawCurves();
    
  // Draw Cursor
  _DrawCursor();
}
//-----------------------------------------------------------------------------
static void _UpdateChartView()
{
  
  _UpdateSacleRate();
  
  _DrawChart();
}
//-----------------------------------------------------------------------------
// 读波形数据
static void _ReadWavelog()
{
  
  if( 0 != m_FormState.FRecorder )
    WAVELOG_ReadWavelog( m_FormState.FuwLogIndex, m_FormState.FRecorder->Measure );
}
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
static void _Init(void const * argument)
{

  memset( &m_FormState, 0, sizeof(m_FormState) );
  m_FormState.FucScale = FSCALE_MAX;

  if( -1ul != (int32_t)argument )
    m_FormState.FParent = (const GWinForm*)argument;
  else
    m_FormState.FParent = 0;
}
//-----------------------------------------------------------------------------
static void _Show(void const * argument)
{

  // 
  m_FormState.FRecorder = WAVLOG_GetRecorder();

  if( 0 <= (int32_t)argument )
    {
    // get wavelog index and summary
    m_FormState.FuwLogIndex = (uint32_t)argument;

    if( 0 != WAVLOG_GetLogSummary( &m_FormState.FLogSummary, 
                                    m_FormState.FuwLogIndex ) )
      {
      m_FormState.FLogSummary.Time.Year = 0;
      
      m_FormState.FRecorder = 0;
      }
    else
      {
      // 光标放在0时刻
      m_FormState.FuCursor = m_FormState.FLogSummary.State.Type * 
                            NUM_SAMPLOG_PER_PEROID;
        
      // 读取波形数据
      _ReadWavelog();
      }
    }
  else if( 0 != m_FormState.FRecorder )
    {
    // get wavelog index and summary
    m_FormState.FuwLogIndex = 0;

    memcpy( &m_FormState.FLogSummary, 
            &m_FormState.FRecorder->Summary,
             sizeof(m_FormState.FRecorder->Summary) );

    // 光标放在0时刻
    m_FormState.FuCursor = m_FormState.FLogSummary.State.Type * 
                          NUM_SAMPLOG_PER_PEROID;
    }

  // Show form
  _FlushForm();

  // 更新显示
  _UpdateChartView();

  // Next Tick
  m_FormState.FuNextTick = GUI_GetTime() + 500;
  m_FormState.FuTick = 0;
}
//-----------------------------------------------------------------------------
static void _Close(void const * argument)
{
}
//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{

  if( uTick > m_FormState.FuNextTick )
    {
    m_FormState.FuNextTick =  uTick+ 500;
    
    m_FormState.FuTick++;
    }
}
//-----------------------------------------------------------------------------
static void _OnKeyDown(uint16_t uwKey)
{

  switch( uwKey )
    {
    case KEY_RIGHT:
      {
      // 向右移动光标
      uint32_t uSampleEnd = m_FormState.FuBegin + 
                            ((ChartView_WIDTH - 1) * m_FormState.FucScale);
      // 
      if( m_FormState.FuCursor + m_FormState.FucScale <= uSampleEnd )
        {
        _DrawCursor();
          
        uint32_t uCursor = m_FormState.FuCursor + m_FormState.FucScale;
        m_FormState.FuCursor = uCursor;
          
        _DrawCursor();
        _UpdateTimeOffset();
        }
      else if( uSampleEnd <= m_FormState.FuBegin + getVisSamples / 3 )
        {
        uint32_t uBegin = m_FormState.FuBegin + getVisSamples / 3;          
        
        m_FormState.FuBegin = uBegin;

        _DrawChart();
        }
      else if( SIZE_SAMPLOG_ADCCHL - getVisSamples > m_FormState.FuBegin )
        {
        uint32_t uBegin = SIZE_SAMPLOG_ADCCHL - getVisSamples;          
        m_FormState.FuBegin = uBegin;
 
        _DrawChart();
        }
        
      break;
      }

    case KEY_LEFT:
      {  
      // 向左移动光标
      if( SIZE_SAMPLOG_ADCCHL >= m_FormState.FuBegin + m_FormState.FucScale )
        {
        _DrawCursor();
          
        uint32_t uCursor    = m_FormState.FuCursor - m_FormState.FucScale;
        m_FormState.FuCursor = uCursor;
          
        _DrawCursor();
        _UpdateTimeOffset();
        }
      else if( m_FormState.FuBegin > getVisSamples / 3 )
        {
        uint32_t uBegin = m_FormState.FuBegin - getVisSamples / 3;          
        
        m_FormState.FuBegin = uBegin;

        _DrawChart();
        }
      else if( m_FormState.FuBegin > 0 )
        {
        m_FormState.FuBegin = 0;

        _DrawChart();
        }

      break;
      }
      
    case KEY_UP:
    case KEY_PAGEUP:
      {
      // 向前翻页
      if(  m_FormState.FuBegin >= getVisSamples )
        {
        uint32_t uBegin  = m_FormState.FuBegin - getVisSamples,
                 uCursor = uBegin + m_FormState.FuCursor - m_FormState.FuBegin;          
        
        m_FormState.FuBegin  = uBegin;
        m_FormState.FuCursor = uCursor;

        _DrawChart();
        }
      else if( m_FormState.FuBegin > 0 )
        {
        uint32_t uCursor = 0 + m_FormState.FuCursor - m_FormState.FuBegin; 

        m_FormState.FuBegin  = 0;
        m_FormState.FuCursor = uCursor;

        _DrawChart();
        }

      break;
      }
      
    case KEY_DOWN:
    case KEY_PAGEDOWN:
      {
      // 向后翻页
      uint32_t uNewBegin = m_FormState.FuBegin + getVisSamples;
      if( SIZE_SAMPLOG_ADCCHL >= uNewBegin + getVisSamples )
        {
        uint32_t uBegin  = uNewBegin,
                 uCursor = uBegin + m_FormState.FuCursor - m_FormState.FuBegin;
        
        m_FormState.FuBegin  = uBegin;
        m_FormState.FuCursor = uCursor;

        _DrawChart();
        }
      else if( SIZE_SAMPLOG_ADCCHL > uNewBegin )
        {
        uint32_t uBegin  = SIZE_SAMPLOG_ADCCHL - getVisSamples,
                 uCursor = uBegin + m_FormState.FuCursor - m_FormState.FuBegin;          
        m_FormState.FuBegin  = uBegin;
        m_FormState.FuCursor = uCursor;
 
        _DrawChart();
        }
      break;
      }
      
    case KEY_ADD:
      {
      if( m_FormState.FucScale > 1 )
        {
        if( m_FormState.FucScale == FSCALE_MAX )
          m_FormState.FucScale = 32;
        else
          m_FormState.FucScale /= FSCALE_STEP;
        _UpdateChartView();  
        }
      break;
      }
      
    case KEY_SUB:
      {
      if( m_FormState.FucScale < FSCALE_MAX )
        {
        m_FormState.FucScale *= FSCALE_STEP;
        if( m_FormState.FucScale > FSCALE_MAX )
          m_FormState.FucScale = FSCALE_MAX;
          
        if( SIZE_SAMPLOG_ADCCHL - getVisSamples < m_FormState.FuBegin )
          {
          uint32_t uBegin  = (getVisSamples >= SIZE_SAMPLOG_ADCCHL)? 
                             0 : (SIZE_SAMPLOG_ADCCHL >= getVisSamples),
                   uCursor = uBegin + m_FormState.FuCursor - m_FormState.FuBegin;          
          m_FormState.FuBegin  = uBegin;
          m_FormState.FuCursor = uCursor;
          }

        _UpdateChartView();  
        }

      break;
      }

    case KEY_ENTER:
      {
      // 光标使能
      if( 0 != m_FormState.FucCursorNum )
        {
        _DrawCursor();
          
        m_FormState.FucCursorNum = 0;
        }
      else
        {
        m_FormState.FucCursorNum = 0x55;
        
        _DrawCursor();          
        }
        
      break;
      }

    case KEY_ESCAPE:
    case KEY_RETURN:
      {
      if( 0 != m_FormState.FParent )
        {
        GM_MESSAGE aMsg = { GM_EDITOR_CANCEL, 0 };
        m_FormState.FParent->pMsg( &aMsg );
        }
      else
        {
        GUIFormSwitch( WID_MenuForm, 1 );
        }
      break;
      }
    }
}
//-----------------------------------------------------------------------------
static void _OnMessage( GM_MESSAGE* pMsg)
{
  
  if( 0 == pMsg )
    return ;
  
  switch( pMsg->MsgId )
    {
    case GM_TIMER_TICK:
      { 
      _OnTick( pMsg->Data.v );
        
      pMsg->MsgId = 0;
      
      break;
      }

    case GM_KEYDOWN:
      {
      if( pMsg->Param )
        {
        _OnKeyDown( pMsg->Param );
          
        pMsg->MsgId = 0;
        }
        
      break;
      }
    }
}
//=============================================================================
// 窗体句柄
//-----------------------------------------------------------------------------
const GWinForm FWavelogViewForm = 
{
  _Init,
  _Show,
  _Close,
  _OnMessage
};
//-----------------------------------------------------------------------------
#endif //WAVELOGGER_EN > 0
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GPSplashForm.cpp
 Version     : V1.12
 By          : Wey. Silver Grid

 Description : Splash form — CSG decoder test rig.
               Cycles through all 11 ImageRes sub-pictures.
               Logs GUI_ALLOC memory stats to detect leaks.

 Date        : 2026.06.26 (V1.12 — ImageRes cycle test + memory leak detection)
              2023.12.05 (V1.10 — original implementation)
*/
//-----------------------------------------------------------------------------//-----------------------------------------------------------------------------
#include "GPSplashForm.h"

#include "GUI.h"
#include "WM.h"
#include "GUI_Private.h"
#include "GWinTypes.h"

#include "GForm.h"
#include "GUICntr.h"
#include "GUIConf.h"
#include "GUIMisc.h"
#include "GUIMessage.h"
#include "FontSGRes.h"

#include "GUIPicture.h"
#include "PictureRes.h"
#include "Graphics/ImageRes.h"
#include "Strings/TextStrs.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "GFormRegistrar.h"
#include <GUI_Type.h>

//=============================================================================
// Image metadata for the cycle display
//-----------------------------------------------------------------------------
static const struct {
    int         picIndex;
    const char* name;
    int         w, h;
    const char* casName;
} kImageList[] = {
    {picIdxMA_Logo78x18,       "Logo78x18",      78,19, "MiniLZ77"},
    {picIdxMA_ACPow32x59Cyan,  "ACPow32x59Cyan", 32,59, "MiniLZ77"},
    {picIdxMA_Ctrl78x61Cyan,   "Ctrl78x61Cyan",  78,61, "DEFLATE"},
    {picIdxMA_Brkr59x60Cyan,   "Brkr59x60Cyan",  59,60, "DEFLATE"},
    {picIdxMA_Battey44x24C1,   "Battey44x24C1",  44,24, "MiniLZ77"},
    {picIdxMA_Battey44x24C2,   "Battey44x24C2",  44,24, "MiniLZ77"},
    {picIdxMA_Battey44x24C3,   "Battey44x24C3",  44,24, "MiniLZ77"},
    {picIdxMA_Battey44x24C4,   "Battey44x24C4",  44,24, "MiniLZ77"},
    {picIdxMA_Battey44x24C5,   "Battey44x24C5",  44,24, "MiniLZ77"},
    {picIdxMA_Fan16x16Cyan,    "Fan16x16Cyan",   16,16, "RLE"},
    {picIdxMA_Fire16x16,       "Fire16x16",      16,16, "RLE"},
};
static constexpr int kImageCount = sizeof(kImageList) / sizeof(kImageList[0]);

//=============================================================================
// Memory tracking
//-----------------------------------------------------------------------------
static int  s_memBaselineFree  = 0;   // free bytes after _Show initial draw
static int  s_memMaxUsed       = 0;   // peak used bytes across all draws
static long s_drawCount        = 0;

static void MemLog(const char* label) {
#ifdef __vmSIMULATOR__
    int freeBytes = (int)GUI_ALLOC_GetNumFreeBytes();
    int usedBytes = (int)GUI_ALLOC_GetNumUsedBytes();
    int maxUsed   = (int)GUI_ALLOC_GetMaxUsedBytes();
    if (maxUsed > s_memMaxUsed) s_memMaxUsed = maxUsed;

    if (s_memBaselineFree == 0) s_memBaselineFree = freeBytes;

    const char* tmp = getenv("TEMP"); if (!tmp) tmp = ".";
    char path[256];
    sprintf(path, "%s\\sg1210_mem_log.txt", tmp);
    FILE* f = fopen(path, "a");
    if (f) {
        int delta = freeBytes - s_memBaselineFree;
        fprintf(f, "[%3ld] %-20s  free=%6d  used=%6d  maxUsed=%6d  delta=%+d%s\n",
                s_drawCount, label, freeBytes, usedBytes, maxUsed, delta,
                (delta != 0) ? "  << LEAK?" : "");
        fclose(f);
    }
#endif
}

//=============================================================================
// Data
//-----------------------------------------------------------------------------
typedef struct tagFormState
{
    uint32_t uNextTick;
    int      curImage;       // current index into kImageList
    int      drawX, drawY;   // computed draw position (centered)
} TFormState;

static TFormState m_FormState;

//=============================================================================
//
//-----------------------------------------------------------------------------
static void _DrawCurrentImage()
{
    const auto& img = kImageList[m_FormState.curImage];

    // Clear to black background
    GUI_SetBkColor(GUI_BLACK);
    GUI_Clear();

    // Compute centered position
    int x = (320 - img.w) / 2;
    int y = (240 - img.h) / 2;

    // Save baseline before draw
    int freeBefore = (int)GUI_ALLOC_GetNumFreeBytes();
    s_memBaselineFree = freeBefore;

    // Draw the CSG image
    GUI_DrawPicture(&picMAImageRescsg, x, y, img.picIndex);

    // ---- Debug overlay ----
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);

    GUI_SetFont(GUI_FONT_AA4_ASCII16B);
    char buf[64];
    sprintf(buf, "[%d/%d] %s", m_FormState.curImage + 1, kImageCount, img.name);
    GUI_DispStringAt(buf, 4, 2);

    sprintf(buf, "%dx%d  %s", img.w, img.h, img.casName);
    GUI_DispStringAt(buf, 4, 18);

    int freeAfter = (int)GUI_ALLOC_GetNumFreeBytes();
    int usedBytes = (int)GUI_ALLOC_GetNumUsedBytes();
    sprintf(buf, "free:%d used:%d", freeAfter, usedBytes);
    GUI_DispStringAt(buf, 4, 34);

    if (freeAfter != freeBefore) {
        GUI_SetColor(GUI_RED);
        sprintf(buf, "LEAK delta=%+d", freeAfter - freeBefore);
        GUI_DispStringAt(buf, 4, 50);
    }

    GUI_SetFont(&GUI_Font6x8);
    GUI_SetColor(GUI_GRAY);
    GUI_DispStringAt("ENTER=Main  ESC=Quit  SPACE=Next", 4, 226);

    GUI_SetFont(GUI_FONT_AA4_ASCII16B);
    GUI_SetColor(0x00b5f2);
    GUI_DispStringAt("SG1210 CSG Decoder Test", 140, 226);

    // Log memory (increment counter first so baseline logic works)
    ++s_drawCount;

    char label[32];
    sprintf(label, "draw #%d %s", img.picIndex, img.name);
    MemLog(label);

    m_FormState.drawX = x;
    m_FormState.drawY = y;
}

//-----------------------------------------------------------------------------
static void _Init(const void* argument)
{
    memset(&m_FormState, 0, sizeof(m_FormState));
#ifdef __vmSIMULATOR__
    // Clear log file
    const char* tmp = getenv("TEMP"); if (!tmp) tmp = ".";
    char path[256];
    sprintf(path, "%s\\sg1210_mem_log.txt", tmp);
    FILE* f = fopen(path, "w");
    if (f) { fprintf(f, "=== SG1210 CSG Decoder Test ===\n\n"); fclose(f); }
#endif
}
//-----------------------------------------------------------------------------
static void _Show(const void* argument)
{
    MemLog("_Show entry");
    m_FormState.curImage = 0;
    _DrawCurrentImage();
    MemLog("_Show exit");
}
//-----------------------------------------------------------------------------
static void _Close(const void* argument)
{
    MemLog("_Close");
}
//-----------------------------------------------------------------------------
static void _OnTick(uint32_t uTick)
{
    (void)uTick;  // gform::Tick() sends Data.v=0, use GUI_GetTime() instead
    uint32_t now = GUI_GetTime();
    if (now > m_FormState.uNextTick) {
        m_FormState.curImage = (m_FormState.curImage + 1) % kImageCount;
        _DrawCurrentImage();
        m_FormState.uNextTick = now + 1500;
    }
}
//-----------------------------------------------------------------------------
static void _OnKeyUp(uint16_t uwKey)
{
    if (uwKey == KEY_ENTER) {
        MemLog("KEY_ENTER -> MainForm");
        gform::ReplaceForm(WID_MainForm, nullptr);
    } else if (uwKey == KEY_ESCAPE) {
        MemLog("KEY_ESCAPE -> MainForm");
        gform::ReplaceForm(WID_MainForm, nullptr);
    } else if (uwKey == ' ') {  // SPACE — manual next
        m_FormState.curImage = (m_FormState.curImage + 1) % kImageCount;
        _DrawCurrentImage();
        m_FormState.uNextTick = GUI_GetTime() + 5000;  // pause auto-cycle
    } else if (uwKey == KEY_LEFT) {
        if (m_FormState.curImage > 0) {
            m_FormState.curImage--;
            _DrawCurrentImage();
        }
    } else if (uwKey == KEY_RIGHT) {
        m_FormState.curImage = (m_FormState.curImage + 1) % kImageCount;
        _DrawCurrentImage();
    }
}
//-----------------------------------------------------------------------------
static void _OnMessage(GM_MESSAGE* pMsg)
{
    if (nullptr == pMsg) return;

    switch (pMsg->MsgId) {
    case GM_TIMER_TICK:
        _OnTick(pMsg->Data.v);
        break;
    case GM_KEYUP:
        if (pMsg->Param) {
            _OnKeyUp(pMsg->Param);
            pMsg->MsgId = 0;
        }
        break;
#if GUI_SUPPORT_TOUCH
    case GM_TOUCH:
        if (pMsg->Param == TOUCH_UP) {
            // Touch anywhere → next image
            m_FormState.curImage = (m_FormState.curImage + 1) % kImageCount;
            _DrawCurrentImage();
            m_FormState.uNextTick = GUI_GetTime() + 5000;
        }
        pMsg->MsgId = 0;
        break;
#endif
    }
}
//=============================================================================
// Form
//-----------------------------------------------------------------------------
const GWinForm FSplashForm =
{
    _Init,
    _Show,
    _Close,
    _OnMessage
};

// Auto-register with new GForm system
static const gform::FormRegistrar kRegSplash(WID_SplashForm, &FSplashForm, "Splash");
//-----------------------------------------------------------------------------

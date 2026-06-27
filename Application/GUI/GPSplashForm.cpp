//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
 File        : GPSplashForm.cpp
 Version     : V1.15
 By          : Wey. Silver Grid

 Description : Splash form — CSG decoder test rig.
               Cycles 14 images (11 atlas + 3 320x240 backgrounds) × 6 sat levels.
               B key toggles bgOnly.  Logs memory + decode/draw elapsed time (us).

 Date        : 2026.06.26 (V1.15 — added microsecond timing to perf log)
              2026.06.26 (V1.13 — saturation test: 100/80/60/40/20/10% cycle)
              2026.06.26 (V1.12 — ImageRes cycle test + memory leak detection)
              2023.12.05 (V1.10 — original implementation)
*/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
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
#include "CSGDraw.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "GFormRegistrar.h"
#include <GUI_Type.h>

#ifdef __vmSIMULATOR__
#include <windows.h>
#endif

//=============================================================================
// Image metadata for the cycle display
//-----------------------------------------------------------------------------
static const struct {
    const TGUIPicture* pPic;       // CSG picture pointer (atlas or standalone)
    int         picIndex;          // sub-picture index within atlas, or 0 for standalone
    const char* name;
    int         w, h;
    const char* casName;
    bool        skipSat;           // true = no saturation test (always 100%)
} kImageList[] = {
    // Atlas sub-pictures (picMAImageRescsg)
    {&picMAImageRescsg, picIdxMA_Logo78x18,       "Logo78x18",      78, 19, "MiniLZ77", false},
    {&picMAImageRescsg, picIdxMA_ACPow32x59Cyan,  "ACPow32x59Cyan", 32, 59, "MiniLZ77", false},
    {&picMAImageRescsg, picIdxMA_Ctrl78x61Cyan,   "Ctrl78x61Cyan",  78, 61, "DEFLATE",  false},
    {&picMAImageRescsg, picIdxMA_Brkr56x60Cyan,   "Brkr59x60Cyan",  59, 60, "DEFLATE",  false},
    {&picMAImageRescsg, picIdxMA_Battey44x24C1,   "Battey44x24C1",  44, 24, "MiniLZ77", false},
    {&picMAImageRescsg, picIdxMA_Battey44x24C2,   "Battey44x24C2",  44, 24, "MiniLZ77", false},
    {&picMAImageRescsg, picIdxMA_Battey44x24C3,   "Battey44x24C3",  44, 24, "MiniLZ77", false},
    {&picMAImageRescsg, picIdxMA_Battey44x24C4,   "Battey44x24C4",  44, 24, "MiniLZ77", false},
    {&picMAImageRescsg, picIdxMA_Battey44x24C5,   "Battey44x24C5",  44, 24, "MiniLZ77", false},
    {&picMAImageRescsg, picIdxMA_Fan16x16Cyan,    "Fan16x16Cyan",   16, 16, "RLE",      false},
    {&picMAImageRescsg, picIdxMA_Fire16x16,       "Fire16x16",      16, 16, "RLE",      false},
    // Standalone background images (no saturation test)
    {&picbkg320x240Rcsg,  0, "bkg320x240R", 320, 240, "RLE",     true},
    { &picbkg320x240Lcsg, 0, "bkg320x240L", 320, 240, "MiniLZ77", true },
};
static constexpr int kBgFirstIdx = 11;  // first background index
static constexpr int kBgLastIdx  = 12;  // last background index
static constexpr int kImageCount = sizeof(kImageList) / sizeof(kImageList[0]);

// Saturation test levels (percent)
static constexpr int kSatLevels[] = {100, 80, 60, 40, 20, 10};
static constexpr int kSatCount   = sizeof(kSatLevels) / sizeof(kSatLevels[0]);

//=============================================================================
// Timing helpers
//-----------------------------------------------------------------------------
#ifdef __vmSIMULATOR__
static LARGE_INTEGER s_freq;
static bool s_freqOk = false;

static long long GetTimeUs() {
    if (!s_freqOk) {
        QueryPerformanceFrequency(&s_freq);
        s_freqOk = true;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (now.QuadPart * 1000000LL) / s_freq.QuadPart;
}
#else
static long long GetTimeUs() {
    return static_cast<long long>(GUI_GetTime()) * 1000LL;  // ms → us (coarse)
}
#endif

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
    int      satIndex;       // current index into kSatLevels
    bool     bgOnly;         // true = cycle only background images (idx 11-12)
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

    // Compute draw position — backgrounds are full-screen, small images centered
    int x = (320 - img.w) / 2;
    int y = (240 - img.h) / 2;

    // Save baseline before draw
    int freeBefore = (int)GUI_ALLOC_GetNumFreeBytes();
    s_memBaselineFree = freeBefore;

    // Draw the CSG image (background or atlas sub-picture)
    int sat = img.skipSat ? 100 : kSatLevels[m_FormState.satIndex];
    long long t0 = GetTimeUs();
    CSG_DrawPicture(img.pPic, x, y, img.picIndex, sat);
    long long t1 = GetTimeUs();
    long long elapsedUs = t1 - t0;

    // ---- Debug overlay ----
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextMode(GUI_TEXTMODE_TRANS);

    GUI_SetFont(GUI_FONT_AA4_ASCII16B);
    char buf[64];
    sprintf(buf, "[%d/%d] %s%s", m_FormState.curImage + 1, kImageCount, img.name,
            m_FormState.bgOnly ? "  BG ONLY" : "");
    GUI_DispStringAt(buf, 4, 2);

    sprintf(buf, "%dx%d  %s  sat=%d%%", img.w, img.h, img.casName, sat);
    GUI_DispStringAt(buf, 4, 18);

    sprintf(buf, "%lld us", elapsedUs);
    GUI_DispStringAt(buf, 4, 34);

    int freeAfter = (int)GUI_ALLOC_GetNumFreeBytes();
    int usedBytes = (int)GUI_ALLOC_GetNumUsedBytes();
    sprintf(buf, "free:%d used:%d", freeAfter, usedBytes);
    GUI_DispStringAt(buf, 4, 50);

    if (freeAfter != freeBefore) {
        GUI_SetColor(GUI_RED);
        sprintf(buf, "LEAK delta=%+d", freeAfter - freeBefore);
        GUI_DispStringAt(buf, 4, 66);
    }

    GUI_SetFont(&GUI_Font6x8);
    GUI_SetColor(GUI_GRAY);
    GUI_DispStringAt(m_FormState.bgOnly
        ? "ENTER=Main  ESC=Quit  SPACE=Next  B=All"
        : "ENTER=Main  ESC=Quit  SPACE=Next  B=bgOnly", 4, 226);

    GUI_SetFont(GUI_FONT_AA4_ASCII16B);
    GUI_SetColor(0x00b5f2);
    GUI_DispStringAt("SG1210 CSG Decoder Test", 10, 200);

    // Log memory (increment counter first so baseline logic works)
    ++s_drawCount;

    char label[64];
    sprintf(label, "draw #%ld %s %lldus", s_drawCount, img.name, elapsedUs);
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
    m_FormState.satIndex = 0;
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
        const auto& img = kImageList[m_FormState.curImage];
        // Cycle saturation first, then image (skipSat backgrounds stay at satIndex=0)
        if (!img.skipSat && m_FormState.satIndex < kSatCount - 1) {
            m_FormState.satIndex++;
        } else {
            m_FormState.satIndex = 0;
            if (m_FormState.bgOnly) {
                m_FormState.curImage = (m_FormState.curImage >= kBgLastIdx)
                    ? kBgFirstIdx : m_FormState.curImage + 1;
            } else {
                m_FormState.curImage = (m_FormState.curImage + 1) % kImageCount;
            }
        }
        _DrawCurrentImage();
        m_FormState.uNextTick = now + 300;
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
    } else if (uwKey == 'B' || uwKey == 'b') {
        // Toggle background-only mode
        m_FormState.bgOnly = !m_FormState.bgOnly;
        if (m_FormState.bgOnly) {
            m_FormState.curImage = kBgFirstIdx;
            m_FormState.satIndex = 0;
        } else {
            m_FormState.curImage = 0;
            m_FormState.satIndex = 0;
        }
        _DrawCurrentImage();
        m_FormState.uNextTick = GUI_GetTime() + 5000;
    } else if (uwKey == ' ') {  // SPACE — advance saturation or next image
        const auto& img = kImageList[m_FormState.curImage];
        if (!img.skipSat && m_FormState.satIndex < kSatCount - 1) {
            m_FormState.satIndex++;
        } else {
            m_FormState.satIndex = 0;
            if (m_FormState.bgOnly) {
                m_FormState.curImage = (m_FormState.curImage >= kBgLastIdx)
                    ? kBgFirstIdx : m_FormState.curImage + 1;
            } else {
                m_FormState.curImage = (m_FormState.curImage + 1) % kImageCount;
            }
        }
        _DrawCurrentImage();
        m_FormState.uNextTick = GUI_GetTime() + 5000;  // pause auto-cycle
    } else if (uwKey == KEY_LEFT) {
        if (m_FormState.curImage > 0) {
            m_FormState.curImage--;
            m_FormState.satIndex = 0;
            _DrawCurrentImage();
        }
    } else if (uwKey == KEY_RIGHT) {
        if (m_FormState.bgOnly) {
            m_FormState.curImage = (m_FormState.curImage >= kBgLastIdx)
                ? kBgFirstIdx : m_FormState.curImage + 1;
        } else {
            m_FormState.curImage = (m_FormState.curImage + 1) % kImageCount;
        }
        m_FormState.satIndex = 0;
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
            // Touch → advance saturation or next image
            const auto& img = kImageList[m_FormState.curImage];
            if (!img.skipSat && m_FormState.satIndex < kSatCount - 1) {
                m_FormState.satIndex++;
            } else {
                m_FormState.satIndex = 0;
                if (m_FormState.bgOnly) {
                    m_FormState.curImage = (m_FormState.curImage >= kBgLastIdx)
                        ? kBgFirstIdx : m_FormState.curImage + 1;
                } else {
                    m_FormState.curImage = (m_FormState.curImage + 1) % kImageCount;
                }
            }
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

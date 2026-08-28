/*
 * CrashSmokeScaler
 * -----------------
 * A minimal X-Plane 12 plugin that:
 *   1. Registers a writable float dataref, "crashsmokescaler/smoke_scale",
 *      which you wire into your crash-effect .pss particle keyframes as a
 *      multiplier on Scale / Rate / Alpha.
 *   2. Shows a small floating window with a slider bound to that dataref,
 *      so you can drag it live, in real time, while the sim is running.
 *   3. Listens for XPLM_MSG_PLANE_CRASHED and automatically pops the
 *      window open when the user's plane crashes, so the slider is right
 *      there when the crash smoke effect fires.
 *
 * Build: see CMakeLists.txt in this folder. Requires the official
 * X-Plane SDK (headers + stub libs), downloaded separately from
 * https://developer.x-plane.com/sdk/plugin-sdk-downloads/
 *
 * Once built, the resulting binary goes in:
 *   <X-Plane>/Resources/plugins/CrashSmokeScaler/<platform>/<binary>
 * exactly as laid out by the CMake install step (see README.txt).
 */

/* XPLM400/303/301/300/210/200 are already defined via CMakeLists.txt's
 * target_compile_definitions, so they are NOT redefined here (doing so in
 * both places just produces C4005 macro-redefinition warnings). */

#include <string.h>
#include <stdio.h>

#include "XPLMDataAccess.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMMenus.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"

/* ---- Tunable range for the slider ---- */
#define SMOKE_SCALE_MIN 0.1f
#define SMOKE_SCALE_MAX 4.0f
#define SMOKE_SCALE_DEFAULT 1.0f

/* Internal resolution used by the XPWidgets scroll bar, which only understands
 * integers. We map SMOKE_SCALE_MIN..MAX onto 0..SLIDER_STEPS and convert. */
#define SLIDER_STEPS 390 /* gives ~0.01 resolution over the 0.1-4.0 range */

/* ---- Globals ---- */
static float          gSmokeScale = SMOKE_SCALE_DEFAULT;
static XPWidgetID     gWindow      = NULL;
static XPWidgetID     gSlider      = NULL;
static XPWidgetID     gCaption     = NULL;
static XPWidgetID     gValueLabel  = NULL;
static char           gValueText[32];

/* ---------------------------------------------------------------------
 * Dataref accessors for "crashsmokescaler/smoke_scale"
 * --------------------------------------------------------------------- */
static float GetSmokeScale(void *inRefcon)
{
    (void)inRefcon;
    return gSmokeScale;
}

static void SetSmokeScale(void *inRefcon, float inValue)
{
    (void)inRefcon;
    if (inValue < SMOKE_SCALE_MIN) inValue = SMOKE_SCALE_MIN;
    if (inValue > SMOKE_SCALE_MAX) inValue = SMOKE_SCALE_MAX;
    gSmokeScale = inValue;
}

/* ---------------------------------------------------------------------
 * Helpers to keep the widget slider and gSmokeScale in sync
 * --------------------------------------------------------------------- */
static int ScaleToSliderPos(float scale)
{
    float t = (scale - SMOKE_SCALE_MIN) / (SMOKE_SCALE_MAX - SMOKE_SCALE_MIN);
    return (int)(t * SLIDER_STEPS + 0.5f);
}

static float SliderPosToScale(int pos)
{
    float t = (float)pos / (float)SLIDER_STEPS;
    return SMOKE_SCALE_MIN + t * (SMOKE_SCALE_MAX - SMOKE_SCALE_MIN);
}

static void RefreshValueLabel(void)
{
    snprintf(gValueText, sizeof(gValueText), "Smoke size: %.2fx", gSmokeScale);
    if (gValueLabel)
        XPSetWidgetDescriptor(gValueLabel, gValueText);
}

/* ---------------------------------------------------------------------
 * Widget message handler (slider drag + close box)
 * --------------------------------------------------------------------- */
static int WindowWidgetHandler(XPWidgetMessage inMessage, XPWidgetID inWidget,
                                intptr_t inParam1, intptr_t inParam2)
{
    (void)inParam2;

    if (inMessage == xpMessage_CloseButtonPushed && inWidget == gWindow)
    {
        XPHideWidget(gWindow);
        return 1;
    }

    if (inMessage == xpMsg_ScrollBarSliderPositionChanged && inWidget == gSlider)
    {
        int pos = (int)XPGetWidgetProperty(gSlider, xpProperty_ScrollBarSliderPosition, NULL);
        gSmokeScale = SliderPosToScale(pos);
        RefreshValueLabel();
        return 1;
    }

    (void)inParam1;
    return 0;
}

/* ---------------------------------------------------------------------
 * Build the floating window + widgets once, at startup
 * --------------------------------------------------------------------- */
static void CreateUI(void)
{
    int left = 80, top = 600, right = 360, bottom = 500;

    gWindow = XPCreateWidget(left, top, right, bottom, 0, "Crash Smoke Scaler",
                              1, NULL, xpWidgetClass_MainWindow);
    XPSetWidgetProperty(gWindow, xpProperty_MainWindowHasCloseBoxes, 1);
    XPSetWidgetProperty(gWindow, xpProperty_MainWindowType, xpMainWindowStyle_Translucent);

    gCaption = XPCreateWidget(left + 10, top - 30, right - 10, top - 50, 1,
                               "Crash smoke particle size", 0, gWindow, xpWidgetClass_Caption);

    gSlider = XPCreateWidget(left + 10, top - 60, right - 10, top - 80, 1,
                              "", 0, gWindow, xpWidgetClass_ScrollBar);
    XPSetWidgetProperty(gSlider, xpProperty_ScrollBarType, xpScrollBarTypeSlider);
    XPSetWidgetProperty(gSlider, xpProperty_ScrollBarMin, 0);
    XPSetWidgetProperty(gSlider, xpProperty_ScrollBarMax, SLIDER_STEPS);
    XPSetWidgetProperty(gSlider, xpProperty_ScrollBarPageAmount, 1);
    XPSetWidgetProperty(gSlider, xpProperty_ScrollBarSliderPosition,
                         ScaleToSliderPos(gSmokeScale));

    RefreshValueLabel();
    gValueLabel = XPCreateWidget(left + 10, top - 90, right - 10, top - 110, 1,
                                  gValueText, 0, gWindow, xpWidgetClass_Caption);

    XPAddWidgetCallback(gWindow, WindowWidgetHandler);
    XPHideWidget(gWindow); /* stays hidden until a crash, or the menu item */
}

/* ---------------------------------------------------------------------
 * Plugin menu: lets you open the slider manually too, not just on crash
 * --------------------------------------------------------------------- */
static void MenuHandler(void *inMenuRef, void *inItemRef)
{
    (void)inMenuRef;
    (void)inItemRef;
    if (gWindow)
        XPShowWidget(gWindow);
}

/* ---------------------------------------------------------------------
 * Standard XPLM plugin entry points
 * --------------------------------------------------------------------- */
PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
    strcpy(outName, "Crash Smoke Scaler");
    strcpy(outSig, "yourname.crashsmokescaler");
    strcpy(outDesc, "Live slider to scale the crash smoke particle size.");

    XPLMRegisterDataAccessor(
        "crashsmokescaler/smoke_scale",
        xplmType_Float,
        1,                            /* writable */
        NULL, NULL,                   /* int accessors        (unused) */
        GetSmokeScale, SetSmokeScale, /* float accessors      (used)   */
        NULL, NULL,                   /* double accessors     (unused) */
        NULL, NULL,                   /* int-array accessors  (unused) */
        NULL, NULL,                   /* float-array accessors(unused) */
        NULL, NULL,                   /* data(byte) accessors (unused) */
        NULL, NULL);                  /* read/write refcons   (unused) */

    CreateUI();

    int menuIdx = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "Crash Smoke Scaler", NULL, 1);
    XPLMMenuID menu = XPLMCreateMenu("Crash Smoke Scaler", XPLMFindPluginsMenu(),
                                      menuIdx, MenuHandler, NULL);
    XPLMAppendMenuItem(menu, "Show slider", NULL, 1);

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (gWindow)
    {
        XPDestroyWidget(gWindow, 1);
        gWindow = NULL;
    }
}

PLUGIN_API int XPluginEnable(void)
{
    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void *inParam)
{
    (void)inFrom;
    (void)inParam;

    if (inMsg == XPLM_MSG_PLANE_CRASHED)
    {
        if (gWindow)
            XPShowWidget(gWindow);
    }
}

/*
 * CrashSmokeScaler
 * -----------------
 * X-Plane 12 plugin that exposes a writable smoke-scale dataref and a
 * small slider window for changing it live.
 */

#include <string.h>
#include <stdio.h>

#include "XPLMDataAccess.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include "XPLMMenus.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"

#define SMOKE_SCALE_MIN 0.1f
#define SMOKE_SCALE_MAX 4.0f
#define SMOKE_SCALE_DEFAULT 1.0f
#define SLIDER_STEPS 390

static float       gSmokeScale = SMOKE_SCALE_DEFAULT;
static XPLMDataRef gSmokeScaleRef = NULL;
static XPWidgetID  gWindow = NULL;
static XPWidgetID  gSlider = NULL;
static XPWidgetID  gCaption = NULL;
static XPWidgetID  gValueLabel = NULL;
static char        gValueText[32];

static float GetSmokeScale(void *inRefcon)
{
    (void)inRefcon;
    return gSmokeScale;
}

static void SetSmokeScale(void *inRefcon, float inValue)
{
    (void)inRefcon;

    if (inValue < SMOKE_SCALE_MIN)
        inValue = SMOKE_SCALE_MIN;
    if (inValue > SMOKE_SCALE_MAX)
        inValue = SMOKE_SCALE_MAX;

    gSmokeScale = inValue;
}

static int ScaleToSliderPos(float scale)
{
    float t = (scale - SMOKE_SCALE_MIN) /
              (SMOKE_SCALE_MAX - SMOKE_SCALE_MIN);

    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;

    return (int)(t * SLIDER_STEPS + 0.5f);
}

static float SliderPosToScale(int pos)
{
    if (pos < 0)
        pos = 0;
    if (pos > SLIDER_STEPS)
        pos = SLIDER_STEPS;

    return SMOKE_SCALE_MIN +
           ((float)pos / (float)SLIDER_STEPS) *
           (SMOKE_SCALE_MAX - SMOKE_SCALE_MIN);
}

static void RefreshValueLabel(void)
{
    snprintf(gValueText, sizeof(gValueText),
             "Smoke size: %.2fx", gSmokeScale);

    if (gValueLabel)
        XPSetWidgetDescriptor(gValueLabel, gValueText);
}

static int WindowWidgetHandler(XPWidgetMessage inMessage,
                               XPWidgetID inWidget,
                               intptr_t inParam1,
                               intptr_t inParam2)
{
    (void)inParam1;
    (void)inParam2;

    if (inMessage == xpMessage_CloseButtonPushed && inWidget == gWindow)
    {
        XPHideWidget(gWindow);
        return 1;
    }

    if (inMessage == xpMsg_ScrollBarSliderPositionChanged &&
        inWidget == gSlider)
    {
        int pos = (int)XPGetWidgetProperty(
            gSlider,
            xpProperty_ScrollBarSliderPosition,
            NULL);

        SetSmokeScale(NULL, SliderPosToScale(pos));
        RefreshValueLabel();
        return 1;
    }

    return 0;
}

static void CreateUI(void)
{
    int left = 80;
    int top = 600;
    int right = 360;
    int bottom = 500;

    gWindow = XPCreateWidget(
        left, top, right, bottom,
        0,
        "Crash Smoke Scaler",
        1,
        NULL,
        xpWidgetClass_MainWindow);

    XPSetWidgetProperty(
        gWindow,
        xpProperty_MainWindowHasCloseBoxes,
        1);

    XPSetWidgetProperty(
        gWindow,
        xpProperty_MainWindowType,
        xpMainWindowStyle_Translucent);

    gCaption = XPCreateWidget(
        left + 10, top - 30,
        right - 10, top - 50,
        1,
        "Crash smoke particle size",
        0,
        gWindow,
        xpWidgetClass_Caption);

    gSlider = XPCreateWidget(
        left + 10, top - 60,
        right - 10, top - 80,
        1,
        "",
        0,
        gWindow,
        xpWidgetClass_ScrollBar);

    XPSetWidgetProperty(
        gSlider,
        xpProperty_ScrollBarType,
        xpScrollBarTypeSlider);

    XPSetWidgetProperty(
        gSlider,
        xpProperty_ScrollBarMin,
        0);

    XPSetWidgetProperty(
        gSlider,
        xpProperty_ScrollBarMax,
        SLIDER_STEPS);

    XPSetWidgetProperty(
        gSlider,
        xpProperty_ScrollBarPageAmount,
        1);

    XPSetWidgetProperty(
        gSlider,
        xpProperty_ScrollBarSliderPosition,
        ScaleToSliderPos(gSmokeScale));

    RefreshValueLabel();

    gValueLabel = XPCreateWidget(
        left + 10, top - 90,
        right - 10, top - 110,
        1,
        gValueText,
        0,
        gWindow,
        xpWidgetClass_Caption);

    XPAddWidgetCallback(gWindow, WindowWidgetHandler);
    XPHideWidget(gWindow);
}

static void MenuHandler(void *inMenuRef, void *inItemRef)
{
    (void)inMenuRef;
    (void)inItemRef;

    if (gWindow)
        XPShowWidget(gWindow);
}

PLUGIN_API int XPluginStart(char *outName,
                            char *outSig,
                            char *outDesc)
{
    strcpy(outName, "Crash Smoke Scaler");
    strcpy(outSig, "yourname.crashsmokescaler");
    strcpy(outDesc,
           "Live slider to scale the crash smoke particle size.");

    /*
     * XPLMRegisterDataAccessor has 17 parameters:
     * name, type, writable, six get/set pairs, and two refcons.
     */
    gSmokeScaleRef = XPLMRegisterDataAccessor(
        "crashsmokescaler/smoke_scale",
        xplmType_Float,
        1,
        NULL, NULL,                  /* int get/set */
        GetSmokeScale, SetSmokeScale,/* float get/set */
        NULL, NULL,                  /* double get/set */
        NULL, NULL,                  /* int array get/set */
        NULL, NULL,                  /* float array get/set */
        NULL, NULL,                  /* data get/set */
        NULL, NULL);                 /* read/write refcons */

    if (!gSmokeScaleRef)
        return 0;

    CreateUI();

    int menuIdx = XPLMAppendMenuItem(
        XPLMFindPluginsMenu(),
        "Crash Smoke Scaler",
        NULL,
        1);

    XPLMMenuID menu = XPLMCreateMenu(
        "Crash Smoke Scaler",
        XPLMFindPluginsMenu(),
        menuIdx,
        MenuHandler,
        NULL);

    if (menu)
    {
        XPLMAppendMenuItem(
            menu,
            "Show slider",
            NULL,
            1);
    }

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (gWindow)
    {
        XPDestroyWidget(gWindow, 1);
        gWindow = NULL;
        gSlider = NULL;
        gCaption = NULL;
        gValueLabel = NULL;
    }

    if (gSmokeScaleRef)
    {
        XPLMUnregisterDataAccessor(gSmokeScaleRef);
        gSmokeScaleRef = NULL;
    }
}

PLUGIN_API int XPluginEnable(void)
{
    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom,
                                      int inMsg,
                                      void *inParam)
{
    (void)inFrom;
    (void)inParam;

    if (inMsg == XPLM_MSG_PLANE_CRASHED)
    {
        if (gWindow)
            XPShowWidget(gWindow);
    }
}

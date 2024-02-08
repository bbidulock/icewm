#ifndef ICEWMBG_PREFS_H
#define ICEWMBG_PREFS_H

/* Synchronize with MAXWORKSPACES from wmmgr.h */
#define MAX_WORKSPACES  20
#define ICEBG_MAX_ARGS  5632

#include "yconfig.h"

XIV(bool, scaleBackground, false)
XIV(bool, centerBackground, false)
XIV(bool, multiheadBackground, false)
XIV(bool, supportSemitransparency, true)
XIV(bool, shuffleBackgroundImages, false)
XIV(int, cycleBackgroundsPeriod, 0)

void addBgImage(const char *name, const char *value, bool append);

cfoption icewmbg_prefs[] = {
    OBV("DesktopBackgroundMultihead",  &multiheadBackground,
        "Paint the background image over all multihead monitors combined"),

    OBV("DesktopBackgroundCenter",  &centerBackground,
        "Display desktop background centered and not tiled"),

    OBV("SupportSemitransparency",  &supportSemitransparency,
        "Support for semitransparent terminals like Eterm or gnome-terminal"),

    OBV("DesktopBackgroundScaled",  &scaleBackground,
        "Resize desktop background to full screen"),

    OKF("DesktopBackgroundImage",   addBgImage,
        "Desktop background image(s), comma separated"),

    OKF("DesktopBackgroundColor",   addBgImage,
        "Desktop background color(s), comma separated"),

    OKF("DesktopTransparencyImage", addBgImage,
        "Image(s) to announce for semitransparent windows"),

    OKF("DesktopTransparencyColor", addBgImage,
        "Color(s) to announce for semitransparent windows"),

    OBV("ShuffleBackgroundImages",  &shuffleBackgroundImages,
        "Choose a random selection from the list of background images"),

    OIV("CycleBackgroundsPeriod",  &cycleBackgroundsPeriod, 0, INT_MAX,
        "Seconds between cycling over all background images, default zero is off"),

#ifdef ICEWMBG
    OBV("XRRDisable",            &xrrDisable,                   nullptr),

    OIV("XineramaPrimaryScreen", &xineramaPrimaryScreen, 0, 63, nullptr),

    OSV("XRRPrimaryScreenName",  &xineramaPrimaryScreenName,    nullptr),
#endif

    OK0()
};

#endif

// vim: set sw=4 ts=4 et:

#ifndef ICEWMBG_PREFS_H
#define ICEWMBG_PREFS_H

/* Synchronize with MAXWORKSPACES from wmmgr.h */
#define MAX_WORKSPACES  20
#define ICEBG_MAX_ARGS  2000

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
        "Desktop background image(s)"),

    OKF("DesktopBackgroundColor",   addBgImage,
        "Desktop background color(s)"),

    OKF("DesktopTransparencyImage", addBgImage,
        "Image(s) to announce for semitransparent windows"),

    OKF("DesktopTransparencyColor", addBgImage,
        "Color(s) to announce for semitransparent windows"),

    OBV("ShuffleBackgroundImages",  &shuffleBackgroundImages,
        "Choose a random selection from the list of background images"),

    OIV("CycleBackgroundsPeriod",  &cycleBackgroundsPeriod, 0, INT_MAX,
        "Seconds between cycling over all background images, default zero is off"),

    OK0()
};

#endif

// vim: set sw=4 ts=4 et:

#ifndef ICEWMBG_PREFS_H
#define ICEWMBG_PREFS_H

/* Synchronize with MAXWORKSPACES from wmmgr.h */
#define MAX_WORKSPACES  20

#include "yconfig.h"

XSV(const char *, desktopBackgroundColorstr, "rgb:00/20/40")
XSV(const char *, desktopBackgroundFilename, 0)
XSV(const char *, desktopTransparencyColorstr, 0)
XSV(const char *, desktopTransparencyFilename, 0)
XIV(bool, scaleBackground, false)
XIV(bool, centerBackground, false)
XIV(bool, multiheadBackground, false)
XIV(bool, supportSemitransparency, true)

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

    OK0()
};

#endif

// vim: set sw=4 ts=4 et:

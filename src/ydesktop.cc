/*
 *  IceWM - Implementation of a parser for GNOME/KDE desktop entry files
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  The specification for desktop entry files is available from:
 *      http://www.freedesktop.org/standards/desktop-entry-spec.html
 */

#include "config.h"
#include "base.h"
#include "ydesktop.h"
#include "ylocale.h"

#include <string.h>

void YAbstractDesktopParser::parseStream() {
    resetParser();
    skipWhitespace();

    while (good()) {
        char token[64];

        if (getSectionTag(token, sizeof(token))) {
            if (good()) beginSection(token);

            skipLine();
        } else if (*getIdentifier(token, sizeof(token), true)) {
            char locale[16];
            getSectionTag(locale, sizeof(locale));
            skipWhitespace();

            if (good() && '=' == currChar()) {
                nextChar(); skipWhitespace();

                char value[200];
                getLine(value, sizeof(value));

                if (good())
                    setValue(token, locale, value);
            } else {
                reportSeparatorExpected();
            }
        } else {
            reportInvalidToken();
            skipLine();
        }

        skipWhitespace();
    }
}

YDesktopParser::YDesktopParser():
    fInDesktopSection(false),
    fType(NULL), fExec(NULL),
    fName(NULL), fNameLocale(NULL), fNameQuality(-1),
    fIcon(NULL), fIconLocale(NULL), fIconQuality(-1) {
}

YDesktopParser::~YDesktopParser() {
    resetParser();
}

void YDesktopParser::resetParser() {
    delete[] fType; fType = NULL;
    delete[] fExec; fExec = NULL;
    delete[] fName; fName = NULL;
    delete[] fIcon; fIcon = NULL;
    delete[] fNameLocale; fNameLocale = NULL;
    delete[] fIconLocale; fIconLocale = NULL;
    fNameQuality = -1; fIconQuality = -1;
}

void YDesktopParser::beginSection(const char *name) {
    fInDesktopSection = !(strcasecmp(name, "Desktop Entry") &&
                          strcasecmp(name, "KDE Desktop Entry"));
}

void YDesktopParser::setValue(const char *key, const char *locale,
                              const char *value) {
    if (fInDesktopSection) {
        if (!strcasecmp(key, "Type") && '\0' == *locale) {
            delete[] fType;
            fType = newstr(value);
        } else if (!strcasecmp(key, "Exec") && '\0' == *locale) {
            delete[] fExec;
            fExec = newstr(value);
        } else if (!strcasecmp(key, "Name")) {
            int quality = YLocale::getRating(locale);
            if (quality > fNameQuality) {
                delete[] fName;
                fName = newstr(value);
                fNameQuality = quality;
            }
        } else if (!strcasecmp(key, "Icon")) {
            int quality = YLocale::getRating(locale);
            
            if (quality > fIconQuality) {
                delete[] fIcon;
                fIcon = newstr(value);
                fIconQuality = quality;
            }
        }
    }
}

/// TODO #warning /usr/share/applnk
/// TODO #warning /etc/X11/applnk


/*
 *  IceWM - Definition of a parser for GNOME/KDE desktop entry files
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  The specification for desktop entry files is available from:
 *  http://www.freedesktop.org/standards/desktop-entry-spec.html
 */

#ifndef __YDESKTOP_PARSER_H
#define __YDESKTOP_PARSER_H

#include "yparser.h"

/*******************************************************************************
 * A .desktop parser
 ******************************************************************************/

class YAbstractDesktopParser: public YParser {
protected:
    virtual void beginSection(const char *name) = 0;
    virtual void setValue(const char *key, const char *locale,
                          const char *value) = 0;

    virtual void parseStream();
    virtual void resetParser() = 0;
};

class YDesktopParser: public YAbstractDesktopParser {
public:
    YDesktopParser();
    virtual ~YDesktopParser();

    const char *getExec() const { return fExec; }
    const char *getType() const { return fType; }
    const char *getName() const { return fName; }
    const char *getIcon() const { return fIcon; }

protected:
    virtual void beginSection(const char *name);
    virtual void setValue(const char *key, const char *locale,
                          const char *value);

    virtual void resetParser();

private:
    bool fInDesktopSection;

    char *fType, *fExec;

    char *fName, *fNameLocale;
    int fNameQuality;

    char *fIcon, *fIconLocale;
    int fIconQuality;
};

#endif

/*
 *  IceWM - Definition of a parser for IceWM's menu files
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YMENU_PARSER_H
#define __YMENU_PARSER_H

#include "yparser.h"

class YIcon;
class YAction;

/*******************************************************************************
 * A .menu file parser
 ******************************************************************************/

class YAbstractMenuParser: public YParser {
protected:
    virtual void createSeparator(void) = 0;
    virtual void createProgram(const char *name, YIcon *icon,
                               const char *wmclass, const char *command,
                               bool restart) = 0;
    virtual void createMenu(const char *name, YIcon *icon) = 0;
    virtual void createAction(const char *name, YIcon *icon,
                              YAction *action) = 0;
    virtual void createInclusion(const char *path, const char *args) = 0;


    virtual void parseStream();
    const char *parseProgram(const char *type);
    const char *parseMenu();
    const char *parseSeparator();
    const char *parseInclusion();
    const char *parseAction();

private:
    int menuLevel;
};

class YMenuParser: public YAbstractMenuParser {
public:
    YMenuParser();
    virtual ~YMenuParser();

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
    bool fInMenuSection;

    char *fType, *fExec;

    char *fName, *fNameLocale;
    int fNameQuality;

    char *fIcon, *fIconLocale;
    int fIconQuality;
};

#endif

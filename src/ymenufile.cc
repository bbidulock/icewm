/*
 *  IceWM - Implementation of a parser for IceWM's menu files
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#error obsolete

#if 0

#include "config.h"
#include "base.h"
#include "intl.h"
#include "ymenufile.h"
#include "ypaint.h"

#include <string.h>
#include <limits.h>

void YAbstractMenuParser::parseStream() {
    skipWhitespace();

    while (good()) {
        char token[32];

        getIdentifier(token, sizeof(token));

        if (good()) {
            const char *parseError = 0;

            if (!(strcmp(token, "prog") &&
                  strcmp(token, "program") &&
                  strcmp(token, "restart") &&
                  strcmp(token, "runonce"))) {
                parseError = parseProgram(token);
            } else if (!strcmp(token, "menu")) {
                parseError = parseMenu();
            } else if (!strcmp(token, "menufile")) {
/// TOOD #warning "fix or remove"
                ///parseError = parseMenuFile();
            } else if (!strcmp(token, "menuprog")) {
/// TODO #warning "fix or remove"
                ///parseError = parseMenuProg();
            } else if (!strcmp(token, "separator")) {
                parseError = parseSeparator();
            } else if (!strcmp(token, "include")) {
                parseError = parseInclusion();
            } else if (!strcmp(token, "action")) {
                parseError = parseAction();
            } else {
                reportUnexpectedIdentifier(token);
                skipLine();
            }

            if (parseError) {
                reportParseError(parseError);
                skipLine();
            }

            skipWhitespace();
        } else {
            reportIdentifierExpected();
        }
    }
}

const char *YAbstractMenuParser::parseProgram(const char *type) {
    char caption[96], iconname[PATH_MAX];
    char wmclass[160], command[PATH_MAX];

    getString(caption, sizeof(caption));
    if (!good()) return _("program label expected");

    getString(iconname, sizeof(iconname));
    if (!good()) return _("icon name expected");

    if ('u' == type[1]) { // "runonce"
        getString(wmclass, sizeof(wmclass));
        return _("window management class expected");
    }

/// TODO #warning read cmdline (command + argv)!!!

    YIcon *icon = 0;
#ifndef LITE
    if ('-' != *iconname) icon = YIcon::getIcon(iconname);
#endif
    const bool restart('e' == type[1]);

    createProgram(caption, icon, *wmclass ? wmclass : 0, command, restart);
    return 0;
}

const char *YAbstractMenuParser::parseMenu() {
    char caption[96], iconname[PATH_MAX];

    getString(caption, sizeof(caption));
    if (!good()) return _("menu caption expected");
            
    getString(iconname, sizeof(iconname));
    if (!good()) return _("icon name expected");

    skipWhitespace();

    if (!good() || '=' != currChar()) 
        return _("opening curly expected");

    YIcon *icon = 0;
#ifndef LITE
    if ('-' != *iconname) icon = YIcon::getIcon(iconname);
#endif

    createMenu(caption, icon);
    return 0;
}

const char *YAbstractMenuParser::parseSeparator() {
    skipLine();
    createSeparator();
    return 0;
}

const char *YAbstractMenuParser::parseInclusion() {
/// TODO #warning read cmdline (command + argv)!!!
/// TODO #warning support static files, scripts, .desktop dirs, theme dirs,
/// TODO #warning try gnome-vfs first (if supported)
    createInclusion(0, 0);
    return 0;
}

const char *YAbstractMenuParser::parseAction() {
    char caption[96], iconname[PATH_MAX], actionname[64];

    getString(caption, sizeof(caption));
    if (!good()) return _("menu caption expected");
            
    getString(iconname, sizeof(iconname));
    if (!good()) return _("icon name expected");

    getString(actionname, sizeof(actionname));
    if (!good()) return _("action name expected");

    YAction *action = 0;
    if (!action) return _("unknown action");

    YIcon *icon = 0;
#ifndef LITE
    if ('-' != *iconname) icon = YIcon::getIcon(iconname);
#endif

/// TODO #warning create array of actions!!!
    createAction(caption, icon, action);
    return 0;
}

#endif

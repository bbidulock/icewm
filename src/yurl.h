/*
 *  IceWM - Definition of an URL decoder
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YURL_H
#define __YURL_H

#include "mstring.h"

/*******************************************************************************
 * An URL decoder
 ******************************************************************************/

class YURL {
public:
    YURL();
    YURL(mstring url);

    void operator=(mstring url);
    static mstring unescape(mstring str);

    mstring scheme;
    mstring user;
    mstring pass;
    mstring host;
    mstring port;
    mstring path;
};

#endif

// vim: set sw=4 ts=4 et:

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
    YURL(ustring url);

    void operator=(ustring url);
    static ustring unescape(ustring str);

    cstring scheme;
    cstring user;
    cstring pass;
    cstring host;
    cstring port;
    cstring path;
};

#endif

// vim: set sw=4 ts=4 et:

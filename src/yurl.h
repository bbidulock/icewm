/*
 *  IceWM - Definition of an URL decoder
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YURL_H
#define __YURL_H

#include "ref.h"
#include "mstring.h"

/*******************************************************************************
 * An URL decoder
 ******************************************************************************/

class YURL: public refcounted {
public:
    YURL();
    YURL(ustring url, bool expectInetScheme = true);
    ~YURL();

#if 0
    YURL& operator= (char const * url) { assign(url); return *this; }
#endif

    ustring scheme() const { return fScheme; }
    ustring user() const { return fUser; }
    ustring password() const { return fPassword; }
    ustring host() const { return fHost; }
    ustring port() const { return fPort; }
    ustring path() const { return fPath; }

    static ustring unescape(ustring str);
private:
    ustring fScheme;
    ustring fUser;
    ustring fPassword;
    ustring fHost;
    ustring fPort;
    ustring fPath;
//    char * fScheme, * fUser, * fPassword, * fHost, *fPort, * fPath;

    void assign(ustring url, bool expectInetScheme = true);
};

#endif

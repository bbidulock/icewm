/*
 *  IceWM - Definition of an URL decoder
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YURL_H
#define __YURL_H

/*******************************************************************************
 * An URL decoder
 ******************************************************************************/

class YURL {
public:
    YURL();
    YURL(char const * url, bool expectInetScheme = true);
    ~YURL();

    void assign(char const * url, bool expectInetScheme = true);
    YURL& operator= (char const * url) { assign(url); return *this; }

    char const * scheme() const { return fScheme; }
    char const * user() const { return fUser; }
    char const * password() const { return fPassword; }
    char const * host() const { return fHost; }
    char const * port() const { return fPort; }
    char const * path() const { return fPath; }

    static char * unescape(char * str);
    static char * unescape(char const * str);
    
private:
    char * fScheme, * fUser, * fPassword, * fHost, *fPort, * fPath;
};

#endif

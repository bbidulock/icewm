/*
 *  IceWM - Resource paths
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/03/24: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */

#include "config.h"

#include "default.h"

#include "base.h"
#include "ypaths.h"

#include "intl.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <limits.h>
#include <fcntl.h>

char * YPathElement::joinPath(char const *base, char const *name) const {
    char const *b(base ? base : "");

    if (sub)
        return strJoin(*root, rdir, *sub, "/", b, name, NULL);
    else
        return strJoin(*root, rdir, b, name, NULL);
}

YResourcePaths const & 
YResourcePaths::operator= (YResourcePaths const & other) {
    delete[] fPaths;
    
    if (other.fPaths) {
	unsigned peCount(0);
	for (YPathElement const * pe(other.fPaths); pe->root; ++pe, ++peCount);
	
	fPaths = new YPathElement[peCount];
	memcpy(fPaths, other.fPaths, peCount * sizeof(YPathElement));
    } else
        fPaths = NULL;

    return *this;
}

void YResourcePaths::init (char const * subdir, bool themeOnly) {
    delete[] fPaths;

    static char const * home(::getenv("HOME"));

	static char themeSubdir[PATH_MAX];
	static char const * themeDir(themeSubdir);

	strncpy(themeSubdir, themeName, sizeof(themeSubdir));
	themeSubdir[sizeof(themeSubdir) - 1] = '\0';
	    
	char *dirname(::strrchr(themeSubdir, '/'));
	if (dirname) *dirname = '\0';

    if (themeName && *themeName == '/') {
	if (themeOnly) {
	    static YPathElement const paths[] = {
	        { &themeDir, "/", NULL },
	        { NULL, NULL, NULL }
	    };

	    fPaths = new YPathElement[ACOUNT(paths)];
	    memcpy(fPaths, paths, sizeof(paths));
	} else {
	    static YPathElement const paths[] = {
	        { &themeDir, "/", NULL },
	        { &home, "/.icewm/", NULL },
	        { &configDir, "/", NULL },
	        { &libDir, "/", NULL },
	        { NULL, NULL, NULL }
	    };

	    fPaths = new YPathElement[ACOUNT(paths)];
	    memcpy(fPaths, paths, sizeof(paths));
	}
    } else {
	if (themeOnly) {
	    static YPathElement const paths[] = {
		{ &home, "/.icewm/themes/", &themeDir },
		{ &configDir, "/themes/", &themeDir },
		{ &libDir, "/themes/", &themeDir },
		{ NULL, NULL, NULL }
	    };

	    fPaths = new YPathElement[ACOUNT(paths)];
	    memcpy(fPaths, paths, sizeof(paths));
	} else {
	    static YPathElement const paths[] = {
		{ &home, "/.icewm/themes/", &themeDir },
		{ &home, "/.icewm/", NULL },
		{ &configDir, "/themes/", &themeDir },
		{ &configDir, "/", NULL },
		{ &libDir, "/themes/", &themeDir },
		{ &libDir, "/", NULL },
		{ NULL, NULL, NULL }
	    };
	    
	    fPaths = new YPathElement[ACOUNT(paths)];
	    memcpy(fPaths, paths, sizeof(paths));
	}
    }

    verifyPaths(subdir);
}
    
void YResourcePaths::verifyPaths(char const *base) {
    unsigned j(0), i(0);

    for (; fPaths[i].root; i++) {
        char *path(fPaths[i].joinPath(base));

        if (access(path, R_OK) == 0)
            fPaths[j++] = fPaths[i];

        delete path;
    }
    fPaths[j] = fPaths[i];
}

YPixmap * YResourcePaths::loadPixmap(const char *base, const char *name) {
    YPixmap * pixmap(NULL);

    for (YPathElement * pe(fPaths); pe->root && pixmap == NULL; pe++) {
        char * path(pe->joinPath(base, name));

        if (isreg(path) && (pixmap = new YPixmap(path)) == NULL)
	    die(1, _("Out of memory for pixmap %s"), path);

        delete[] path;
    }
#ifdef DEBUG
    if (debug)
        warn(_("Could not find pixmap %s"), name);
#endif

    return pixmap;
}

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

#include "ylib.h"
#include "default.h"
#include "ypixbuf.h"

#include "base.h"
#include "ypaths.h"
#include "yapp.h"
#include "yprefs.h"

#include "intl.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <limits.h>
#include <fcntl.h>

upath YPathElement::joinPath(upath base) const {
    upath p = fPath;
    if (base != null)
        p = p.relative(base);
    return p;
}

upath YPathElement::joinPath(upath base, upath name) const {
    upath p = fPath;
    if (base != null)
        p = p.relative(base);
    if (name != null)
        p = p.relative(name);
    return p;
}

void YResourcePaths::addDir(upath root, upath rdir, upath sub) {
    upath p = upath(root);
    if (rdir != null)
        p = p.relative(rdir);
    if (sub != null)
        p = p.relative(sub);
    if (p.dirExists())
        fPaths.append(new YPathElement(p));
}

ref<YResourcePaths> YResourcePaths::subdirs(upath subdir, bool themeOnly) {
    ref<YResourcePaths> paths;

    paths.init(new YResourcePaths());

    static char themeSubdir[PATH_MAX];
    static char const *themeDir(themeSubdir);
    static const char *homeDir(YApplication::getPrivConfDir());

    strncpy(themeSubdir, themeName, sizeof(themeSubdir));
    themeSubdir[sizeof(themeSubdir) - 1] = '\0';

    char *dirname(::strrchr(themeSubdir, '/'));
    if (dirname) *dirname = '\0';

    if (themeName && *themeName == '/') {
        MSG(("Searching `%s' resources at absolute location", cstring(subdir.path()).c_str()));

        if (themeOnly) {
            paths->addDir(themeDir, null, null);
        } else {
            paths->addDir(homeDir, null, null);
            paths->addDir(themeDir, null, null);
            paths->addDir(YApplication::getConfigDir(), null, null);
            paths->addDir(YApplication::getLibDir(), null, null);
        }
    } else {
        MSG(("Searching `%s' resources at relative locations", cstring(subdir.path()).c_str()));

        if (themeOnly) {
            paths->addDir(homeDir, "themes", themeDir);
            paths->addDir(YApplication::getConfigDir(), "themes", themeDir);
            paths->addDir(YApplication::getLibDir(), "themes", themeDir);

        } else {
            paths->addDir(homeDir, "/themes/", themeDir);
            paths->addDir(homeDir, null, null);
            paths->addDir(YApplication::getConfigDir(), "/themes/", themeDir);
            paths->addDir(YApplication::getConfigDir(), null, null);
            paths->addDir(YApplication::getLibDir(), "/themes/", themeDir);
            paths->addDir(YApplication::getLibDir(), null, null);
        }
    }

    DBG {
        MSG(("Initial search path:"));
        for (int i = 0; i < paths->getCount(); i++) {
            upath path = paths->getPath(i)->joinPath("/icons/");
            cstring cs(path.path());
            MSG(("%s", cs.c_str()));
        }
    }

    paths->verifyPaths(subdir);
    return paths;
}

void YResourcePaths::verifyPaths(upath base) {
    for (int i = 0; i < getCount(); i++) {
        upath path = getPath(i)->joinPath(base);

        if (!path.isReadable()) {
            fPaths.remove(i);
            i--;
        }
    }
}

ref<YPixmap> YResourcePaths::loadPixmap(upath base, upath name) const {
    ref<YPixmap> pixmap;

    for (int i = 0; i < getCount() && pixmap == null; i++) {
        upath path = getPath(i)->joinPath(base, name);

        if (path.fileExists()) {
            if (!path.isReadable()) {
                cstring cs(path.path());
                warn(_("Image not readable: %s"), cs.c_str());
            } else {
                pixmap = YPixmap::load(path);
                if (pixmap == null) {
                    cstring cs(path.path());
                    warn(_("Out of memory for image %s"), cs.c_str());
                }
            }
        }
    }
#ifdef DEBUG
    if (debug)
        warn(_("Could not find image %s"), cstring(name.path()).c_str());
#endif

    return pixmap;
}

ref<YImage> YResourcePaths::loadImage(upath base, upath name) const {
    ref<YImage> pixbuf;

    for (int i = 0; i < getCount() && pixbuf == null; i++) {
        upath path = getPath(i)->joinPath(base, name);

        if (path.fileExists()) {
            if (!path.isReadable()) {
                cstring cs(path.path());
                warn(_("Image not readable: %s"), cs.c_str());
            } else {
                pixbuf = YImage::load(path);
                if (pixbuf == null) {
                    warn(_("Out of memory for image: %s"), cstring(path.path()).c_str());
                }
            }
        }
    }
#ifdef DEBUG
    if (debug)
        warn(_("Could not find image: %s"), cstring(name.path()).c_str());
#endif

    return pixbuf;
}

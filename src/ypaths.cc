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

void YResourcePaths::addDir(const upath& dir) {
    if (dir.dirExists())
        fPaths.append(new upath(dir));
}

ref<YResourcePaths> YResourcePaths::subdirs(upath subdir, bool themeOnly) {
    ref<YResourcePaths> paths(new YResourcePaths());

    upath xdgDir(YApplication::getXdgConfDir());
    upath homeDir(YApplication::getPrivConfDir());
    upath themeDir(upath(themeName).parent());

    if (themeName && *themeName == '/') {
        MSG(("Searching `%s' resources at absolute location", cstring(subdir.path()).c_str()));

        if (themeOnly) {
            paths->addDir(themeDir);
        } else {
            paths->addDir(xdgDir);
            paths->addDir(homeDir);
            paths->addDir(themeDir);
            paths->addDir(YApplication::getConfigDir());
            paths->addDir(YApplication::getLibDir());
        }
    } else {
        MSG(("Searching `%s' resources at relative locations", cstring(subdir.path()).c_str()));

        upath themes("/themes/");
        upath themesPlusThemeDir(themes + themeDir);

        if (themeOnly) {
            paths->addDir(xdgDir + themesPlusThemeDir);
            paths->addDir(homeDir + themesPlusThemeDir);
            paths->addDir(YApplication::getConfigDir() + themesPlusThemeDir);
            paths->addDir(YApplication::getLibDir() + themesPlusThemeDir);

        } else {
            paths->addDir(xdgDir + themesPlusThemeDir);
            paths->addDir(xdgDir);
            paths->addDir(homeDir + themesPlusThemeDir);
            paths->addDir(homeDir);
            paths->addDir(YApplication::getConfigDir() + themesPlusThemeDir);
            paths->addDir(YApplication::getConfigDir());
            paths->addDir(YApplication::getLibDir() + themesPlusThemeDir);
            paths->addDir(YApplication::getLibDir());
        }
    }

    DBG {
        MSG(("Initial search path:"));
        for (int i = 0; i < paths->getCount(); i++) {
            upath path = paths->getPath(i) + "/icons/";
            cstring cs(path.path());
            MSG(("%s", cs.c_str()));
        }
    }

    paths->verifyPaths(subdir);
    return paths;
}

void YResourcePaths::verifyPaths(upath base) {
    for (int i = getCount(); --i >= 0; ) {
        upath path = getPath(i) + base;

        if (!path.isReadable()) {
            fPaths.remove(i);
        }
    }
}

template<class Pict>
void YResourcePaths::loadPict(const upath& baseName, ref<Pict>* pict) const {
    for (int i = 0; i < getCount(); ++i) {
        upath path = getPath(i) + baseName;

        if (path.fileExists()) {
            if (path.isReadable()) {
                if ((*pict = Pict::load(path)) != null) {
                    return;
                } else
                    warn(_("Out of memory for image %s"),
                            path.string().c_str());
            } else
                warn(_("Image not readable: %s"), path.string().c_str());
        }
    }
#ifdef DEBUG
    if (debug)
        warn(_("Could not find image %s"), baseName.string().c_str());
#endif
}


ref<YPixmap> YResourcePaths::loadPixmap(upath base, upath name) const {
    ref<YPixmap> p;
    loadPict(base + name, &p);
    return p;
}

ref<YImage> YResourcePaths::loadImage(upath base, upath name) const {
    ref<YImage> p;
    loadPict(base + name, &p);
    return p;
}

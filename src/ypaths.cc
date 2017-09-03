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
#include "ypaths.h"
#include "yapp.h"
#include "yprefs.h"
#include "intl.h"
#include <unistd.h>
#include <fcntl.h>


void YResourcePaths::addDir(const upath& dir) {
    if (dir.dirExists())
        fPaths.append(new upath(dir));
}

ref<YResourcePaths> YResourcePaths::subdirs(upath subdir, bool themeOnly) {
    ref<YResourcePaths> paths(new YResourcePaths());

    upath xdgDir(YApplication::getXdgConfDir());
    upath homeDir(YApplication::getPrivConfDir());

    upath themeFile(themeName);
    pstring themeExt(themeFile.getExtension());
    upath themeDir(themeExt.isEmpty() ? themeFile : themeFile.parent());

    if (themeDir.isAbsolute()) {
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
    for (IterType iter = reverseIterator(); ++iter; ) {
        if (iter->relative(base).isExecutable() == false) {
            iter.remove();
        }
    }
}

template<class Pict>
bool YResourcePaths::loadPictFile(const upath& file, ref<Pict>* pict) {
    if (file.isReadable()) {
        *pict = Pict::load(file);
        if (*pict != null) {
            return true;
        }
        else
            warn(_("Out of memory for image %s"),
                    file.string().c_str());
    } else
        warn(_("Image not readable: %s"), file.string().c_str());
    return false;
}

ref<YPixmap> YResourcePaths::loadPixmapFile(const upath& file) {
    ref<YPixmap> p;
    loadPictFile(file, &p);
    return p;
}

ref<YImage> YResourcePaths::loadImageFile(const upath& file) {
    ref<YImage> p;
    loadPictFile(file, &p);
    return p;
}

template<class Pict>
void YResourcePaths::loadPict(const upath& baseName, ref<Pict>* pict) const {
    for (int i = 0; i < getCount(); ++i) {
        upath path = getPath(i) + baseName;
        bool exist = path.fileExists();
        if (exist && loadPictFile(path, pict))
            return;
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

/*
 * IceWM
 *
 * Copyright (C) 1997-2000 Marko Macek
 */
#include "config.h"
#include "base.h"
#include "yapp.h"
#include "yresource.h"
#include "ycstring.h"
#include "yfilepath.h"

#include "ypaint.h"
//#include "sysdep.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

class YCachedIcon;

static YCachedIcon *firstIcon = 0;
YResourcePath *YIcon::fIconPaths = 0;

class YCachedIcon {
public:
    YCachedIcon(YIcon *icon) {
        fNext = firstIcon;
        firstIcon = this;

        fIcon = icon;
    }
    ~YCachedIcon() {
        delete fIcon;
    }

    YIcon *getIcon() { return fIcon; }
    YCachedIcon *next() { return fNext; }

private:
    YIcon *fIcon;
    YCachedIcon *fNext;
private: // not-used
    YCachedIcon(const YCachedIcon &);
    YCachedIcon &operator=(const YCachedIcon &);
};

//extern YResourcePath *iconPaths;

bool YIcon::findIcon(char *base, char **fullPath) {
#warning "cleanup icon searching -- multiple paths?"
    /// fix: do this at startup (merge w/ iconPath)
    for (int i = 0; i < fIconPaths->getCount(); i++) {
        const char *path = fIconPaths->getPath(i)->path()->c_str();

        //puts (path);
        if (__findPath(path, R_OK, base, fullPath, true)) {
            return true;
        }
    }
#if 0
    if (findPath(iconPath, R_OK, base, fullPath, true))
        return true;
#endif
    return false;
}

bool YIcon::findIcon(char **fullPath, int size) {
    char icons_size[1024];

    sprintf(icons_size, "%s_%dx%d.xpm", REDIR_ROOT(fPath), size, size);

    if (findIcon(icons_size, fullPath))
        return true;
    
    if (size == ICON_LARGE) {
        sprintf(icons_size, "%s.xpm", REDIR_ROOT(fPath));
    } else {
        char name[1024];
        char *p;

        sprintf(icons_size, "%s.xpm", REDIR_ROOT(fPath));
        p = strrchr(icons_size, '/');
        if (!p)
            p = icons_size;
        else
            p++;
        strcpy(name, p);
        sprintf(p, "mini/%s", name);
    }

    if (findIcon(icons_size, fullPath))
        return true;

#ifdef CONFIG_IMLIB
    sprintf(icons_size, "%s", REDIR_ROOT(fPath));
    if (findIcon(icons_size, fullPath))
        return true;
#endif

#ifdef DEBUG
    if (debug)
        fprintf(stderr, "Icon '%s' not found.\n", fPath);
#endif
    return false;
}

YPixmap *YIcon::loadIcon(int size) {
    YPixmap *icon = 0;

    if (icon == 0) {
#ifdef CONFIG_IMLIB
        if (fPath[0] == '/' && is_reg(fPath)) {
            icon = new YPixmap(fPath, size, size);
            if (icon == 0)
                fprintf(stderr, "Out of memory for pixmap %s", fPath);
        } else
#endif
        {
            char *fullPath;

            if (findIcon(&fullPath, size)) {
#ifdef CONFIG_IMLIB
                icon = app->loadPixmap(fullPath, size, size);
#else
                icon = app->loadPixmap(fullPath);
#endif
                if (icon == 0)
                    fprintf(stderr, "Out of memory for pixmap %s", fullPath);
                delete fullPath;
            }
        }
    }
    return icon;
}

YIcon *YIcon::getIcon(const char *name) {
    if (fIconPaths == 0)
        initIcons();

    YCachedIcon *icn = firstIcon;

    while (icn) {
        if (strcmp(name, icn->getIcon()->iconName()) == 0)
            return icn->getIcon();
        icn = icn->next();
    }
    YIcon *i = new YIcon(name);
    new YCachedIcon(i); // side-effect
    return i;
}

void YIcon::freeIcons() {
    YCachedIcon *icn, *next;

    for (icn = firstIcon; icn; icn = next) {
        next = icn->next();
        delete icn;
    }
    firstIcon = 0;
    delete fIconPaths; fIconPaths = 0;
}

void YIcon::initIcons() {

#warning "use YFilePath for resource paths"
    fIconPaths = new YResourcePath();

    if (fIconPaths) {
        CStr *p;
        char *home = getenv("HOME");

        p = CStr::join(home, "/." PNAME "/icons/", 0);
        if (p && access(p->c_str(), R_OK | X_OK) == 0)
            fIconPaths->__addPath(p->c_str());
        delete p;

        p = CStr::join(home, "/." PNAME "/theme/icons/", 0);
        if (p && access(p->c_str(), R_OK | X_OK) == 0)
            fIconPaths->__addPath(p->c_str());
        delete p;

        p = CStr::newstr("/etc/" PNAME "/icons/");
        if (p && access(p->c_str(), R_OK | X_OK) == 0)
            fIconPaths->__addPath(p->c_str());
        delete p;

        p = CStr::join(DATADIR, "/icons/", 0); // !!! ???
        if (p && access(p->c_str(), R_OK | X_OK) == 0)
            fIconPaths->__addPath(p->c_str());
        delete p;
    }
}

#include "config.h"
#include "yapp.h"
#include "yresource.h"
#include "base.h"
#include "debug.h"
#include "prefs.h"

//#include <stdlib.h>
//#include <string.h>

#include "sysdep.h"

YResourcePath::~YResourcePath() {
    for (int i = 0; i < fCount; i++)
        delete [] fPaths[i];
    delete fPaths;
}

void YResourcePath::addPath(char *path) {
    char **new_paths;

    new_paths = (char **)realloc(fPaths, sizeof(char *) * (fCount + 1));
    if (new_paths) {
        fPaths = new_paths;
        fPaths[fCount] = path; //!!!newstr(path);
        if (fPaths[fCount])
            fCount++;
    }
}

static char *joinPath(pathelem *pe, const char *base) {
    const char *b = base ? base : "";

    if (pe->sub)
        return strJoin(*pe->root, pe->rdir, *pe->sub, "/", b, NULL);
    else
        return strJoin(*pe->root, pe->rdir, b, NULL);
}


YResourcePath *YApplication::getResourcePath(const char *base) {
    //!!!
    static const char *home = 0;
    if (home == 0)
        home = getenv("HOME");
    static char themeSubdir[256];

    strcpy(themeSubdir, themeName);
    { char *p = strchr(themeSubdir, '/'); if (p) *p = 0; }
    static const char *themeDir = themeSubdir;

    pathelem paths[] = {
        { &home, "/.icewm/themes/", &themeDir },
        { &home, "/.icewm/", 0,},
        { &configDir, "/themes/", &themeDir },
        { &configDir, "/", 0 },
        { &libDir, "/themes/", &themeDir },
        { &libDir, "/", 0 },
        { 0, 0, 0 }
    };

    //verifyPaths(paths, base);

    YResourcePath *rp = new YResourcePath();

    for (int i = 0; paths[i].root != 0; i++) {
        char *p = joinPath(paths + i, base);
        //puts(p);
        if (p && access(p, R_OK | X_OK) == 0) {
            rp->addPath(p);
        }
    }
    return rp;
}

YPixmap *YApplication::loadResourcePixmap(YResourcePath *rp, const char *name) {
    char *path;

    for (int i = 0; i < rp->getCount(); i++) {
        path = strJoin(rp->getPath(i), name, NULL);

        //puts(path);
        if (path && is_reg(path)) {
            YPixmap *pixmap = app->loadPixmap(path);

            if (pixmap == 0)
                warn("failed to load pixmap %s", path);

            delete path;
            return pixmap;
        }
        delete path;
    }
    warn("could not find resource pixmap %s", name);
    return 0;
}

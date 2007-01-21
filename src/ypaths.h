/*
 *  IceWM - Definition for resource paths
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YRESOURCEPATH_H
#define __YRESOURCEPATH_H

#include <stddef.h>
#include "ypaint.h"
#include "yarray.h"
#include "upath.h"

upath findPath(ustring path, int mode, upath name,
               bool path_relative = false);

struct YPathElement {
    YPathElement(upath p): fPath(p) {}

    upath joinPath(upath base) const;
    upath joinPath(upath base, upath name) const;

    upath fPath;
};

class YResourcePaths: public refcounted {
public:

    static ref<YResourcePaths> paths();
    static ref<YResourcePaths> subdirs(upath subdir, bool themeOnly = false);

private:
    YResourcePaths() {}
    void addDir(upath root, upath rdir, upath sub);
public:
    virtual ~YResourcePaths() { }

    ref<YPixmap> loadPixmap(upath base, upath name) const;
///    ref<YPixbuf> loadPixbuf(upath base, upath name, bool const fullAlpha) const;
    ref<YImage> loadImage(upath base, upath name) const;
    ref<YIcon> loadIcon(upath base, upath name) const;

    int getCount() const { return fPaths.getCount(); }
    YPathElement *getPath(int index) const { return fPaths[index]; }
protected:
    void verifyPaths(upath base);
    
private:
    YArray<YPathElement *> fPaths;
};

#endif

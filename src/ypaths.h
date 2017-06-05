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

class YResourcePaths: public refcounted {
public:

    static ref<YResourcePaths> paths();
    static ref<YResourcePaths> subdirs(upath subdir, bool themeOnly = false);

private:
    YResourcePaths() {}
    void addDir(const upath& dir);
    
    template<class Pict>
    void loadPict(const upath& baseName, ref<Pict>* pict) const;

public:
    ref<YPixmap> loadPixmap(upath base, upath name) const;
    ref<YImage> loadImage(upath base, upath name) const;

    int getCount() const { return fPaths.getCount(); }
    const upath& getPath(int index) const { return *fPaths[index]; }
protected:
    void verifyPaths(upath base);
    
private:
    YObjectArray<upath> fPaths;
};

#endif

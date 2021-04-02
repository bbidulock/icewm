/*
 *  IceWM - Definition for resource paths
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef YRESOURCEPATH_H
#define YRESOURCEPATH_H

#include "ypaint.h"
#include "yarray.h"
#include "upath.h"

class YResourcePaths: public refcounted {
public:
    static ref<YResourcePaths> subdirs(const char* subdir = nullptr,
                                       bool themeOnly = false);

    ref<YPixmap> loadPixmap(upath base, upath name) const;
    ref<YImage> loadImage(upath base, upath name) const;

    static ref<YPixmap> loadPixmapFile(const upath& file);
    static ref<YImage> loadImageFile(const upath& file);

    upath* begin() const { return (upath *) (void *) fPaths.begin(); }
    upath* end()   const { return (upath *) (void *) fPaths.end(); }

private:
    MStringArray fPaths;

    YResourcePaths() {}
    void addDir(upath dir);
    void verifyPaths(const char* base);

    template<class Pict>
    bool loadPict(upath baseName, ref<Pict>* pict) const;
    template<class Pict>
    static bool loadPictFile(upath file, ref<Pict>* pict);
};

#endif

// vim: set sw=4 ts=4 et:

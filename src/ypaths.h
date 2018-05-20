/*
 *  IceWM - Definition for resource paths
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YRESOURCEPATH_H
#define __YRESOURCEPATH_H

#include "ypaint.h"
#include "yarray.h"
#include "upath.h"

class YResourcePaths: public refcounted {
public:
    static ref<YResourcePaths> subdirs(upath subdir, bool themeOnly = false);

    ref<YPixmap> loadPixmap(upath base, upath name) const;
    ref<YImage> loadImage(upath base, upath name) const;

    static ref<YPixmap> loadPixmapFile(const upath& file);
    static ref<YImage> loadImageFile(const upath& file);

    int getCount() const { return fPaths.getCount(); }
    const upath& getPath(int index) const { return *fPaths[index]; }

    typedef YObjectArray<upath>::IterType IterType;
    IterType iterator() { return fPaths.iterator(); }
    IterType reverseIterator() { return fPaths.reverseIterator(); }

protected:
    void verifyPaths(upath base);

private:
    YObjectArray<upath> fPaths;

    YResourcePaths() {}
    void addDir(const upath& dir);

    template<class Pict>
    bool loadPict(const upath& baseName, ref<Pict>* pict) const;
    template<class Pict>
    static bool loadPictFile(const upath& file, ref<Pict>* pict);
};

#endif

// vim: set sw=4 ts=4 et:

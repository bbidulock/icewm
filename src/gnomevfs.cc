/*
 *  IceWM - Implementation of GNOME VFS 2.0 support
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#include "config.h"
#include "base.h"
#include "gnomevfs.h"

YGnomeVFS::YGnomeVFS():
YSharedLibrary("libgnomevfs-2.so") {
    if (loaded()) {
        ASSIGN_SYMBOL(Init, gnome_vfs_init);
        ASSIGN_SYMBOL(Shutdown, gnome_vfs_init);
        ASSIGN_SYMBOL(DirectoryVisit, gnome_vfs_directory_visit);

        if (!(available() && mInit())) unload();
    }
}

YGnomeVFS::~YGnomeVFS() {
    if (mShutdown) mShutdown();
}

bool YGnomeVFS::available() const { 
    return YSharedLibrary::loaded() &&
           mInit && mShutdown && mDirectoryVisit;
}

YGnomeVFS::Result YGnomeVFS::traverse(const char *uri, Visitor *visitor, 
                                      FileInfoOptions info_options,
                                      DirectoryVisitOptions visit_options) {
    return mDirectoryVisit ? mDirectoryVisit(uri, info_options, visit_options,
                                             &YGnomeVFS::visit, visitor)
                           : ErrorNotSupported;
}

int YGnomeVFS::visit(const char *filename, FileInfo *info,
                     int recursing_will_loop, void *data, int *recurse) {
    if (data) {
        return ((Visitor *) data)->visit(filename, info, recursing_will_loop, 
                                         recurse);
    } else {
        *recurse = !recursing_will_loop;
        msg("%s", filename);
        return true;
    }
}

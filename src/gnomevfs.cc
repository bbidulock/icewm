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
msg("going down...");
    if (mShutdown) mShutdown();
msg("head crash!");
}

bool YGnomeVFS::available() const { 
    return YSharedLibrary::loaded() &&
           mInit && mShutdown && mDirectoryVisit;
}

YGnomeVFS::Result YGnomeVFS::traverse(const char *uri, Visitor *visitor, 
                                      FileInfoOptions infoOptions,
                                      DirectoryVisitOptions visitOptions) {
    return mDirectoryVisit ? mDirectoryVisit(uri, infoOptions, visitOptions,
                                             &YGnomeVFS::visit, visitor)
                           : ErrorNotSupported;
}

int YGnomeVFS::visit(const char *filename, FileInfo *info,
                     int recursingWillLoop, void *data, int *recurse) {
    if (data) {
        Visitor *visitor = (Visitor *) data;
        return visitor->visit(filename, *info, recursingWillLoop, *recurse);
    } else {
        *recurse = !recursingWillLoop;
        msg("%s", filename);
        return true;
    }
}

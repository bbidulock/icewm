/*
 *  IceWM - Definitions for GNOME VFS 2.0 support
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YGNOMEVFS_H
#define __YGNOMEVFS_H

#include "ylibrary.h"

class YGnomeVFS: protected YSharedLibrary {
public:
    enum Result {
        Ok,
        ErrorNotFound,
        ErrorGeneric,
        ErrorInternal,
        ErrorBadParameters,
        ErrorNotSupported,
        ErrorIo,
        ErrorCorruptedData,
        ErrorWrongFormat,
        ErrorBadFile,
        ErrorTooBig,
        ErrorNoSpace,
        ErrorReadOnly,
        ErrorInvalidUri,
        ErrorNotOpen,
        ErrorInvalidOpenMode,
        ErrorAccessDenied,
        ErrorTooManyOpenFiles,
        ErrorEof,
        ErrorNotADirectory,
        ErrorInProgress,
        ErrorInterrupted,
        ErrorFileExists,
        ErrorLoop,
        ErrorNotPermitted,
        ErrorIsDirectory,
        ErrorNoMemory,
        ErrorHostNotFound,
        ErrorInvalidHostName,
        ErrorHostHasNoAddress,
        ErrorLoginFailed,
        ErrorCancelled,
        ErrorDirectoryBusy,
        ErrorDirectoryNotEmpty,
        ErrorTooManyLinks,
        ErrorReadOnlyFileSystem,
        ErrorNotSameFileSystem,
        ErrorNameTooLong,
        ErrorServiceNotAvailable,
        ErrorServiceObsolete,
        ErrorProtocolError,
    };

    enum FileInfoOptions {
        fiDefault           = 0,
        fiGetMIMEType       = 1 << 0,
        fiForceFastMIMEType = 1 << 1,
        fiForceSlowMIMEType = 1 << 2,
        fiFollowLinks       = 1 << 3
    };

    enum DirectoryVisitOptions {
        dvDefault           = 0,
        dvSameFilesystem    = 1 << 0,
        dvLoopCheck         = 1 << 1
    };

    typedef void *FileInfo;

    class Visitor {
    public:
        virtual bool visit(const char *filename, FileInfo *info,
                           int recursing_will_loop, int *recurse) = 0;
    };

    YGnomeVFS();
    virtual ~YGnomeVFS();

    bool available() const;
    
    Result traverse(const char *uri, Visitor *visitor, 
                    FileInfoOptions info_options = fiDefault,
                    DirectoryVisitOptions visit_options = dvDefault);

protected:
    typedef int (* Init)();
    typedef void (* Shutdown)();

    typedef int (* DirectoryVisitFunc)
        (const char *filename, FileInfo *info,
         int recursing_will_loop, void *data, int *recurse);

    typedef Result (* DirectoryVisit)
        (const char *uri, FileInfoOptions info_options,
         DirectoryVisitOptions visit_options,
         DirectoryVisitFunc callback, void *data);

    static int visit(const char *filename, FileInfo *info,
                     int recursing_will_loop, void *data, int *recurse);

private:
    Init mInit;
    Shutdown mShutdown;
    DirectoryVisit mDirectoryVisit;
};

#endif

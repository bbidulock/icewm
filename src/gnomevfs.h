/*
 *  IceWM - Definitions for GNOME VFS 2.0 support
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  This header is based on gnome-vfs2-2.0.0.0.200206210447-0.snap.ximian.1
 *  built by Ximian on Don 27 Jun 2002 02:41:24 GMT.
 */

#ifndef __YGNOMEVFS_H
#define __YGNOMEVFS_H

#include "ylibrary.h"

#include <sys/stat.h>
#include <sys/types.h>

class YGnomeVFS: protected YSharedLibrary {
public:
    typedef unsigned long long FileSize;
    typedef FileSize InodeNumber;

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

    enum FileFlags {
	    ffNone          = 0,
	    ffSymlink       = 1 << 0,
	    ffLocal         = 1 << 1,
    };

    enum FileType {
        ftUnknown,
        ftRegular,
        ftDirectory,
        ftFifo,
        ftSocket,
        ftCharacterDevice,
        ftBlockDevice,
        ftSymbolicLink
    };

    enum FilePermissions {
	permSuid        = S_ISUID,
	permSgid        = S_ISGID,
	permSticky      = 01000, // S_ISVTX not defined on all systems
	permUserRead    = S_IRUSR,
	permUserWrite   = S_IWUSR,
	permUserExec    = S_IXUSR,
	permUserAll     = S_IRUSR | S_IWUSR | S_IXUSR,
	permGroupRead   = S_IRGRP,
	permGroupWrite  = S_IWGRP,
	permGroupExec   = S_IXGRP,
	permGroupAll    = S_IRGRP | S_IWGRP | S_IXGRP,
	permOtherRead   = S_IROTH,
	permOtherWrite  = S_IWOTH,
	permOtherExec   = S_IXOTH,
	permOtherAll    = S_IROTH | S_IWOTH | S_IXOTH
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

    enum FileInfoFields {
        fifNone             = 0,
        fifType             = 1 << 0,
        fifPermissions      = 1 << 1,
        fifFlags            = 1 << 2,
        fifDevice           = 1 << 3,
        fifInode            = 1 << 4,
        fifLinkCount        = 1 << 5,
        fifSize             = 1 << 6,
        fifBlockCount       = 1 << 7,
        fifIoBlockSize      = 1 << 8,
        fifAtime            = 1 << 9,
        fifMtime            = 1 << 10,
        fifCtime            = 1 << 11,
        fifSymlinkName      = 1 << 12,
        fifMimeType         = 1 << 13
    };

    struct FileInfo {
	char *name;

	FileInfoFields validFields;

	FileType type;
	FilePermissions permissions;
	FileFlags flags;

	dev_t device;
	InodeNumber inode;

	unsigned linkCount;

	unsigned uid;
	unsigned gid;

	FileSize size;

	FileSize blockCount;
	unsigned ioBlockSize;

	time_t atime;
	time_t mtime;
	time_t ctime;

	char *symlinkName;
	char *mimeType;

	unsigned refcount;
    };

    class Visitor {
    public:
        virtual bool visit(const char *filename, const FileInfo &info,
                           int recursingWillLoop, int &recurse) = 0;
    };

    YGnomeVFS();
    virtual ~YGnomeVFS();

    bool available() const;
    
    Result traverse(const char *uri, Visitor *visitor, 
                    FileInfoOptions infoOptions = fiDefault,
                    DirectoryVisitOptions visitOptions = dvDefault);

protected:
    typedef int (* Init)();
    typedef void (* Shutdown)();

    typedef int (* DirectoryVisitFunc)
        (const char *filename, FileInfo *info,
         int recursingWillLoop, void *data, int *recurse);

    typedef Result (* DirectoryVisit)
        (const char *uri, FileInfoOptions infoOptions,
         DirectoryVisitOptions visitOptions,
         DirectoryVisitFunc callback, void *data);

    static int visit(const char *filename, FileInfo *info,
                     int recursingWillLoop, void *data, int *recurse);

    virtual void unload();

private:
    Init mInit;
    Shutdown mShutdown;
    DirectoryVisit mDirectoryVisit;
};

#endif

#ifndef __YFILEPATH_H__
#define __YFILEPATH_H__

class CStr;

class YFilePath {
public:
    YFilePath(const char *path);
    YFilePath(const CStr *path);
    ~YFilePath();

    YFilePath *getParent() const;
    YFilePath *getRelative(const char *sub) const;

    bool isRegularFile();
    bool isDirectory();
    bool isSpecialFile();

    const CStr *path() const { return fPath; }

private:
    CStr *fPath;
};

#endif

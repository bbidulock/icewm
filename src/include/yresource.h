#ifndef __YRESOURCE_H
#define __YRESOURCE_H

class YFilePath;

class YResourcePath {
public:
    YResourcePath() { fCount = 0; fPaths = 0; }
    ~YResourcePath();

    int getCount() { return fCount; }
    const YFilePath *getPath(int i) { return fPaths[i]; }

    void addPath(const char *path);

private:
    int fCount;
    YFilePath **fPaths;
};

#endif

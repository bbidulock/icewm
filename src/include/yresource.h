#ifndef __YRESOURCE_H
#define __YRESOURCE_H

class YResourcePath {
public:
    YResourcePath() { fCount = 0; fPaths = 0; }
    ~YResourcePath();

    int getCount() { return fCount; }
    const char *getPath(int i) { return fPaths[i]; }

    void addPath(char *path);

private:
    int fCount;
    char **fPaths;
};

#endif

#ifndef __YHISTORY_H
#define __YHISTORY_H

#include "ypaths.h"

class YHistory {
public:
    YHistory(char const *id):
        fFilename(getFilename(id)) {
    }
    
    ~YHistory(void) {
        delete[] fFilename;
    }

    static char *getFilename(char const *id);

private:
    char *fFilename;
};

#endif

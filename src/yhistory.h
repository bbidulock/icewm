#ifndef __YHISTORY_H
#define __YHISTORY_H

#include <stddef.h>

class YHistory {
public:
    YHistory(char const *id);
    ~YHistory(void);

    int head() const { return fHead; }
    int count() const { return fCount; }

    void add(char const * str);
    char const *get(int index) const;

    char *getFilename(void) { return getFilename(fId); }
    static char *getFilename(char const *id);

    void restore(void);
    void commit(void);

private:
    char *fId;
    char **fItems;
    int fHead, fCount;
    bool fCommitChanges;
};

#endif

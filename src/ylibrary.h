/*
 *  IceWM - Definition of support for runtime bound shared libraries
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YSHARED_LIBRARY_H
#define __YSHARED_LIBRARY_H

#define ASSIGN_SYMBOL(InternName, ExternName) do { \
    m ## InternName = (InternName) getSymbol(#ExternName); \
} while(0);

class YSharedLibrary {
public:
    YSharedLibrary(const char *filename, bool global = false, bool lazy = true);
    virtual ~YSharedLibrary();

    bool loaded() const { return (fLibrary != 0); }
    operator bool() const { return loaded(); }

    void *getSymbol(const char *symname);
    static const char *getLastError();

protected:
    virtual void unload();

private:
    void *fLibrary;
};

#endif

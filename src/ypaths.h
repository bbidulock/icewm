/*
 *  IceWM - Definition for resource paths
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU General Public License
 */

#ifndef __YRESOURCEPATH_H
#define __YRESOURCEPATH_H

#include <stddef.h>
#include "ypaint.h"

int findPath(char const *path, int mode, char const *name,
	     char **fullname, bool path_relative = false);

struct YPathElement {
    char *joinPath(char const *base, char const *name = NULL) const;

    char const **root;
    char const *rdir;
    char const **sub;
};

class YResourcePaths {
public:
    YResourcePaths() : fPaths(NULL) {}
    YResourcePaths(YResourcePaths const & other) :
	fPaths(NULL) { operator= (other); }
    YResourcePaths(char const *subdir, bool themeOnly = false) : 
	fPaths(NULL) { init(subdir, themeOnly); }
    YResourcePaths(YResourcePaths const & other, char const *subdir,
    		   bool themeOnly = false) : 
	fPaths(NULL) { operator= (other); init(subdir, themeOnly); }
    ~YResourcePaths() { delete[] fPaths; }

    YResourcePaths const & operator= (YResourcePaths const & other);

    void init (char const * subdir, bool themeOnly = false);
    void init (YResourcePaths const & other, char const * subdir,
    	       bool themeOnly = false) {
	operator= (other); init(subdir, themeOnly);
    }

    YPixmap * loadPixmap(char const *base, char const *name);

    operator YPathElement const * () { return fPaths; }

protected:
    void verifyPaths(char const *base);
    
private:
    YPathElement * fPaths;
};

#endif

#include "config.h"
#include "base.h"
#include "gnomevfs.h"

#include <time.h>
#include <string.h>

char const *ApplicationName("testgnomevfs");

class VerboseVisitor : public YGnomeVFS::Visitor {
    virtual bool visit(const char *filename, const YGnomeVFS::FileInfo &info,
                       int recursingWillLoop, int &recurse) {
        char *mtime = ctime(&info.mtime);
        mtime[strlen(mtime) - 1] = '\0';

        msg("%s [%s][%s]", filename, info.mimeType, mtime);

        recurse = !recursingWillLoop;
        return true;
    }
};

int main(int argc, char *argv[]) {
    YGnomeVFS gnomeVFS;
    
    if (gnomeVFS.available()) {
        YGnomeVFS::Visitor *visitor = 0;
        unsigned pathCount = 0;
        
        for (char **arg = argv + 1; arg < argv + argc; ++arg)
            if (**arg == '-') {
                if (!visitor && !strcmp(*arg, "-v"))
                    visitor = new VerboseVisitor();
            } else {
                ++pathCount;
            }

        if (pathCount > 0) {
            for (char **arg = argv + 1; arg < argv + argc; ++arg) {
                if (**arg != '-') gnomeVFS.traverse(*arg, visitor);
            }
        } else {
            gnomeVFS.traverse("applications:", visitor);
        }

        delete visitor;

        return 0;
    } else {
        warn("GNOME VFS 2.0 not found: %s", YSharedLibrary::getLastError());
        return 1;
    }
}

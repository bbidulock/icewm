#include "config.h"
#include "base.h"
#include "gnomevfs.h"

char const *ApplicationName("testgnomevfs");

int main(int argc, char *argv[]) {
    YGnomeVFS gnomeVFS;
    
    if (gnomeVFS.available()) {
        return gnomeVFS.traverse(argc > 1 ? argv[1] : "applications:", 0);
    } else {
        warn("GNOME VFS 2.0 not found: %s", YSharedLibrary::getLastError());
        return 1;
    }
}

#include "config.h"
#include "base.h"
#include "debug.h"
#include "ydesktop.h"
#include "ylocale.h"

char const *ApplicationName("testdesktop");
bool multiByte(false);

int main(int argc, char *argv[])
{
#ifdef DEBUG
    debug = true;
#endif

    YLocale locale("de_DE@euro");
    YDesktopParser parser;
    
    for (char **path = argv + 1; path < argv + argc; ++path) {
        msg("path: %s\n", *path);
        
        if (!parser.parse(*path)) {
            msg("type: %s", parser.getType());
            msg("exec: %s", parser.getExec());
            msg("icon: %s", parser.getIcon());
            msg("name: %s", parser.getName());
        }
    }

    return 0;
}

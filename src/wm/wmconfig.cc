/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "ykey.h"

#include "ypaint.h"
#include "sysdep.h"
#include "yaction.h"
#include "yapp.h"

#include "bindkey.h"
#include "default.h"
#define CFGDEF
#include "bindkey.h"
#include "default.h"

int configurationLoaded = 0;

void addWorkspace(const char *name);

#if !defined(NO_CONFIGURE) || !defined(NO_CONFIGURE_MENUS)

char *getArgument(char *dest, int maxLen, char *p, bool comma) {
    char *d;
    int len = 0;
    int in_str = 0;
    
    while (*p && (*p == ' ' || *p == '\t'))
        p++;

    d = dest;
    len = 0;
    while (*p && len < maxLen - 1 &&
           (in_str || (*p != ' ' && *p != '\t' && *p != '\n' && (!comma || *p != ','))))
    {
        if (in_str && *p == '\\' && p[1]) {
            p++;
            char c = *p++; // *++p++ doesn't work :(

            switch (c) {
            case 'a': *d++ = '\a'; break;
            case 'b': *d++ = '\b'; break;
            case 'e': *d++ = 27; break;
            case 'f': *d++ = '\f'; break;
            case 'n': *d++ = '\n'; break;
            case 'r': *d++ = '\r'; break;
            case 't': *d++ = '\t'; break;
            case 'v': *d++ = '\v'; break;
            case 'x':
#define UNHEX(c) \
    (\
    ((c) >= '0' && (c) <= '9') ? (c) - '0' : \
    ((c) >= 'A' && (c) <= 'F') ? (c) - 'A' + 0xA : \
    ((c) >= 'a' && (c) <= 'f') ? (c) - 'a' + 0xA : 0 \
    )
                if (p[0] && p[1]) { // only two digits taken
                    int a = UNHEX(p[0]);
                    int b = UNHEX(p[1]);

                    int n = (a << 4) + b;

                    p += 2;
                    *d++ = (unsigned char)(n & 0xFF);

                    a -= '0';
                    if (a > '9')
                        a = a + '0' - 'A';
                    break;
                }
            default:
                *d++ = c;
                break;
            }
            len++;
        } else if (*p == '"') {
            in_str = !in_str;
            p++;
        } else {
            *d++ = *p++;
            len++;
        }
    }
    *d = 0;

    return p;
}

#endif

#if 0
#ifndef NO_CONFIGURE

char *setOption(char *name, char *arg, char *rest) {
#if 0
    unsigned int a;
#endif

    MSG(("SET %s := %s ;\n", name, arg));

#if 0
    for (a = 0; a < ACOUNT(bool_options); a++)
        if (strcmp(name, bool_options[a].option) == 0) {
            if ((arg[0] == '1' || arg[0] == '0') && arg[1] == 0)
                *(bool_options[a].value) = (arg[0] == '1') ? true : false;
            else {
                fprintf(stderr, "bad argument: %s for %s\n", arg, name);
                return rest;
            }
            return rest;
        }

    for (a = 0; a < ACOUNT(uint_options); a++)
        if (strcmp(name, uint_options[a].option) == 0) {
            unsigned int v = atoi(arg);
            
            if (v >= uint_options[a].min && v <= uint_options[a].max)
                *(uint_options[a].value) = v;
            else {
                fprintf(stderr, "bad argument: %s for %s\n", arg, name);
                return rest;
            }
            return rest;
        }
    for (a = 0; a < ACOUNT(string_options); a++)
        if (strcmp(name, string_options[a].option) == 0) {
            if (!string_options[a].initial)
                delete (char *)*string_options[a].value;
            *string_options[a].value = newstr(arg);
            string_options[a].initial = false;
            return rest;
        }
#endif
#ifndef NO_KEYBIND
    for (a = 0; a < ACOUNT(key_options); a++)
        if (strcmp(name, key_options[a].option) == 0) {
            WMKey *wk = key_options[a].value;

            if (parseKey(arg, &wk->key, &wk->mod)) {
                if (!wk->initial)
                    delete (char *)wk->name;
                wk->name = newstr(arg);
                wk->initial = false;
            }
            return rest;
        }
#endif
    if (strcmp(name, "WorkspaceNames") == 0) { // !!! needs a pref (new type: list-pref?)
        addWorkspace(arg);
        return rest;
    }
    fprintf(stderr, "Bad option: %s\n", name);
    ///!!! check
    return rest;
}

// parse option name and argument
// name is string without spaces up to =
// option is a " quoted string or characters up to next space
char *parseOption(char *str) {
    char name[64];
    char argument[256];
    char *p = str;
    unsigned int len = 0;

    while (*p && *p != '=' && *p != ' ' && *p != '\t' && len < sizeof(name) - 1)
        p++, len++;

    strncpy(name, str, len);
    name[len] = 0;

    while (*p && *p != '=')
        p++;
    if (*p != '=')
        return 0;
    p++;

    do {
        p = getArgument(argument, sizeof(argument), p, true);

        p = setOption(name, argument, p);

        if (p == 0)
            return 0;

        while (*p && (*p == ' ' || *p == '\t'))
            p++;

        if (*p != ',')
            break;
        p++;
    } while (1);

    return p;
}

void parseConfiguration(char *data) {
    char *p = data;

    while (p && *p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || (*p == '\\' && p[1] == '\n'))
            p++;
        
        if (*p != '#')
            p = parseOption(p);
        else {
            while (*p && *p != '\n') {
                if (*p == '\\' && p[1] != 0)
                    p++;
                p++;
            }
        }
    }
}

void loadConfiguration(const char *fileName) {
    printf("Load config: %s\n", fileName);
    int fd = open(fileName, O_RDONLY | O_TEXT);

    if (fd == -1)
        return ;

    struct stat sb;

    if (fstat(fd, &sb) == -1)
        return ;

    int len = sb.st_size;

    char *buf = new char[len + 1];
    if (buf == 0)
        return ;

    if (read(fd, buf, len) != len)
        return;

    buf[len] = 0;
    close(fd);
    parseConfiguration(buf);
    delete buf;
    
    configurationLoaded = 1;
}

void freeConfig() {
#if 0
    for (unsigned int a = 0; a < ACOUNT(string_options); a++)
        if (!string_options[a].initial) {
            delete (char *)*string_options[a].value;
            string_options[a].initial = 0;
            string_options[a].value = 0;
        }
#endif
}
#endif

#endif

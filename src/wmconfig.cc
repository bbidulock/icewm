/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "wmoption.h"

#include "ypaint.h"
#include "wmmgr.h"
#include "sysdep.h"
#include "prefs.h"
#include "yaction.h"

bool configurationNeeded(true);

#define CFGDEF
#include "bindkey.h"
#include "default.h"

#include "intl.h"

long workspaceCount = 0;
char *workspaceNames[MAXWORKSPACES];
YAction *workspaceActionActivate[MAXWORKSPACES];
YAction *workspaceActionMoveTo[MAXWORKSPACES];

char * findPath(const char *path, int mode, const char *name, bool /*path_relative*/) {
#ifdef __EMX__
    char name_exe[1024];

    if (mode & X_OK)
	name = strcat(strcpy(name_exe, name), ".exe");
#endif

    if (*name == '/') { // check for root in XFreeOS/2
#ifdef __EMX__
        if (!access(name, 0))
            return newstr(name);
#else
        if (!access(name, mode) && isreg(name))
            return newstr(name);
#endif
    } else {
        if (NULL == path) return NULL;

        unsigned const nameLen(strlen(name));
        char prog[1024];

        if (nameLen > sizeof(prog))
            return NULL;

        for (char const *p = path, *q = path; *q; q = p) {
            while (*p && *p != PATHSEP) p++;

            unsigned len(p - q);
	    if (*p) ++p;

            if (len > 0 && len < sizeof(prog) - nameLen - 2) {
                strncpy(prog, q, len);

                if (!ISSLASH(prog[len - 1]))
                    prog[len++] = SLASH;

                strcpy(prog + len, name);

#ifdef __EMX__
                if (!access(prog, 0))
                    return newstr(prog);
#else
                if (!access(prog, mode) && isreg(prog))
                    return newstr(prog);
#endif
            }
        }
    }

    return NULL;
}

void addWorkspace(const char *name) {
    if (workspaceCount >= MAXWORKSPACES)
        return;
    workspaceNames[workspaceCount] = newstr(name);
    workspaceActionActivate[workspaceCount] = new YAction(); // !! fix
    workspaceActionMoveTo[workspaceCount] = new YAction();
    PRECONDITION(workspaceNames[workspaceCount] != NULL);
    workspaceCount++;
}

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
            p++; char c = *p++; // *++p++ doesn't work :(

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

#ifndef NO_CONFIGURE

bool parseKey(const char *arg, KeySym *key, unsigned int *mod) {
    const char *orig_arg = arg;

    *mod = 0;
    for (;;) {
        if (strncmp("Alt+", arg, 4) == 0) {
            *mod |= kfAlt;
            arg += 4;
        } else if (strncmp("Ctrl+", arg, 5) == 0) {
            *mod |= kfCtrl;
            arg += 5;
        } else if (strncmp("Shift+", arg, 6) == 0) {
            *mod |= kfShift;
            arg += 6;
        } else if (strncmp("Meta+", arg, 5) == 0) {
            *mod |= kfMeta;
            arg += 5;
        } else if (strncmp("Super+", arg, 6) == 0) {
            *mod |= kfSuper;
            arg += 6;
        } else if (strncmp("Hyper+", arg, 6) == 0) {
            *mod |= kfHyper;
            arg += 6;
        } else
            break;
    }
    if (strcmp(arg, "") == 0) {
        *key = NoSymbol;
        return true;
    } else if (strcmp(arg, "Esc") == 0)
        *key = XK_Escape;
    else if (strcmp(arg, "Enter") == 0)
        *key = XK_Return;
    else if (strcmp(arg, "Space") == 0)
        *key = ' ';
    else if (strcmp(arg, "BackSp") == 0)
        *key = XK_BackSpace;
    else if (strcmp(arg, "Del") == 0)
        *key = XK_Delete;
    else if (strlen(arg) == 1 && arg[0] >= 'A' && arg[0] <= 'Z') {
        char s[2];
        s[0] = arg[0] - 'A' + 'a';
        s[1] = 0;
        *key = XStringToKeysym(s);
    } else {
        *key = XStringToKeysym(arg);
    }

    if (*key == NoSymbol) {
        msg(_("Unknown key name %s in %s"), arg, orig_arg);
        return false;
    }
    return true;
}

char *setOption(char *name, char *arg, char *rest) {
    unsigned int a;

    MSG(("SET %s := %s ;", name, arg));
    
    for (a = 0; a < ACOUNT(bool_options); a++)
        if (strcmp(name, bool_options[a].option) == 0) {
            if ((arg[0] == '1' || arg[0] == '0') && arg[1] == 0) {
                *(bool_options[a].value) = (arg[0] == '1') ? true : false;

		// !!! dirty compatibility hack for TitleBarCentered
		if (bool_options[a].value == &titleBarCentered) {
		    warn(_("Obsolete option: %s"), name);
		    titleBarJustify = (titleBarCentered ? 50 : 0);
		}
            } else {
                msg(_("Bad argument: %s for %s"), arg, name);
                return rest;
            }
            return rest;
        }

    for (a = 0; a < ACOUNT(uint_options); a++)
        if (strcmp(name, uint_options[a].option) == 0) {
            int const v(atoi(arg));
            
            if (v >= uint_options[a].min && v <= uint_options[a].max)
                *(uint_options[a].value) = v;
            else {
                msg(_("Bad argument: %s for %s"), arg, name);
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
    if (strcmp(name, "WorkspaceNames") == 0) {
        addWorkspace(arg);
        return rest;
    } else if (strcmp(name, "Look") == 0) {
#ifdef CONFIG_LOOK_WARP4
        if (strcmp(arg, "warp4") == 0)
            wmLook = lookWarp4;
        else
#endif
#ifdef CONFIG_LOOK_WARP3
        if (strcmp(arg, "warp3") == 0)
            wmLook = lookWarp3;
        else
#endif
#ifdef CONFIG_LOOK_WIN95
        if (strcmp(arg, "win95") == 0)
            wmLook = lookWin95;
        else
#endif
#ifdef CONFIG_LOOK_MOTIF
        if (strcmp(arg, "motif") == 0)
            wmLook = lookMotif;
        else
#endif
#ifdef CONFIG_LOOK_NICE
        if (strcmp(arg, "nice") == 0)
            wmLook = lookNice;
        else
#endif
#ifdef CONFIG_LOOK_PIXMAP
        if (strcmp(arg, "pixmap") == 0)
            wmLook = lookPixmap;
        else
#endif
#ifdef CONFIG_LOOK_METAL
        if (strcmp(arg, "metal") == 0)
            wmLook = lookMetal;
        else
#endif
#ifdef CONFIG_LOOK_GTK
        if (strcmp(arg, "gtk") == 0)
            wmLook = lookGtk;
        else
#endif
        {
            msg(_("Bad Look name"));
            return 0;
        }
        return rest;
    }
    msg(_("Bad option: %s"), name);
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

    if ((len = read(fd, buf, len)) < 0) {
        delete[] buf;
        return;
    }

    buf[len] = 0;
    close(fd);
    parseConfiguration(buf);
    delete[] buf;
    
    configurationNeeded = false;
}

void freeConfig() {
    for (unsigned int a = 0; a < ACOUNT(string_options); a++)
        if (!string_options[a].initial) {
            delete (char *)*string_options[a].value;
            string_options[a].initial = 0;
            string_options[a].value = 0;
        }
}
#endif

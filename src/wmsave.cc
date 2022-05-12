#include "config.h"
#include "wmsave.h"
#include "yconfig.h"
#include "wmconfig.h"
#include "yxapp.h"
#include "intl.h"
#include <regex.h>
#include <fcntl.h>
#include <unistd.h>

extern cfoption icewm_preferences[];

upath SavePrefs::defaultPrefs(upath path) {
    upath conf(YApplication::getConfigDir() + "/preferences");
    if (conf.fileExists() && path.copyFrom(conf))
        return path;

    conf = YApplication::getLibDir() + "/preferences";
    if (conf.fileExists() && path.copyFrom(conf))
        return path;

    return path;
}

upath SavePrefs::preferencesPath(const char* configFile) {
    const int perm = 0600;

    mstring conf(configFile);
    if (conf.nonempty() && conf != "preferences") {
        upath path(xapp->findConfigFile(conf));
        if (path.isWritable())
            return path;
        if (!path.fileExists() && path.testWritable(perm))
            return defaultPrefs(path);
    }

    upath priv(YApplication::getPrivConfFile("preferences", true));
    if (priv.isWritable())
        return priv;
    if (!priv.fileExists() && priv.testWritable(perm))
        return defaultPrefs(priv);

    return null;
}

int SavePrefs::compare(const void* left, const void* right) {
    return *(const int*)left - *(const int*)right;
}

SavePrefs::SavePrefs()
{
}

SavePrefs::SavePrefs(YArray<int>& mods, const char* configFile)
{
    const int n = mods.getCount();
    if (n < 1)
        return;

    if (1 < n)
        qsort(&*mods, n, sizeof(int), compare);

    upath path(preferencesPath(configFile));
    if (path == null) {
        fail(_("Unable to write to %s"), "preferences");
        return;
    }

    if (loadText(path)) {
        applyMods(mods, icewm_preferences);
        saveText(path);
    }
}

bool SavePrefs::loadText(upath path) {
    text = path.loadText();
    if (text == nullptr) {
        text.resize(1);
        if (text)
            *text = '\0';
    }
    return text != nullptr;
}

bool SavePrefs::saveText(upath path) {
    return writePrefs(path, text, strlen(text));
}

void SavePrefs::applyMods(YArray<int>& mods, cfoption* options)
{
    size_t tlen = strlen(text);
    const int n = mods.getCount();
    for (int k = 0; k < n; ++k) {
        cfoption* o = &options[mods[k]];
        char buf[1234] = "";
        if (o->type == cfoption::CF_BOOL) {
            snprintf(buf, sizeof buf, "%s=%d # 0/1\n", o->name, o->boolval());
        }
        else if (o->type == cfoption::CF_KEY) {
            WMKey *wk = o->key();
            snprintf(buf, sizeof buf, "%s=\"%s\"\n", o->name, wk->name);
        }
        else if (o->type == cfoption::CF_INT) {
            snprintf(buf, sizeof buf, "%s=%d # [%d-%d]\n",
                     o->name, o->intval(), o->intmin(), o->intmax());
        }
        else if (o->type == cfoption::CF_UINT) {
            snprintf(buf, sizeof buf, "%s=%u # [%u-%u]\n",
                     o->name, o->uintval(), o->uintmin(), o->uintmax());
        }
        else if (o->type == cfoption::CF_STR) {
            snprintf(buf, sizeof buf, "%s=\"%s\"\n", o->name,
                     Elvis(o->str(), ""));
        }
        else if (o->type == cfoption::CF_FUNC) {
            if (0 == strcmp(o->name, "WorkspaceNames")) {
                int i = 0;
                size_t len = 0;
                for (const char* work : configWorkspaces) {
                    if (++i == 1) {
                        snprintf(buf, sizeof buf, "%s=\"%s\"", o->name, work);
                    }
                    else if (len + 5 + strlen(work) < sizeof buf) {
                        snprintf(buf + len, sizeof buf - len,
                                 ", \"%s\"", work);
                    } else {
                        break;
                    }
                    len += strlen(buf + len);
                }
                if (i && len && len + 1 < sizeof buf) {
                    snprintf(buf + len, sizeof buf - len, "\n");
                }
            }
            else if (0 == strcmp(o->name, "KeyboardLayouts")) {
                int i = 0;
                size_t len = 0;
                for (const char* keyb : configKeyboards) {
                    if (++i == 1) {
                        snprintf(buf, sizeof buf, "%s=\"%s\"", o->name, keyb);
                    }
                    else if (len + 5 + strlen(keyb) < sizeof buf) {
                        snprintf(buf + len, sizeof buf - len,
                                 ", \"%s\"", keyb);
                    } else {
                        break;
                    }
                    len += strlen(buf + len);
                }
                if (i && len && len + 1 < sizeof buf) {
                    snprintf(buf + len, sizeof buf - len, "\n");
                }
            }
            else {
                continue;
            }
        }
        else {
            continue;
        }
        size_t blen = strlen(buf);
        if (blen == 0) {
            continue;
        }
        if (updateOption(o, buf, blen)) {
            tlen = strlen(text);
            continue;
        }
        if (insertOption(o, buf, blen)) {
            tlen = strlen(text);
            continue;
        }
        csmart c(retrieveComment(o));
        size_t clen = strlen(c);
        replace(text, tlen, c, clen, buf, blen);
        tlen = tlen + clen + blen;
    }
}

void SavePrefs::replace(char* s1, size_t n1, char* s2, size_t n2,
                        char* s3, size_t n3, char* s4, size_t n4)
{
    size_t size = 1 + n1 + n2 + n3 + n4;
    fcsmart u;
    u.resize(size);
    if (u) {
        if (n1) memcpy(u, s1, n1);
        if (n2) memcpy(u + n1, s2, n2);
        if (n3) memcpy(u + n1 + n2, s3, n3);
        if (n4) memcpy(u + n1 + n2 + n3, s4, n4);
        u[n1 + n2 + n3 + n4] = '\0';
        text = std::move(u);
    }
}

char* SavePrefs::nextline(char* start) {
    char* s = start;
    for (; *s && (*s != '\n' || (s > start && s[-1] == '\\')); ++s) {
    }
    if (*s == '\n') {
        ++s;
    }
    if (*s == ' ' || *s == '\t') {
        ++s;
        for (; *s == ' ' ||
               *s == '\t' ||
               *s == '\n' ||
               (*s == '\\' && s[1] == '\n');
               ++s)
        {
        }
    }
    return s;
}

bool SavePrefs::updateOption(cfoption* o, char* buf, size_t blen) {
    char temp[123];
    snprintf(temp, sizeof temp, "^[ \t]*%s[ \t]*=", o->name);
    regex_t pat = {};
    int c = regcomp(&pat, temp, REG_EXTENDED | REG_NEWLINE);
    if (c) {
        char err[99] = "";
        regerror(c, &pat, err, sizeof err);
        warn("regcomp save mod %s: %s", temp, err);
        return false;
    }
    int offset = 0;
    int replacements = 0;
    regmatch_t match = {};
    while (0 == regexec(&pat, text.data() + offset, 1, &match, 0)) {
        size_t start = offset + match.rm_so;
        char* next = nextline(text.data() + offset + match.rm_eo);
        size_t nlen = strlen(next);
        offset = (next - text);
        replace(text, start, buf, blen, next, nlen);
        ++replacements;
    }
    regfree(&pat);
    return 0 < replacements;
}

bool SavePrefs::insertOption(cfoption* o, char* buf, size_t blen) {
    char temp[123];
    snprintf(temp, sizeof temp, "^#+[ \t]*%s[ \t]*=", o->name);
    regex_t pat = {};
    int c = regcomp(&pat, temp, REG_EXTENDED | REG_NEWLINE);
    if (c) {
        char err[99] = "";
        regerror(c, &pat, err, sizeof err);
        warn("regcomp save mod %s: %s", buf, err);
        return true;
    }
    regmatch_t match = {};
    c = regexec(&pat, text, 1, &match, 0);
    regfree(&pat);
    if (c == 0) {
        char* start = text + match.rm_so;
        char* next = nextline(start);
        size_t tlen = (start - text);
        replace(text, tlen, buf, blen, next, strlen(next));
    }
    return c == 0;
}

char* SavePrefs::retrieveComment(cfoption* o) {
    upath path(YApplication::getLibDir() + "/preferences");
    char* res = nullptr;
    fcsmart text(path.loadText());
    if (text) {
        char buf[123];
        snprintf(buf, sizeof buf,
                 "(\n(#  .*\n)*)#+[ \t]*%s[ \t]*=",
                 o->name);
        regex_t pat = {};
        int c = regcomp(&pat, buf, REG_EXTENDED | REG_NEWLINE);
        if (c == 0) {
            regmatch_t m[2] = {};
            c = regexec(&pat, text, 2, m, 0);
            regfree(&pat);
            if (c == 0) {
                int len = m[1].rm_eo - m[1].rm_so;
                res = newstr(text + m[1].rm_so, len);
            }
        } else {
            char err[99] = "";
            regerror(c, &pat, err, sizeof err);
            warn("regcomp save mod %s: %s", buf, err);
        }
    }
    return res ? res : newstr("\n");
}

bool SavePrefs::writePrefs(upath dest, const char* text, size_t tlen) {
    bool done = false;
    upath temp(dest.path() + ".tmp");
    int fd = temp.open(O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd == -1) {
        fail(_("Unable to write to %s"), temp.string());
    }
    else if (write(fd, text, tlen) != ssize_t(tlen) || close(fd)) {
        fail(_("Unable to write to %s"), temp.string());
        close(fd);
        temp.remove();
    }
    else if (temp.renameAs(dest)) {
        fail(_("Unable to rename %s to %s"), temp.string(), dest.string());
        temp.remove();
    }
    else {
        done = true;
    }
    return done;
}



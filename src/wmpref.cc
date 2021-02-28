#include "config.h"
#include "wmpref.h"
#include "yconfig.h"
#include "ymenu.h"
#include "ymenuitem.h"
#include "yinputline.h"
#include "wmapp.h"
#include "intl.h"
#include <regex.h>
#include <fcntl.h>
#include <unistd.h>

#define utf8ellipsis "\xe2\x80\xa6"
const unsigned utf32ellipsis = 0x2026;
extern cfoption icewm_preferences[];
extern ref<YFont> menuFont;
YArray<int> PrefsMenu::mods;

PrefsMenu::PrefsMenu() :
    count(countPrefs()),
    message(nullptr),
    modify(nullptr)
{
    YMenu *fo, *qs, *tb, *sh, *al, *mz, *ke, *ks, *kw, *sc, *st;
    YMenuItem *item;
    bool useElps = menuFont != null && menuFont->supports(utf32ellipsis);

    addSubmenu("_Focus", -2, fo = new YMenu, "focus");
    addSubmenu("_QuickSwitch", -2, qs = new YMenu, "pref");
    addSubmenu("_TaskBar", -2, tb = new YMenu, "pref");
    addSubmenu("_Show", -2, sh = new YMenu, "pref");
    addSubmenu(useElps ?
                    "_A" utf8ellipsis " - L" utf8ellipsis :
                    "_A... - L...", -2, al = new YMenu, "pref");
    addSubmenu(useElps ?
                    "_M" utf8ellipsis " - Z" utf8ellipsis :
                    "_M... - Z...", -2, mz = new YMenu, "pref");
    addSubmenu("_KeyWin", -2, ke = new YMenu, "key");
    addSubmenu("K_eySys", -2, ks = new YMenu, "key");
    addSubmenu("KeySys_Workspace", -2, kw = new YMenu, "key");
    addSubmenu("S_calar", -2, sc = new YMenu, "key");
    addSubmenu("St_ring", -2, st = new YMenu, "key");

    int index[count];
    for (int i = 0; i < count; ++i) {
        index[i] = i;
    }
    qsort(index, count, sizeof(int), sortPrefs);

    for (int k = 0; k < count; ++k) {
        cfoption* o = &icewm_preferences[index[k]];
        YAction actionIndex(EAction(index[k] + 1));
        if (o->type == cfoption::CF_BOOL) {
            YMenu* m = 0 == strncmp(o->name, "Task", 4) ? tb :
                       0 == strncmp(o->name, "Quic", 4) ? qs :
                       0 == strncmp(o->name, "Show", 4) ||
                       0 == strncmp(o->name, "Page", 4) ? sh :
                       nullptr != strstr(o->name, "Focus") ? fo :
                       o->name[0] <= 'L' ? al : mz;
            item = m->addItem(o->name, -2, null, actionIndex);
            if (o->boolval()) {
                item->setChecked(true);
            }
        }
        else if (o->type == cfoption::CF_KEY) {
            const char* key = o->key()->name;
            if (0 == strncmp(o->name, "KeySysWork", 10)) {
                kw->addItem(o->name, -2, key, actionIndex);
            } else if (0 == strncmp(o->name, "KeySys", 6)) {
                ks->addItem(o->name, -2, key, actionIndex);
            } else {
                ke->addItem(o->name, -2, key, actionIndex);
            }
        }
        else if (o->type == cfoption::CF_INT) {
            int val = o->intval();
            sc->addItem(o->name, -2, mstring(val), actionIndex);
        }
        else if (o->type == cfoption::CF_STR) {
            const char* str = o->str();
            if (str) {
                char val[40];
                size_t len = strlcpy(val, str, sizeof val);
                const char* elp = useElps ? utf8ellipsis : "...";
                if (len >= sizeof val)
                    strlcpy(val + sizeof val - 4, elp, 4);
                str = strstr(val, "://");
                if (str && strlen(str) > 6)
                    strlcpy(val + (str - val) + 3, elp, 4);
                st->addItem(o->name, -2, val, actionIndex);
            }
        }
    }

    addSeparator();
    addItem(_("Save Modifications"), -2, null, actionSaveMod, "save");
    setActionListener(this);
}

void PrefsMenu::actionPerformed(YAction action, unsigned int modifiers) {
    if (action == actionSaveMod) {
        SavePrefs save(mods);
        return;
    }

    const int i = action.ident() - 1;
    if (inrange(i, 0, count - 1)) {
        cfoption* o = i + icewm_preferences;
        if (o->type == cfoption::CF_BOOL) {
            *o->v.b.bool_value ^= true;
            msg("%s = %d", o->name, o->boolval());
            int k = find(mods, i);
            return k >= 0 ? mods.remove(k) : mods.append(i);
        }
        else if (o->type == cfoption::CF_KEY) {
            WMKey* wk = o->key();
            query(o, (wk && wk->name) ? wk->name : "");
        }
        else if (o->type == cfoption::CF_INT) {
            char buf[99];
            snprintf(buf, sizeof buf, "%d", o->intval());
            query(o, buf);
        }
        else if (o->type == cfoption::CF_STR) {
            query(o, o->str());
        }
    }
}

void PrefsMenu::query(cfoption* opt, const char* old) {
    if (message) {
        message->unmanage();
    }

    csmart retrieved(SavePrefs::retrieveComment(opt));
    if (nonempty(retrieved)) {
        char* dest = retrieved;
        char* term = retrieved;
        for (char* tok = strtok(retrieved, "# \t\n");
             tok; tok = strtok(nullptr, "# \t\n"))
        {
            if (strlen(tok) + (dest - term) < 60) {
                if (term < dest) {
                    *dest++ = ' ';
                }
            }
            else if (term < dest) {
                *dest++ = '\n';
                term = dest;
            }
            while (*tok) {
                *dest++ = *tok++;
            }
        }
        *dest = '\0';
    }

    char name[123];
    if (opt->type == cfoption::CF_INT) {
        snprintf(name, sizeof name, "%s [%d-%d]",
                 opt->name, opt->intmin(), opt->intmax());
    } else {
        strlcpy(name, opt->name, sizeof name);
    }
    char text[123];
    snprintf(text, sizeof text, _("Enter a new value for %s: "), name);

    size_t size = 123 + (retrieved ? strlen(retrieved) : 0) + strlen(text);
    csmart heading(new char[size]);
    if (heading) {
        if (nonempty(retrieved)) {
            snprintf(heading, size, "%-60s\n\n%-60s", (char*)retrieved, text);
        } else {
            snprintf(heading, size, "%-60s", text);
        }
    }

    message = new YMsgBox(YMsgBox::mbAll, opt->name, heading, this);
    if (nonempty(old) && message && message->input()) {
        message->input()->setText(mstring(old), false);
    }
    modify = opt;
}

void PrefsMenu::modified(cfoption* opt, bool set) {
    if (opt >= &icewm_preferences[0] &&
        opt < &icewm_preferences[count])
    {
        int i = int(opt - icewm_preferences);
        if (set) {
            if (find(mods, i) < 0) {
                mods.append(i);
            }
        } else {
            findRemove(mods, i);
        }
    }
}

void PrefsMenu::handleMsgBox(YMsgBox* msgbox, int operation) {
    if (message == msgbox) {
        mstring input;
        if (operation == YMsgBox::mbOK && msgbox->input()) {
            input = msgbox->input()->getText().trim();
        }
        message->unmanage();
        message = nullptr;
        if (operation == YMsgBox::mbOK && modify) {
            if (modify->type == cfoption::CF_KEY && modify->key()) {
                WMKey *wk = modify->key();
                if (input.isEmpty() ? (wk->key = wk->mod = 0, true) :
                    YConfig::parseKey(input, &wk->key, &wk->mod))
                {
                    if (!wk->initial)
                        delete[] const_cast<char *>(wk->name);
                    wk->name = newstr(input);
                    wk->initial = false;
                    modified(modify);
                    msg("%s = \"%s\"", modify->name, wk->name);
                    wmapp->actionPerformed(actionReloadKeys);
                }
                else modified(modify, false);
            }
            else if (modify->type == cfoption::CF_INT && input.nonempty()) {
                int value = 0, len = 0;
                if (sscanf(input, "%d%n", &value, &len) == 1 &&
                    size_t(len) == input.length() &&
                    inrange(value, modify->intmin(), modify->intmax()))
                {
                    *modify->v.i.int_value = value;
                    modified(modify);
                    msg("%s = %d", modify->name, value);
                }
                else modified(modify, false);
            }
            else if (modify->type == cfoption::CF_STR) {
                if (modify->v.s.initial == false)
                    delete[] const_cast<char *>(*modify->v.s.string_value);
                *modify->v.s.string_value = newstr(input);
                modify->v.s.initial = false;
                modified(modify);
                msg("%s = \"%s\"", modify->name, modify->str());
            }
        }
    }
}

int PrefsMenu::countPrefs() {
    for (int k = 0; ; ++k)
        if (icewm_preferences[k].type == cfoption::CF_NONE)
            return k;
}

int PrefsMenu::sortPrefs(const void* p1, const void* p2) {
    const int i1 = *(const int *)p1;
    const int i2 = *(const int *)p2;
    cfoption* o1 = i1 + icewm_preferences;
    cfoption* o2 = i2 + icewm_preferences;
    if (o1->type == cfoption::CF_KEY &&
        o2->type == cfoption::CF_KEY &&
        0 == strncmp(o1->name, "KeySysWork", 10) &&
        0 == strncmp(o2->name, "KeySysWork", 10))
        return i1 - i2;
    return strcmp(o1->name, o2->name);
}

upath SavePrefs::defaultPrefs(upath path) {
    upath conf(YApplication::getConfigDir() + "/preferences");
    if (conf.fileExists() && path.copyFrom(conf))
        return path;

    conf = YApplication::getLibDir() + "/preferences";
    if (conf.fileExists() && path.copyFrom(conf))
        return path;

    return path;
}

upath SavePrefs::preferencesPath() {
    const int perm = 0600;

    mstring conf(wmapp->getConfigFile());
    if (conf.nonempty() && conf != "preferences") {
        upath path(wmapp->findConfigFile(conf));
        if (path.isWritable())
            return path;
        if (!path.fileExists() && path.testWritable(perm))
            return defaultPrefs(path);
    }

    upath priv(YApplication::getPrivConfDir() + "/preferences");
    if (priv.isWritable())
        return priv;
    if (!priv.fileExists() && priv.testWritable(perm))
        return defaultPrefs(priv);

    return null;
}

int SavePrefs::compare(const void* left, const void* right) {
    return *(const int*)left - *(const int*)right;
}

SavePrefs::SavePrefs(YArray<int>& modified) :
    mods(modified)
{
    const int n = mods.getCount();
    if (n < 1)
        return;

    if (1 < n)
        qsort(&*mods, n, sizeof(int), compare);

    upath path(preferencesPath());
    if (path == null) {
        fail(_("Unable to write to %s"), "preferences");
        return;
    }

    text = path.loadText();
    if (text == nullptr) {
        text.resize(1);
        if (text)
            *text = '\0';
        else
            return;
    }

    size_t tlen = strlen(text);
    for (int k = 0; k < n; ++k) {
        cfoption* o = &icewm_preferences[mods[k]];
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
        else if (o->type == cfoption::CF_STR) {
            snprintf(buf, sizeof buf, "%s=\"%s\"\n", o->name, o->str());
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
    writePrefs(path, text, tlen);
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
        size_t slen = (next - start);
        size_t tlen = (start - text);
        if (blen <= slen) {
            memcpy(start, buf, blen);
            for (char* p = start + blen; p + 1 < next; ++p) {
                *p = ' ';
                p[1] = '\n';
            }
        } else {
            replace(text, tlen, buf, blen, next, strlen(next));
        }
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

void SavePrefs::writePrefs(upath dest, const char* text, size_t tlen) {
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
        mods.clear();
    }
}



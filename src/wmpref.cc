#include "config.h"
#include "wmpref.h"
#include "yconfig.h"
#include "ymenu.h"
#include "ymenuitem.h"
#include "yinputline.h"
#include "wmapp.h"
#include "wmsave.h"
#include "intl.h"
#include <regex.h>
#include <fcntl.h>
#include <unistd.h>

#define utf8ellipsis "\xe2\x80\xa6"
const unsigned utf32ellipsis = 0x2026;
extern cfoption icewm_preferences[];
extern YFont menuFont;
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
            long val = o->intval();
            sc->addItem(o->name, -2, mstring(val), actionIndex);
        }
        else if (o->type == cfoption::CF_UINT) {
            long val = o->uintval();
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
        SavePrefs save(mods, wmapp->getConfigFile());
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
        else if (o->type == cfoption::CF_UINT) {
            char buf[99];
            snprintf(buf, sizeof buf, "%u", o->uintval());
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

    char name[99];
    if (opt->type == cfoption::CF_INT) {
        snprintf(name, sizeof name, "%s [%d-%d]",
                 opt->name, opt->intmin(), opt->intmax());
    }
    else if (opt->type == cfoption::CF_UINT) {
        snprintf(name, sizeof name, "%s [%u-%u]",
                 opt->name, opt->uintmin(), opt->uintmax());
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

    const char* iconName = !strncmp(opt->name, "Key", 3) ? "key" : "pref";
    message = new YMsgBox(YMsgBox::mbAll, opt->name, heading, this, iconName);
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
            input = msgbox->input()->getText();
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
                input = input.trim();
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
            else if (modify->type == cfoption::CF_UINT && input.nonempty()) {
                unsigned value = 0;
                int len = 0;
                input = input.trim();
                if (sscanf(input, "%u%n", &value, &len) == 1 &&
                    size_t(len) == input.length() &&
                    inrange(value, modify->uintmin(), modify->uintmax()))
                {
                    *modify->v.u.uint_value = value;
                    modified(modify);
                    msg("%s = %u", modify->name, value);
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


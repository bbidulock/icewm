/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"

#define NEED_TIME_H

#include "wmprog.h"
#include "prefs.h"
#include "wmapp.h"
#include "sysdep.h"
#include "wmmgr.h"
#include "themes.h"
#include "browse.h"
#include "appnames.h"
#include "wmswitch.h"
#include "ypointer.h"
#include <regex.h>
#include "intl.h"

DObjectMenuItem::DObjectMenuItem(DObject *object):
    YMenuItem(object->getName(), -3, null, YAction(), 0)
{
    fObject = object;
    if (object->getIcon() != null)
        setIcon(object->getIcon());
}

DObjectMenuItem::~DObjectMenuItem() {
    delete fObject;
}

void DObjectMenuItem::actionPerformed(YActionListener * /*listener*/, YAction /*action*/, unsigned int /*modifiers*/) {
    wmapp->signalGuiEvent(geLaunchApp);
    fObject->open();
}

DFile::DFile(IApp *app, const ustring &name, ref<YIcon> icon, upath path): DObject(app, name, icon) {
    this->app = app;
    fPath = path;
}

DFile::~DFile() {
}

void DFile::open() {
    const char *args[] = { openCommand, fPath.string(), 0 };
    app->runProgram(openCommand, args);
}

ObjectMenu::ObjectMenu(YActionListener *actionListener, YWindow *parent): YMenu(parent) {
    setActionListener(actionListener);
}

ObjectMenu::~ObjectMenu() {
}

void ObjectMenu::addObject(DObject *fObject) {
    add(new DObjectMenuItem(fObject));
}

void ObjectMenu::addObject(DObject *fObject, const char *icons) {
    add(new DObjectMenuItem(fObject), icons);
}

void ObjectMenu::addSeparator() {
    YMenu::addSeparator();
}

void ObjectMenu::addContainer(const ustring &name, ref<YIcon> icon, ObjectMenu *container) {
    if (container) {
        YMenuItem *item =
            addSubmenu(name, -3, container);

        if (item && icon != null)
            item->setIcon(icon);
    }
}

DObject::DObject(IApp *app, const ustring &name, ref<YIcon> icon):
    fName(name), fIcon(icon)
{
    this->app = app;
}

DObject::~DObject() {
    fIcon = null;
}

void DObject::open() {
}

DProgram::DProgram(
    IApp *app,
    YSMListener *smActionListener,
    const ustring &name,
    ref<YIcon> icon,
    const bool restart,
    const char *wmclass,
    upath exe,
    YStringArray &args)
    : DObject(app, name, icon),
    fRestart(restart),
    fRes(newstr(wmclass)),
    fPid(0),
    fCmd(exe),
    fArgs(args)
{
    this->app = app;
    this->smActionListener = smActionListener;
    if (fArgs.isEmpty() || fArgs.getString(fArgs.getCount() - 1))
        fArgs.append(0);
}

DProgram::~DProgram() {
    delete[] fRes;
}

void DProgram::open() {
    if (fRestart)
        smActionListener->restartClient(fCmd.string(), fArgs.getCArray());
    else if (fRes)
        smActionListener->runOnce(fRes, &fPid, fCmd.string(), fArgs.getCArray());
    else
        app->runProgram(fCmd.string(), fArgs.getCArray());
}

DProgram *DProgram::newProgram(
    IApp *app,
    YSMListener *smActionListener,
    const char *name,
    ref<YIcon> icon,
    const bool restart,
    const char *wmclass,
    upath exe,
    YStringArray &args)
{
    if (exe != null) {
        MSG(("LOOKING FOR: %s\n", exe.string().c_str()));
        upath fullname = findPath(getenv("PATH"), X_OK, exe);
        if (fullname == null) {
            MSG(("Program %s (%s) not found.", name, exe.string().c_str()));
            return 0;
        }

        DProgram *program =
            new DProgram(app, smActionListener, name, icon, restart, wmclass, fullname, args);

        return program;
    }
    return NULL;
}

YObjectArray<KProgram> keyProgs;

KProgram::KProgram(const char *key, DProgram *prog, bool bIsDynSwitchMenuProg)
    : fKey(NoSymbol), fMod(0), bIsDynSwitchMenu(bIsDynSwitchMenuProg), fProg(prog), pSwitchWindow(0)
{
    YConfig::parseKey(key, &fKey, &fMod);
    keyProgs.append(this);
}

class MenuProgSwitchItems: public ISwitchItems {
    MenuProgMenu *menu;
    int zTarget;

    KeySym key;
    unsigned int mod;

public:
    MenuProgSwitchItems(DProgram* prog, KeySym key, unsigned keymod) : ISwitchItems()
        , zTarget(0), key(key), mod(keymod) {
        menu = new MenuProgMenu(wmapp, wmapp, NULL /* no wmaction handling*/,
                "switch popup internal menu", prog->fCmd, prog->fArgs);
    }
    virtual void updateList() OVERRIDE {
        menu->refresh();
        zTarget = 0;
    }
    virtual int getCount() OVERRIDE {
        return menu->itemCount();
    }
    virtual bool isKey(KeySym k, unsigned int mod) OVERRIDE {
        return k == this->key && mod == this->mod;
    }
    virtual void setWMClass(char* wmclass) OVERRIDE {
        if (wmclass) free(wmclass); // unimplemented
    }

    // move the focused target up or down and return the new focused element
    virtual int moveTarget(bool zdown) OVERRIDE {
        int count = menu->itemCount();
        zTarget = (zTarget + count + (zdown?1:-1)) % count;
        // no further gimmicks
        return zTarget;
    }
    // move the focused target up or down and return the new focused element
        virtual int setTarget(int where) OVERRIDE {
            int count = menu->itemCount();
            return zTarget = inrange(where, 0, count) ? zTarget : 0;
        }

    /// Show changed focus preview to user
    virtual void displayFocusChange(int idxFocused) OVERRIDE {}
    // set target cursor and implementation specific stuff in the beginning
    virtual void begin(bool zdown) OVERRIDE {
        updateList();
        moveTarget(zdown);
    }
    virtual void reset() OVERRIDE {
        zTarget=0;
    }
    virtual void cancel() OVERRIDE {
    }
    virtual void accept(IClosablePopup *parent) OVERRIDE {
        YMenuItem* item=menu->getItem(zTarget);
        if (!item) return;
        // even through all the obscure "abstraction" it should just run DObjectMenuItem::actionPerformed
        item->actionPerformed(0, actionRun, 0);
        parent->close();
    }

    virtual int getActiveItem() OVERRIDE {
        return zTarget;
    }
    virtual ustring getTitle(int idx) OVERRIDE {
        if (idx<0 || idx>=this->getCount())
            return null;
        return menu->getItem(idx)->getName();
    }
    virtual ref<YIcon> getIcon(int idx) OVERRIDE {
        if (idx<0 || idx>=this->getCount())
            return null;
        return menu->getItem(idx)->getIcon();
    }

    // Manager notification about windows disappearing under the fingers
    virtual void destroyedItem(void* framePtr) OVERRIDE {
    }

};

void KProgram::open(unsigned mods) {
    if (!fProg) return;

    if (bIsDynSwitchMenu) {
        if (!pSwitchWindow) {
            pSwitchWindow = new SwitchWindow(manager, new MenuProgSwitchItems(fProg, fKey, fMod), quickSwitchVertical);
        }
        pSwitchWindow->begin(true, mods);
    }
    else
        fProg->open();
}

MenuFileMenu::MenuFileMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    ustring name,
    YWindow *parent)
    :
    ObjectMenu(wmActionListener, parent),
    MenuLoader(app, smActionListener, wmActionListener),
    fName(name),
    fModTime(0),
    app(app)
{
}

MenuFileMenu::~MenuFileMenu() {
}

void MenuFileMenu::updatePopup() {
    if (!autoReloadMenus && fPath != null)
        return;

    upath np = app->findConfigFile(upath(fName));
    bool rel = false;


    if (fPath == null) {
        fPath = np;
        rel = true;
    } else {
        if (np == null || np.equals(fPath)) {
            fPath = np;
            rel = true;
        } else
            np = null;
    }

    if (fPath == null) {
        refresh();
    } else {
        struct stat sb;
        if (stat(fPath.string(), &sb) != 0) {
            fPath = null;
            refresh();
        } else if (sb.st_mtime > fModTime || rel) {
            fModTime = sb.st_mtime;
            refresh();
        }
    }
}

void MenuFileMenu::refresh() {
    removeAll();
    if (fPath != null)
        loadMenus(fPath, this);
}

MenuProgMenu::MenuProgMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    ustring name,
    upath command,
    YStringArray &args,
    long timeout,
    YWindow *parent)
    :
    ObjectMenu(wmActionListener, parent),
    MenuLoader(app, smActionListener, wmActionListener),
    fName(name),
    fCommand(command),
    fArgs(args),
    fModTime(0),
    fTimeout(timeout)
{
}

MenuProgMenu::~MenuProgMenu() {
}

void MenuProgMenu::updatePopup() {
    time_t now = time(NULL);
    if (fModTime == 0 || (0 < fTimeout && now >= fModTime + fTimeout)) {
        refresh();
        fModTime = now;
    }
}

void MenuProgMenu::refresh()
{
    removeAll();
    if (fCommand != null)
        progMenus(fCommand.string(), fArgs.getCArray(), this);
}

StartMenu::StartMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    const char *name,
    YWindow *parent)
    : MenuFileMenu(app, smActionListener, wmActionListener, name, parent),
    smActionListener(smActionListener),
    wmActionListener(wmActionListener)
{
}

bool StartMenu::handleKey(const XKeyEvent &key) {
    // If meta key, close the popup
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        int m = KEY_MODMASK(key.state);

        if ((k == xapp->Win_L ||
             k == XK_Multi_key ||
             k == xapp->Win_R) && m == 0)
        {
            cancelPopup();
            return true;
        }
    }
    return MenuFileMenu::handleKey(key);
}

void StartMenu::updatePopup() {
    MenuFileMenu::updatePopup();
}

FocusMenu::FocusMenu() {
    struct FocusModelNameAction {
        FocusModels mode;
        const char *name;
        YAction action;
    } foci[] = {
        { FocusClick, _("_Click to focus"), actionFocusClickToFocus },
        { FocusExplicit, _("_Explicit focus"), actionFocusExplicit },
        { FocusSloppy, _("_Sloppy mouse focus"), actionFocusMouseSloppy },
        { FocusStrict, _("S_trict mouse focus"), actionFocusMouseStrict },
        { FocusQuiet, _("_Quiet sloppy focus"), actionFocusQuietSloppy },
        { FocusCustom, _("Custo_m"), actionFocusCustom },
    };
    for (size_t k = 0; k < ACOUNT(foci); ++k) {
        YMenuItem *item = addItem(foci[k].name, -2, null, foci[k].action);
        if (wmapp->getFocusMode() == foci[k].mode) {
            item->setEnabled(false);
            item->setChecked(true);
        }
    }
}

extern cfoption icewm_preferences[];

class PrefsMenu: public YMenu, private YActionListener {
private:
    const int count;
    const YAction actionSaveMod;
    static YArray<int> mods;

public:
    PrefsMenu() :
        count(countPrefs())
    {
        YMenu *fo, *qs, *tb, *sh, *al, *mz, *ke, *ks, *kw, *sc, *st;
        YMenuItem *item;

        addSubmenu("_Focus", -2, fo = new YMenu, "focus");
        addSubmenu("_QuickSwitch", -2, qs = new YMenu, "pref");
        addSubmenu("_TaskBar", -2, tb = new YMenu, "pref");
        addSubmenu("_Show", -2, sh = new YMenu, "pref");
        addSubmenu("_A... - L...", -2, al = new YMenu, "pref");
        addSubmenu("_M... - Z...", -2, mz = new YMenu, "pref");
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
            if (o->type == cfoption::CF_BOOL) {
                YMenu* m = 0 == strncmp(o->name, "Task", 4) ? tb :
                           0 == strncmp(o->name, "Quic", 4) ? qs :
                           0 == strncmp(o->name, "Show", 4) ||
                           0 == strncmp(o->name, "Page", 4) ? sh :
                           0 != strstr(o->name, "Focus") ? fo :
                           o->name[0] <= 'L' ? al : mz;
                item = m->addItem(o->name, -2, null, EAction(index[k] + 1));
                if (o->boolval()) {
                    item->setChecked(true);
                }
            }
            else if (o->type == cfoption::CF_KEY) {
                const char* key = o->key()->name;
                if (0 == strncmp(o->name, "KeySysWork", 10)) {
                    kw->addItem(o->name, -2, key, actionNull);
                } else if (0 == strncmp(o->name, "KeySys", 6)) {
                    ks->addItem(o->name, -2, key, actionNull);
                } else {
                    ke->addItem(o->name, -2, key, actionNull);
                }
            }
            else if (o->type == cfoption::CF_INT) {
                int val = o->intval();
                sc->addItem(o->name, -2, mstring(val), actionNull);
            }
            else if (o->type == cfoption::CF_STR) {
                const char* str = o->str();
                if (str) {
                    char val[40];
                    size_t len = strlcpy(val, str, sizeof val);
                    if (len >= sizeof val)
                        strlcpy(val + sizeof val - 4, "...", 4);
                    if ((str = strstr(val, "://")) != 0 && strlen(str) > 6)
                        strlcpy(val + (str - val) + 3, "...", 4);
                    st->addItem(o->name, -2, val, actionNull);
                }
            }
        }

        addSeparator();
        addItem(_("Save Modifications"), -2, null, actionSaveMod, "save");
        setActionListener(this);
    }

    virtual void actionPerformed(YAction action, unsigned int modifiers) {
        if (action == actionSaveMod)
            return saveModified();

        const int i = action.ident() - 1;
        if (inrange(i, 0, count - 1)) {
            cfoption* o = i + icewm_preferences;
            if (o->type == cfoption::CF_BOOL) {
                *o->v.b.bool_value ^= true;
                msg("%s = %d", o->name, o->boolval());
                int k = find(mods, i);
                return k >= 0 ? mods.remove(k) : mods.append(i);
            }
        }
    }

    static int countPrefs() {
        for (int k = 0; ; ++k)
            if (icewm_preferences[k].type == cfoption::CF_NONE)
                return k;
    }

    static int sortPrefs(const void* p1, const void* p2) {
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

    static upath defaultPrefs(upath path) {
        upath conf(YApplication::getConfigDir() + "/preferences");
        if (conf.fileExists() && path.copyFrom(conf))
            return path;

        conf = YApplication::getLibDir() + "/preferences";
        if (conf.fileExists() && path.copyFrom(conf))
            return path;

        return path;
    }

    static upath preferencesPath() {
        const int perm = 0600;

        ustring conf(wmapp->getConfigFile());
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

    static void saveModified() {
        const int n = mods.getCount();
        if (n < 1)
            return;
        qsort(&*mods, n, sizeof(int), sortPrefs);

        upath path(preferencesPath());
        if (path == null)
            return fail(_("Unable to write to %s"), "preferences");

        csmart text(path.loadText());
        if (text == 0)
            (text = new char[1])[0] = 0;
        size_t tlen = strlen(text);
        for (int k = 0; k < n; ++k) {
            const int i = mods[k];
            cfoption* o = i + icewm_preferences;
            if (updateOption(o, text))
                continue;
            if (insertOption(o, text))
                continue;
            csmart c(retrieveComment(o));
            char buf[99];
            snprintf(buf, sizeof buf, "%s=%d # 0/1\n",
                     o->name, o->boolval());
            size_t clen = c ? strlen(c) : 0;
            size_t blen = strlen(buf);
            size_t size = 1 + tlen + clen + blen;
            char* u = new char[size];
            memcpy(u, text, tlen);
            memcpy(u + tlen, c, clen);
            memcpy(u + tlen + clen, buf, blen + 1);
            text = u;
            tlen += clen + blen;
        }
        writePrefs(path, text, tlen);
    }

    static bool updateOption(cfoption* o, char* text) {
        char buf[99];
        snprintf(buf, sizeof buf, "^[ \t]*%s[ \t]*=[ \t]*[!-~]", o->name);
        regex_t pat = {};
        int c = regcomp(&pat, buf, REG_EXTENDED | REG_NEWLINE);
        if (c) {
            char err[99] = "";
            regerror(c, &pat, err, sizeof err);
            warn("regcomp save mod %s: %s", buf, err);
            return true;
        }
        char* s = text;
        regmatch_t m = {};
        while (0 == regexec(&pat, s, 1, &m, 0)) {
            s += m.rm_eo;
            s[-1] = '0' + o->boolval();
            s = replaceComment(s, "# 0/1");
        }
        regfree(&pat);
        return s > text;
    }

    static char* replaceComment(char* start, const char* comment) {
        char* s = start;

        if (*s && *s != '\n' && *s != '#' && *comment != ' ')
            *s++ = ' ';
        for (const char* c = comment; *s && *s != '\n' && *c; ++c)
            *s++ = *c;
        while (*s && *s != '\n')
            *s++ = ' ';

        return s;
    }

    static bool insertOption(cfoption* o, char* text) {
        char buf[99];
        snprintf(buf, sizeof buf, "^#+[ \t]*%s[ \t]*=[ \t]*[!-~]", o->name);
        regex_t pat = {};
        int c = regcomp(&pat, buf, REG_EXTENDED | REG_NEWLINE);
        if (c) {
            char err[99] = "";
            regerror(c, &pat, err, sizeof err);
            warn("regcomp save mod %s: %s", buf, err);
            return true;
        }
        regmatch_t m = {};
        c = regexec(&pat, text, 1, &m, 0);
        regfree(&pat);
        if (c == 0) {
            char* s = text + m.rm_so;
            snprintf(buf, sizeof buf, "%s=%d", o->name, o->boolval());
            size_t blen = strlen(buf);
            memcpy(s, buf, blen);
            s += blen;
            s = replaceComment(s, "# 0/1");
        }
        return c == 0;
    }

    static char* retrieveComment(cfoption* o) {
        static const char path[] = LIBDIR "/preferences";
        char* res = 0;
        csmart text(load_text_file(path));
        if (text) {
            char buf[99];
            snprintf(buf, sizeof buf,
                     "(\n(#  .*\n)*)#+[ \t]*%s[ \t]*=[ \t]*[!-~]",
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

    static void writePrefs(const upath dest, const char* text, size_t tlen) {
        const upath temp(dest.path() + ".tmp");
        int fd = temp.open(O_CREAT | O_WRONLY | O_TRUNC, 0600);
        if (fd == -1) {
            fail(_("Unable to write to %s"), temp.string().c_str());
        } else {
            ssize_t w = write(fd, text, tlen);
            if (size_t(w) != tlen)
                fail(_("Unable to write to %s"), temp.string().c_str());
            close(fd);
            if (size_t(w) == tlen) {
                if (temp.renameAs(dest))
                    fail(_("Unable to rename %s to %s"),
                         temp.string().c_str(),
                         dest.string().c_str());
                else
                    mods.clear();
            }
        }
    }
};

YArray<int> PrefsMenu::mods;

HelpMenu::HelpMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener)
    : ObjectMenu(wmActionListener)
{
    struct HelpMenuItem {
        const char *name;
        const char *menu;
        const char *clas;
    } help[] = {
        { ICEHELPIDX, _("_Manual"), "browser.Manual" },
        { "icewm.1.html", _("_Icewm(1)"), "browser.IceWM" },
        { "icewmbg.1.html", _("Icewm_Bg(1)"), "browser.IcewmBg" },
        { "icesound.1.html", _("Ice_Sound(1)"), "browser.IceSound" },
    };
    for (size_t k = 0; k < ACOUNT(help); ++k) {
        YStringArray args(3);
        args.append(ICEHELPEXE);
        if (k == 0) {
            args.append(help[k].name);
        } else {
            upath path = upath(ICEHELPIDX).parent() + help[k].name;
            args.append(path.string());
        }
        args.append(0);

        DProgram *prog = DProgram::newProgram(
            app,
            smActionListener,
            help[k].menu,
            null,
            false,
            help[k].clas,
            ICEHELPEXE,
            args);

        if (prog)
            addObject(prog, "help");
    }
}

void StartMenu::refresh() {
    MenuFileMenu::refresh();

    if (itemCount())
        addSeparator();

/// TODO #warning "make this into a menuprog (ala gnome.cc), and use mime"
    if (openCommand && openCommand[0]) {
        upath path[] = { upath::root(), YApplication::getHomeDir() };
        YMenu *sub;
        ref<YIcon> folder = YIcon::getIcon("folder");
        for (unsigned int i = 0; i < ACOUNT(path); i++) {
            upath& p = path[i];

            sub = new BrowseMenu(app, smActionListener, wmActionListener, p);
            DFile *file = new DFile(app, p, null, p);
            YMenuItem *item = add(new DObjectMenuItem(file));
            if (item && sub) {
                item->setSubmenu(sub);
                if (folder != null)
                    item->setIcon(folder);
            }
            else if (sub) {
                delete sub;
            }
        }
        addSeparator();
    }

    int const oldItemCount = itemCount();

    if (showPrograms) {
        ObjectMenu *programs = new MenuFileMenu(app, smActionListener, wmActionListener, "programs", 0);
        ///    if (programs->itemCount() > 0)
        addSubmenu(_("Programs"), 0, programs, "programs");
    }

    if (showRun) {
        if (runDlgCommand && runDlgCommand[0])
            addItem(_("_Run..."), -2, null, actionRun, "run");
    }

    if (itemCount() != oldItemCount) addSeparator();
    if (showWindowList)
        addItem(_("_Windows"), -2, actionWindowList, windowListMenu, "windows");

    if ( (!showTaskBar && showAbout) ||
        showHelp ||
        showFocusModeMenu ||
        showThemesMenu ||
        showSettingsMenu
    )
    if (showWindowList)
        addSeparator();

    if (!showTaskBar) {
        if (showAbout)
            addItem(_("_About"), -2, actionAbout, 0, "about");
    }

    if (showHelp) {
        HelpMenu *helpMenu =
            new HelpMenu(app, smActionListener, wmActionListener);
            addSubmenu(_("_Help"), -2, helpMenu, "help");
    }

    if (showSettingsMenu || showFocusModeMenu || showThemesMenu) {
        // When we have only 2 entries (focus + themes) then
        // it doesn't make sense to create a whole new YMenu.
        // Therefore we will reuse 'this' as value for settings.
        YMenu *settings = this; // new YMenu();

        if (showFocusModeMenu) {
            FocusMenu *focus = new FocusMenu();
            settings->addSubmenu(_("_Focus"), -2, focus, "focus");
        }

        if (showSettingsMenu) {
            PrefsMenu *prefs = new PrefsMenu();
            settings->addSubmenu(_("_Preferences"), -2, prefs, "pref");
        }

        if (showThemesMenu) {
            YMenu *themes = new ThemesMenu(app, smActionListener, wmActionListener);
            if (themes)
                settings->addSubmenu(_("_Themes"), -2, themes, "themes");
        }

        // Only add a menu if we created one:
        if (this != settings)
            addSubmenu(_("Se_ttings"), -2, settings, "settings");
    }

    if (logoutMenu) {
        addSeparator();
        if (showLogoutSubMenu)
            addItem(_("_Logout..."), -2, actionLogout, logoutMenu, "logout");
        else
            addItem(_("_Logout..."), -2, null, actionLogout, "logout");
    }
}

// vim: set sw=4 ts=4 et:

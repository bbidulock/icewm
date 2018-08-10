/*
 *  FDOmenu - Menu code generator for icewm
 *  Copyright (C) 2015-2018 Eduard Bloch
 *
 *  Inspired by icewm-menu-gnome2 and Freedesktop.org specifications
 *  Using pure glib/gio code and a built-in menu structure instead
 *  the XML based external definition (as suggested by FD.o specs)
 *
 *  Released under terms of the GNU Library General Public License
 *  (version 2.0)
 *
 *  2015/02/05: Eduard Bloch <edi@gmx.de>
 *  - initial version
 */

#include "config.h"
#include "base.h"
#include "sysdep.h"
#include "intl.h"
#include "appnames.h" // for QUOTE macro

char const *ApplicationName;

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gio/gdesktopappinfo.h>
#include "ycollections.h"

template<typename T, void TFreeFunc(T)>
struct auto_raii {
    T m_p;
    auto_raii(T xp) :
            m_p(xp) {
    }
    ~auto_raii() {
        if (m_p)
            TFreeFunc(m_p);
    }
};
typedef auto_raii<gpointer, g_free> auto_gfree;
typedef auto_raii<gpointer, g_object_unref> auto_gunref;

static int cmpUtf8(const void *p1, const void *p2) {
    return g_utf8_collate((const char *) p1, (const char *) p2);
}

typedef YVec<const gchar*> tCharVec;
tCharVec sys_folders, home_folders;
tCharVec* sys_home_folders[] = { &sys_folders, &home_folders, 0 };
tCharVec* home_sys_folders[] = { &home_folders, &sys_folders, 0 };

// for optional bits that are not consuming much and
// it's OK to leak them, OS will clean up soon enough
//#define FREEASAP
#ifdef FREEASAP
#define opt_g_free(x) g_free(x);
#else
#define opt_g_free(x)
#endif

struct tListMeta {
    const char *title, *key, *icon_path;
    bool description_found;
    GTree* store;

    static int cmpTitleUtf8(const void *a, const void *b) {
        tListMeta *A((tListMeta*) a), *B((tListMeta*) b);
        return cmpUtf8(A->title, B->title);
    }

    static int cmpTitle(const void *a, const void *b) {
        tListMeta *A((tListMeta*) a), *B((tListMeta*) b);
        return strcmp(A->title, B->title);
    }

    static int cmpCategoryThroughPtr(const void *a, const void *b) {
        tListMeta **A((tListMeta**) a), **B((tListMeta**) b);
        return strcmp((**A).key, (**B).key);
    }
};

tListMeta menuinfo[] =
{
    { N_("Accessibility"),"Accessibility", NULL, false,  NULL },
    { N_("Settings"),"Settings", NULL, false,  NULL },
    { N_("Screensavers"),"Screensavers", NULL, false,  NULL },
    { N_("Accessories"),"Accessories", NULL, false,  NULL },
    { N_("Development"),"Development", NULL, false,  NULL },
    { N_("Education"),"Education", NULL, false,  NULL },
    { N_("Games"),"Games", NULL, false,  NULL },
    { N_("Graphics"),"Graphics", NULL, false,  NULL },
    { N_("Multimedia"),"Multimedia", NULL, false,  NULL },
    { N_("Audio"),"Audio", NULL, false,  NULL },
    { N_("Video"),"Video", NULL, false,  NULL },
    { N_("AudioVideo"),"AudioVideo", NULL, false,  NULL },
    { N_("Network"),"Network", NULL, false,  NULL },
    { N_("Office"),"Office", NULL, false,  NULL },
    { N_("Science"),"Science", NULL, false,  NULL },
    { N_("System"),"System", NULL, false,  NULL },
    { N_("WINE"),"WINE", NULL, false,  NULL },
    { N_("Editors"),"Editors", NULL, false,  NULL },
    { N_("Utility"),"Utility", NULL, false,  NULL },
    { N_("Other"), "Other", NULL, false, NULL }
};

tListMeta* menuInfoRefsByCategory[ACOUNT(menuinfo)];
tListMeta* pOthers = &menuinfo[ACOUNT(menuinfo) - 1];

void add_menu_entry(const char* pName, tListMeta& tgt, gchar* menuCmd) {
    if (!tgt.store) {
        tgt.store = g_tree_new(cmpUtf8);
    }
    g_tree_replace(tgt.store, g_strdup(pName), menuCmd);
}

class tDesktopInfo {
public:
    GDesktopAppInfo *pInfo;
    tDesktopInfo(const char *szFileName) {
        pInfo = g_desktop_app_info_new_from_filename(szFileName);
        if (!pInfo)
            return;
// tear down if not permitted
        if (!g_app_info_should_show(*this)) {
            g_object_unref(pInfo);
            pInfo = 0;
            return;
        }
    }

    inline operator GAppInfo*() {
        return (GAppInfo*) pInfo;
    }

    inline operator GDesktopAppInfo*() {
        return pInfo;
    }

    ~tDesktopInfo() {
//        if (pInfo)            g_object_unref(pInfo);
        pInfo = 0;
    }

    const char *get_name() {
        if (!pInfo)
            return 0;
        return g_app_info_get_name(*this);
    }

    char * get_icon_path() {
        GIcon *pIcon = g_app_info_get_icon((GAppInfo*) pInfo);
        auto_gunref free_icon((GObject*) pIcon);

        if (pIcon) {
            char *icon_path = g_icon_to_string(pIcon);
            if (!icon_path)
                return 0;
            // if absolute then we are done here
            if (icon_path[0] == '/')
                return icon_path;
            // err, not owned! auto_gfree free_orig_icon_path(icon_path);

            // this is legacy code, and only partly rewritten
            // probably not needed anymore since icewm itself is capable of resolving best icon path now

#if 0
            if (G_IS_THEMED_ICON(pIcon))
            {
                static const char *pats[] = {
#if defined(CONFIG_LIBRSVG)
                    "%s/icons/hicolor/48x48/apps/%s.svg",
                    "%s/pixmaps/%s.svg",
#endif
#if defined(CONFIG_LIBPNG) || defined(CONFIG_CONFIG_GDK_PIXBUF_XLIB)
                    "%s/icons/hicolor/48x48/apps/%s.png",
                    "%s/pixmaps/%s.png",
#endif
#if defined(CONFIG_XPM) || defined(CONFIG_CONFIG_GDK_PIXBUF_XLIB)
                    "%s/pixmaps/%s.xpm",
#endif
                    NULL
                };
                for(tCharVec* x = home_sys_folders;*x;++x)
                {
                    for(auto y = x->data; y < x->data + x->size; ++y) {
                        for(auto z = pats; *z; ++z) {
#if 0
                            char *pathToFile = g_strdup_printf (z, icon_theme_name);
                            if (g_file_test (pathToFile, G_FILE_TEST_EXISTS)) {
                                return pathToFile;
                            } else {
                                g_free ((pathToFile));
                            }
#endif
                        }
                    }
#if 0
                    YVec<const char*> iconSearchOrder;
                    iconSearchOrder.add ("/usr/share/icons/hicolor/48x48/apps/%s.png");
                    iconSearchOrder.add ("/usr/share/pixmaps/%s.png");
                    iconSearchOrder.add ("/usr/share/pixmaps/%s.xpm");

                    for (const char **oneSearchPath =
                            iconSearchOrder.data; oneSearchPath <iconSearchOrder.data + iconSearchOrder.size;
                            ++oneSearchPath) {
                        char *pathToFile = g_strdup_printf (*oneSearchPath, icon_theme_name);
                        if (g_file_test (pathToFile, G_FILE_TEST_EXISTS)) {
                            return pathToFile;
                        } else {
                            g_free ((pathToFile));
                        }
                    }
                    return g_strdup ("noicon.png");
                }
#endif
            }

#endif
            return icon_path;
        }
        return 0;
    }
};

tListMeta* lookup_category(const char *key) {
    static tListMeta sample = { 0 };
    static tListMeta *pKey = &sample;
    sample.key = key;
    void* ret = bsearch(&pKey, menuInfoRefsByCategory,
            ACOUNT(menuInfoRefsByCategory), sizeof(menuInfoRefsByCategory[0]),
            tListMeta::cmpCategoryThroughPtr);
    if (!ret)
        return NULL;
    return *(tListMeta**) ret;
}

typedef void (*tFuncInsertInfo)(const char* szDesktopFile);
void pickup_folder_info(const char* szDesktopFile) {
    GKeyFile *kf = g_key_file_new();
    auto_raii<GKeyFile*, g_key_file_free> free_kf(kf);

    if (!g_key_file_load_from_file(kf, szDesktopFile, G_KEY_FILE_NONE, 0))
        return;
    const char* icon_name = g_key_file_get_string(kf, "Desktop Entry", "Icon",
            0);
    const char* cat_name = g_key_file_get_string(kf, "Desktop Entry", "Name",
            0);
    const char* cat_title = g_key_file_get_locale_string(kf, "Desktop Entry",
            "Name", NULL, NULL);
    tListMeta* pCat = lookup_category(cat_name);
    if (!pCat)
        return;
    pCat->description_found = true;
    // XXX: memory leak?
    pCat->icon_path = icon_name;
    pCat->title = cat_title;
}

void insert_app_info(const char* szDesktopFile) {
    tDesktopInfo dinfo(szDesktopFile);
    if (!dinfo.pInfo)
        return;

    const char *cmdraw = g_app_info_get_commandline(dinfo);
    if (!cmdraw || !*cmdraw)
        return;

    // if the strings contains the exe and then only file/url tags that we wouldn't
    // set anyway, THEN create a simplified version and use it later (if bSimpleCmd is true)
    // OR use the original command through a wrapper (if bSimpleCmd is false)
    bool bUseSimplifiedCmd = true;
    gchar * cmdMod = g_strdup(cmdraw);
    auto_gfree cmdfree(cmdMod);
    gchar *pcut = strpbrk(cmdMod, " \f\n\r\t\v");

    if (pcut) {
        bool bExpectXchar = false;
        for (gchar *p = pcut; *p && bUseSimplifiedCmd; ++p) {
            int c = (unsigned) *p;
            if (bExpectXchar) {
                if (strchr("FfuU", c))
                    bExpectXchar = false;
                else
                    bUseSimplifiedCmd = false;
                continue;
            } else if (c == '%') {
                bExpectXchar = true;
                continue;
            } else {
                if (isspace(unsigned(c)))
                    continue;
                else {
                    if (!strchr(p, '%'))
                        goto cmdMod_is_good_as_is;
                    else
                        bUseSimplifiedCmd = false;
                }
            }
        }

        if (bExpectXchar)
            bUseSimplifiedCmd = false;
        if (bUseSimplifiedCmd)
            *pcut = '\0';
        cmdMod_is_good_as_is: ;
    }

    const char* pName = dinfo.get_name();
    if (!pName)
        return;
    const char *pCats = g_desktop_app_info_get_categories(dinfo.pInfo);
    if (!pCats)
        pCats = "Other";
    if (0 == strncmp(pCats, "X-", 2))
        return;

    const char *sicon = "-";
    auto_gfree free_icon_path(0);
    gchar* real_path = dinfo.get_icon_path();
    if (real_path) {
        free_icon_path.m_p = real_path;
        sicon = real_path;
    }

    gchar *menuOutputLine;
    bool bForTerminal = false;
#if GLIB_VERSION_CUR_STABLE >= G_ENCODE_VERSION(2, 36)
    bForTerminal = g_desktop_app_info_get_boolean(dinfo, "Terminal");
#else
    // cannot check terminal property, callback is as safe bet
    bUseSimplifiedCmd = false;
#endif

    if (bUseSimplifiedCmd && !bForTerminal) // best case
        menuOutputLine = g_strjoin(" ", sicon, cmdMod, NULL);
#ifdef XTERMCMD
    else if (bForTerminal && bUseSimplifiedCmd)
    menuOutputLine = g_strjoin(" ", sicon, QUOTE(XTERMCMD), "-e", cmdMod, NULL);
#endif
    else
        // not simple command or needs a terminal started via launcher callback, or both
        menuOutputLine = g_strdup_printf("%s %s \"%s\"", sicon, ApplicationName,
                szDesktopFile);

    // Pigeonholing roughly by guessed menu structure

    gchar **ppCats = g_strsplit(pCats, ";", -1);

    int n = 0;
    for (gchar **pCatKey = ppCats; pCatKey && *pCatKey; ++pCatKey) {
        if (!**pCatKey)
            continue; // empty?
        tListMeta *pResolved = lookup_category(*pCatKey);
        if (!pResolved)
            continue;
        add_menu_entry(pName, *pResolved, menuOutputLine);
        n++;
    }
    if (!n) // no hit at all
        add_menu_entry(pName, *pOthers, menuOutputLine);
    g_strfreev(ppCats);

}

void proc_dir_rec(const char *syspath, unsigned depth,
        tFuncInsertInfo process_keyfile, const char *szSubfolder,
        const char *szFileSfx) {
    gchar *path = g_strjoin("/", syspath, szSubfolder, NULL);
    auto_gfree relmem_path(path);
    GDir *pdir = g_dir_open(path, 0, NULL);
    if (!pdir)
        return;
    auto_raii<GDir*, g_dir_close> dircloser(pdir);

    const gchar *szFilename(NULL);
    while (NULL != (szFilename = g_dir_read_name(pdir))) {
        if (!szFilename)
            continue;
        const char* pSfx = strrchr(szFilename, '.');
        if (!pSfx || 0 != strcmp(pSfx, szFileSfx))
            continue;

        gchar *szFullName = g_strjoin("/", path, szFilename, NULL);
        auto_gfree xxfree(szFullName);
        static GStatBuf buf;
        if (g_stat(szFullName, &buf))
            return;
        if (S_ISDIR(buf.st_mode)) {
            static ino_t reclog[6];
            for (unsigned i = 0; i < depth; ++i) {
                if (reclog[i] == buf.st_ino)
                    goto dir_visited_before;
            }
            if (depth < ACOUNT(reclog)) {
                reclog[++depth] = buf.st_ino;
                proc_dir_rec(szFullName, depth, process_keyfile, szSubfolder,
                        szFileSfx);
                --depth;
            }
            dir_visited_before: ;
        }

        if (!S_ISREG(buf.st_mode))
            continue;

        process_keyfile(szFullName);
    }
}

static gboolean printKey(const char *key, const char *value, void*) {
    printf("prog \"%s\" %s\n", key, value);
    return FALSE;
}

void print_submenu(const tListMeta& section) {

    if (!section.store || !g_tree_nnodes(section.store))
        return;
    printf("menu \"%s\" %s {\n", section.title,
            section.icon_path ? section.icon_path : "folder");
    g_tree_foreach(section.store, (GTraverseFunc) printKey, NULL);
    puts("}");
}

void dump_menu() {

    for (tListMeta *p = menuinfo; p < menuinfo + ACOUNT(menuinfo); ++p) {
        if (p == pOthers)
            puts("separator");
        print_submenu(*p);
    }
}

bool launch(const char *dfile, const char **argv, int argc) {
    GDesktopAppInfo *pInfo = g_desktop_app_info_new_from_filename(dfile);
    if (!pInfo)
        return false;
#if 0 // g_file_get_uri crashes, no idea why, even enforcing file prefix doesn't help
    if (argc>0)
    {
        GList* parms=NULL;
        for (int i=0; i<argc; ++i)
        parms=g_list_append(parms,
                g_strdup_printf("%s%s", strstr(argv[i], "://") ? "" : "file://",
                        argv[i]));
        return g_app_info_launch ((GAppInfo *)pInfo,
                parms, NULL, NULL);
    }
    else
#else
    (void) argv;
    (void) argc;
#endif
    return g_app_info_launch((GAppInfo *) pInfo,
    NULL,
    NULL, NULL);
}

static void init() {
#ifdef CONFIG_I18N
    setlocale(LC_ALL, "");
#endif

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    for (unsigned i = 0; i < ACOUNT(menuInfoRefsByCategory); ++i) {
        tListMeta& what = menuinfo[i];
#ifdef ENABLE_NLS
        what.title = gettext(what.title);
#endif
        menuInfoRefsByCategory[i] = &what;
    }
    qsort(menuInfoRefsByCategory, ACOUNT(menuInfoRefsByCategory),
            sizeof(menuInfoRefsByCategory[0]),
            tListMeta::cmpCategoryThroughPtr);
}

static void help(const char *home, const char *dirs, FILE* out, int xit) {
    g_fprintf(out,
            "This program doesn't use command line options. It only listens to\n"
                    "environment variables defined by XDG Base Directory Specification.\n"
                    "XDG_DATA_HOME=%s\n"
                    "XDG_DATA_DIRS=%s\n", home, dirs);
    exit(xit);
}

void split_folders(const char* path_string, tCharVec& where) {
    for (gchar** p = g_strsplit(path_string, ":", -1); *p; ++p) {
        where.add(*p);
    }
}

void process_apps(const tCharVec& where) {
    for (const gchar* const * p = where.data; p < where.data + where.size;
            ++p) {
        proc_dir_rec(*p, 0, insert_app_info, "applications", ".desktop");
    }
}

/**
 * @return True if all categories received description data
 */
void process_folders(const tCharVec& where) {
    for (const gchar* const * p = where.data; p < where.data + where.size;
            ++p) {
        proc_dir_rec(*p, 0, pickup_folder_info, "desktop-directories",
                ".directory");
#if 0
        // need to do anything?
#warning wasting a few cycles, replace with a counter check and chain second invocation upon success
        for (tListMeta* p=menuinfo; p<menuinfo+ACOUNT(menuinfo); ++p)
        if(!p->description_found) goto get_more_folder_descriptions;
        return;
        get_more_folder_descriptions:

        gchar *folderPath = g_strjoin("/", *p, "desktop-directories", NULL);
        auto_gfree clean_fpath(folderPath);
        // if extra data folder does not exist whatsoever, skip it!
        if (access(folderPath, X_OK))
        continue;

        bool bStillMissingData = true;
        for (size_t i = 0; i < ACOUNT(menuinfo); ++i)
        {
            if (menuinfo[i].description_found)
            continue;
            gchar *deskFilePath = g_strjoin("", folderPath, "/",
                    menuinfo[i].key, ".desktop", NULL);
            auto_gfree clean_deskpath(deskFilePath);
            if (0 == access(deskFilePath, X_OK))
            {
                menuinfo[i].description_found = true;
                tDesktopInfo info(deskFilePath);
                if (!info.pInfo)
                continue;
                bStillMissingData = false;
                const char *better_name = info.get_name();
                if (better_name)
                menuinfo[i].title = better_name;
                menuinfo[i].icon_path = info.get_icon_path();
            }
        }
        // found all extra descriptions?
        if (!bStillMissingData)
        break;
#endif
    }
}

int main(int argc, const char **argv) {
    ApplicationName = my_basename(argv[0]);

    init();

    const char * usershare = getenv("XDG_DATA_HOME");
    if (!usershare || !*usershare)
        usershare = g_strjoin(NULL, getenv("HOME"), "/.local/share", NULL);

    // system dirs, either from environment or from static locations
    const char *sysshare = getenv("XDG_DATA_DIRS");
    if (!sysshare || !*sysshare)
        sysshare = "/usr/local/share:/usr/share";

    if (argc > 1) {
        if (is_version_switch(argv[1]))
            print_version_exit(VERSION);
        if (is_help_switch(argv[1]))
            help(usershare, sysshare, stdout, EXIT_SUCCESS);

        if (strstr(argv[1], ".desktop") && launch(argv[1], argv + 2, argc - 2))
            return EXIT_SUCCESS;

        help(usershare, sysshare, stderr, EXIT_FAILURE);
    }
    split_folders(sysshare, sys_folders);
    split_folders(usershare, home_folders);

    process_apps(sys_folders);
    process_apps(home_folders);
    process_folders(sys_folders);
    process_folders(home_folders);

    // sort but leave the last entry (Other) where it is!
    qsort(menuinfo, ACOUNT(menuinfo) - 1, sizeof(menuinfo[0]),
            tListMeta::cmpTitleUtf8);

    dump_menu();

    return EXIT_SUCCESS;
}

// vim: set sw=4 ts=4 et:

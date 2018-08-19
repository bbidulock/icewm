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
#include <gmodule.h>
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

class tDesktopInfo;
typedef YVec<const gchar*> tCharVec;
tCharVec sys_folders, home_folders;
tCharVec* sys_home_folders[] = { &sys_folders, &home_folders, 0 };
tCharVec* home_sys_folders[] = { &home_folders, &sys_folders, 0 };

bool add_sep_before(false), add_sep_after(false), no_sep_others(false);

struct tListMeta {
    const char *title, *key, *icon, *parent_sec;

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
GHashTable* meta_lookup_data;
#define lookup_category(key) ((tListMeta*) g_hash_table_lookup(meta_lookup_data, key))


struct t_menu_node;

// very basic, avoid vtables!
struct t_menu_node {
protected:
    // for leafs -> NULL, otherwise sub-menu contents
    GTree* store;
    const char *progCmd;
public:
    tListMeta *meta;
    t_menu_node(tListMeta* desc): store(0), progCmd(0), meta(desc) {}

    struct t_print_meta {
        int count, level;
        t_menu_node* print_separated;
    };
    static gboolean print_node(gpointer key, gpointer value, gpointer pr_meta) {
        ((t_menu_node*) value)->print((t_print_meta*) pr_meta);
        return FALSE;
    }

    void print(t_print_meta *ctx) {
        if(!meta) return;
        const char* title = Elvis(meta->title, meta->key);

        if(!store)
        {
            printf("prog \"%s\" %s %s\n",
                    title,
                    meta->icon,
                    progCmd);
            ctx->count++;
            return;
        }

        if (!store || !g_tree_nnodes(store))
            return;
        // some gimmicks on the top level
        if (ctx->level == 0 && add_sep_before) {
            puts("separator");
            add_sep_before = false;
        }

        if (ctx->level == 1 && !no_sep_others
                && 0 == strcmp(meta->key, "Other")) {
            ctx->print_separated = this;
            return;
        }

        // root level does not have a name, for others open category menu
        if (ctx->level > 0) {
            printf("menu \"%s\" %s {\n", meta->title,
                    meta->icon ? meta->icon : "folder");
        }
        ctx->level++;
        g_tree_foreach(store, print_node, ctx);
        if(ctx->level == 1 && ctx->print_separated)
        {
            puts("separator");
            ctx->print_separated->print(ctx);
        }
        ctx->level--;
        if (ctx->level > 0)
            puts("}");
    }

    /**
     * Usual print method for the root node
     */
    void print() {
        t_print_meta ctx = {0,0,0};
        print(&ctx);
    }


    void add(t_menu_node* node) {
        if (!store)
            store = g_tree_new(cmpUtf8);
        //const char* name = node->meta->title ? node->meta->title : node->meta->key;
        g_tree_replace(store, (gpointer) Elvis(node->meta->title, node->meta->key), (gpointer) node);
    }

    /**
     * Returns a sub-menu which is named by the title (or key) of the provided information.
     * Creates one as needed or finds existing one.
     */
    t_menu_node* get_subtree(tListMeta* info) {
        const char* title = Elvis(info->title, info->key);
        if (store) {
            void* existing = g_tree_lookup(store, title);
            if (existing)
                return (t_menu_node*) existing;
        }
        t_menu_node* tree = new t_menu_node(info);
        add(tree);
        return tree;
    }

    void add_by_categories(t_menu_node* pNode, gchar **ppCats) {
        t_menu_node* pSubmenu(0);

        for (gchar **pCatKey = ppCats; pCatKey && *pCatKey; ++pCatKey) {
            if (!**pCatKey)
                continue; // empty?
            tListMeta *pResolved = lookup_category(*pCatKey);
            if (pResolved && !pResolved->parent_sec) {
                // valid main category
                pSubmenu = get_subtree(pResolved);
                pSubmenu->add(pNode);
            }
        }
        if (!pSubmenu) // no hit at all?
        {
            pSubmenu = get_subtree(lookup_category("Other"));
            pSubmenu->add(pNode);
        }
    }
};

tListMeta menuinfo[] =
{
    { N_("Accessibility"),"Accessibility", NULL, NULL},
    { N_("Settings"),"Settings", NULL, NULL},
    { N_("Screensavers"),"Screensavers", NULL, NULL},
    { N_("Accessories"),"Accessories", NULL, NULL},
    { N_("Development"),"Development", NULL, NULL},
    { N_("Education"),"Education", NULL, NULL},
    { N_("Games"),"Games", NULL, NULL},
    { N_("Graphics"),"Graphics", NULL, NULL},
    { N_("Multimedia"),"Multimedia", NULL, NULL},
    { N_("Audio"),"Audio", NULL, NULL},
    { N_("Video"),"Video", NULL, NULL},
    { N_("AudioVideo"),"AudioVideo", NULL, NULL},
    { N_("Network"),"Network", NULL, NULL},
    { N_("Office"),"Office", NULL, NULL},
    { N_("Science"),"Science", NULL, NULL},
    { N_("System"),"System", NULL, NULL},
    { N_("WINE"),"WINE", NULL, NULL},
    { N_("Editors"),"Editors", NULL, NULL},
    { N_("Utility"),"Utility", NULL, NULL},
    { N_("Other"), "Other", NULL, NULL }
// XXX: add subsections here
};

bool checkSuffix(const char* szFilename, const char* szFileSfx)
{
    const char* pSfx = strrchr(szFilename, '.');
    return pSfx && 0 == strcmp(pSfx+1, szFileSfx);
}

// for short-living objects describing the information we get from desktop files
class tDesktopInfo {
public:
    GDesktopAppInfo *pInfo;
    const char *d_file;
    tDesktopInfo(const char *szFileName) : d_file(szFileName)  {
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

    const char *get_name() const {
        if (!pInfo)
            return 0;
        return g_app_info_get_display_name((GAppInfo*) pInfo);
    }

    char * get_icon_path() const {
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
            return icon_path;
        }
        return 0;
    }
};

tListMeta no_description = {0,0,0,0};

t_menu_node root(&no_description);

// variant with local description data
struct t_menu_node_app : t_menu_node
{
    tListMeta description;
    t_menu_node_app(const tDesktopInfo& dinfo) : t_menu_node(&description),
            description(no_description) {
        description.icon = Elvis((const char*) dinfo.get_icon_path(), "-");

        const char *cmdraw = g_app_info_get_commandline((GAppInfo*) dinfo.pInfo);
        if (!cmdraw || !*cmdraw)
            return;

        description.title = description.key = Elvis(dinfo.get_name(), "<UNKNOWN>");

        // if the strings contains the exe and then only file/url tags that we wouldn't
        // set anyway, THEN create a simplified version and use it later (if bSimpleCmd is true)
        // OR use the original command through a wrapper (if bSimpleCmd is false)
        bool bUseSimplifiedCmd = true;
        gchar * cmdMod = g_strdup(cmdraw);
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

        bool bForTerminal = false;
    #if GLIB_VERSION_CUR_STABLE >= G_ENCODE_VERSION(2, 36)
        bForTerminal = g_desktop_app_info_get_boolean(dinfo.pInfo, "Terminal");
    #else
        // cannot check terminal property, callback is as safe bet
        bUseSimplifiedCmd = false;
    #endif

        if (bUseSimplifiedCmd && !bForTerminal) // best case
            progCmd = cmdMod;
    #ifdef XTERMCMD
        else if (bForTerminal && bUseSimplifiedCmd)
            pNode->progCmd = g_strjoin(" ", QUOTE(XTERMCMD), "-e", cmdMod, NULL);
    #endif
        else
            // not simple command or needs a terminal started via launcher callback, or both
            progCmd = g_strdup_printf("%s \"%s\"", ApplicationName, dinfo.d_file);
    }
};

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
    pCat->icon = icon_name;
    pCat->title = cat_title;
}

void insert_app_info(const char* szDesktopFile) {
    tDesktopInfo dinfo(szDesktopFile);
    if (!dinfo.pInfo)
        return;

    const char *pCats = g_desktop_app_info_get_categories(dinfo.pInfo);
    if (!pCats)
        pCats = "Other";
    if (0 == strncmp(pCats, "X-", 2))
        return;

    t_menu_node* pNode = new t_menu_node_app(dinfo);
    // Pigeonholing roughly by guessed menu structure

    gchar **ppCats = g_strsplit(pCats, ";", -1);
    root.add_by_categories(pNode, ppCats);
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
        if (!szFilename || !checkSuffix(szFilename, szFileSfx))
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

    meta_lookup_data = g_hash_table_new(g_str_hash, g_str_equal);

    for (unsigned i = 0; i < ACOUNT(menuinfo); ++i) {
        tListMeta& what = menuinfo[i];
#ifdef ENABLE_NLS
        // internal translations just for the main sections
        if(!what.parent_sec)
            what.title = gettext(what.title);
#endif
        // enforce non-const since we are not destroying that data ever, no key_destroy_func set!
        g_hash_table_insert(meta_lookup_data, (gpointer) what.key, &what);
    }
}

static void help(const char *home, const char *dirs, FILE* out, int xit) {
    g_fprintf(out,
            "USAGE: icewm-menu-fdo OPTIONS\n"
            "OPTIONS:\n"
            "--seps  \tPrint separators before and after contents\n"
            "--sep-after\tPrint separator only after contents\n"
            "--no-sep-others\tNo separation of the 'Others' menu point\n"
            "*.desktop\tAny .desktop file to launch the application command from there\n"
            "This program also listens to "
                    "environment variables defined by the\nXDG Base Directory Specification:\n"
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
        proc_dir_rec(*p, 0, insert_app_info, "applications", "desktop");
    }
}

/**
 * @return True if all categories received description data
 */
void load_folder_descriptions(const tCharVec& where) {
    for (const gchar* const * p = where.data; p < where.data + where.size;
            ++p) {
        proc_dir_rec(*p, 0, pickup_folder_info, "desktop-directories",
                "directory");
    }
}

int main(int argc, const char **argv) {
    ApplicationName = my_basename(argv[0]);

    const char * usershare = getenv("XDG_DATA_HOME");
    if (!usershare || !*usershare)
        usershare = g_strjoin(NULL, getenv("HOME"), "/.local/share", NULL);

    // system dirs, either from environment or from static locations
    const char *sysshare = getenv("XDG_DATA_DIRS");
    if (!sysshare || !*sysshare)
        sysshare = "/usr/local/share:/usr/share";

    for(const char **pArg = argv+1; pArg<argv+argc; ++pArg)
    {
        if (is_version_switch(*pArg))
            print_version_exit(VERSION);
        if (is_help_switch(*pArg))
            help(usershare, sysshare, stdout, EXIT_SUCCESS);
        if(is_long_switch(*pArg, "seps"))
        {
            add_sep_before = add_sep_after = true;
            continue;
        }
        if(is_long_switch(*pArg, "sep-before"))
        {
            add_sep_before = true;
            continue;
        }
        if(is_long_switch(*pArg, "sep-after"))
        {
            add_sep_after = true;
            continue;
        }
        if(is_long_switch(*pArg, "no-sep-others"))
        {
            no_sep_others = true;
            continue;
        }
        if (checkSuffix(argv[1], "desktop") && launch(argv[1], argv + 2, argc - 2))
            return EXIT_SUCCESS;
        else // unknown option?
            help(usershare, sysshare, stderr, EXIT_FAILURE);
    }

    init();
    split_folders(sysshare, sys_folders);
    split_folders(usershare, home_folders);

    load_folder_descriptions(sys_folders);
    load_folder_descriptions(home_folders);

    process_apps(sys_folders);
    process_apps(home_folders);

    root.print();

    return EXIT_SUCCESS;
}

// vim: set sw=4 ts=4 et:

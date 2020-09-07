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
 *  2018/08:
 *  - overhauled program design and menu construction code, added sub-category handling
 */

#include "config.h"
#include "base.h"
#include "sysdep.h"
#include "intl.h"
#include "appnames.h"
#include <ctype.h>
#include "debug.h"

char const *ApplicationName(nullptr);

#ifndef LPCSTR // mind the MFC
// easier to read...
typedef const char* LPCSTR;
#endif

#include <glib.h>
#include <gmodule.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gio/gdesktopappinfo.h>
#include "ycollections.h"

// program options
bool add_sep_before(false), add_sep_after(false), no_sep_others(false), no_sub_cats(false);

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
    return g_utf8_collate((LPCSTR ) p1, (LPCSTR ) p2);
}

class tDesktopInfo;
typedef YVec<const gchar*> tCharVec;
tCharVec sys_folders, home_folders;
tCharVec* sys_home_folders[] = { &sys_folders, &home_folders, nullptr };
tCharVec* home_sys_folders[] = { &home_folders, &sys_folders, nullptr };

struct tListMeta {
    LPCSTR title, key, icon;
    LPCSTR const * const parent_sec;
    enum eLoadFlags {
        NONE = 0,
        DEFAULT_TRANSLATED = 1,
        SYSTEM_TRANSLATED = 2,
        DEFAULT_ICON = 4,
        FALLBACK_ICON = 8,
        SYSTEM_ICON = 16,
    };
    short load_state_icon, load_state_title;
};
GHashTable* meta_lookup_data;
tListMeta* lookup_category(LPCSTR key)
{
    tListMeta* ret = (tListMeta*) g_hash_table_lookup(meta_lookup_data, key);
    if(ret)
    {
        // auto-translate the default title for the user's language
        if(ret->title == nullptr)
            ret->title = _(ret->key);
    }
    return ret;
}



struct t_menu_node;
extern t_menu_node root;

// very basic, avoid vtables!
struct t_menu_node {
protected:
    // for leafs -> NULL, otherwise sub-menu contents
    GTree* store;
    LPCSTR progCmd;
public:
    const tListMeta *meta;
    t_menu_node(const tListMeta* desc): store(nullptr), progCmd(nullptr), meta(desc) {}

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
        LPCSTR title = Elvis(meta->title, meta->key);

        if(!store)
        {
            if(title && progCmd) {
                if(ctx->count == 0 && add_sep_before)
                    puts("separator");
                printf("prog \"%s\" %s %s\n",
                        title,
                        meta->icon,
                        progCmd);
            }
            ctx->count++;
            return;
        }

        if (!g_tree_nnodes(store))
            return;

        if (ctx->level == 1 && !no_sep_others
                && 0 == strcmp(meta->key, "Other")) {
            ctx->print_separated = this;
            return;
        }

        // root level does not have a name, for others open category menu
        if (ctx->level > 0) {
            if (ctx->count == 0 && add_sep_before)
                puts("separator");
            ctx->count++;
            printf("menu \"%s\" %s {\n", title, meta->icon);
        }
        ctx->level++;
        g_tree_foreach(store, print_node, ctx);
        if(ctx->level == 1 && ctx->print_separated)
        {
            puts("separator");
            no_sep_others = true;
            ctx->print_separated->print(ctx);
        }
        ctx->level--;
        if (ctx->level > 0)
#ifndef DEBUG
            puts("}");
#else
            printf("# end of menu \"%s\"\n}\n", title);
#endif
        if(add_sep_after && ctx->level == 0 && ctx->count > 0)
            puts("separator");

    }

    /**
     * Usual print method for the root node
     */
    void print() {
        t_print_meta ctx = {0,0,nullptr};
        print(&ctx);
    }

    void add(t_menu_node* node) {
        if (!store)
            store = g_tree_new(cmpUtf8);
        if(node->meta->title || node->meta->key)
            g_tree_replace(store, (gpointer) Elvis(node->meta->title, node->meta->key), (gpointer) node);
    }

    /**
     * Returns a sub-menu which is named by the title (or key) of the provided information.
     * Creates one as needed or finds existing one.
     */
    t_menu_node* get_subtree(const tListMeta* info) {
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

    /**
     * Find and examine the possible subcategory, try to assign it to the particular main category with the proper structure.
     * When succeeded, blank out the main category pointer in matched_main_cats.
     */
    void try_add_to_subcat(t_menu_node* pNode, const tListMeta* subCatCandidate, YVec<tListMeta*> &matched_main_cats)
    {
        t_menu_node *pTree = &root;
        // skip the rest of the further nesting (cannot fit into any)
        bool skipping = false;

        tListMeta* pNewCatInfo = nullptr;
        tListMeta** ppLastMainCat = nullptr;

        for (const char * const *pSubCatName = subCatCandidate->parent_sec;
                *pSubCatName; ++pSubCatName) {
            // stop nesting, add to the last visited/created submenu
            bool store_here = **pSubCatName == '|';
            if (skipping && store_here) {
                skipping = false;
                pTree = &root;
                continue;
            }
            if (store_here) {
                pTree->get_subtree(subCatCandidate)->add(pNode);
                // main menu was served, don't come here again
                if (ppLastMainCat)
                    *ppLastMainCat = nullptr;
            }

            skipping = true;
            // check main category filter, enter from root for main categories

            for (tListMeta** ppMainCat = matched_main_cats.data;
                    ppMainCat < matched_main_cats.data + matched_main_cats.size;
                    ++ppMainCat) {

                if (!*ppMainCat)
                    continue;

                // empty filter means ANY
                if (**pSubCatName == '\0'
                        || 0 == strcmp(*pSubCatName, (**ppMainCat).key)) {
                    // the category is enabled!
                    skipping = false;
                    pTree = &root;
                    pNewCatInfo = *ppMainCat;
                    ppLastMainCat = ppMainCat;
                    break;
                }
            }
            if (skipping)
                continue;
            // if not on first node, make or find submenues for the comming tokens
            if(!pNewCatInfo)
                pNewCatInfo = lookup_category(*pSubCatName);
            if(!pNewCatInfo)
                return; // heh? fantasy category? Let caller handle it
            pTree = pTree->get_subtree(pNewCatInfo);
        }
    }
    void add_by_categories(t_menu_node* pNode, gchar **ppCats) {
        static YVec<tListMeta*> matched_main_cats, matched_sub_cats;
        matched_main_cats.size = matched_sub_cats.size = 0;

        for (gchar **pCatKey = ppCats; pCatKey && *pCatKey; ++pCatKey) {
            if (!**pCatKey)
                continue; // empty?
            tListMeta *pResolved = lookup_category(*pCatKey);
            if (!pResolved) continue;
            if(!pResolved->parent_sec)
                matched_main_cats.add(pResolved);
            else
                matched_sub_cats.add(pResolved);
        }
        if (matched_main_cats.size == 0)
            matched_main_cats.add(lookup_category("Other"));
        if(!no_sub_cats) {
            for (tListMeta** p = matched_sub_cats.data;
                    p < matched_sub_cats.data + matched_sub_cats.size; ++p) {

                try_add_to_subcat(pNode, *p, matched_main_cats);
            }
        }
        for (tListMeta** p = matched_main_cats.data;
                p < matched_main_cats.data + matched_main_cats.size; ++p) {

            if (*p == nullptr)
                continue;
            get_subtree(*p)->add(pNode);
        }
    }
};

/*
 * Two relevant columns from https://specifications.freedesktop.org/menu-spec/latest/apas02.html
 * exported as CSV with , delimiter and with manual fix of HardwareSettings order.
 *
 * Powered by PERL! See contrib/conv_cat.pl
 */

#include "fdospecgen.h"

bool checkSuffix(const char* szFilename, const char* szFileSfx)
{
    const char* pSfx = strrchr(szFilename, '.');
    return pSfx && 0 == strcmp(pSfx+1, szFileSfx);
}

// for short-living objects describing the information we get from desktop files
class tDesktopInfo {
public:
    GDesktopAppInfo *pInfo;
    LPCSTR d_file;
    tDesktopInfo(LPCSTR szFileName) : d_file(szFileName)  {
        pInfo = g_desktop_app_info_new_from_filename(szFileName);
        if (!pInfo)
            return;
// tear down if not permitted
        if (!g_app_info_should_show(*this)) {
            g_object_unref(pInfo);
            pInfo = nullptr;
            return;
        }
    }

    inline operator GAppInfo*() {
        return (GAppInfo*) pInfo;
    }

    inline operator GDesktopAppInfo*() {
        return pInfo;
    }

    ~tDesktopInfo() { }

    LPCSTR get_name() const {
        if (!pInfo)
            return nullptr;
        return g_app_info_get_display_name((GAppInfo*) pInfo);
    }

    char * get_icon_path() const {
        GIcon *pIcon = g_app_info_get_icon((GAppInfo*) pInfo);
        auto_gunref free_icon((GObject*) pIcon);

        if (pIcon) {
            char *icon_path = g_icon_to_string(pIcon);
            if (!icon_path)
                return nullptr;
            // if absolute then we are done here
            if (icon_path[0] == '/')
                return icon_path;
            // err, not owned! auto_gfree free_orig_icon_path(icon_path);
            return icon_path;
        }
        return nullptr;
    }
};

tListMeta no_description = {nullptr,nullptr,nullptr,nullptr};

t_menu_node root(&no_description);

// variant with local description data
struct t_menu_node_app : t_menu_node
{
    tListMeta description;
    t_menu_node_app(const tDesktopInfo& dinfo) : t_menu_node(&description),
            description(no_description) {
        description.icon = Elvis((const char*) dinfo.get_icon_path(), "-");

        LPCSTR cmdraw = g_app_info_get_commandline((GAppInfo*) dinfo.pInfo);
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
            progCmd = g_strjoin(" ", QUOTE(XTERMCMD), "-e", cmdMod, NULL);
    #endif
        else
            // not simple command or needs a terminal started via launcher callback, or both
            progCmd = g_strdup_printf("%s \"%s\"", ApplicationName, dinfo.d_file);
    }
};

struct tFromTo { LPCSTR from; LPCSTR to;};
// match transformations applied by some DEs
const tFromTo SameCatMap[] = {
        {"Utilities", "Utility"},
        {"Sound & Video", "AudioVideo"},
        {"Sound", "Audio"},
        {"File", "FileTools"},
        {"Terminal Applications", "TerminalEmulator"},
        {"Mathematics", "Math"},
        {"Arcade", "ArcadeGame"},
        {"Card Games", "CardGame"}
};
const tFromTo SameIconMap[] = {
        { "AudioVideo", "Audio" },
        { "AudioVideo", "Video" }
};
typedef void (*tFuncInsertInfo)(LPCSTR szDesktopFile);
void pickup_folder_info(LPCSTR szDesktopFile) {
    GKeyFile *kf = g_key_file_new();
    auto_raii<GKeyFile*, g_key_file_free> free_kf(kf);

    if (!g_key_file_load_from_file(kf, szDesktopFile, G_KEY_FILE_NONE, nullptr))
        return;
    LPCSTR cat_name = g_key_file_get_string(kf, "Desktop Entry", "Name", nullptr);
    // looks like bad data
    if(!cat_name || !*cat_name)
        return;
    // try a perfect match by name or file name
    tListMeta* pCat = lookup_category(cat_name);
    if(!pCat)
    {
        gchar* bn(g_path_get_basename(szDesktopFile));
        auto_gfree cleanr(bn);
        char* dot = strchr(bn, '.');
        if(dot)
        {
            *dot = 0x0;
            pCat = lookup_category(bn);
        }
    }
    for(const tFromTo* p=SameCatMap;
            !pCat && p < SameCatMap+ACOUNT(SameCatMap);
            ++p) {

        if(0 == strcmp(cat_name, p->from))
            pCat = lookup_category(p->to);
    }

    if(!pCat)
        return;
    if (pCat->load_state_icon < tListMeta::SYSTEM_ICON) {
        LPCSTR icon_name = g_key_file_get_string (kf, "Desktop Entry",
                                                       "Icon", nullptr);
        if (icon_name && *icon_name) {
            pCat->icon = icon_name;
            pCat->load_state_icon = tListMeta::SYSTEM_ICON;
        }
    }
    if (pCat->load_state_title < tListMeta::SYSTEM_TRANSLATED) {
        LPCSTR cat_title = g_key_file_get_locale_string (kf,
                                                              "Desktop Entry",
                                                              "Name", nullptr,
                                                              nullptr);
        if (!cat_title) return;
        pCat->title = cat_title;
        LPCSTR cat_title_c = g_key_file_get_string (kf, "Desktop Entry",
                                                         "Name", nullptr);
        bool same_trans = 0 == strcmp (cat_title_c, cat_title);
        if (!same_trans) pCat->load_state_title = tListMeta::SYSTEM_TRANSLATED;
        // otherwise: not sure, keep searching for a better translation
    }
    // something special, donate the icon to similar items unless they have a better one

    for(const tFromTo* p=SameIconMap; p < SameIconMap + ACOUNT(SameIconMap);
            ++p) {

        if (strcmp (pCat->key, p->from))
            continue;

        tListMeta *t = lookup_category (p->to);
        // give them at least some icon for now
        if (t && t->load_state_icon < tListMeta::FALLBACK_ICON) {
            t->icon = pCat->icon;
            t->load_state_icon = tListMeta::FALLBACK_ICON;
        }

    }
}

void insert_app_info(const char* szDesktopFile) {
    tDesktopInfo dinfo(szDesktopFile);
    if (!dinfo.pInfo)
        return;

    LPCSTR pCats = g_desktop_app_info_get_categories(dinfo.pInfo);
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

void proc_dir_rec(LPCSTR syspath, unsigned depth,
        tFuncInsertInfo process_keyfile, LPCSTR szSubfolder,
        LPCSTR szFileSfx) {
    gchar *path = g_strjoin("/", syspath, szSubfolder, NULL);
    auto_gfree relmem_path(path);
    GDir *pdir = g_dir_open(path, 0, nullptr);
    if (!pdir)
        return;
    auto_raii<GDir*, g_dir_close> dircloser(pdir);

    const gchar *szFilename(nullptr);
    while (nullptr != (szFilename = g_dir_read_name(pdir))) {
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

bool launch(LPCSTR dfile, LPCSTR *argv, int argc) {
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
    nullptr,
    nullptr, nullptr);
}

static void init() {
#ifdef CONFIG_I18N
    setlocale(LC_ALL, "");
#endif

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    meta_lookup_data = g_hash_table_new(g_str_hash, g_str_equal);

    for (unsigned i = 0; i < ACOUNT(spec::menuinfo); ++i) {
        tListMeta& what = spec::menuinfo[i];
        if(no_sub_cats && what.parent_sec)
            continue;
        // enforce non-const since we are not destroying that data ever, no key_destroy_func set!
        g_hash_table_insert(meta_lookup_data, (gpointer) what.key, &what);
    }
}

static void help(LPCSTR home, LPCSTR dirs, FILE* out, int xit) {
    g_fprintf(out,
            "USAGE: icewm-menu-fdo OPTIONS\n"
            "OPTIONS:\n"
            "--seps  \tPrint separators before and after contents\n"
            "--sep-after\tPrint separator only after contents\n"
            "--no-sep-others\tNo separation of the 'Others' menu point\n"
            "--no-sub-cats\tNo additional subcategories, just one level of menues\n"
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

#ifdef DEBUG_xxx
void dbgPrint(const gchar *msg)
{
    tlog("%s", msg);
}
#endif

int main(int argc, LPCSTR *argv) {
    ApplicationName = my_basename(argv[0]);

#if !GLIB_CHECK_VERSION(2,36,0)
    g_type_init();
#endif

#ifdef DEBUG_xxx
    // XXX: if enabled, one probably also need to enable debug facilities in its code, like
    // what --debug option does
    g_set_printerr_handler(dbgPrint);
    g_set_print_handler(dbgPrint);
#endif

    LPCSTR usershare = getenv("XDG_DATA_HOME");
    if (!usershare || !*usershare)
        usershare = g_strjoin(nullptr, getenv("HOME"), "/.local/share", NULL);

    // system dirs, either from environment or from static locations
    LPCSTR sysshare = getenv("XDG_DATA_DIRS");
    if (!sysshare || !*sysshare)
        sysshare = "/usr/local/share:/usr/share";

    if (argc == 2 && checkSuffix(argv[1], "desktop")
            && launch(argv[1], argv + 2, argc - 2)) {
        return EXIT_SUCCESS;
    }

    for (LPCSTR *pArg = argv + 1; pArg < argv + argc; ++pArg) {
        if (is_version_switch(*pArg))
            print_version_exit(VERSION);
        if (is_copying_switch(*pArg))
            print_copying_exit();
        if (is_help_switch(*pArg))
            help(usershare, sysshare, stdout, EXIT_SUCCESS);
        if (is_long_switch(*pArg, "seps")) {
            add_sep_before = add_sep_after = true;
            continue;
        }
        if (is_long_switch(*pArg, "sep-before")) {
            add_sep_before = true;
            continue;
        }
        if (is_long_switch(*pArg, "sep-after")) {
            add_sep_after = true;
            continue;
        }
        if (is_long_switch(*pArg, "no-sep-others")) {
            no_sep_others = true;
            continue;
        }
        if (is_long_switch(*pArg, "no-sub-cats")) {
            no_sub_cats = true;
            continue;
        }
        // unknown option?
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

/*
 *  FDOmenu - Menu code generator for icewm
 *  Copyright (C) 2015-2024 Eduard Bloch
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
 *  - overhauled program design and menu construction code, added sub-category
 * handling
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "appnames.h"
#include "base.h"
#include "config.h"
#include "intl.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <set>
#include <string>
#include <utility> // For std::move
#include <vector>

#include <functional>
#include <initializer_list>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#include "fdospecgen.h"

char const *ApplicationName;

#ifdef DEBUG
#define DBGMSG(x) cerr << x << endl;
#else
#define DBGMSG(x)
#endif

// program options
bool no_sep_others = false;
bool no_sub_cats = false;
bool generic_name = false;
bool right_to_left = false;
bool flat_output = false;
bool match_in_section = false;
bool match_in_section_only = false;
bool no_only_child = false;
bool no_only_child_hint = false;
bool add_comments = false;

auto substr_filter = "";
auto substr_filter_nocase = "";

unsigned prog_name_cut = 0;

// use a more visually appealing default with TTF fonts
#ifdef CONFIG_COREFONTS
#define ellipsis "..."
auto flat_sep = " / ";
#else
#define ellipsis "\xe2\x80\xa6"
auto flat_sep = " • ";
#endif

char *terminal_command;
char *terminal_option;

// global defaults and helpers
static const string ICON_FOLDER("folder"), OTH("Other");
string indent_hint("");
// use this to dump optional comment data, deque is pointer-stable
deque<string> comment_pool;
set<string> valid_main_cats;

/*
 * Certain parts borrowed from apt-cacher-ng by its autor, either from older
 * branches (C++11 compatible) or development branch.
 */

#define SPACECHARS " \f\n\r\t\v"

#define startsWith(where, what) (0 == (where).compare(0, (what).size(), (what)))
#define startsWithSz(where, what)                                              \
    (0 == (where).compare(0, sizeof((what)) - 1, (what)))
#define endsWith(where, what)                                                  \
    ((where).size() >= (what).size() &&                                        \
     0 == (where).compare((where).size() - (what).size(), (what).size(),       \
                          (what)))
#define endsWithSzAr(where, what)                                              \
    ((where).size() >= (sizeof((what)) - 1) &&                                 \
     0 == (where).compare((where).size() - (sizeof((what)) - 1),               \
                          (sizeof((what)) - 1), (what)))
inline void trimBack(string &s, const char *junk = SPACECHARS) {
    auto pos = s.find_last_not_of(junk);
    s.erase(pos + 1);
}

/**
 * Container helpers and adaptors
 */

struct tLessOp4Localized {
    std::locale loc; // default locale
    const std::collate<char> &coll = std::use_facet<std::collate<char>>(loc);
    bool operator()(const std::string &a, const std::string &b) {
        return coll.compare(a.data(), a.data() + a.size(), b.data(),
                            b.data() + b.size()) < 0;
    }
};
tLessOp4Localized *loc_comper;

struct tLessOp4LocalizedDeref {
    std::locale loc; // default locale
    const std::collate<char> &coll = std::use_facet<std::collate<char>>(loc);
    bool operator()(const std::string *a, const std::string *b) {
        return coll.compare(a->data(), a->data() + a->size(), b->data(),
                            b->data() + b->size()) < 0;
    }
};

template <typename T> struct lessByDerefAdaptor {
    bool operator()(const T *a, const T *b) { return *a < *b; }
};

template <typename T> const T &iback(const initializer_list<T> &q) {
    return *(q.end() - 1);
}

/**
 * Basic base implementation of a reference-counted class
 */
struct tLintRefcounted {
  private:
    size_t m_nRefCount = 0;

  public:
    inline void __inc_ref() noexcept { m_nRefCount++; }
    inline void __dec_ref() {
        if (--m_nRefCount == 0)
            delete this;
    }
    virtual ~tLintRefcounted() {}
    inline size_t __ref_cnt() { return m_nRefCount; }
};

/**
 * Lightweight intrusive smart pointer with ordinary reference counting
 */
template <class T> class lint_ptr {
    T *m_ptr = nullptr;

  public:
    explicit lint_ptr() {}
    /**
     * @brief lint_ptr Captures the pointer and ensures that it's released when
     * the refcount goes to zero, unless initialyTakeRef is set to false. If
     * initialyTakeRef is false, the operation is asymmetric, i.e. one extra
     * __dec_ref operation will happen in the end.
     *
     * @param rawPtr
     * @param initialyTakeRef
     */
    explicit lint_ptr(T *rawPtr, bool initialyTakeRef = true) : m_ptr(rawPtr) {
        if (rawPtr && initialyTakeRef)
            rawPtr->__inc_ref();
    }
    T *construct() {
        reset(new T);
        return m_ptr;
    }
    lint_ptr(const ::lint_ptr<T> &orig) : m_ptr(orig.m_ptr) {
        if (!m_ptr)
            return;
        m_ptr->__inc_ref();
    }
    lint_ptr(::lint_ptr<T> &&orig) {
        if (this == &orig)
            return;
        m_ptr = orig.m_ptr;
        orig.m_ptr = nullptr;
    }
    inline ~lint_ptr() {
        if (!m_ptr)
            return;
        m_ptr->__dec_ref();
    }
    T *get() const { return m_ptr; }
    bool operator==(const T *raw) const { return get() == raw; }
    inline void reset(T *rawPtr) noexcept {
        if (rawPtr == m_ptr) // heh?
            return;
        reset();
        m_ptr = rawPtr;
        if (rawPtr)
            rawPtr->__inc_ref();
    }
    inline void swap(lint_ptr<T> &other) { std::swap(m_ptr, other.m_ptr); }
    inline void reset() noexcept {
        if (m_ptr)
            m_ptr->__dec_ref();
        m_ptr = nullptr;
    }
    lint_ptr<T> &operator=(const lint_ptr<T> &other) {
        if (m_ptr == other.m_ptr)
            return *this;
        reset(other.m_ptr);
        return *this;
    }
    lint_ptr<T> &operator=(lint_ptr<T> &&other) {
        if (m_ptr == other.m_ptr)
            return *this;

        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
        return *this;
    }
    // pointer-like access options
    explicit inline operator bool() const noexcept { return m_ptr; }
    inline T &operator*() const noexcept { return *m_ptr; }
    inline T *operator->() const noexcept { return m_ptr; }
    // pointer-like access options
    inline bool operator<(const lint_ptr<T> &vs) const noexcept {
        return m_ptr < vs.m_ptr;
    }
    // pointer-like access options
    inline bool operator==(const lint_ptr<T> &vs) const noexcept {
        return m_ptr == vs.m_ptr;
    }
    /**
     * @brief release returns the pointer and makes this invalid while keeping
     * the refcount
     * @return Raw pointer
     */
    T *release() noexcept {
        auto ret = m_ptr;
        m_ptr = nullptr;
        return ret;
    }
};

class tSplitWalk {
    using mstring = string;
    using cmstring = const string;
#define stmiss string::npos

    cmstring &s;
    mutable mstring::size_type start, len, oob;
    const char *m_seps;

  public:
    inline tSplitWalk(cmstring &line, decltype(m_seps) separators,
                      unsigned begin = 0)
        : s(line), start(begin), len(stmiss), oob(line.size()),
          m_seps(separators) {}
    inline bool Next() const {
        if (len != stmiss) // not initial state, find the next position
            start = start + len + 1;

        if (start >= oob)
            return false;

        start = s.find_first_not_of(m_seps, start);

        if (start < oob) {
            len = s.find_first_of(m_seps, start);
            len = (len == stmiss) ? oob - start : len - start;
        } else if (len != stmiss) // not initial state, end reached
            return false;
        else if (s.empty()) // initial state, no parts
            return false;
        else // initial state, use the whole string
        {
            start = 0;
            len = oob;
        }

        return true;
    }
    inline mstring str() const { return s.substr(start, len); }
    inline operator mstring() const { return str(); }
    inline const char *remainder() const { return s.c_str() + start; }
};

void replace_all(std::string &str, const std::string &from,
                 const std::string &to) {
    if (from.empty()) {
        return;
    }

    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

template <typename T, typename C, C TFreeFunc(T)> struct auto_raii {
    T m_p;
    auto_raii(T xp) : m_p(xp) {}
    ~auto_raii() {
        if (m_p)
            TFreeFunc(m_p);
    }
};

const char *rtls[] = {
    "ar", // arabic
    "fa", // farsi
    "he", // hebrew
    "ps", // pashto
    "sd", // sindhi
    "ur", // urdu
};

const char *getCheckedExplicitLocale(bool lctype) {
    auto loc = setlocale(lctype ? LC_CTYPE : LC_MESSAGES, NULL);
    if (loc == NULL)
        return NULL;
    return (islower(*loc & 0xff) && islower(loc[1] & 0xff) &&
            !isalpha(loc[2] & 0xff))
               ? loc
               : NULL;
}

bool userFilter(const char *s, bool isSection) {
    if (match_in_section_only && !isSection)
        return true;
    if ((!match_in_section && !match_in_section_only) && isSection)
        return true;

    if (*substr_filter_nocase)
        return strcasestr(s, substr_filter_nocase);
    if (*substr_filter)
        return strstr(s, substr_filter);
    return true;
}

struct DesktopFile : public tLintRefcounted {
    bool Terminal = false, IsApp = true, NoDisplay = false,
         CommandMassaged = false;

  private:
    string Name, Exec, TryExec;
    string NameLoc, GenericName, GenericNameLoc;

  public:
    string Icon, Categories;
    const string *comment = nullptr;

    DesktopFile(string filePath, const string &langWanted);
    DesktopFile(const string &_name, const string &_nameLoc,
                const string &_icon)
        : Name(_name), NameLoc(_nameLoc), Icon(_icon) {}

    const string &GetCommand();
    const string &GetName() { return Name; }
    const string *GetNamePtr() { return &Name; }

    /// Translate with built-in l10n if needed, and cache it
    const string &GetTranslatedName() {
        if (NameLoc.empty() && !Name.empty())
            NameLoc = gettext(Name.c_str());
        return NameLoc;
    }

    const string &GetTranslatedGenericName() {
        if (GenericNameLoc.empty() && !GenericName.empty())
            GenericNameLoc = gettext(GenericName.c_str());
        return GenericNameLoc;
    }

    static lint_ptr<DesktopFile> load_visible(string &&path,
                                              const string &lang) {
        auto ret = lint_ptr<DesktopFile>();
        try {
            ret.reset(new DesktopFile(path, lang));
            if (ret->NoDisplay || !userFilter(ret->Name.c_str(), false) ||
                !userFilter(ret->GetTranslatedName().c_str(), false)) {
                ret.reset();
            } else {
                if (add_comments) {
                    comment_pool.push_back(std::move(path));
                    ret->comment = (&comment_pool.back());
                }
            }
        } catch (const std::exception &) {
        }
        return ret;
    }

    ostream &print_comment(ostream &strm) {
        if (comment)
            strm << endl << indent_hint << "# " << *comment << endl;
        return strm;
    }
};

using DesktopFilePtr = lint_ptr<DesktopFile>;

inline string safeTrans(DesktopFilePtr &node, const string &altRaw) {
    return node ? node->GetTranslatedName() : gettext(altRaw.c_str());
}

const string &DesktopFile::GetCommand() {

    if (CommandMassaged)
        return Exec;

    CommandMassaged = true;

    if (Terminal && terminal_command) {
        Exec = string(terminal_command) + " -e " + Exec;
    }

    // let's try whether the command line is toxic, expecting stuff from
    // https://specifications.freedesktop.org/desktop-entry-spec/latest/exec-variables.html
    if (string::npos == Exec.find('%'))
        return Exec;

    for (const auto &bad : {"%F", "%U", "%f", "%u"})
        replace_all(Exec, bad, "");
    replace_all(Exec, "%c", Name);
    replace_all(Exec, "%i", Icon);

    return Exec;
}

DesktopFile::DesktopFile(string filePath, const string &langWanted) {

    std::ifstream dfile;
    dfile.open(filePath);
    string line;
    bool reading = false;

    auto get_value = [&](size_t start) -> string {
        auto p = line.find('=', start);
        if (p == string::npos)
            p = start;
        auto v = line.find_first_not_of(SPACECHARS, p + 1);
        if (v == string::npos)
            v = p + 1;
        auto e = line.find_last_not_of(SPACECHARS, v);
        e = (e == string::npos) ? e + 1 : string::npos;
        return line.substr(v, e - v);
    };

    auto take_loc_best = [&](size_t start, string &out, string &outLoc) {
        auto peq = line.find('=', start);
        if (peq == string::npos)
            return; // w00t, no value assigned at all?
        // calc value start and end
        auto v = line.find_first_not_of(SPACECHARS, peq + 1);
        if (v == string::npos)
            v = peq + 1;
        auto e = line.find_last_not_of(SPACECHARS, v);
        e = (e == string::npos) ? e + 1 : string::npos;
        // identify language filter before value assignment
        auto l = line.find('[', start);
        // i18n neutral version
        if (l >= peq || l == string::npos) {
            out = line.substr(v, e - v);
            return;
        }
        // translation found but not looking for localized version here?
        if (langWanted.size() < 2)
            return;
        l++;
        // the exact match always overrides, the short is considered optional
        if (0 == line.compare(l, langWanted.size(), langWanted))
            outLoc = line.substr(v, e - v);
        else if (outLoc.empty() && 0 == line.compare(l, 2, langWanted, 0, 2))
            outLoc = line.substr(v, e - v);
    };

    while (dfile) {
        line.clear();
        std::getline(dfile, line);
        if (line.empty()) {
            if (dfile.eof())
                break;
            continue;
        }
        if (startsWithSz(line, "[Desktop ")) {
            if (startsWithSz(line, "[Desktop Entry")) {
                reading = true;
            } else if (reading) // finished with desktop entry contents
                break;
        }
        if (!reading)
            continue;
        int kl = -1;
#define DFCHECK(x) (kl = sizeof(x) - 1, strncmp(line.c_str(), x, kl) == 0)
#define DFVALUE get_value(kl)
#define DFTRUE(x) (0 == strcasecmp("true", x.c_str()))
        if (DFCHECK("Terminal"))
            Terminal = DFTRUE(DFVALUE);
        else if (DFCHECK("Type")) {
            auto v = DFVALUE;
            if (v == "Application")
                IsApp = true;
            else if (v == "Directory")
                IsApp = false;
        } else if (DFCHECK("NoDisplay"))
            NoDisplay = DFTRUE(DFVALUE);
        else if (DFCHECK("Name"))
            take_loc_best(kl, Name, NameLoc);
        else if (DFCHECK("OnlyShowIn"))
            NoDisplay = true;
        else if (DFCHECK("Icon"))
            Icon = DFVALUE;
        else if (DFCHECK("GenericName") && generic_name)
            take_loc_best(kl, GenericName, GenericNameLoc);
        else if (DFCHECK("Categories"))
            Categories = DFVALUE;
        else if (DFCHECK("Exec"))
            Exec = DFVALUE;
    }
}

class FsScan {
  private:
    std::set<std::pair<ino_t, dev_t>> reclog;
    function<bool(string &&)> cb;
    string sFileNameExtFilter;
    bool recursive;

  public:
    FsScan(decltype(FsScan::cb) cb, const string &sFileNameExtFilter = "",
           bool recursive = true)
        : cb(cb), sFileNameExtFilter(sFileNameExtFilter), recursive(recursive) {
    }
    void scan(const string &sStartdir) { proc_dir_rec(sStartdir); }

  private:
    void proc_dir_rec(const string &path) {

        DBGMSG("enter: " << path);

        auto pdir = opendir(path.c_str());
        if (!pdir)
            return;
        auto_raii<DIR *, int, closedir> dircloser(pdir);
        auto fddir(dirfd(pdir));

        dirent *pent;
        struct stat stbuf;

        while (nullptr != (pent = readdir(pdir))) {
            if (pent->d_name[0] == '.')
                continue;

            string fname(pent->d_name);

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) ||        \
    defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
            // Take the shortcuts where possible, no need to analyze directory
            // properties for descending if that's known to be a plain file
            // already.
            if (pent->d_type == DT_REG)
                goto process_reg_file;

            // Don't have device id here, OTOH hitting another folder on another
            // drive with exactly the same inode is very very unlikely!
            if (recursive && pent->d_type == DT_DIR) {
                stbuf.st_ino = pent->d_ino;
                stbuf.st_dev = 0;
                goto process_dir;
            }
#endif

            if (fstatat(fddir, pent->d_name, &stbuf, 0))
                continue;

            if (S_ISREG(stbuf.st_mode))
                goto process_reg_file;

            if (recursive && S_ISDIR(stbuf.st_mode)) {
            process_dir:
                // link loop detection
                auto prev = make_pair(stbuf.st_ino, stbuf.st_dev);
                auto hint = reclog.insert(prev);
                if (hint.second) { // we added a new one, otherwise do not
                                   // descend
                    proc_dir_rec(path + "/" + fname);
                    reclog.erase(hint.first);
                }
            }

        process_reg_file:
            if (!sFileNameExtFilter.empty() &&
                !endsWith(fname, sFileNameExtFilter)) {

                continue;
            }

            if (!cb(path + "/" + fname))
                break;
        }
    }
};

struct AppEntry {
    DesktopFilePtr deco;
    struct tSfx {
        char before, after;
        string sfx;
    };
    list<tSfx> extraSfx;

    AppEntry(DesktopFilePtr a, list<tSfx> b = list<tSfx>())
        : deco(a), extraSfx(b) {}

    AppEntry(const AppEntry &orig) : deco(orig.deco), extraSfx(orig.extraSfx) {}

    AppEntry(AppEntry &&orig)
        : deco(orig.deco), extraSfx(std::move(orig.extraSfx)) {}

    void AddSfx(const string &sfx, const char *deco) {
        if (sfx.empty())
            return;
        for (const auto &s : extraSfx)
            if (s.sfx == sfx)
                return;
        extraSfx.emplace_back(tSfx{deco[0], deco[1], sfx});
    }
    string TransWithSfx() {
        string ret;
        const auto &sTitle = deco->GetTranslatedName();
        if (right_to_left) {
            for (auto rit = extraSfx.rbegin(); rit != extraSfx.rend(); ++rit)
                if (sTitle != rit->sfx) {
                    ret += rit->before;
                    ret += rit->sfx;
                    ret += rit->after;
                    ret += " ";
                }

            ret += sTitle;
        } else {
            ret = sTitle;
            for (const auto &p : extraSfx)
                if (sTitle != p.sfx) {
                    ret += " ";
                    ret += p.before;
                    ret += p.sfx;
                    ret += p.after;
                }
        }
        if (prog_name_cut > 0 && ret.size() > prog_name_cut) {
            ret.erase(prog_name_cut);
            trimBack(ret);
            ret += ellipsis;
        }
        return ret;
    }
};

/**
 * The own menu deco info is not part of this class.
 * It's fetched on-demand with a supplied resolver function.
 */
struct MenuNode {

    map<string, MenuNode> submenus;
    DesktopFilePtr deco;
    map<const string *, AppEntry, lessByDerefAdaptor<string>> apps;

    void sink_in(DesktopFilePtr df);
    void print(std::ostream &prt_strm);
    void print_flat(std::ostream &prt_strm, const string &pfx_before);
    bool empty() { return apps.empty() && submenus.empty(); }

    /**
     * Returns a temporary list of visited node references.
     */
    multimap<string, MenuNode *> fixup();

    // Second run, contains all deco information now
    void fixup2();

  private:
    struct t_menu_item {
        string translated;
        MenuNode *pNode;
    };
    using t_sorted_menus = vector<t_menu_item>;

    t_sorted_menus GetSortedMenus() {
        t_sorted_menus ret;
        int n = submenus.size();
        ret.reserve(n);

        for (auto &m : this->submenus)
            ret.push_back(
                t_menu_item{safeTrans(m.second.deco, m.first), &m.second});

        // plain bubble sort is totally sufficient for in-place sorting on small
        // ranges like here
        for (int i = 0; i < n - 1; i++) {
            for (int j = 0; j < n - i - 1; j++) {
                if (loc_comper->operator()(ret[j + 1].translated,
                                           ret[j].translated)) {
                    std::swap(ret[j], ret[j + 1]);
                }
            }
        }

        return ret;
    }

    struct t_app_item {
        const string *pTrName;
        AppEntry *pAppEntry;
    };
    using t_sorted_apps = vector<t_app_item>;

    t_sorted_apps GetSortedApps() {
        t_sorted_apps sortedApps;
        int n = apps.size();
        sortedApps.reserve(n);

        for (auto &it : apps)
            sortedApps.emplace_back(
                t_app_item{&it.second.deco->GetTranslatedName(), &it.second});

        // plain bubble sort is totally sufficient for in-place sorting on small
        // ranges like here
        for (int i = 0; i < n - 1; i++) {
            for (int j = 0; j < n - i - 1; j++) {
                if (loc_comper->operator()(*sortedApps[j + 1].pTrName,
                                           *sortedApps[j].pTrName)) {
                    std::swap(sortedApps[j], sortedApps[j + 1]);
                }
            }
        }
        return sortedApps;
    }
};

const auto lessFirstStr = [](const t_menu_path &a, const t_menu_path &b) {
    return strcmp(*a.begin(), *b.begin()) < 0;
};

void MenuNode::sink_in(DesktopFilePtr pDf) {
    // XXX: that is okay for small strings here (because SSO) but might still
    // switch to string_views
    static vector<string> main_cats, sub_cats;
    main_cats.clear();
    sub_cats.clear();

    for (tSplitWalk split(pDf->Categories, ";"); split.Next();) {
        string k(split);
        auto hit = valid_main_cats.find(k);
        if (hit != valid_main_cats.end()) {
            DBGMSG("mcat: " << k);
            main_cats.push_back(std::move(k));
        } else {
            DBGMSG("scat: " << k);
            sub_cats.push_back(std::move(k));
        }
    }
    bool added_somewhere = false;
    auto install = [&pDf, &added_somewhere](MenuNode *cur) {
        if (!cur)
            return;
        added_somewhere = true;
        cur->apps.emplace(pDf->GetNamePtr(), AppEntry(pDf));
    };

    auto add_sub_menues = [&](const t_menu_path &mp, MenuNode *cur,
                              int roffset = -1) {
        auto itLast = mp.end() + roffset;
        for (auto it = itLast; it >= mp.begin(); --it)
            cur = &cur->submenus[*it];
        return cur;
    };

    for (const auto &sk : sub_cats) {
        t_menu_path refval = {sk.c_str()};

        for (auto w = valid_paths.begin(); w != valid_paths.end(); ++w) {
            auto rng =
                std::equal_range(w->begin(), w->end(), refval, lessFirstStr);
            for (auto iPath = rng.first; iPath != rng.second; ++iPath) {
                string mk = iback(*iPath);
                if (no_sub_cats) {
                    if (!mk.empty())
                        install(&submenus[mk]);
                } else if (!mk.empty()) {
                    // easy case, path to the exact main category is known
                    install(add_sub_menues(*iPath, this, -1));
                } else {
                    // apply to all cats as per spec
                    if (main_cats.empty())
                        install(add_sub_menues(*iPath, &submenus[OTH], -2));
                    else {
                        for (const auto &any_mk : main_cats) {
                            install(
                                add_sub_menues(*iPath, &submenus[any_mk], -2));
                        }
                    }
                }
            }
        }
    }

    // catch-all
    if (!added_somewhere) {

        if (main_cats.empty())
            install(&submenus[OTH]);
        else {
            for (const auto &mk : main_cats)
                install(&submenus[mk]);
        }
    }
}

void MenuNode::print(std::ostream &prt_strm) {
    // translated name to icon and submenu (sorted by translated name)
    auto sorted = GetSortedMenus();
    for (auto &m : sorted) {
        auto &deco = m.pNode->deco;

        prt_strm << indent_hint << "menu \"" << m.translated << "\" "
                 << ((deco && !deco->Icon.empty()) ? deco->Icon : ICON_FOLDER)
                 << " {\n";

        indent_hint += "\t";
        m.pNode->print(prt_strm);
        indent_hint.pop_back();

        prt_strm << indent_hint << "}\n";
    }

    auto sortedApps = GetSortedApps();
    for (auto &p : sortedApps) {
        auto &pi = p.pAppEntry->deco;
        pi->print_comment(prt_strm)
            << indent_hint << "prog \"" << p.pAppEntry->TransWithSfx() << "\" "
            << pi->Icon << " " << pi->GetCommand() << "\n";
    }
}

void MenuNode::print_flat(std::ostream &prt_strm, const string &pfx_before) {
    auto sorted = GetSortedMenus();
    for (auto &m : sorted) {
        auto pfx = right_to_left
                       ? (string(flat_sep) + m.translated + pfx_before)
                       : (pfx_before + m.translated + flat_sep);
        m.pNode->print_flat(prt_strm, pfx);
    }

    auto sortedApps = GetSortedApps();
    for (auto &p : sortedApps) {
        auto &pi = p.pAppEntry->deco;

        pi->print_comment(prt_strm) << "prog \"";
        if (right_to_left)
            prt_strm << p.pAppEntry->TransWithSfx() << pfx_before;
        else
            prt_strm << pfx_before << p.pAppEntry->TransWithSfx();

        prt_strm << "\" " << pi->Icon << " " << pi->GetCommand() << "\n";
    }
}

multimap<string, MenuNode *> MenuNode::fixup() {

    multimap<string, MenuNode *> ret;

    // descend deep and then check whether the same app has been added somewhere
    // in the parent nodes, then remove it there
    deque<MenuNode *> checkStack;
    std::function<void(MenuNode *)> go_deeper;
    go_deeper = [&](MenuNode *cur) {
        checkStack.push_back(cur);

        for (auto it = cur->submenus.begin(); it != cur->submenus.end(); ++it) {
            ret.insert(make_pair(it->first, &it->second));
            go_deeper(&it->second);
        }

        for (auto &appIt : cur->apps) {
            // delete dupes in the parent menu nodes (compare by deco identity)
            for (auto ancestorIt = checkStack.begin();
                 ancestorIt != checkStack.end() - 1; ++ancestorIt) {

                for (auto it = (**ancestorIt).apps.begin();
                     it != (**ancestorIt).apps.end(); it++) {
                    if (it->second.deco == appIt.second.deco) {
                        (**ancestorIt).apps.erase(it);
                        break;
                    }
                }
            }
        }

        checkStack.pop_back();
    };
    go_deeper(this);
    return ret;
}

void MenuNode::fixup2() {

    auto vit = submenus.find("AudioVideo");
    if (vit != submenus.end() && vit->second.deco) {
        for (auto &s : {"Audio", "Video"}) {
            auto it = submenus.find(s);
            if (it != submenus.end() && !it->second.deco) {
                it->second.deco.reset(
                    new DesktopFile(it->first, "", vit->second.deco->Icon));
            }
        }
    }

    // Do a depth-first-scan
    using t_iter = decltype(submenus)::iterator;
    deque<t_iter> mpath;
    std::function<bool(const string &, MenuNode &)> descend;

    // subcalls may modify ancenstor's app but not submenus. Menu operation can
    // get last node erased by returning false.
    descend = [&](const string &menu_key, MenuNode &node) -> bool {
        for (auto it = node.submenus.begin(); it != node.submenus.end();) {
            mpath.push_back(it);
            it =
                descend(it->first, it->second) ? ++it : node.submenus.erase(it);
            mpath.pop_back();
        }

        // now do content processing

        if (generic_name) {
            for (auto &p : node.apps) {
                p.second.AddSfx(p.second.deco->GetTranslatedGenericName(),
                                "()");
            }
        }

        if (!(userFilter(menu_key.c_str(), true) ||
              (node.deco &&
               userFilter(node.deco->GetTranslatedName().c_str(), true)))) {

            return false;
        }

        // Special mode where we detect single elements, in which case the
        // menu's apps are moved to ours and the menu is skipped from the
        // printed set.

        if (no_only_child && mpath.size() > 1) {

            auto relocate = [&](const string *app_key, AppEntry &app_entry,
                                decltype(MenuNode::apps) &parent_apps,
                                const string &origin) {
                // if there is another one with the same key -> hands off!
                if (parent_apps.find(app_key) != parent_apps.end())
                    return false;

                if (no_only_child_hint)
                    app_entry.AddSfx(safeTrans(node.deco, origin), "[]");
                parent_apps.emplace(app_key, std::move(app_entry));

                return true;
            };
            if (node.submenus.empty() && node.apps.size() == 1 &&
                mpath.size() > 1) {

                if (relocate(node.apps.begin()->first,
                             node.apps.begin()->second,
                             mpath[mpath.size() - 2]->second.apps, menu_key)) {
                    node.apps.clear();
                }
            }
            // only one sub-menu which contains only applications
            if (node.apps.empty() && node.submenus.size() == 1 &&
                node.submenus.begin()->second.submenus.empty()) {
                bool some_remained = false;
                for (auto &it : node.submenus.begin()->second.apps) {
                    some_remained |=
                        !relocate(it.first, it.second,
                                  mpath[mpath.size() - 1]->second.apps,
                                  node.submenus.begin()->first);
                }
                if (!some_remained)
                    node.submenus.clear();
            }
        }
        return !node.empty();
    };
    descend("", *this);
}

static void help(bool to_stderr, int xit) {
    (to_stderr ? cerr : cout)
        << _("USAGE: icewm-menu-fdo [OPTIONS] [FILENAME]\n"
             "OPTIONS:\n"
             "-g, --generic\t\tInclude GenericName in parentheses of progs\n"
             "-o, --output=FILE\tWrite the output to FILE\n"
             "-t, --terminal=NAME\tUse NAME for a terminal that has '-e'\n"
             "-s, --no-lone-app\tMove lone elements to parent menu\n"
             "-S, --no-lone-hint\tLike -s but append the original submenu's\n"
             "-d, --deadline-apps=N\tStop loading app information after N ms\n"
             "-D, --deadline-all=N\tStop all loading and print what we got so "
             "far\n"
             "-m, --match=PAT\t\tDisplay only apps with title containing PAT\n"
             "-M, --imatch=PAT\tLike --match but ignores the letter case\n"
             "-L, --limit-max-len=N\tCrop app titles at length N, add ...\n"
             "--seps  \tPrint separators before and after contents\n"
             "--sep-before\tPrint separator before the contents\n"
             "--sep-after\tPrint separator only after contents\n"
             "--no-sep-others\tLegacy, has no effect\n"
             "--no-sub-cats\tNo additional subcategories, just one level of "
             "menues\n"
             "--flat\t\tDisplay all apps in one layer with category hints\n"
             "--flat-sep=STR\tCategory separator string used in flat mode "
             "(default: ' / ')\n"
             "--match-sec\tApply --match or --imatch to apps AND sections\n"
             "--match-osec\tApply --match or --imatch only to sections\n"
             "--orig-comment\tPrint source .desktop file as comment\n"
             "-C, --copying\tPrint copyright information\n"
             "-V, --version\tPrint version information\n"
             "-h, --help\tPrints this usage screen and exits.\n"
             "\nFILENAME\tAny .desktop file to launch its application Exec "
             "command.\n\n"
             "This program also listens to environment variables defined by\n"
             "the XDG Base Directory Specification:\n")
        << "XDG_DATA_HOME=" << Elvis(getenv("XDG_DATA_HOME"), (char *)"")
        << "\nXDG_DATA_DIRS=" << Elvis(getenv("XDG_DATA_DIRS"), (char *)"")
        << endl;
    exit(xit);
}

int main(int argc, char **argv) {

    // basic framework and environment initialization
    ApplicationName = my_basename(argv[0]);

    std::ios_base::sync_with_stdio(false);

#ifdef CONFIG_I18N
    setlocale(LC_ALL, "");

    auto msglang = getCheckedExplicitLocale(false);
    right_to_left =
        msglang && *msglang &&
        std::any_of(rtls, rtls + ACOUNT(rtls), [&](const char *rtl) {
            return rtl[0] == msglang[0] && rtl[1] == msglang[1];
        });
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif
    loc_comper = new tLessOp4Localized();

    deque<string> sharedirs;
    auto add_split = [&](const char *p) {
        if (!p || !*p)
            return false;
        string q(p);
        for (tSplitWalk w(q, ":"); w.Next();)
            sharedirs.push_back(w);
        return true;
    };

    // system dirs, either from environment or from static locations
    const char *p = nullptr;
    if (!add_split(getenv("XDG_DATA_HOME")) && (p = getenv("HOME")) && *p)
        sharedirs.push_back(string(p) + "/.local/share");

    if (!add_split(getenv("XDG_DATA_DIRS")))
        sharedirs.insert(sharedirs.end(), {"/usr/local/share", "/usr/share"});

    // option parameters
    bool add_sep_before = false;
    bool add_sep_after = false;
    const char *opt_deadline_apps = nullptr;
    const char *opt_deadline_all = nullptr;

    std::chrono::time_point<std::chrono::steady_clock> deadline_apps,
        deadline_all;

    string desktop_file_to_start;

    for (auto pArg = argv + 1; pArg < argv + argc; ++pArg) {
        if (is_version_switch(*pArg)) {
            cout << "icewm-menu-fdo " VERSION ", Copyright 2015-2024 Eduard "
                    "Bloch, 2017-2023 Bert Gijsbers."
                 << endl;
            exit(0);
        } else if (is_copying_switch(*pArg))
            print_copying_exit();
        else if (is_help_switch(*pArg))
            help(false, EXIT_SUCCESS);
        else if (is_long_switch(*pArg, "seps"))
            add_sep_before = add_sep_after = true;
        else if (is_long_switch(*pArg, "sep-before"))
            add_sep_before = true;
        else if (is_long_switch(*pArg, "sep-after"))
            add_sep_after = true;
        else if (is_long_switch(*pArg, "no-sep-others"))
            no_sep_others = true;
        else if (is_long_switch(*pArg, "no-sub-cats"))
            no_sub_cats = true;
        else if (is_long_switch(*pArg, "orig-comment"))
            add_comments = true;
        else if (is_long_switch(*pArg, "flat"))
            flat_output = no_sep_others = true;
        else if (is_long_switch(*pArg, "match-sec"))
            match_in_section = true;
        else if (is_long_switch(*pArg, "match-osec"))
            match_in_section = match_in_section_only = true;
        else if (is_switch(*pArg, "g", "generic-name"))
            generic_name = true;
        else if (is_switch(*pArg, "s", "no-lone-app"))
            no_only_child = true;
        else if (is_switch(*pArg, "S", "no-lone-hint"))
            no_only_child = no_only_child_hint = true;
        else {
            char *value = nullptr, *expand = nullptr;
            if (GetArgument(value, "o", "output", pArg, argv + argc)) {
                if (*value == '~')
                    value = expand = tilde_expansion(value);
                else if (*value == '$')
                    value = expand = dollar_expansion(value);
                if (nonempty(value)) {
                    if (freopen(value, "w", stdout) == nullptr)
                        fflush(stdout);
                }
                if (expand)
                    delete[] expand;
            } else if (GetArgument(value, "m", "match", pArg, argv + argc))
                substr_filter = value;
            else if (GetArgument(value, "M", "imatch", pArg, argv + argc))
                substr_filter_nocase = value;
            else if (GetArgument(value, "F", "flat-sep", pArg, argv + argc))
                flat_sep = value;
            else if (GetArgument(value, "t", "terminal", pArg, argv + argc))
                terminal_option = value;
            else if (GetArgument(value, "L", "limit-max-len", pArg,
                                 argv + argc))
                prog_name_cut = atoi(value);
            else if (GetArgument(value, "d", "deadline-apps", pArg,
                                 argv + argc))
                opt_deadline_apps = value;
            else if (GetArgument(value, "D", "deadline-all", pArg, argv + argc))
                opt_deadline_all = value;
            else {
                if (argc == 2 && !(desktop_file_to_start = argv[1]).empty() &&
                    endsWithSzAr(desktop_file_to_start, ".desktop")) {
                    DBGMSG("shall invoke: " << desktop_file_to_start);
                } else // unknown option
                    help(true, EXIT_FAILURE);
            }
        }
    }

    const char *terminals[] = {terminal_option, getenv("TERMINAL"), TERM,
                               "urxvt",         "alacritty",        "roxterm",
                               "xterm"};
    for (auto term : terminals)
        if (term && (terminal_command = path_lookup(term)) != nullptr)
            break;

    if (!desktop_file_to_start.empty()) {
        DesktopFile df(argv[1], Elvis(msglang, ""));
        auto cmd = df.GetCommand();
        if (cmd.empty())
            return EXIT_FAILURE;
        return system(cmd.c_str());
    }

    if (opt_deadline_all || opt_deadline_apps) {
        auto a = opt_deadline_apps ? atoi(opt_deadline_apps) : 0;
        auto b = opt_deadline_all ? atoi(opt_deadline_all) : 0;
        if (!a && b)
            a = b;
        if (b && !a)
            b = a;
        if (a > b)
            a = b;

        // also need to preload gettext (preheat the cache), otherwise the
        // remaining runtime on printing becomes unpredictable
        b += (strlen(gettext("Audio")) > 1234) +
             (strlen(gettext("Zarathustra")) > 5678);

        auto now = std::chrono::steady_clock::now();
        deadline_apps = now + std::chrono::milliseconds(a);
        deadline_all = now + std::chrono::milliseconds(b);
    }

    for (const auto &p : *(valid_paths.end() - 1))
        valid_main_cats.insert(*p.begin());

    auto justLang = string(msglang ? msglang : "");
    justLang = justLang.substr(0, justLang.find('.'));

    MenuNode *leaky = new MenuNode;
    auto &root = *leaky;
    bool in_timeout = false;

    {
        auto desktop_loader = FsScan(
            [&](string &&fPath) {
                DBGMSG("reading: " << fPath);
                if (opt_deadline_apps &&
                    std::chrono::steady_clock::now() > deadline_apps) {
                    return false;
                }

                auto df = DesktopFile::load_visible(std::move(fPath), justLang);
                if (df)
                    root.sink_in(df);

                return true;
            },
            ".desktop");

        for (const auto &sdir : sharedirs) {
            DBGMSG("checkdir: " << sdir);
            desktop_loader.scan(sdir + "/applications");
        }
    }

    auto section_entries = root.fixup();

    // okay, now let's decorate the remaining menus
    {
        auto dir_loader = FsScan(
            [&](string &&fPath) {
                if (opt_deadline_all &&
                    std::chrono::steady_clock::now() > deadline_all) {
                    in_timeout = true;
                    return false;
                }

                auto df = DesktopFile::load_visible(std::move(fPath), justLang);
                if (!df)
                    return true;

                // get all menu nodes of that name
                auto rng = section_entries.equal_range(df->GetName());
                for (auto it = rng.first; it != rng.second; ++it) {
                    if (!it->second->deco)
                        it->second->deco = df;
                }
                // No menus of that name? Try using the plain filename, some
                // .directory files use the category as file name stem but
                // differing in the Name attribute
                if (rng.first == rng.second) {
                    auto cpos = fPath.find_last_of("/");
                    auto mcatName =
                        fPath.substr(cpos + 1, fPath.length() - cpos - 11);
                    rng = section_entries.equal_range(mcatName);
                    DBGMSG("altname: " << mcatName);

                    for (auto it = rng.first; it != rng.second; ++it) {
                        if (!it->second->deco)
                            it->second->deco = df;
                    }
                }

                return true;
            },
            ".directory", false);

        for (const auto &sdir : sharedirs) {
            dir_loader.scan(sdir + "/desktop-directories");
        }
    }

    if (add_sep_before && !root.empty())
        cout << "separator" << endl;

    if (!in_timeout)
        root.fixup2();

    if (flat_output)
        root.print_flat(cout, "");
    else
        root.print(cout);

    if (in_timeout || (opt_deadline_apps &&
                       std::chrono::steady_clock::now() > deadline_apps)) {
        cout << "prog \"" << _("System too slow! Failed to load menu content!")
             << "\" stop " << argv[0]
             << " --match=to-be-never-matched --match-osec\n"
             << "prog \"" << _("Please push HERE and retry after some seconds.")
             << "\" view-refresh " << argv[0]
             << " --match=to-be-never-matched --match-osec\n";
    }

    if (add_sep_after && !root.empty())
        cout << "separator" << endl;

    return EXIT_SUCCESS;
}

// vim: set sw=4 ts=4 et:

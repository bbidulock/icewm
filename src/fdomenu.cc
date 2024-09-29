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

#include <cstring>
#include <stack>
#include <string>
// does not matter, string from C++11 is enough
// #include <string_view>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <regex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <functional>
#include <initializer_list>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

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

auto substr_filter = "";
auto substr_filter_nocase = "";
auto flat_sep = " / ";
char *terminal_command;
char *terminal_option;

/*
 * Certain parts borrowed from apt-cacher-ng by its autor, either from older
 * branches (C++11 compatible) or development branch.
 */

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

/*!
 * \brief Simple and convenient split function, outputs resulting tokens into a
 * string vector. Operates on user-provided vector, with or without purging the
 * previous contents.
 */
vector<string> &Tokenize(const string &in, const char *sep,
                         vector<string> &inOutVec, bool bAppend = false,
                         std::string::size_type nStartOffset = 0) {
    if (!bAppend)
        inOutVec.clear();

    auto pos = nStartOffset, pos2 = nStartOffset, oob = in.length();
    while (pos < oob) {
        pos = in.find_first_not_of(sep, pos);
        if (pos == string::npos) // no more tokens
            break;
        pos2 = in.find_first_of(sep, pos);
        if (pos2 == string::npos) // no more terminators, EOL
            pos2 = oob;
        inOutVec.emplace_back(in.substr(pos, pos2 - pos));
        pos = pos2 + 1;
    }
    return inOutVec;
}

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

auto line_matcher =
    std::regex("^\\s*(Terminal|Type|Name|GenericName|Exec|TryExec|Icon|"
               "Categories|NoDisplay)"
               "(\\[((\\w\\w)(_\\w\\w)?)\\])?\\s*=\\s*(.*){0,1}?\\s*$",
               std::regex_constants::ECMAScript);

struct DesktopFile : public tLintRefcounted {
    bool Terminal = false, IsApp = true, NoDisplay = false,
         CommandMassaged = false;

  private:
    string Name, Exec, TryExec;
    string NameLoc, GenericName, GenericNameLoc;

  public:
    string Icon;
    vector<string> Categories;

    DesktopFile(string filePath, const string &langWanted);

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

    static lint_ptr<DesktopFile> load_visible(const string &path,
                                              const string &lang) {
        auto ret = lint_ptr<DesktopFile>();
        try {
            ret.reset(new DesktopFile(path, lang));
            if (ret->NoDisplay)
                ret.reset();
        } catch (const std::exception &) {
        }
        return ret;
    }
};

using DesktopFilePtr = lint_ptr<DesktopFile>;

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
    if (!TryExec.empty())
        return (Exec = TryExec); // copy over so we stick to it in case of
                                 // later calls

    for (const auto &bad : {"%F", "%U", "%f", "%u"})
        replace_all(Exec, bad, "");
    replace_all(Exec, "%c", Name);
    replace_all(Exec, "%i", Icon);

    return Exec;
}

DesktopFile::DesktopFile(string filePath, const string &langWanted) {
    // cout << "filterlang: " << lang <<endl;

    // bare short version to compare later
    auto langWantShort = langWanted.substr(0, 2);

    std::ifstream dfile;
    dfile.open(filePath);
    string line;
    std::smatch m;
    bool reading = false;

    auto take_loc_best = [&langWanted, &langWantShort](
                             decltype(m[0]) &value, decltype(m[0]) &langLong,
                             decltype(m[0]) &langShort, string &out,
                             string &outLoc) {
        if (langWanted.size() > 3 && langLong == langWanted) {
            // perfect hit always takes preference
            outLoc = value;
        } else if (langShort.matched && langShort == langWantShort) {
            if (outLoc.empty())
                outLoc = value;
        } else if (!langLong.matched) {
            out = value;
        }
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

        std::regex_search(line, m, line_matcher);
        if (m.empty())
            continue;

        // for(auto x: m) cout << x << " - "; cout << " = " << m.size() <<
        // endl;

        const auto &value = m[6], key = m[1], langLong = m[3], langShort = m[4];
        // cerr << "val: " << value << ", key: " << key << ", langLong: " <<
        // langLong << ", langShort: " << langShort << endl;

        if (key == "Terminal")
            Terminal = value.compare("true") == 0;
        else if (key == "NoDisplay")
            NoDisplay = value.compare("true") == 0;
        else if (key == "Icon")
            Icon = value;
        else if (key == "Categories")
            Tokenize(value, ";", Categories);
        else if (key == "Exec")
            Exec = value;
        else if (key == "TryExec")
            TryExec = value;
        else if (key == "Type") {
            if (value == "Application")
                IsApp = true;
            else if (value == "Directory")
                IsApp = false;
        } else if (key == "Name") {
            take_loc_best(value, langLong, langShort, Name, NameLoc);
        } else if (generic_name && key == "GenericName") {
            take_loc_best(value, langLong, langShort, GenericName,
                          GenericNameLoc);
        }
    }
}

class FsScan {
  private:
    std::set<std::pair<ino_t, dev_t>> reclog;
    function<bool(const string &)> cb;
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

        // const gchar *szFilename(nullptr);
        dirent *pent;
        struct stat stbuf;

        while (nullptr != (pent = readdir(pdir))) {
            if (pent->d_name[0] == '.')
                continue;

            string fname(pent->d_name);

            // Take the shortcuts where possible, no need to analyze directory
            // properties for descending if that's known to be a plain file
            // already.
            if (pent->d_type == DT_REG)
                goto process_reg_file;

            if (recursive && pent->d_type == DT_DIR)
                goto process_dir;

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
    void AddSfx(const string &sfx, const char *deco) {
        for (const auto &s : extraSfx)
            if (s.sfx == sfx)
                return;
        extraSfx.emplace_back(tSfx{deco[0], deco[1], sfx});
    }
    void PrintWithSfx(ostream &strm, const string &sTitle) {
        if (extraSfx.empty()) {
            strm << sTitle;
            return;
        }
        if (right_to_left) {
            for (auto rit = extraSfx.rbegin(); rit != extraSfx.rend(); ++rit)
                strm << rit->before << rit->sfx << rit->after << " ";
            strm << sTitle;
        } else {
            strm << sTitle;
            for (const auto &p : extraSfx)
                strm << " " << p.before << p.sfx << p.after;
        }
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
    bool empty() { return apps.empty() && submenus.empty(); }
    /**
     * Returns a temporary list of visited node references.
     */
    unordered_multimap<string, MenuNode *> fixup();
};

void MenuNode::sink_in(DesktopFilePtr pDf) {

    auto add_sub_menues = [&](const t_menu_path &mp) {
        MenuNode *cur = this;

        // work around the lack of reverse iterator, can be made easier in C++14
        // with std::rbegin() conversion

        if (!mp.size())
            return cur;
        for (auto it = mp.end() - 1;; --it) {
            auto key = (*it && **it) ? *it : "Other";
            cur = &cur->submenus[key];
            // cerr << "adding submenu: " << key << endl;
            if (mp.begin() == it)
                break;
        }
        return cur;
    };

    for (const auto &cat : pDf->Categories) {
        // cerr << "where does it fit? " << cat << endl;
        t_menu_path refval = {cat.c_str()};
        static auto comper = [](const t_menu_path &a, const t_menu_path &b) {
            // cerr << "left: " << *a.begin() << " vs. right: " << *b.begin() <<
            // endl;
            return strcmp(*a.begin(), *b.begin()) < 0;
        };
        for (const auto &w : valid_paths) {
            // cerr << "try paths: " << (uintptr_t)&w << endl;

            // ignore deeper paths, fallback to the main cats only
            if (no_sub_cats && w.begin()->size() > 1)
                continue;

            auto rng = std::equal_range(w.begin(), w.end(), refval, comper);
            for (auto it = rng.first; it != rng.second; ++it) {
                auto &tgt = *add_sub_menues(*it);
                tgt.apps.emplace(pDf->GetNamePtr(), AppEntry{pDf});
            }
        }
    }
}

static string ICON_FOLDER("folder");
string indent_hint("");

void MenuNode::print(std::ostream &prt_strm) {
    // translated name to icon and submenu (sorted by translated name)
    map<string, std::pair<string, MenuNode *>, tLessOp4Localized> sorted;

    for (auto &m : this->submenus) {
        auto &name = m.first;
        auto &menu = m.second;
        auto &menuDeco = menu.deco;

        // Special mode where we detect single elements, in which case the
        // menu's apps are moved to ours and the menu is skipped from the
        // printed set
        if (no_only_child && m.second.apps.size() == 1 &&
            m.second.submenus.empty()) {

            auto &lone_app_entry = m.second.apps.begin()->second;
            auto &lone_app_key = m.second.apps.begin()->first;

            if (no_only_child_hint)
                lone_app_entry.AddSfx(
                    menuDeco ? menuDeco->GetTranslatedName() : name, "[]");

            apps.emplace(lone_app_key, move(lone_app_entry));
            m.second.apps.clear();
        } else {
            sorted[menuDeco ? menuDeco->GetTranslatedName() : name] =
                make_pair(menuDeco ? menuDeco->Icon : ICON_FOLDER, &m.second);
        }
    }
    for (auto &m : sorted) {
        auto &name = m.first;
        prt_strm << indent_hint << "menu \"";
        if (m.second.second->deco)
            prt_strm << m.second.second->deco->GetTranslatedName();
        else
            prt_strm << gettext(name.c_str());

        prt_strm << "\" "
                 << ((m.second.second->deco &&
                      !m.second.second->deco->Icon.empty())
                         ? m.second.second->deco->Icon
                         : ICON_FOLDER)
                 << " {\n";

        indent_hint += "\t";
        m.second.second->print(prt_strm);
        indent_hint.pop_back();

        prt_strm << indent_hint << "}\n";
    }

    map<const string *, AppEntry, tLessOp4LocalizedDeref> sortedApps;
    for (auto &p : this->apps)
        sortedApps[&(p.second.deco->GetTranslatedName())] = p.second;

    for (auto &p : sortedApps) {
        auto &pi = p.second.deco;

        prt_strm << indent_hint << "prog \"";
        if (generic_name)
            p.second.AddSfx(pi->GetTranslatedGenericName(), "()");
        p.second.PrintWithSfx(prt_strm, pi->GetTranslatedName());
        prt_strm << "\" " << pi->Icon << " " << pi->GetCommand() << "\n";
    }
}

unordered_multimap<string, MenuNode *> MenuNode::fixup() {

    unordered_multimap<string, MenuNode *> ret;

    if (no_sep_others)
        submenus.erase("Other");

    // descend deep and then check whether the same app has been added somewhere
    // in the parent nodes, then remove it there
    vector<MenuNode *> checkStack;
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

static void help(bool to_stderr, int xit) {
    (to_stderr ? cerr : cout)
        << "USAGE: icewm-menu-fdo [OPTIONS] [FILENAME]\n"
           "OPTIONS:\n"
           "-g, --generic\tInclude GenericName in parentheses of progs\n"
           "-o, --output=FILE\tWrite the output to FILE\n"
           "-t, --terminal=NAME\tUse NAME for a terminal that has '-e'\n"
           "-s, --no-lone-app\tMove lone elements to parent menu\n"
           "-S, --no-lone-hint\tLike -s but append the original submenu's\n"
           "-d, --deadline-apps=N\tStop loading app information after N ms\n"
           "-D, --deadline-all=N\tStop all loading and print what we got so "
           "far\n"
           "title\n"
           "--seps  \tPrint separators before and after contents\n"
           "--sep-before\tPrint separator before the contents\n"
           "--sep-after\tPrint separator only after contents\n"
           "--no-sep-others\tDon't print uncategorized apps in Other menu\n"
           "--no-sub-cats\tNo additional subcategories, just one level of "
           "menues\n"
           "--flat\tDisplay all apps in one layer with category hints\n"
           "--flat-sep STR\tCategory separator string used in flat mode "
           "(default: ' / ')\n"
           "--match PAT\tDisplay only apps with title containing PAT\n"
           "--imatch PAT\tLike --match but ignores the letter case\n"
           "--match-sec\tApply --match or --imatch to apps AND sections\n"
           "--match-osec\tApply --match or --imatch only to sections\n"
           "FILENAME\tAny .desktop file to launch its application Exec "
           "command\n"
           "This program also listens to environment variables defined by\n"
           "the XDG Base Directory Specification:\n"
           "XDG_DATA_HOME="
        << Elvis(getenv("XDG_DATA_HOME"), (char *)"")
        << "\n"
           "XDG_DATA_DIRS="
        << Elvis(getenv("XDG_DATA_DIRS"), (char *)"") << endl;
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
        msglang && std::any_of(rtls, rtls + ACOUNT(rtls), [&](const char *rtl) {
            return rtl[0] == msglang[0] && rtl[1] == msglang[1];
        });
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

    vector<string> sharedirs;
    const char *p;
    auto pUserShare = getenv("XDG_DATA_HOME");
    if (pUserShare && *pUserShare)
        Tokenize(pUserShare, ":", sharedirs, true);
    else if (nullptr != (p = getenv("HOME")) && *p)
        sharedirs.push_back(string(p) + "/.local/share");

    // system dirs, either from environment or from static locations
    auto sysshare = getenv("XDG_DATA_DIRS");
    if (sysshare && !*sysshare)
        Tokenize(sysshare, ":", sharedirs, true);
    else
        sharedirs.push_back({"/usr/local/share", "/usr/share"});

    // option parameters
    bool add_sep_before = false;
    bool add_sep_after = false;
    const char *opt_deadline_apps = nullptr;
    const char *opt_deadline_all = nullptr;

    std::chrono::time_point<std::chrono::steady_clock> deadline_apps,
        deadline_all;

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
            else if (GetArgument(value, "d", "deadline-apps", pArg,
                                 argv + argc))
                opt_deadline_apps = value;
            else if (GetArgument(value, "D", "deadline-all", pArg, argv + argc))
                opt_deadline_all = value;
            else // unknown option
                help(true, EXIT_FAILURE);
        }
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

    auto justLang = string(msglang ? msglang : "");
    justLang = justLang.substr(0, justLang.find('.'));

    const char *terminals[] = {terminal_option, getenv("TERMINAL"), TERM,
                               "urxvt",         "alacritty",        "roxterm",
                               "xterm"};
    for (auto term : terminals)
        if (term && (terminal_command = path_lookup(term)) != nullptr)
            break;

    MenuNode root;

    auto desktop_loader = FsScan(
        [&](const string &fPath) {
            DBGMSG("reading: " << fPath);
            if (opt_deadline_apps &&
                std::chrono::steady_clock::now() > deadline_apps)
                return false;

            auto df = DesktopFile::load_visible(fPath, justLang);
            if (df)
                root.sink_in(df);

            return true;
        },
        ".desktop");

    for (const auto &sdir : sharedirs) {
        DBGMSG("checkdir: " << sdir);
        desktop_loader.scan(sdir + "/applications");
    }

    auto menu_lookup = root.fixup();

    /*
        unordered_set<string> filter;
        for(const auto& kv: root.menu_nodes_by_name)
            filter.insert(kv.first);
    */

    auto dir_loader = FsScan(
        [&](const string &fPath) {
            if (opt_deadline_apps &&
                std::chrono::steady_clock::now() > deadline_all)
                return false;

            auto df = DesktopFile::load_visible(fPath, justLang /*, filter*/);
            if (!df)
                return true;

            // get all menu nodes of that name
            auto rng = menu_lookup.equal_range(df->GetName());
            for (auto it = rng.first; it != rng.second; ++it) {
                if (!it->second->deco)
                    it->second->deco = df;
            }
            // No menus of that name? Try using the plain filename, some
            // .directory files use the category as file name stem but differing
            // in the Name attribute
            if (rng.first == rng.second) {
                auto cpos = fPath.find_last_of("/");
                auto mcatName =
                    fPath.substr(cpos + 1, fPath.length() - cpos - 11);
                rng = menu_lookup.equal_range(mcatName);
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

    if (add_sep_before && !root.empty())
        cout << "separator" << endl;

    root.print(cout);

    if (add_sep_after && !root.empty())
        cout << "separator" << endl;

    return EXIT_SUCCESS;
}

// vim: set sw=4 ts=4 et:

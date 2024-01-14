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
 *  - overhauled program design and menu construction code, added sub-category handling
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "config.h"
#include "base.h"
//#include "sysdep.h"
#include "intl.h"
#include "appnames.h"
#include "ylocale.h"

#include <stack>
#include <string>
#include <cstring>
// does not matter, string from C++11 is enough
//#include <string_view>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <memory>
#include <array>
#include <locale>
#include <vector>
#include <fstream>
#include <regex>
#include <iostream>

using namespace std;

#include "fdospecgen.h"

char const* ApplicationName;

/*
* Certain parts borrowed from apt-cacher-ng by its autor, either from older branches (C++11 compatible) or development branch.
*/
#if 0
constexpr unsigned int str2int(const char* str, int h = 0)
{
    //return strhash(str);
    // same as pure constexpr
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

#define SPACECHARS " \f\n\r\t\v"
#define KVJUNK SPACECHARS "[]="

inline void trimFront(std::string &s, const char* junk = SPACECHARS, string::size_type start=0)
{
        auto pos = s.find_first_not_of(junk, start);
        if(pos == std::string::npos)
                s.clear();
        else if(pos>0)
                s.erase(0, pos);
}
inline void trimBack(std::string &s, const string_view junk=SPACECHARS)
{
        auto pos = s.find_last_not_of(junk);
        if(pos == std::string::npos)
                s.clear();
        else if(pos>0)
                s.erase(pos+1);
}
#define CSTR_AND_LEN(sz) sz, sizeof(sz) - 1
#define LEN_AND_CSTR(sz) sizeof(sz) - 1, sz
#endif

#define startsWith(where, what) (0==(where).compare(0, (what).size(), (what)))
#define startsWithSz(where, what) (0==(where).compare(0, sizeof((what))-1, (what)))
#define endsWith(where, what) ((where).size()>=(what).size() && \
                0==(where).compare((where).size()-(what).size(), (what).size(), (what)))
#define endsWithSzAr(where, what) ((where).size()>=(sizeof((what))-1) && \
		0==(where).compare((where).size()-(sizeof((what))-1), (sizeof((what))-1), (what)))

/**
 * Basic base implementation of a reference-counted class
 */
struct tLintRefcounted
{
private:
	size_t m_nRefCount = 0;
public:
	inline void __inc_ref() noexcept
	{
		m_nRefCount++;
	}
    inline void __dec_ref()
    {
        if(--m_nRefCount == 0)
            delete this;
    }
    virtual ~tLintRefcounted() {}
    inline size_t __ref_cnt() { return m_nRefCount; }
};

/**
 * Lightweight intrusive smart pointer with ordinary reference counting
 */
template<class T>
class lint_ptr
{
	T * m_ptr = nullptr;
public:
	explicit lint_ptr()
	{
	}
	/**
	 * @brief lint_ptr Captures the pointer and ensures that it's released when the refcount goes to zero, unless initialyTakeRef is set to false.
	 * If initialyTakeRef is false, the operation is asymmetric, i.e. one extra __dec_ref operation will happen in the end.
	 *
	 * @param rawPtr
	 * @param initialyTakeRef
	 */
	explicit lint_ptr(T *rawPtr, bool initialyTakeRef = true) :
		m_ptr(rawPtr)
	{
		if(rawPtr && initialyTakeRef)
			rawPtr->__inc_ref();
	}
	T* construct()
	{
		reset(new T);
		return m_ptr;
	}
	lint_ptr(const ::lint_ptr<T> & orig) :
			m_ptr(orig.m_ptr)
	{
		if(!m_ptr) return;
		m_ptr->__inc_ref();
	}
	lint_ptr(::lint_ptr<T> && orig)
	{
		if (this == &orig)
			return;
		m_ptr = orig.m_ptr;
		orig.m_ptr = nullptr;
	}
	inline ~lint_ptr()
	{
		if(!m_ptr) return;
		m_ptr->__dec_ref();
	}
	T* get() const
	{
		return m_ptr;
	}
	bool operator==(const T* raw) const
	{
		return get() == raw;
	}
	inline void reset(T *rawPtr) noexcept
	{
		if(rawPtr == m_ptr) // heh?
			return;
		reset();
		m_ptr = rawPtr;
		if(rawPtr)
			rawPtr->__inc_ref();
	}
	inline void swap(lint_ptr<T>& other)
	{
		std::swap(m_ptr, other.m_ptr);
	}
	inline void reset() noexcept
	{
		if(m_ptr)
			m_ptr->__dec_ref();
		m_ptr = nullptr;
	}
	lint_ptr<T>& operator=(const lint_ptr<T> &other)
	{
		if(m_ptr == other.m_ptr)
			return *this;
		reset(other.m_ptr);
		return *this;
	}
	lint_ptr<T>& operator=(lint_ptr<T> &&other)
	{
		if(m_ptr == other.m_ptr)
			return *this;

		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
		return *this;
	}
	// pointer-like access options
	explicit inline operator bool() const noexcept
	{
		return m_ptr;
	}
	inline T& operator*() const noexcept
	{
		return *m_ptr;
	}
	inline T* operator->() const noexcept
	{
		return m_ptr;
	}
	// pointer-like access options
	inline bool operator<(const lint_ptr<T> &vs) const noexcept
	{
		return m_ptr < vs.m_ptr;
	}
	// pointer-like access options
	inline bool operator==(const lint_ptr<T> &vs) const noexcept
	{
		return m_ptr == vs.m_ptr;
	}
	/**
	 * @brief release returns the pointer and makes this invalid while keeping the refcount
	 * @return Raw pointer
	 */
	T* release() noexcept
	{
		auto ret = m_ptr;
		m_ptr = nullptr;
		return ret;
	}
};


/*!
 * \brief Simple and convenient split function, outputs resulting tokens into a string vector.
 * Operates on user-provided vector, with or without purging the previous contents.
 */
vector<string> & Tokenize(const string & in, const char *sep,
		vector<string> & inOutVec, bool bAppend = false, std::string::size_type nStartOffset = 0)
{
	if(!bAppend)
		inOutVec.clear();
	
	auto pos=nStartOffset, pos2=nStartOffset, oob=in.length();
	while (pos<oob)
	{
		pos=in.find_first_not_of(sep, pos);
		if (pos==string::npos) // no more tokens
			break;
		pos2=in.find_first_of(sep, pos);
		if (pos2==string::npos) // no more terminators, EOL
			pos2=oob;
		inOutVec.emplace_back(in.substr(pos, pos2-pos));
		pos=pos2+1;
	}
    return inOutVec;
}


auto line_matcher = std::regex("^\\s*(Terminal|Type|Name|Exec|Icon|Categories)(\\[(..)\\])?\\s*=\\s*(.*){0,1}?\\s*$", std::regex_constants::ECMAScript);

struct DesktopFile : public tLintRefcounted {
    bool Terminal = false, IsApp = true;
    string Name, NameLoc, Exec, Icon;
    vector<string> Categories;

    /// Translate with built-in l10n if needed
    string& GetTranslatedName() {
        if (NameLoc.empty()) {
            NameLoc = gettext(Name.c_str());
        }
        return NameLoc;
    }

    DesktopFile(string filePath, const string &lang) {
        std::ifstream dfile;
        dfile.open(filePath);
        string line;
        std::smatch m;
        while (dfile) {
                line.clear();
                std::getline(dfile, line);
                if (line.empty()) {
                    if (dfile.eof())
                        break;
                    continue;
                }
                std::regex_search(line, m, line_matcher);
                if (m.empty())
                    continue;
                if (m[3].matched && m[3].compare(lang))
                    continue;
                //for(auto x: m) cout << x << " - ";
                //cout << " = " << m.size() << endl;

                if (m[1] == "Terminal")
                    Terminal = m[4].compare("true") == 0;
                else if (m[1] == "Icon")
                    Icon = m[4];
                else if (m[1] == "Categories") {
                    Tokenize(m[4], ";", Categories);
                }
                else if (m[1] == "Exec") {
                    Exec = m[4];
                }
                else if (m[1] == "Type") {
                    if (m[4] == "Application")
                        IsApp = true;
                    else if (m[4] == "Directory")
                        IsApp = false;
                    else continue;
                }
                else { // must be name
                    if (m[3].matched)
                        NameLoc = m[4];
                    else
                        Name = m[4];
                }


                //cout << m.size() << endl;
                //cout << m[0] << " - " << m[1] << " - " << m[2] << " - " << m[3] <<endl;
                
                #if 0
                trimFront(line);
                if (line[0] == '#' || line[0] == '[')
                    continue;
                
                unsigned matchlen = 0;
                #define match(key) { if (startsWithSz(line, key)) {matchlen = sizeof(key) - 1;}}
                #define find_val_or_continue() auto p = line.find_first_not_of(KVJUNK, matchlen); if (p == string::npos) continue;
                match("Terminal")
                if (matchlen) {
                    find_val_or_continue()
                    Terminal = line[p] == 't';
                    continue;
                }
                match("Type")
                if (matchlen) {
                    find_val_or_continue()
                    if (0 == line.compare(p, LEN_AND_CSTR("Application")))
                        IsApp = false;
                    else if (0 == line.compare(p, LEN_AND_CSTR("Directory")))
                        IsApp = true;
                    continue;
                }
                
                #define grab(x, y) match(x); if (matchlen) { find_val_or_continue(); y = line.erase(p); continue; }
                grab("Categories", Categories)
                grab("Icon", Icon)
                grab("Exec", Exec)

                match("Name")
                if (matchlen) {
                    line.erase(matchlen);
                    trimFront(line);
                    #error murks
                }
                #endif
        }
    }

    static lint_ptr<DesktopFile> load(const string& path, const string& lang) {
    try
    {
        auto ret = new DesktopFile(path, lang);
        return lint_ptr<DesktopFile>(ret);
    }
    catch(const std::exception&)
    {
        return lint_ptr<DesktopFile>();
    }
}

};


#if 0

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
#endif

// program options
bool add_sep_before = false;
bool add_sep_after = false;
bool no_sep_others = false;
bool no_sub_cats = false;
bool generic_name = false;
bool right_to_left = false;
bool flat_output = false;
bool match_in_section = false;
bool match_in_section_only = false;

auto substr_filter = "";
auto substr_filter_nocase = "";
auto flat_sep = " / ";
char* terminal_command;
char* terminal_option;

template<typename T, typename C, C TFreeFunc(T)>
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


const char* rtls[] = {
    "ar",   // arabic
    "fa",   // farsi
    "he",   // hebrew
    "ps",   // pashto
    "sd",   // sindhi
    "ur",   // urdu
};

#if 0
typedef auto_raii<gpointer, g_free> auto_gfree;
typedef auto_raii<gpointer, g_object_unref> auto_gunref;

auto cstr_deleter = [](char* ptr)
{
    free((void*) ptr);
};
using cstr_unique = std::unique_ptr<char*, decltype(cstr_deleter)>;

/*
static int cmpUtf8(const void *p1, const void *p2) {
    return g_utf8_collate((LPCSTR ) p1, (LPCSTR ) p2);
}
*/

/*
struct tLtUtf8 {
    bool operator() (const std::string& a, const std::string& b) {
        //return g_utf8_collate(a.c_str(), b.c_str()) < 0;
        //static auto& f = std::use_facet<std::collate<const char>>(std::locale::classic);

    }
} lessThanUtf8;
*/
struct tLessOp4Localized {
    std::locale loc; // default locale
    const std::collate<char>& coll = std::use_facet<std::collate<char> >(loc);
    bool operator() (const std::string& a, const std::string& b) {
        return coll.compare (a.data(), a.data() + a.size(), 
                                     b.data(), b.data()+b.size()) < 0;
    }
} locStringComper;

class tDesktopInfo;
typedef YVec<const gchar*> tCharVec;
tCharVec sys_folders, home_folders;
std::array<tCharVec*, 2> sys_home_folders = { &sys_folders, &home_folders };
std::array<tCharVec*, 2> home_sys_folders = { &home_folders, &sys_folders };


char const * const par_sec_dummy[] = { nullptr };

void decorate_and_print();


// little adapter class to extract information we get from desktop files using GLib methods
class tDesktopInfo {
    GDesktopAppInfo *pInfo = nullptr;
    mutable std::string found_name;
public:
    tDesktopInfo(LPCSTR szFileName) {
        pInfo = g_desktop_app_info_new_from_filename(szFileName);
        if (!pInfo)
            return;

        // tear down if not permitted
        if (!g_app_info_should_show(*this)) {
            g_object_unref(pInfo);
            pInfo = nullptr;
        }
        found_name = g_app_info_get_display_name((GAppInfo*) pInfo);
        if (found_name.empty())
            found_name = g_desktop_app_info_get_generic_name(pInfo);
    }

    inline operator GAppInfo*() {
        return (GAppInfo*) pInfo;
    }

    inline operator GDesktopAppInfo*() {
        return pInfo;
    }

    inline operator bool() {
        return pInfo;
    }

    //~tDesktopInfo() {
        // XXX: call g_free on pInfo? free on d_file?
     //}

    const std::string& get_name() const {
        return found_name;
    }

    bool operator<(const tDesktopInfo& other) const {
        return locStringComper.operator()(get_name(), other.get_name());
    }

    char * get_icon_path() const {
        auto pIcon = g_app_info_get_icon((GAppInfo*) pInfo);
        auto_gunref free_icon((GObject*) pIcon);
        return pIcon ? g_icon_to_string(pIcon) : nullptr;
    }

    // split tokens into a list, to be cleared by g_strfreev
    gchar** get_cat_tokens() {

        LPCSTR pCats = g_desktop_app_info_get_categories(pInfo);
        if (!pCats)
            pCats = "Other";
        if (0 == strncmp(pCats, "X-", 2))
            return (gchar**) malloc(0);

        return g_strsplit(pCats, ";", -1);
    }

    std::string get_launch_cmd() {
        LPCSTR cmdraw = g_app_info_get_commandline((GAppInfo*) pInfo);
        if (!cmdraw || !*cmdraw)
            return "-";
        using namespace std;
        string ret(cmdraw);
        auto p = ret.find_first_not_of(" \f\n\r\t\v");
        if (p != string::npos)
            ret = ret.substr(p);
        
        // detect URLs and expansion patterns which wouldn't work with plain exec
        auto use_direct_call = ret.find_first_of(":%") == string::npos;

        auto bForTerminal = false;
    #if GLIB_VERSION_CUR_STABLE >= G_ENCODE_VERSION(2, 36)
        bForTerminal = g_desktop_app_info_get_boolean(pInfo, "Terminal");
    #else
        // cannot check terminal property, callback is as safe bet
        use_direct_call = false;
    #endif

        if (use_direct_call && !bForTerminal) // best case
            return ret;
        
        if (bForTerminal && nonempty(terminal_command))
            return std::string(terminal_command) + " -e " + ret;
        
        // not simple command or needs a terminal started via launcher callback, or both
        return std::string(ApplicationName) + " \"" + g_desktop_app_info_get_filename(pInfo) + "\"";
    }
};

struct tMenuAnchor {
    // list of application elements, ordered by the translated names
    std::set<tDesktopInfo> translated_apps;
    bool is_localized_builtin = false;
    bool is_localized_system = false;
    std::string icon_name = "folder";

    static tMenuAnchor& find_menu_anchor(tDesktopInfo& dinfo);
    void add(tDesktopInfo&& app) { translated_apps.emplace(std::move(app)); }
};

// helper to collect everything later
//struct tMenuAnchorKey {
//    std::array<LPCSTR, 4> levels = {0, 0, 0, 0};
//};

using tMenuAnchorKey = std::array<LPCSTR, 4>;

// all pointer ordered which is okay, we operate on static entries
std::unordered_map<tMenuAnchorKey, tMenuAnchor> all_content;
std::unordered_set<LPCSTR> seen_cats;
tMenuAnchor garbage_bin;

tMenuAnchor& setup_anchor(tMenuAnchorKey key) {
    for(auto x: key) if(x) seen_cats.emplace(x);
    return all_content[key];
}
/*
tMenuAnchor& setup_anchor(LPCSTR main_cat, LPCSTR sub_cat = nullptr,
                         LPCSTR sub_sub_cat = nullptr,
                         LPCSTR sub_sub_sub_cat = nullptr) {
return setup_anchor({main_cat, sub_cat, sub_sub_cat, sub_sub_sub_cat});
}
*/
LPCSTR dig_key(tMenuAnchorKey& key, LPCSTR prev_key, unsigned depth) {
    

       
            spec::mapping wanted(scat, nullptr, nullptr);
            auto from_to = equal_range(spec::subcat_to_maincat, spec::subcat_to_maincat + ACOUNT(spec::subcat_to_maincat),
             wanted, 
                [](const spec::mapping &a, const spec::mapping &b) { return strcmp(get<0>(a), get<0>(b)) < 0; } );


    depth++;
    if(depth >= key.size())
        return;
    key[depth] = dig_key(key, depth);
}

tMenuAnchor& tMenuAnchor::find_menu_anchor(tDesktopInfo& dinfo) {

    LPCSTR pCats = g_desktop_app_info_get_categories(dinfo);
    
    if (!pCats)
        pCats = "Other";
    if (0 == strncmp(pCats, "X-", 2))
        return garbage_bin;

    auto ppCats = dinfo.get_cat_tokens();
    if (!ppCats)
        return garbage_bin;

    // Pigeonholing roughly by guessed menu structure
    using namespace std;
    static vector<LPCSTR> mcats, scats;
    mcats.clear();
    scats.clear();

    for(auto p = *ppCats; *p; p++) {
        auto endex = spec::mcat::sall + ACOUNT(spec::mcat::sall);
        auto is_mcat = binary_search(spec::mcat::sall, endex,
             [](LPCSTR a, LPCSTR b) { return strcmp(a, b) < 0; } );
        (is_mcat ? mcats : scats).push_back(p);
    }

    auto endex = spec::subcat_to_maincat + ACOUNT(spec::subcat_to_maincat);
    bool found_subcat = false;
    for(auto &mcat: mcats) {
        for (auto &scat: scats) {
            spec::mapping wanted(scat, nullptr, nullptr);
            auto from_to = equal_range(spec::subcat_to_maincat, endex, wanted, 
                [](const spec::mapping &a, const spec::mapping &b) { return strcmp(get<0>(a), get<0>(b)) < 0; } );
            if (from_to.first == endex)
                continue;
            found_subcat = true;

            for(auto it = from_to.first; it != from_to.second; ++it) {
                auto mc = get<0>(*it);
                auto sc = get<1>(*it);
                auto altsc = get<2>(*it);
                if (mc) {
                    // perfect match, fast path
                    setup_anchor({mc, sc, 0, 0}).add(move(dinfo));
                }
                else {
                    tMenuAnchorKey key = {}
                    for(unsigned i = 0; i < 4; ++i) {

                    }
                }
            }
        }
    }


    g_strfreev(ppCats);
}


struct tStaticMenuDescription {
    LPCSTR key, icon;
    const char ** parent_sec;
};

// shim class which describes the Menu related attributes, which can be fetched from static data or adjusted with localized or additional things from filesystem
class tListMeta {
    LPCSTR title = nullptr, icon = nullptr, key = nullptr;
    tStaticMenuDescription *builtin_data = nullptr;
    tListMeta() {}
    /*
    enum eLoadFlags :char {
        NONE = 0,
        DEFAULT_TRANSLATED = 1,
        SYSTEM_TRANSLATED = 2,
        DEFAULT_ICON = 4,
        FALLBACK_ICON = 8,
        SYSTEM_ICON = 16,
    };
    short load_state_icon, load_state_title;
*/

    std::unordered_map<std::string, const tListMeta*> meta_lookup_data;
    std::unordered_map<std::string, std::string> known_directory_files;

public:
    tListMeta(const tDesktopInfo&);
    static tListMeta make_dummy() { return tListMeta();}
    LPCSTR get_key() const {
        if (key)
            return key;
        if (builtin_data && builtin_data->key)
            return builtin_data->key;
        return "<UNKNOWN>";
    }
    LPCSTR get_icon() const {
        if (icon)
            return icon;
        if (builtin_data && builtin_data->icon)
            return builtin_data->icon;
        return "-";
    }
    LPCSTR get_title() const {
        if (title)
            return title;
        if (builtin_data && builtin_data->key)
            return builtin_data->key;
        return "<UNKNOWN>";
    }
    decltype(tStaticMenuDescription::parent_sec) get_parent_secs() const {
        if (builtin_data)
            return builtin_data->parent_sec;
        return (const char**) &par_sec_dummy;
    }
};


tListMeta* lookup_category(const std::string& key)
{
            #ifdef OLD_IMP

    auto it = meta_lookup_data.find(key);
    if (it == meta_lookup_data.end())
        return nullptr;
    auto* ret = (tListMeta*) it->second;
#ifdef CONFIG_I18N
        // auto-translate the default title for the user's language
    //if (ret && !ret->title)
    //    ret->title = gettext(ret->key);
    //printf("Got title? %s -> %s\n", ret->key, ret->title);
#endif
    return ret;
    #else
return nullptr;
    #endif
}

std::stack<std::string> flat_pfxes;

const char* flat_pfx()
{
    if (!flat_output || flat_pfxes.empty())
        return "";
    return flat_pfxes.top().c_str();
}

void flat_add_level(const char *name)
{
    if (!flat_output)
        return;
    if (flat_pfxes.empty())
        flat_pfxes.push(std::string(name) + flat_sep);
    else
        flat_pfxes.push(flat_pfxes.top() + name + flat_sep);
}

void flat_drop_level()
{
    flat_pfxes.pop();
}

bool filter_matched(const char* title, const char* section_prefix)
{
    if (!match_in_section)
        section_prefix = nullptr;
    if (match_in_section_only)
        title = nullptr;

    bool hasFilter(substr_filter && *substr_filter),
            hasIfilter (substr_filter_nocase && *substr_filter_nocase);

    if (hasFilter) {
         if(title && strstr(title, substr_filter))
             return true;
         if(section_prefix && strstr(section_prefix, substr_filter))
             return true;
    }
#ifdef __GNU_LIBRARY__
    if (hasIfilter) {
        if(title && strcasestr(title, substr_filter_nocase))
            return true;
        if(section_prefix && strcasestr(section_prefix, substr_filter_nocase))
            return true;
    }
#endif
    return ! (hasFilter || hasIfilter);
}

struct t_menu_node;
extern t_menu_node root;

// very basic, avoid vtables!
struct t_menu_node {
protected:
    // for leafs -> NULL, otherwise sub-menu contents
    std::map<std::string, const t_menu_node*, tLtUtf8> store;
    char* progCmd;
    const char* generic;
public:
    const tListMeta *meta;
    t_menu_node(const tListMeta* desc):
        progCmd(nullptr), generic(nullptr), meta(desc) {}

    struct t_print_visitor {
        int count, level;
        t_menu_node* print_separated;
    };

    void print(t_print_visitor *ctx) {
        if (!meta)
            return;
        auto title = meta->get_title();

        if (store.empty())
        {
            if (title && progCmd &&
                    filter_matched(title, flat_pfxes.empty()
                                   ? nullptr : flat_pfxes.top().c_str()))
            {
                if (ctx->count == 0 && add_sep_before)
                    puts("separator");
                if (nonempty(generic) && !strcasestr(title, generic)) {
                    if (right_to_left) {
                        printf("prog \"(%s) %s%s\" %s %s\n",
                                generic, flat_pfx(), title, meta->get_icon(), progCmd);
                    } else {
                        printf("prog \"%s%s (%s)\" %s %s\n",
                                flat_pfx(), title, generic, meta->get_icon(), progCmd);
                    }
                }
                else
                    printf("prog \"%s%s\" %s %s\n", flat_pfx(), title, meta->get_icon(), progCmd);
            }
            ctx->count++;
            return;
        }

        // else: render as menu

        if (ctx->level == 1 && !no_sep_others
                && 0 == strcmp(meta->get_key(), "Other")) {

            ctx->print_separated = this;
            return;
        }

        // root level does not have a name, for others open category menu
        if (ctx->level > 0) {
            if (ctx->count == 0 && add_sep_before)
                puts("separator");
            ctx->count++;
            if (flat_output)
                flat_add_level(title);
            else
                printf("menu \"%s\" %s {\n", title, meta->get_icon());
        }
        ctx->level++;

        for(auto& it: this->store) {
            ((t_menu_node*) it.second)->print(ctx);
        }

        if (ctx->level == 1 && ctx->print_separated)
        {
            fflush(stdout);
            puts("separator");
            no_sep_others = true;
            ctx->print_separated->print(ctx);
        }
        ctx->level--;
        if (ctx->level > 0) {
            if (flat_output)
                flat_drop_level();
            else {
#ifndef DEBUG
                puts("}");
#else
                printf("# end of menu \"%s\"\n}\n", title);
#endif
            }
        }
        if (add_sep_after && ctx->level == 0 && ctx->count > 0)
            puts("separator");

    }

    /**
     * Usual print method for the root node
     */
    void print() {
        t_print_visitor ctx = {0,0,nullptr};
        print(&ctx);
    }

    t_menu_node* add(t_menu_node* node) {
        auto nam = node->meta->get_title();
        if (nam)
            store[nam] = node;
        return node;
    }

    /**
     * Returns a sub-menu which is named by the title (or key) of the provided information.
     * Creates one as needed or finds existing one.
     */
    t_menu_node* get_subtree(const tListMeta* info) {
        auto title = info->get_title();
        auto it = store.find(title);
        return (it != store.end()) ? (t_menu_node*) it->second
                                   : add(new t_menu_node(info));
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

        for (auto pSubCatName = subCatCandidate->get_parent_secs();
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
                    || 0 == strcmp(*pSubCatName, (**ppMainCat).get_key())) {
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
            if (!pNewCatInfo)
                pNewCatInfo = lookup_category(*pSubCatName);
            if (!pNewCatInfo)
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
            if (!pResolved)
                continue;
            if (!pResolved->get_parent_secs())
                matched_main_cats.add(pResolved);
            else
                matched_sub_cats.add(pResolved);
        }
        if (matched_main_cats.size == 0)
            matched_main_cats.add(lookup_category("Other"));
        if (!no_sub_cats) {
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



tListMeta::tListMeta(const tDesktopInfo& dinfo) {
    icon = Elvis((const char*) dinfo.get_icon_path(), "-");
    title = key = Elvis(dinfo.get_name(), "<UNKNOWN>");
}

        #ifdef OLD_IMP

t_menu_node root(tListMeta::make_dummy());
#endif

// variant with local description data
struct t_menu_node_app : t_menu_node
{
    tListMeta description;

    t_menu_node_app(const tDesktopInfo& dinfo) : t_menu_node(&description),
            description(dinfo) {

        LPCSTR cmdraw = g_app_info_get_commandline((GAppInfo*) dinfo.pInfo);
        if (!cmdraw || !*cmdraw)
            return;

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
        else if (bForTerminal && nonempty(terminal_command))
            progCmd = g_strjoin(" ", terminal_command, "-e", cmdMod, NULL);
        else
            // not simple command or needs a terminal started via launcher callback, or both
            progCmd = g_strdup_printf("%s \"%s\"", ApplicationName, dinfo.d_file);
        if (cmdMod && cmdMod != progCmd)
            g_free(cmdMod), cmdMod = nullptr;
        generic = dinfo.get_generic();
    }
    ~t_menu_node_app() {
        g_free(progCmd);
    }
};

std::string get_stem(const std::string& full_path) {
    LPCSTR dot(nullptr), start(full_path.c_str());
    for(auto p = start; ; ++p)
    {
        if ('/' == *p)
            start = p + 1;
        else if ('.' == *p) {
            dot = p;
        }
        else if (!*p) {
            if (!dot)
                dot = p;
            else if (dot < start)
                dot = p;

            break;
        }
    }
    return std::string(start, dot-start);
}

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
    if (!cat_name || !*cat_name)
        return;
    // try a perfect match by path or file name
    auto pCat = lookup_category(cat_name);
    if (!pCat)
    {
        auto bn = get_stem(szDesktopFile);
        if (!bn.empty())
            pCat = lookup_category(bn);
    }
    for (const tFromTo* p = SameCatMap;
            !pCat && p < SameCatMap+ACOUNT(SameCatMap);
            ++p) {

        if (0 == strcmp(cat_name, p->from))
            pCat = lookup_category(p->to);
    }

    if (!pCat)
        return;

        #ifdef OLD_IMP
    if (pCat->load_state_icon < tListMeta::SYSTEM_ICON) {
        LPCSTR icon_name = g_key_file_get_string (kf, "Desktop Entry",
                                                       "Icon", nullptr);
        if (icon_name && *icon_name) {
            pCat->icon = icon_name;
            pCat->load_state_icon = tListMeta::SYSTEM_ICON;
        }
    }
    if (pCat->load_state_title < tListMeta::SYSTEM_TRANSLATED) {
        char* cat_title = g_key_file_get_locale_string(kf, "Desktop Entry",
                                                       "Name", nullptr, nullptr);
        if (!cat_title)
            return;
        pCat->title = cat_title;
        char* cat_title_c = g_key_file_get_string(kf, "Desktop Entry",
                                                  "Name", nullptr);
        bool same_trans = 0 == strcmp (cat_title_c, cat_title);
        if (!same_trans)
            pCat->load_state_title = tListMeta::SYSTEM_TRANSLATED;

        // otherwise: not sure, keep searching for a better translation
    }
    // something special, donate the icon to similar items unless they have a better one

    for (auto p = SameIconMap; p < SameIconMap + ACOUNT(SameIconMap); ++p) {

        if (strcmp (pCat->key, p->from))
            continue;

        tListMeta *t = lookup_category (p->to);
        // give them at least some icon for now
        if (t && t->load_state_icon < tListMeta::FALLBACK_ICON) {
            t->icon = pCat->icon;
            t->load_state_icon = tListMeta::FALLBACK_ICON;
        }
    }
                #endif

}

void insert_app_info(const char* szDesktopFile) {
    tDesktopInfo dinfo(szDesktopFile);
    if (!dinfo)
        return;

    auto& to_where = tMenuAnchor::find_menu_anchor(dinfo);
    to_where.translated_apps.insert(std::move(dinfo));
}

void proc_dir_rec(LPCSTR syspath, unsigned depth,
        tFuncInsertInfo cb, LPCSTR szSubfolder,
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
        if (0 != g_stat(szFullName, &buf))
            continue;
        if (S_ISDIR(buf.st_mode)) {
            static ino_t reclog[6];
            for (unsigned i = 0; i < depth; ++i) {
                if (reclog[i] == buf.st_ino)
                    goto dir_visited_before;
            }
            if (depth < ACOUNT(reclog)) {
                reclog[++depth] = buf.st_ino;
                proc_dir_rec(szFullName, depth, cb, szSubfolder,
                        szFileSfx);
                --depth;
            }
            dir_visited_before: ;
        }

        if (!S_ISREG(buf.st_mode))
            continue;

        cb(szFullName);
    }
}

bool launch(LPCSTR dfile, char** argv, int argc) {
    GDesktopAppInfo *pInfo = g_desktop_app_info_new_from_filename(dfile);
    if (!pInfo)
        return false;
#if 0 // g_file_get_uri crashes, no idea why, even enforcing file prefix doesn't help
    if (argc > 0)
    {
        GList* parms = NULL;
        for (int i = 0; i < argc; ++i)
        parms = g_list_append(parms,
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
    return g_app_info_launch((GAppInfo *) pInfo, nullptr, nullptr, nullptr);
}

static void init() {
#ifdef CONFIG_I18N
    setlocale(LC_ALL, "");

    auto loc = YLocale::getCheckedExplicitLocale(false);
    right_to_left = loc && std::any_of(rtls, rtls + ACOUNT(rtls),
        [&](const char* rtl) {
            return rtl[0] == loc[0] && rtl[1] == loc[1];
        }
    );

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

#endif

    //meta_lookup_data = g_hash_table_new(g_str_hash, g_str_equal);
    //gh_directory_files = g_hash_table_new(g_str_hash, g_str_equal);

#ifdef OLD_IMP

    for (auto& what: spec::menuinfo) {
        if (no_sub_cats && what.parent_sec)
            continue;
        meta_lookup_data[what.key] = &what;
    }
#endif
    const char* terminals[] = { terminal_option, getenv("TERMINAL"), TERM,
                                "urxvt", "alacritty", "roxterm", "xterm",
                                "x-terminal-emulator", "terminator" };
    for (auto term : terminals)
        if (term && (terminal_command = path_lookup(term)) != nullptr)
            break;
}


#ifdef DEBUG_xxx
void dbgPrint(const gchar *msg)
{
    tlog("%s", msg);
}
#endif

int main(int argc, char** argv) {
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

    char* usershare = getenv("XDG_DATA_HOME");
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

    for (auto pArg = argv + 1; pArg < argv + argc; ++pArg) {
        if (is_version_switch(*pArg)) {
            g_fprintf(stdout,
                    "icewm-menu-fdo "
                    VERSION
                    ", Copyright 2015-2024 Eduard Bloch, 2017-2023 Bert Gijsbers.\n");
            exit(0);
        }
        else if (is_copying_switch(*pArg))
            print_copying_exit();
        else if (is_help_switch(*pArg))
            help(usershare, sysshare, stdout, EXIT_SUCCESS);
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
        else {
            char *value = nullptr, *expand = nullptr;
            if (GetArgument(value, "o", "output", pArg, argv + argc)) {
                if (*value == '~')
                    value = expand = tilde_expansion(value);
                else if (*value == '$')
                    value = expand = dollar_expansion(value);
                if (nonempty(value)) {
                    if (freopen(value, "w", stdout) == nullptr) {
                        fflush(stdout);
                    }
                }
                if (expand)
                    delete[] expand;
            }
            else if (GetArgument(value, "m", "match", pArg, argv + argc))
                substr_filter = value;
            else if (GetArgument(value, "M", "imatch", pArg, argv + argc))
                substr_filter_nocase = value;
            else if (GetArgument(value, "F", "flat-sep", pArg, argv + argc))
                flat_sep = value;
            else if (GetArgument(value, "t", "terminal", pArg, argv + argc))
                terminal_option = value;
            else // unknown option
                help(usershare, sysshare, stderr, EXIT_FAILURE);
        }
    }

    init();

    auto split_folders = [](const char* path_string, tCharVec& where) {
        for (auto p = g_strsplit(path_string, ":", -1); *p; ++p)
            where.add(*p);
    };
    split_folders(sysshare, sys_folders);
    split_folders(usershare, home_folders);

    for(auto& where: home_sys_folders) {
        for (auto p: *where)
            proc_dir_rec(p, 0, insert_app_info, "applications", "desktop");
    }


    for(auto& where: home_sys_folders) {
        for (auto p: *where) {
            proc_dir_rec(p, 0, pickup_folder_info, "desktop-directories",
                    "directory");
        }
    }

    //root.print();
    decorate_and_print();

    //printf("What? %s\n", gettext("Building"));

    if (nonempty(usershare) && usershare != getenv("XDG_DATA_HOME"))
        g_free(usershare);
    delete[] terminal_command;

    return EXIT_SUCCESS;
}
#else

#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>


typedef void (*tFuncInsertInfo)(const string& absPath, intptr_t any);

class FsScan {
    private:
    std::set<std::pair<ino_t, dev_t>> reclog;
    function<void(const string&)> cb;
    string sFileNameExtFilter;

    public:
    FsScan(decltype(FsScan::cb) cb, const string &sFileNameExtFilter = "") {
        this->cb = cb;
        this->sFileNameExtFilter = sFileNameExtFilter;
    }
    void scan(const string &sStartdir) {
        proc_dir_rec(sStartdir);
    }
private:
void proc_dir_rec(const string &path) {
    auto pdir = opendir(path.c_str());
    if (!pdir)
        return;
    auto_raii<DIR*, int, closedir> dircloser(pdir);
    auto fddir(dirfd(pdir));

    //const gchar *szFilename(nullptr);
    dirent *pent;
    struct stat stbuf;

    while (nullptr != (pent = readdir(pdir))) {
        if (pent->d_name[0] == '.')
            continue;
        pent->d_name[255] = 0;
        string fname(pent->d_name);
        if (!sFileNameExtFilter.empty() && !endsWith(fname, sFileNameExtFilter))
            continue;
        if (fstatat(fddir, fname.c_str(), &stbuf, 0))
            continue;
        if (S_ISDIR(stbuf.st_mode)) {
            // link loop detection
            auto prev = make_pair(stbuf.st_ino, stbuf.st_dev);
            auto hint = reclog.insert(prev);
            if (hint.second) { // we added a new one, otherwise do not descend
                proc_dir_rec(path + "/" + fname);
                reclog.erase(hint.first);
            }
        }

        if (!S_ISREG(stbuf.st_mode))
            continue;

        cb(path + "/" + fname);
    }
}

};

static void help(bool to_stderr, int xit) {
    (to_stderr ? cerr : cout) <<
            "USAGE: icewm-menu-fdo [OPTIONS] [FILENAME]\n"
            "OPTIONS:\n"
            "-g, --generic\tInclude GenericName in parentheses of progs\n"
            "-o, --output=FILE\tWrite the output to FILE\n"
            "-t, --terminal=NAME\tUse NAME for a terminal that has '-e'\n"
            "--seps  \tPrint separators before and after contents\n"
            "--sep-before\tPrint separator before the contents\n"
            "--sep-after\tPrint separator only after contents\n"
            "--no-sep-others\tNo separation of the 'Others' menu point\n"
            "--no-sub-cats\tNo additional subcategories, just one level of menues\n"
            "--flat\tDisplay all apps in one layer with category hints\n"
            "--flat-sep STR\tCategory separator string used in flat mode (default: ' / ')\n"
            "--match PAT\tDisplay only apps with title containing PAT\n"
            "--imatch PAT\tLike --match but ignores the letter case\n"
            "--match-sec\tApply --match or --imatch to apps AND sections\n"
            "--match-osec\tApply --match or --imatch only to sections\n"
            "FILENAME\tAny .desktop file to launch its application Exec command\n"
            "This program also listens to environment variables defined by\n"
            "the XDG Base Directory Specification:\n"
            "XDG_DATA_HOME=" << Elvis(getenv("XDG_DATA_HOME"), (char*)"") << "\n"
            "XDG_DATA_DIRS=" << Elvis(getenv("XDG_DATA_DIRS"), (char*)"") << endl;
    exit(xit);
}

struct tMenuNode {
    void sink_in(lint_ptr<DesktopFile> df);

    map<string,tMenuNode> submenues;
    map<string,tMenuNode> apps;
    unordered_set<string> dont_add_mark;

};

int main(int argc, char** argv) {

    // basic framework and environment initialization
    ApplicationName = my_basename(argv[0]);

#ifdef CONFIG_I18N
    setlocale(LC_ALL, "");

    auto msglang = YLocale::getCheckedExplicitLocale(false);
    right_to_left = msglang && std::any_of(rtls, rtls + ACOUNT(rtls),
        [&](const char* rtl) {
            return rtl[0] == msglang[0] && rtl[1] == msglang[1];
        }
    );
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

#warning FIXME, implement internal launcher which covers the damn %substitutions without glib
#if 0
    if (argc == 2 && ! endsWithSzAr(string(argv[1]), ".desktop"))
            && launch(argv[1], argv + 2, argc - 2) {
        return EXIT_SUCCESS;
    }
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
        sharedirs.push_back("/usr/local/share"), sharedirs.push_back("/usr/share");

    for (auto pArg = argv + 1; pArg < argv + argc; ++pArg) {
        if (is_version_switch(*pArg)) {
            cout << "icewm-menu-fdo "
                    VERSION
                    ", Copyright 2015-2024 Eduard Bloch, 2017-2023 Bert Gijsbers."
                <<endl;
            exit(0);
        }
        else if (is_copying_switch(*pArg))
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
        else {
            char *value = nullptr, *expand = nullptr;
            if (GetArgument(value, "o", "output", pArg, argv + argc)) {
                if (*value == '~')
                    value = expand = tilde_expansion(value);
                else if (*value == '$')
                    value = expand = dollar_expansion(value);
                if (nonempty(value)) {
                    if (freopen(value, "w", stdout) == nullptr) {
                        fflush(stdout);
                    }
                }
                if (expand)
                    delete[] expand;
            }
            else if (GetArgument(value, "m", "match", pArg, argv + argc))
                substr_filter = value;
            else if (GetArgument(value, "M", "imatch", pArg, argv + argc))
                substr_filter_nocase = value;
            else if (GetArgument(value, "F", "flat-sep", pArg, argv + argc))
                flat_sep = value;
            else if (GetArgument(value, "t", "terminal", pArg, argv + argc))
                terminal_option = value;
            else // unknown option
                help(true, EXIT_FAILURE);
        }
    }

    auto shortLang = string(msglang ? msglang : "").substr(0, 2);

    tMenuNode root;

    auto desktop_loader = FsScan([&](const string& fPath) {
            #ifdef DEBUG
            cerr << "reading: " << fPath <<endl;
            #endif
            auto df = DesktopFile::load(fPath, msglang);
            if (df)
                root.sink_in(df);
        }, ".desktop");
    for(const auto& sdir: sharedirs) {
        #ifdef DEBUG
        cerr << "checkdir: " << sdir << endl;
        #endif
        desktop_loader.scan(sdir + "/applications");
    }

    /*
    cout << "lang: " << shortLang << endl;
    int ret = 0;
    for(int i=1; i < argc; ++i) {
    DesktopFile x(argv[i], shortLang);
    ret += x.IsApp;

    }
    */
    return EXIT_SUCCESS;
}
#endif

void tMenuNode::sink_in(lint_ptr<DesktopFile> pDf)
{
    auto& df=*pDf;
    dont_add_mark.clear();
    bool bFoundCategories = false;

    auto add_sub_menues = [&](const t_menu_path& mp) {
        tMenuNode* cur = this;
            for (auto it = std::rbegin(mp); it != std::rend(mp); ++it) {
            cerr << "adding submenu: " <<*it <<endl;
            cur = &cur->submenues[*it];
        }
        return cur;
    };

    for(const auto& cat: df.Categories) {
        cerr << "where does it fit? " << cat << endl;
        t_menu_path refval = {df.Name.c_str()};
        static auto comper = [](const t_menu_path& a, const t_menu_path& b) {
            cerr << "left: " << *a.begin() << " vs. right: " << *b.begin() << endl;
             return strcmp(*a.begin(), *b.begin()) < 0;
              };
    for(const auto& w: valid_paths) {
        auto rng = std::equal_range(w.begin(), w.end(), refval, comper);
        for(auto it = rng.first; it != rng.second; ++it) {
            auto&tgt = * add_sub_menues(*it);
        }

    }

    }
    
}

// vim: set sw=4 ts=4 et:

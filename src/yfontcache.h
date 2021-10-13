#ifndef YFONTCACHE_H
#define YFONTCACHE_H

#include "yarray.h"

class YFontBase;

class YFontStore {
public:
    YFontStore(const char* s, unsigned long h, YFontBase* f)
        : name(s), hash(h), font(f) { }
    ~YFontStore() { delete font; }
    bool match(const char* s, unsigned long h) const {
        return h == hash && 0 == strcmp(s, name);
    }
    YFontBase* getFont() const { return font; }
private:
    const char* name;
    unsigned long hash;
    YFontBase* font;
};

class YFontCache {
public:
    YFontBase* lookup(const char* name) {
        unsigned long hash = strhash(name);
        for (YFontStore* store : storage) {
            if (store->match(name, hash)) {
                return store->getFont();
            }
        }
        return nullptr;
    }
    void store(const char* name, YFontBase* font) {
        unsigned long hash = strhash(name);
        storage.append(new YFontStore(name, hash, font));
    }
    void clear() {
        storage.clear();
    }
private:
    YObjectArray<YFontStore> storage;
};

extern class YFontCache fontCache;

#endif

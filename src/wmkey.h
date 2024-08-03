#ifndef WMKEY_H
#define WMKEY_H

struct WMKey {
    KeySym key;
    unsigned mod;
    const char* name;
    bool initial;

    WMKey() : key(NoSymbol), mod(0), name(""), initial(true) { }
    WMKey(char* s) : key(NoSymbol), mod(0), name(s), initial(false) { parse(); }
    WMKey(KeySym k, unsigned m, const char* s) :
        key(k), mod(m), name(s), initial(true) { }

    bool eq(KeySym k, unsigned m) const { return key == k && mod == m; }
    bool operator==(const WMKey& o) const { return eq(o.key, o.mod); }
    bool operator!=(const WMKey& o) const { return !eq(o.key, o.mod); }
    bool parse();
    bool set(const char* arg);
};

#endif

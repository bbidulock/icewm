#ifndef __YKEYBIND_H
#define __YKEYBIND_H

class YKeyEvent;

class YKeyBind {
public:
    YKeyBind(const char *key);
    YKeyBind(int key, int mod);
    ~YKeyBind();

    bool match(const YKeyEvent &key);
    bool match(int key, int mod);

    int getKey() { return key; }
    int getModifiers() { return modifiers; }
private:
    const char *binding;
    int key;
    int modifiers;
};

#endif

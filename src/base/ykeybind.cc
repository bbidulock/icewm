#include "config.h"
#include "ykeybind.h"
#include "ykeyevent.h"
#include "base.h"

YKeyBind::YKeyBind(const char *aBinding) {
    binding = newstr(aBinding);
    key = -1;
    modifiers = -1;
}

YKeyBind::YKeyBind(int aKey, int aMod) {
    binding = 0;
    key = aKey;
    modifiers = aMod;
}

YKeyBind::~YKeyBind() {
    delete [] binding; binding = 0;
}

bool YKeyBind::match(const YKeyEvent &key) {
    return match(key.getKey(), key.getKeyModifiers());
}

bool YKeyBind::match(int aKey, int aMod) {
    if (key == aKey && modifiers == aMod)
        return true;
    return false;
}

#if 0
//bool YKeyBind::match(const YKeyBind *key) {
//    return match(key->getKey(), key->getModifiers());
//}
#endif

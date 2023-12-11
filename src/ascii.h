#ifndef ASCII_H
#define ASCII_H

namespace ASCII {

    template<class T>
    inline bool isSign(T c) {
        return c == '+' || c == '-';
    }

    template<class T>
    bool isLower(T c) {
        return c >= 'a' && c <= 'z';
    }

    template<class T>
    bool isUpper(T c) {
        return c >= 'A' && c <= 'Z';
    }

    template<class T>
    bool isAlpha(T c) {
        return isLower(c) || isUpper(c);
    }

    template<class T>
    bool isDigit(T c) {
        return c >= '0' && c <= '9';
    }

    template<class T>
    bool isAlnum(T c) {
        return isAlpha(c) || isDigit(c);
    }

    template<class T>
    bool isPrint(T c) {
        return ' ' <= c && c <= '~';
    }

    template<class T>
    bool isControl(T c) {
        return ' ' < c && c <= '~' && !isAlnum(c);
    }

    template<class T>
    T toUpper(T c) {
        return isLower(c) ? (c - ' ') : c;
    }

    template<class T>
    T toLower(T c) {
        return isUpper(c) ? (c + ' ') : c;
    }

    template<class T>
    bool isSpaceOrTab(T c) {
        return c == ' ' || c == '\t';
    }

    template<class T>
    bool isWhiteSpace(T c) {
        return isSpaceOrTab(c) || c == '\n' || c == '\r';
    }

    inline bool isLineEnding(const char* s) {
        return *s == '\n' || (*s == '\r' && s[1] == '\n');
    }

    inline bool isEscapedLineEnding(const char* s) {
        return *s == '\\' && isLineEnding(s + 1);
    }

    template<class T>
    T* pastSpacesAndTabs(T* s) {
        while (isSpaceOrTab(*s))
            ++s;
        return s;
    }

    template<class T>
    T* pastWhiteSpace(T* s) {
        while (isWhiteSpace(*s))
            ++s;
        return s;
    }

    template<class T>
    int hexDigit(T c) {
        return c >= '0' && c <= '9' ? int(c - '0') :
               c >= 'a' && c <= 'f' ? int(c - 'a') + 10 :
               c >= 'A' && c <= 'F' ? int(c - 'A') + 10 :
               -1;
    }

    template<class T>
    int spanLower(T* c) {
        int i = 0;
        while (c[i] >= 'a' && c[i] <= 'z')
            ++i;
        return i;
    }

    template<class T>
    int spanUpper(T* c) {
        int i = 0;
        while (c[i] >= 'A' && c[i] <= 'Z')
            ++i;
        return i;
    }
}

#endif

// vim: set sw=4 ts=4 et:

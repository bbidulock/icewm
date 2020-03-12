#ifndef __ASCII_H
#define __ASCII_H

namespace ASCII {

    template<class T>
    inline bool isSign(T c) {
        return c == '+' || c == '-';
    }

    template<class T>
    static bool isLower(T c) {
        return c >= 'a' && c <= 'z';
    }

    template<class T>
    static bool isUpper(T c) {
        return c >= 'A' && c <= 'Z';
    }

    template<class T>
    static bool isAlpha(T c) {
        return isLower(c) || isUpper(c);
    }

    template<class T>
    static bool isDigit(T c) {
        return c >= '0' && c <= '9';
    }

    template<class T>
    static bool isAlnum(T c) {
        return isAlpha(c) || isDigit(c);
    }

    template<class T>
    static bool isPrint(T c) {
        return ' ' <= c && c <= '~';
    }

    template<class T>
    static bool isControl(T c) {
        return ' ' < c && c <= '~' && !isAlnum(c);
    }

    template<class T>
    static T toUpper(T c) {
        return isLower(c) ? (c - ' ') : c;
    }

    template<class T>
    static T toLower(T c) {
        return isUpper(c) ? (c + ' ') : c;
    }

    template<class T>
    static bool isSpaceOrTab(T c) {
        return c == ' ' || c == '\t';
    }

    template<class T>
    static bool isWhiteSpace(T c) {
        return isSpaceOrTab(c) || c == '\n' || c == '\r';
    }

    static inline bool isLineEnding(const char* s) {
        return *s == '\n' || (*s == '\r' && s[1] == '\n');
    }

    static inline bool isEscapedLineEnding(const char* s) {
        return *s == '\\' && isLineEnding(s + 1);
    }

    template<class T>
    static T* pastSpacesAndTabs(T* s) {
        while (isSpaceOrTab(*s))
            ++s;
        return s;
    }

    template<class T>
    static T* pastWhiteSpace(T* s) {
        while (isWhiteSpace(*s))
            ++s;
        return s;
    }

    template<class T>
    static int hexDigit(T c) {
        return c >= '0' && c <= '9' ? int(c - '0') :
               c >= 'a' && c <= 'f' ? int(c - 'a') + 10 :
               c >= 'A' && c <= 'F' ? int(c - 'A') + 10 :
               -1;
    }

}

#endif

// vim: set sw=4 ts=4 et:

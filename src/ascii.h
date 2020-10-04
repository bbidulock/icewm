#ifndef __ASCII_H
#define __ASCII_H

namespace ASCII {

    static inline bool isSign(char c) {
        return c == '+' || c == '-';
    }

    static bool isLower(char c) {
        return c >= 'a' && c <= 'z';
    }

    static bool isUpper(char c) {
        return c >= 'A' && c <= 'Z';
    }

    static bool isAlpha(char c) {
        return isLower(c) || isUpper(c);
    }

    static bool isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    static bool isAlnum(char c) {
        return isAlpha(c) || isDigit(c);
    }

    static inline bool isPrint(char c) {
        return ' ' <= c && c <= '~';
    }

    inline static bool isControl(char c) {
        return ' ' < c && c <= '~' && !isAlnum(c);
    }

    inline static char toUpper(char c) {
        return isLower(c) ? (c - ' ') : c;
    }

    inline static char toLower(char c) {
        return isUpper(c) ? (c + ' ') : c;
    }

    static bool isSpaceOrTab(char c) {
        return c == ' ' || c == '\t';
    }

    static bool isWhiteSpace(char c) {
        return isSpaceOrTab(c) || c == '\n' || c == '\r';
    }

    static inline bool isLineEnding(const char* s) {
        return *s == '\n' || (*s == '\r' && s[1] == '\n');
    }

    static inline bool isEscapedLineEnding(const char* s) {
        return *s == '\\' && isLineEnding(s + 1);
    }

    static inline const char* pastSpacesAndTabs(const char* s) {
        while (isSpaceOrTab(*s))
            ++s;
        return s;
    }

    static inline char* pastWhiteSpace(char* s) {
        while (isWhiteSpace(*s))
            ++s;
        return s;
    }

    static inline int hexDigit(char c) {
        return c >= '0' && c <= '9' ? int(c - '0') :
               c >= 'a' && c <= 'f' ? int(c - 'a') + 10 :
               c >= 'A' && c <= 'F' ? int(c - 'A') + 10 :
               -1;
    }
}

#endif

// vim: set sw=4 ts=4 et:

#ifndef __ASCII_H
#define __ASCII_H

class ASCII {
public:
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
};

#endif

// vim: set sw=4 ts=4 et:

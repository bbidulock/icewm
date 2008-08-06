#ifndef __ASCII_H
#define __ASCIIH_

class ASCII {
public:
    static bool isLower(char c) {
        return c >= 'a' && c <= 'z';
    }

    static bool isUpper(char c) {
        return c >= 'A' && c <= 'Z';
    }

    static char toUpper(char c) {
        return isLower(c) ? (char)(c - ' ') : c;
    }

    static char toLower(char c) {
        return isUpper(c) ? (char)(c + ' ') : c;
    }

    static bool isSpaceOrTab(char c) {
        return c == ' ' || c == '\t';
    }
};

#endif

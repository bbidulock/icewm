#ifndef __ASCII_H
#define __ASCIIH_

class ASCII {
public:
    static bool isLower(unsigned char c) {
        return c >= 'a' && c <= 'z';
    }

    static bool isUpper(unsigned char c) {
        return c >= 'A' && c <= 'Z';
    }

    static unsigned char toUpper(unsigned char c) {
        return isLower(c) ? c - ' ' : c;
    }

    static unsigned char toLower(unsigned char c) {
        return isUpper(c) ? c + ' ' : c;
    }

    static bool isSpaceOrTab(unsigned char c) {
        return c == ' ' || c == '\t';
    }
};

#endif

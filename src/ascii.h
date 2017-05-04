#ifndef __ASCII_H
#define __ASCII_H

class ASCII {
public:
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

    static char toUpper(char c) {
        return isLower(c) ? (char)(c - ' ') : c;
    }

    static char toLower(char c) {
        return isUpper(c) ? (char)(c + ' ') : c;
    }

    static bool isSpaceOrTab(char c) {
        return c == ' ' || c == '\t';
    }

    static bool isWhiteSpace(char c) {
        return isSpaceOrTab(c) || c == '\n' || c == '\r';
    }

    static bool isLower(int c) {
        return c >= 'a' && c <= 'z';
    }

    static bool isUpper(int c) {
        return c >= 'A' && c <= 'Z';
    }

    static bool isAlpha(int c) {
        return isLower(c) || isUpper(c);
    }

    static bool isDigit(int c) {
        return c >= '0' && c <= '9';
    }

    static bool isAlnum(int c) {
        return isAlpha(c) || isDigit(c);
    }

    static int toUpper(int c) {
        return isLower(c) ? (char)(c - ' ') : c;
    }

    static int toLower(int c) {
        return isUpper(c) ? (char)(c + ' ') : c;
    }

    static bool isSpaceOrTab(int c) {
        return c == ' ' || c == '\t';
    }

    static bool isWhiteSpace(int c) {
        return isSpaceOrTab(c) || c == '\n' || c == '\r';
    }
};

#endif

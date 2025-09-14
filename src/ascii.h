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

    template<class T>
    bool utf0(T c) { return (c & 0xC0) == 0x80; }

    template<class T>
    bool utf1(T c) { return (c & 0xE0) == 0xC0; }

    template<class T>
    bool utf2(T c) { return (c & 0xF0) == 0xE0; }

    template<class T>
    bool utf3(T c) { return (c & 0xF8) == 0xF0; }

    template<class T>
    bool utf1(T c, T d) {
        return utf1(c) && utf0(d);
    }

    template<class T>
    bool utf2(T c, T d, T e) {
        return utf2(c) && utf0(d) && utf0(e);
    }

    template<class T>
    bool utf3(T c, T d, T e, T f) {
        return utf3(c) && utf0(d) && utf0(e) && utf0(f);
    }

    template<class T>
    int codepoint_size(T c) {
        return utf1(c) ? 2 : utf2(c) ? 3 : utf3(c) ? 4 : 1;
    }

    template<class T>
    unsigned codepoint(T* s) {
        if (utf1(s[0])) {
            if (utf0(s[1])) {
               return ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
            }
        }
        else if (utf2(s[0])) {
            if (utf0(s[1]) && utf0(s[2])) {
                return ((s[0] & 0x0F) << 12)
                     | ((s[1] & 0x3F) << 6)
                     | (s[2] & 0x3F);
            }
        }
        else if (utf3(s[0])) {
            if (utf0(s[1]) && utf0(s[2]) && utf0(s[3])) {
                return ((s[0] & 0x07) << 18)
                     | ((s[1] & 0x3F) << 12)
                     | ((s[2] & 0x3F) << 6)
                     | (s[3] & 0x3F);
            }
        }
        return (s[0] & 0xFF);
    }

    template<class T>
    bool is_combining_mark(T cp) {
        return (cp >= 0x0300 && cp <= 0x036F) ||
               (cp >= 0x1AB0 && cp <= 0x1AFF) ||
               (cp >= 0x1DC0 && cp <= 0x1DFF) ||
               (cp >= 0x20D0 && cp <= 0x20FF) ||
               (cp >= 0xFE20 && cp <= 0xFE2F);
    }
}

#endif

// vim: set sw=4 ts=4 et:

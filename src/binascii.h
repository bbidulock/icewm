#ifndef __BINASCII_H
#define __BINASCII_H

class BinAscii {
public:
    static int unhex(int c) {
        return ((c >= '0' && c <= '9') ? c - '0' :
                (c >= 'A' && c <= 'F') ? c - 'A' + 10 :
                (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1);
    }
};

#endif

// vim: set sw=4 ts=4 et:

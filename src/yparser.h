/*
 *  IceWM - Definition of a generic parser
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YPARSER_H
#define __YPARSER_H

#include <cstdio>

/*******************************************************************************
 * A generic parser
 ******************************************************************************/

class YParser {
public:
    int parse(const char *filename);
    int parse(int fd);

protected:
    YParser():
        fStream(NULL), fFilename(NULL), fLine(0), fColumn(0), fChar(EOF) {}

    int nextChar();
    int currChar() const { return fChar; }
    bool good() const { return (NULL != fStream && EOF != fChar); }
    bool eof() const { return (NULL == fStream || EOF == fChar); }
    
    unsigned skipBlanks();
    unsigned skipWhitespace();
    void skipLine();
    
    char *getLine(char *buf, const size_t len);
    char *getIdentifier(char *buf, const size_t len, bool acceptDash = false);
    char *getString(char *buf, const size_t len);
    char *getTag(char *buf, const size_t len, char begin, char end);
    char *getSectionTag(char *buf, const size_t len);
    char *getSGMLTag(char *buf, const size_t len);

    void reportParseError(const char *what);
    void reportUnexpectedIdentifier(const char *id);
    void reportIdentifierExpected();
    void reportSeparatorExpected();
    void reportInvalidToken();
    
    virtual void parseStream() = 0;

private:
    FILE *fStream;
    const char *fFilename;
    size_t fLine, fColumn;
    int fChar;
};

#endif

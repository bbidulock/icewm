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
    int parse(const char * filename);
    
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
    
    char * getIdentifier(char * buf, const size_t len);
    char * getString(char * buf, const size_t len);

    void parseError(const char * what);
    void unexpectedIdentifier(const char * id);
    void identifierExpected();
    void separatorExpected();
    
    virtual void parseStream() = NULL;

private:
    FILE * fStream;
    const char * fFilename;
    size_t fLine, fColumn;
    int fChar;
};

#endif

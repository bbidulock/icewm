/*
 *  IceWM - Definition of a generic parser
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YPARSER_H
#define __YPARSER_H

/*******************************************************************************
 * A generic parser
 ******************************************************************************/

#ifndef YParserFile
#define YParserFile void
#endif

class YParser {
public:
    int parse(const char *filename);
    int parse(int fd);

protected:
    YParser();

    int nextChar();
    int currChar() const { return fChar; }
    bool good() const;
    bool eof() const;
    
    unsigned skipBlanks();
    unsigned skipWhitespace();
    void skipLine();
    
    char *getLine(char *buf, const unsigned int len);
    char *getIdentifier(char *buf, const unsigned int len, bool acceptDash = false);
    char *getString(char *buf, const unsigned int len);
    char *getTag(char *buf, const unsigned int len, char begin, char end);
    char *getSectionTag(char *buf, const unsigned int len);
    char *getSGMLTag(char *buf, const unsigned int len);

    void reportParseError(const char *what);
    void reportUnexpectedIdentifier(const char *id);
    void reportIdentifierExpected();
    void reportSeparatorExpected();
    void reportInvalidToken();
    
    virtual void parseStream() = 0;

private:
    YParserFile *fStream;
    const char *fFilename;
    int fLine, fColumn;
    int fChar;
};

#endif

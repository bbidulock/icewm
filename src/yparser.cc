/*
 *  IceWM - Generic parser
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/01/25: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 *  2001/01/27: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - specialized error message methods
 *  - splitted skipSpaces into skipBlanks and skipWhitespace
 */

#include "base.h"
#include "intl.h"
#include "yparser.h"

#include <cerrno>
#include <cstring>
#include <cctype>
#include <unistd.h>

int YParser::parse(const char * filename) {
    bool isExecutable(access(filename, X_OK) == 0);

    if ((fStream = isExecutable ? popen(filename, "r")
				: fopen(filename, "r")) == NULL) {
	warn("%s: %s", filename, strerror(errno));
	return errno;
    }

    fFilename = filename;
    nextChar();
    
    parseStream();
    
    if (isExecutable)
	pclose(fStream);
    else
	fclose(fStream);

    fStream = NULL;
    fFilename = NULL;
    fLine = 0;
    fColumn = 0;
    
    return 0;
}

int YParser::nextChar() {
    if (fStream)
	if ((fChar = fgetc(fStream)) == '\n' || fLine == 0)
	    ++fLine, fColumn = 0;
	else
	    ++fColumn;
    else
        fChar = EOF;

    return fChar;
}

unsigned YParser::skipBlanks() {
    unsigned spaces(0);
    
    while (good() && isspace(currChar()) && currChar() != '\n') {
	nextChar(); ++spaces;
    }

    if (currChar() == '#') skipLine();
    return spaces;
}

unsigned YParser::skipWhitespace() {
    unsigned spaces(0);
    
    while (good() && (isspace(currChar()) || '#' == currChar())) {
	if (currChar() == '#') skipLine();
	else nextChar();
	++spaces;
    }
    
    return spaces;
}

void YParser::skipLine() {
    while (good() && currChar() != '\n') nextChar(); nextChar();
}

char * YParser::getIdentifier(char * buf, const size_t len) {
    size_t pos(0);

    while (isalnum(currChar()) && pos < len - 1) {
        buf[pos++] = currChar(); nextChar();
    }
    
    buf[pos] = '\0';
    return buf;
}

char * YParser::getString(char * buf, const size_t len) {
    size_t pos(0);
    bool instr(false);

    while ((instr || currChar() == '"' || !isspace(currChar())) && 
	   pos < len - 1) {
        if (currChar() == '\\')
	    switch (nextChar()) {
		default: buf[pos++] = currChar(); break;

		case 'a': buf[pos++] = '\a'; break;
		case 'b': buf[pos++] = '\b'; break;
		case 'e': buf[pos++] =   27; break;
		case 'f': buf[pos++] = '\f'; break;
		case 'n': buf[pos++] = '\n'; break;
		case 'r': buf[pos++] = '\r'; break;
		case 't': buf[pos++] = '\t'; break;
		case 'v': buf[pos++] = '\v'; break;

		case 'x': {
		    int a, b;

		    if ((a = unhex(nextChar())) != -1 &&
			(b = unhex(nextChar())) != -1)
			buf[pos++] = (unsigned char)((a * 16) + b);
		    else
			parseError(_("Pair of hexadecimal digits expected"));
                    break;
		}
            }
        else if (currChar() == '"')
            instr = !instr;
        else
            buf[pos++] = currChar();

	nextChar();
    }

    buf[pos] = '\0';
    return buf;
}

void YParser::parseError(const char * what) {
    warn("%s:%d:%d: %s", fFilename, fLine, fColumn, what);
}

void YParser::unexpectedIdentifier(const char * id) {
    char * msg = strJoin(_("Unexpected identifier"), ": »", id, "«", NULL);
    parseError(msg);
    delete[] msg;
}

void YParser::identifierExpected() {
    char * msg = strJoin(_("Identifier expected"), NULL);
    parseError(msg);
    delete[] msg;
}

void YParser::separatorExpected() {
    parseError(_("Separator expected"));
}

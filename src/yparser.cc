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

#include "config.h"

#include <cstdio>
#define YParserFile FILE
#include "yparser.h"

#include "binascii.h"
#include "base.h"

#include "intl.h"
#include <cerrno>
#include <cstring>
#include <cctype>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


YParser::YParser():
    fStream(NULL),
    fFilename(NULL),
    fLine(0), fColumn(0),
    fChar(EOF)
{
}

bool YParser::good() const {
    return (NULL != fStream && EOF != fChar);
}
bool YParser::eof() const { return (NULL == fStream || EOF == fChar); }

int YParser::parse(const char *filename) {
    fFilename = filename;

    if (access(filename, X_OK)) {
        int fd = open(fFilename, O_RDONLY);
        
        if (-1 != fd) {
            return parse(fd);
        } else {
            warn(_("Failed to open %s: %s"), fFilename, strerror(errno));
            return -1;
        }
    } else {
        struct stat inode;
        
        if (!stat(fFilename, &inode) && S_ISREG(inode.st_mode)) {
            int fds[2];
            
            if (pipe(fds)) {
                warn(_("Failed to create anonymous pipe: %s"), strerror(errno));
                return -1;
            }

            switch(fork()) {
            case 0:
                close(0);
                close(fds[0]);

                if (1 != dup2(fds[1], 1)) {
                    warn(_("Failed to duplicate file descriptor: %s"), strerror(errno));
                    return -1;
                }

                execlp(fFilename, fFilename, NULL);

                warn(_("Failed to execute %s: %s"), fFilename, strerror(errno));
                _exit(99);
                return -1;

            default:
                close(1);
                close(fds[1]);

                return parse(fds[0]);

            case -1:
                warn(_("Failed to create child process: %s"), strerror(errno));
                return -1;
            }
        }
        else {
            warn(_("Not a regular file: %s"), fFilename);
            return -1;
        }
    }
}

int YParser::parse(int fd) {
    if (NULL == (fStream = fdopen(fd, "r"))) {
        warn("%s: %s", fFilename, strerror(errno));
        return errno;
    }

    nextChar();

    parseStream();

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

char *YParser::getLine(char *buf, const unsigned int len) {
    size_t pos(0);
    buf[pos] = '\0';

    while (good() && !strchr("\n#", buf[pos] = currChar()) && pos < len - 1) {
        nextChar();
        ++pos;
    }

    skipLine();

    buf[pos] = '\0';
    return buf;
}

char *YParser::getIdentifier(char *buf, const unsigned int len, bool acceptDash) {
    size_t pos(0);;
    buf[pos] = '\0';

    while (good() && 
          (isalnum(currChar()) || (acceptDash && '-' == currChar())) && 
           pos < len - 1) {
        buf[pos++] = currChar(); 
        nextChar();
    }
    
    buf[pos] = '\0';
    return buf;
}

char *YParser::getString(char *buf, const unsigned int len) {
    size_t pos(0);;
    buf[pos] = '\0';

    bool instr(false);

    while (good() && (instr || currChar() == '"' || !isspace(currChar())) && 
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

                    if ((a = BinAscii::unhex(nextChar())) != -1 &&
                        (b = BinAscii::unhex(nextChar())) != -1)
                        buf[pos++] = (unsigned char)((a << 4) + b);
                    else
                        reportParseError(_("Pair of hexadecimal digits expected"));
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

char *YParser::getTag(char *buf, const unsigned int len, char begin, char end) {
    if (good() && begin == currChar()) {
        size_t pos(0);

        while (good() && end != nextChar() && pos < len - 1) {
            buf[pos++] = currChar();
        }
        
        if (pos >= len - 1) {
            while (good() && end != currChar()) nextChar();
        }

        nextChar();

        buf[pos] = '\0';
    } else {
        buf[0] = '\0';
        buf = NULL;
    }

    return buf;
}

char *YParser::getSectionTag(char *buf, const unsigned int len) {
    return getTag(buf, len, '[', ']');
}

char *YParser::getSGMLTag(char *buf, const unsigned int len) {
    return getTag(buf, len, '<', '>');
}

void YParser::reportParseError(const char *what) {
    warn("%s:%d:%d: %s", fFilename, fLine, fColumn, what);
}

void YParser::reportUnexpectedIdentifier(const char *id) {
    char *msg = strJoin(_("Unexpected identifier"), ": '", id, "'", NULL);
    reportParseError(msg);
    delete[] msg;
}

void YParser::reportIdentifierExpected() {
    reportParseError(_("Identifier expected"));
}

void YParser::reportSeparatorExpected() {
    reportParseError(_("Separator expected"));
}

void YParser::reportInvalidToken() {
    reportParseError(_("Invalid token"));
}

/*
 *  IceWM - Definition of a generic command line parser
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YCMDLINE_H
#define __YCMDLINE_H

/*******************************************************************************
 * A generic command line parser
 ******************************************************************************/

class YCommandLine {
public:
    YCommandLine(int & argc, char **& argv):
        argc(argc), argv(argv) {}
    virtual ~YCommandLine() {}    
    int parse();
    
protected:
    virtual char getArgument(char const * const & arg, char const *& val) = 0;
    virtual int setOption(char const * arg, char opt, char const * val);
    virtual int setArgument(int pos, char const * val);

    char const * getValue(char const * const & arg, char const * vptr);
    void eatArgument(int idx);

    int & argc; 
    char **& argv;
};

#endif

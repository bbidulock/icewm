#include "config.h"
#include "ylocale.h"
#include "yapp.h"

#include <stdio.h>
#include <string.h>

char const * YApplication::Name = "testlocale";

static char *
foreign_str(char const * charset, char const * foreign)
{
    size_t len(strlen(charset) + strlen(foreign));
    char * str = new char [len + 8];

    sprintf (str, "\033%%/1\\%03o\\%03o%s\002%s",
             128 + len / 128, 128 + len % 128, charset, foreign);

    return str;
}

static void
print_string(YLChar * lstr, YUChar * ustr)
{
    printf ("In locale encoding: \"%s\"\n", lstr);
    printf ("In unicode encoding: ");

    for (YUChar * u(ustr); *u; ++u)
	printf("\\U%04x%s", *u, u[1] ? ", " : "");

    puts ("");
}

int main() {
    size_t ulen;

    YLChar * lstr("Möhrenkäuter");
    YUChar * ustr(YLocale("de_DE.iso-8859-1").
	          unicodeString(lstr, strlen(lstr), ulen));
    print_string (lstr, ustr);

    lstr = foreign_str ("ISO8859-15", "Euro sign: ¤");
    ustr = YLocale("de_DE.iso-8859-1").
	           unicodeString(lstr, strlen(lstr), ulen);
    print_string (lstr, ustr);


/*
    YLChar * utf8(YLocale("de_DE.utf8").localeString(unicode));
    printf("utf8: %s\n", utf8);

    YLChar * latin1(YLocale("de_DE.iso-8859-1").localeString(unicode));
    printf("iso-8859-1: %s\n", latin1);
*/
    return 0;
}

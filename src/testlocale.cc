#include "config.h"
#include "ylocale.h"
#include "yapp.h"

#include <stdio.h>
#include <string.h>

char const *ApplicationName("testlocale");
bool multiByte(true);

#ifdef CONFIG_I18N
static char *
foreign_str(char const *charset, char const *foreign)
{
    size_t len(strlen(charset) + strlen(foreign));
    size_t size = len + 80;
    char * str = new char [size];

    snprintf(str, size, "\033%%/1\\%03zo\\%03zo%s\002%s",
             128 + len / 128, 128 + len % 128, charset, foreign);

    return str;
}

static void
print_string(const YLChar *lstr, YUChar *ustr)
{
    printf ("In locale encoding: \"%s\"\n", lstr);
    printf ("In unicode encoding: \"");

    for (YUChar * u(ustr); *u; ++u) printf("\\U%04x", *u);

    puts ("\"");
}

#define TEST_RATING(LocaleFragment) \
    printf("Rating for '" LocaleFragment "': %d\n", \
           YLocale::getRating(LocaleFragment));

#endif

int main() {
#ifdef CONFIG_I18N
    size_t ulen;

    const YLChar *lstr("Möhrenkäuter");
    YUChar *ustr(YLocale("de_DE.iso-8859-1").
                 unicodeString(lstr, strlen(lstr), ulen));
    print_string(lstr, ustr);

    lstr = foreign_str ("ISO8859-15", "Euro sign: ¤");
    ustr = YLocale("de_DE.iso-8859-1").
        unicodeString(lstr, strlen(lstr), ulen);
    print_string(lstr, ustr);


/*
    YLChar * utf8(YLocale("de_DE.utf8").localeString(unicode));
    printf("utf8: %s\n", utf8);

    YLChar * latin1(YLocale("de_DE.iso-8859-1").localeString(unicode));
    printf("iso-8859-1: %s\n", latin1);
*/

    {
        YLocale locale("de_DE@euro");
        printf("Current locale: %s\n", YLocale::getLocaleName());

        TEST_RATING("C");
        TEST_RATING("en");
        TEST_RATING("de");
        TEST_RATING("de_CH");
        TEST_RATING("de_DE");
        TEST_RATING("de_DE@DEM");
        TEST_RATING("de_DE@euro");
    }

    {
        YLocale locale("ru_RU.koi8r");
        printf("Current locale: %s\n", YLocale::getLocaleName());

        TEST_RATING("C");
        TEST_RATING("en");
        TEST_RATING("ru");
        TEST_RATING("ru_UA");
        TEST_RATING("ru_RU");
        TEST_RATING("ru_RU.cp1251");
        TEST_RATING("ru_RU.koi8r");
    }
#else
    puts("I18N disabled.");
#endif
    return 0;
}

// vim: set sw=4 ts=4 et:

#include "config.h"
#include "ylocale.h"
#include "yapp.h"

#include <stdio.h>
#include <string.h>
#include <langinfo.h>

char const *ApplicationName("testlocale");
bool multiByte(true);

#ifdef CONFIG_I18N
static char *
foreign_str(char const *charset, char const *foreign)
{
    size_t len(strlen(charset) + strlen(foreign));
    size_t size = len + 80;
    char * str = new char [size];

    snprintf(str, size, "%s: %s",
             charset, foreign);

    return str;
}

static void
print_string(const char *lstr, wchar_t *ustr, size_t ulen)
{
    printf ("In locale encoding: \"%s\"\n", lstr);
    printf ("In unicode encoding: \"");

    for (wchar_t * u(ustr); *u; ++u) printf("\\U%04x", *u);

    printf ("\" (len=%zu)\n", ulen);
}

#define TEST_RATING(LocaleFragment) \
    printf("Rating for '" LocaleFragment "': %d\n", \
           YLocale::getRating(LocaleFragment));

#endif

int main() {
#ifdef CONFIG_I18N
    {
        YLocale locale;
        printf("Default locale: %s\n", YLocale::getLocaleName());
        printf("nl_langinfo(%d): %s\n", CODESET, nl_langinfo(CODESET));
    }

    size_t ulen;
    wchar_t* ustr;

    {
        const char* lstr = "Möhrenkäuter";
        YLocale locale("de_DE.iso-8859-1");
        ulen = 0;
        ustr = locale.unicodeString(lstr, strlen(lstr), ulen);
        print_string(lstr, ustr, ulen);

        size_t llen = 0;
        char * cstr = locale.localeString(ustr, ulen, llen);
        printf("cstr: %s (%zu)\n", cstr, llen);
    }

    {
        const char* lstr = foreign_str ("ISO8859-15", "Euro sign: ¤");
        YLocale locale("de_DE.iso-8859-1");
        ulen = 0;
        ustr = locale.unicodeString(lstr, strlen(lstr), ulen);
        print_string(lstr, ustr, ulen);

        size_t llen = 0;
        char * cstr = locale.localeString(ustr, ulen, llen);
        printf("cstr: %s (%zu)\n", cstr, llen);
    }

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

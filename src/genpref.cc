#include "config.h"
#include "base.h"
#include "ascii.h"
#include "appnames.h"

char const *ApplicationName;

#define CFGDESC
#include "ykey.h"
#include "sysdep.h"

void addWorkspace(const char *, const char *, bool) {}
void addKeyboard(const char *, const char *, bool) {}
void setLook(const char *, const char *, bool) {}
void addBgImage(const char *, const char *, bool) {}

//#include "bindkey.h"
//#include "default.h"
#define CFGDEF
#define GENPREF
#include "yprefs.h"
#include "bindkey.h"
#include "default.h"
#include "themable.h"
#include "icewmbg_prefs.h"

static const char workspaceNames[] =
    "\" 1 \", \" 2 \", \" 3 \", \" 4 \"";
static bool sorted, internal, themable;

static int count(cfoption* options) {
    for (int i = 0; ; ++i)
        if (options[i].type == cfoption::CF_NONE)
            return i;
}

static int compare(const void* p1, const void* p2) {
    const cfoption* o1 = (const cfoption *) p1;
    const cfoption* o2 = (const cfoption *) p2;
    return strcmp(o1->name, o2->name);
}

static void sort(cfoption* options) {
    qsort(options, count(options), sizeof(cfoption), compare);
}

static void show(cfoption *options) {
    if (sorted)
        sort(options);

    for (unsigned i = 0; options[i].type != cfoption::CF_NONE; i++) {
        size_t deslen = strlen(Elvis(options[i].description, ""));
        if (deslen < 13) {
            die(13, "Invalid description for option \"%s\":\n"
                    "Each icewm option must have a meaningful description\n"
                    " and be documented in the manpage.\n",
                options[i].name);
        }
        const char* adddot = ASCII::isAlnum(options[i].description[deslen - 1])
            ? "." : "";

        printf("#  %s%s\n", options[i].description, adddot);

        switch (options[i].type) {
        case cfoption::CF_BOOL:
            printf("# %s=%d # 0/1\n",
                   options[i].name, options[i].boolval());
            break;
        case cfoption::CF_INT:
            printf("# %s=%d # [%d-%d]\n",
                   options[i].name, *options[i].v.i.int_value,
                   options[i].v.i.min, options[i].v.i.max);
            break;
        case cfoption::CF_UINT:
            printf("# %s=%u # [%u-%u]\n",
                   options[i].name, *options[i].v.u.uint_value,
                   options[i].v.u.min, options[i].v.u.max);
            break;
        case cfoption::CF_STR:
            printf("# %s=\"%s\"\n", options[i].name, Elvis(options[i].str(), ""));
            break;
        case cfoption::CF_KEY:
            printf("# %s=\"%s\"\n", options[i].name, options[i].key()->name);
            break;
        case cfoption::CF_FUNC:
            if (0 == strcmp("WorkspaceNames", options[i].name)) {
                printf("WorkspaceNames=%s\n", workspaceNames);
            }
            else if (0 == strcmp("Look", options[i].name)) {
                char look[16] = QUOTE(CONFIG_DEFAULT_LOOK);
                look[4] = ASCII::toLower(look[4]);
                printf("# %s=\"%s\"\n", options[i].name, look + 4);
            }
            else {
                printf("# %s=\"\"\n", options[i].name);
            }
            break;
        case cfoption::CF_NONE:
            break;
        }

        puts("");
    }
}

static void ppod(cfoption *options) {
    if (sorted)
        sort(options);

    for (unsigned int i = 0; options[i].type != cfoption::CF_NONE; i++) {
        switch (options[i].type) {
        case cfoption::CF_BOOL:
            printf("=item B<%s>=%d\n",
                   options[i].name, options[i].boolval());
            break;
        case cfoption::CF_INT:
            printf("=item B<%s>=%d\n",
                   options[i].name, *options[i].v.i.int_value);
            break;
        case cfoption::CF_UINT:
            printf("=item B<%s>=%u\n",
                   options[i].name, *options[i].v.u.uint_value);
            break;
        case cfoption::CF_STR:
            printf("=item B<%s>=\"%s\"\n", options[i].name, Elvis(options[i].str(), ""));
            break;
        case cfoption::CF_KEY:
            printf("=item B<%s>=\"%s\"\n", options[i].name, options[i].key()->name);
            break;
        case cfoption::CF_FUNC:
            if (0 == strcmp("WorkspaceNames", options[i].name)) {
                printf("=item B<WorkspaceNames>=%s\n", workspaceNames);
            }
            else if (0 == strcmp("Look", options[i].name)) {
                char look[16] = QUOTE(CONFIG_DEFAULT_LOOK);
                look[4] = ASCII::toLower(look[4]);
                printf("=item B<%s>=\"%s\"\n", options[i].name, look + 4);
            }
            else {
                printf("=item B<%s>=\"\"\n", options[i].name);
            }
            break;
        case cfoption::CF_NONE:
            break;
        }
        puts("");

        size_t deslen = strlen(Elvis(options[i].description, ""));
        if (deslen < 13) {
            die(13, "Invalid description for option \"%s\":\n"
                    "Each icewm option must have a meaningful description\n"
                    " and be documented in the manpage.\n",
                options[i].name);
        }
        const char* adddot = ASCII::isAlnum(options[i].description[deslen - 1])
            ? "." : "";
        printf("%s%s\n\n", options[i].description, adddot);
    }
}

static void prepare()
{
    const unsigned wmapp_count = ACOUNT(wmapp_preferences);
    wmapp_preferences[wmapp_count - 2] = wmapp_preferences[wmapp_count - 1];
}

static void genpref_header()
{
    printf("# %s preferences(%s) - generated by genpref\n\n", PACKAGE, VERSION);
    printf("# This file should be copied to %s or $HOME/.icewm/\n", CFGDIR);
    printf("# NOTE: All settings are commented out by default.\n"
           "# Be sure to uncomment them if you change them!\n"
           "\n");
}

static void genpref_internal()
{
    show(wmapp_preferences);

    show(icewm_preferences);
}

static void genpref_themable()
{
    printf("# -----------------------------------------------------------\n"
           "# Themable preferences. Themes will override these.\n"
           "# To override the themes, place them in ~/.icewm/prefoverride\n"
           "# -----------------------------------------------------------\n\n");

    show(icewm_themable_preferences);

    printf("#\n"
           "# icewmbg preferences\n"
           "#\n"
           "\n"
           );
    show(icewmbg_prefs);
}

static void genpref()
{
    if (internal && themable)
        genpref_header();
    if (internal)
        genpref_internal();
    if (themable)
        genpref_themable();
}

static void podpref()
{
    if (internal) {
        ppod(wmapp_preferences);
        ppod(icewm_preferences);
    }
    if (themable) {
        ppod(icewm_themable_preferences);
        ppod(icewmbg_prefs);
    }
}

static const char* help_text()
{
    return
        "  -n                  Print only non-themable preferences.\n"
        "  -o, --output=FILE   Write preferences to FILE.\n"
        "  -p                  Use perlpod output format.\n"
        "  -s                  Sort preferences by name.\n"
        "  -t                  Print only themable preferences.\n"
        ;
}

int main(int argc, char **argv)
{
    bool pod = false;

    check_argv(argc, argv, help_text, VERSION);

    char* output = nullptr;
    for (char **arg = argv + 1; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            char *value(nullptr);
            if (GetArgument(value, "o", "output", arg, argv + argc)) {
                output = value;
            }
            else if (is_short_switch(*arg, "n")) {
                internal = true;
            }
            else if (is_short_switch(*arg, "p")) {
                pod = true;
            }
            else if (is_short_switch(*arg, "s")) {
                sorted = true;
            }
            else if (is_short_switch(*arg, "t")) {
                themable = true;
            }
            else {
                warn("Unrecognized option '%s'.", *arg);
            }
        }
    }
    if (output) {
        if (nullptr == freopen(output, "w", stdout)) {
            fail("%s", output);
            exit(1);
        }
    }
    if ((internal | themable) == 0) {
        internal = themable = true;
    }

    prepare();
    if (pod)
        podpref();
    else
        genpref();
    fflush(stdout);

    return 0;
}

// vim: set sw=4 ts=4 et:

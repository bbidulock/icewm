#undef _
#undef N_
#undef bindtextdomain
#undef textdomain

#if defined(ENABLE_NLS)
#       include <libintl.h>
#
#       define gettext_noop(String) (String)
#
#       define _(String) gettext(String)
#       define N_(String) gettext_noop(String)
#else
#       define bindtextdomain(catalog, locale_dir)
#       define textdomain(catalog)
#
#       define _(String) (String)
#       define N_(String) (String)
#endif

// vim: set sw=4 ts=4 et:

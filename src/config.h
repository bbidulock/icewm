/* src/config.h.  Generated automatically by configure.  */
/* src/config.h.in.  Generated automatically from configure.in by autoheader.  */
/* define if have old kstat (on solaris only?) */
/* #undef HAVE_OLD_KSTAT */

/* the locale dependent property describing the codeset (langinfo.h) */
/* #undef HAVE_CODESET */
/* #undef HAVE__NL_CTYPE_CODESET_NAME */
/* #undef HAVE__NL_MESSAGES_CODESET */

/* define how to query the current locale's codeset */
#define CONFIG_NL_CODESETS _NL_MESSAGES_CODESET, _NL_CTYPE_CODESET_NAME, CODESET, 0

/* define when iconv is imported from GNU libiconv */
/* #undef CONFIG_LIBICONV */

/* preferred Unicode set */
/* #undef CONFIG_UNICODE_SET */

/* Address bar */
#define CONFIG_ADDRESSBAR 1

/* Define to enable antialiasing. */
/* #undef CONFIG_ANTIALIASING */

/* APM status applet */
#define CONFIG_APPLET_APM 1

/* LCD clock applet */
#define CONFIG_APPLET_CLOCK 1

/* CPU status applet */
#define CONFIG_APPLET_CPU_STATUS 1

/* Mailbox applet */
#define CONFIG_APPLET_MAILBOX 1

/* Network status applet */
#define CONFIG_APPLET_NET_STATUS 1

/* Define to make IceWM more GNOME-friendly */
/* #undef CONFIG_GNOME_MENUS */

/* Name of a root window property indicating that GNOME is active. */
#define CONFIG_GNOME_ROOT_PROPERTY "GNOME_SM_PROXY"

/* Define to enable gradient support. */
/* #undef CONFIG_GRADIENTS */

/* Define to enable GUI events support. */
/* #undef CONFIG_GUIEVENTS */

/* Define to enable internationalization */
#define CONFIG_I18N 1

/* Define to use Imlib for image rendering */
/* #undef CONFIG_IMLIB */

/* Define when using libiconv */
/* #undef CONFIG_LIBICONV */

/* Define to enable Move/Resize FX. */
/* #undef CONFIG_MOVESIZE_FX */

/* Define to support the X session managment protocol */
#define CONFIG_SESSION 1

/* Define to enable X shape extension */
#define CONFIG_SHAPE 1

/* Define to allow transparent frame borders. */
/* #undef CONFIG_SHAPED_DECORATION */

/* Taskbar */
#define CONFIG_TASKBAR 1

/* Tooltips */
#define CONFIG_TOOLTIP 1

/* Window tray */
#define CONFIG_TRAY 1

/* OS/2 like window list */
#define CONFIG_WINLIST 1

/* Window menu */
#define CONFIG_WINMENU 1

/* Define to enable /proc/wm-session resource management. */
/* #undef CONFIG_WM_SESSION */

/* Define to enable x86 assembly code. */
#define CONFIG_X86_ASM 1

/* Define to enable XFreeType support. */
/* #undef CONFIG_XFREETYPE */

/* Define to use libXpm for image rendering */
#define CONFIG_XPM 1

/* Define if you want to debug IceWM */
/* #undef DEBUG */

/* Define to enable ESD support. */
/* #undef ENABLE_ESD */

/* Define to enable internationalized message */
#define ENABLE_NLS 1

/* Define to enable OSS support. */
/* #undef ENABLE_OSS */

/* Define to enable YIFF support. */
/* #undef ENABLE_YIFF */

/* Define if you have the `basename' function. */
#define HAVE_BASENAME 1

/* Define if you have the <dirent.h> header file, and it defines `DIR'. */
#define HAVE_DIRENT_H 1

/* Define if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define if you have the <esd.h> header file. */
/* #undef HAVE_ESD_H */

/* Define if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the `iconv' function. */
#define HAVE_ICONV 1

/* Define if you have the `iconv_close' function. */
#define HAVE_ICONV_CLOSE 1

/* Define if you have the <iconv.h> header file. */
#define HAVE_ICONV_H 1

/* Define if you have the `iconv_open' function. */
#define HAVE_ICONV_OPEN 1

/* Define if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if you have the <kstat.h> header file. */
/* #undef HAVE_KSTAT_H */

/* Define if you have the <langinfo.h> header file. */
#define HAVE_LANGINFO_H 1

/* Define if you have the `esd' library (-lesd). */
/* #undef HAVE_LIBESD */

/* Define if you have the `intl' library (-lintl). */
/* #undef HAVE_LIBINTL */

/* Define if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define if you have the <linux/tasks.h> header file. */
/* #undef HAVE_LINUX_TASKS_H */

/* Define if you have the <linux/threads.h> header file. */
#define HAVE_LINUX_THREADS_H 1

/* Define if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define if you have the `putenv' function. */
/* #undef HAVE_PUTENV */

/* Define if you have the `select' function. */
/* #undef HAVE_SELECT */

/* Define if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Define if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define if you have the `strtol' function. */
/* #undef HAVE_STRTOL */

/* Define if you have the `strtoul' function. */
/* #undef HAVE_STRTOUL */

/* Define if you have the <sys/dir.h> header file, and it defines `DIR'. */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define if you have the <X11/Xft/Xft.h> header file. */
/* #undef HAVE_X11_XFT_XFT_H */

/* Define to enable XInternAtoms */
#define HAVE_XINTERNATOMS 1

/* Define if you have the <Y2/Y.h> header file. */
/* #undef HAVE_Y2_Y_H */

/* Lite version */
/* #undef LITE */

/* Define if you need an implementation of the allocation operators. (gcc 3.0)
   */
#define NEED_ALLOC_OPERATORS 1

/* Define to disable preferences support. */
/* #undef NO_CONFIGURE */

/* Define to disable configurable menu support. */
/* #undef NO_CONFIGURE_MENUS */

/* Define to disable keybinding support. */
/* #undef NO_KEYBIND */

/* Define to disable configurable window options support. */
/* #undef NO_WINDOW_OPTIONS */

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to the type of arg 1 for `select'. */
#define SELECT_TYPE_ARG1 int

/* Define to the type of args 2, 3 and 4 for `select'. */
#define SELECT_TYPE_ARG234 (fd_set *)

/* Define to the type of arg 5 for `select'. */
#define SELECT_TYPE_ARG5 (struct timeval *)

/* The size of a `char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of a `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of a `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* Define if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Define if the X Window System is missing or not being used. */
/* #undef X_DISPLAY_MISSING */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */

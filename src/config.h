/* src/config.h.  Generated automatically by configure.  */
/* src/config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have the strftime function.  */
#define HAVE_STRFTIME 1

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to the type of arg1 for select(). */
#define SELECT_TYPE_ARG1 int

/* Define to the type of args 2, 3 and 4 for select(). */
#define SELECT_TYPE_ARG234 (fd_set *)

/* Define to the type of arg5 for select(). */
#define SELECT_TYPE_ARG5 (struct timeval *)

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define if the X Window System is missing or not being used.  */
/* #undef X_DISPLAY_MISSING */

/* #undef HAVE_OLD_KSTAT */

/* Define if you have the gettimeofday function.  */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the strtol function.  */
#define HAVE_STRTOL 1

/* Define if you have the strtoul function.  */
#define HAVE_STRTOUL 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <kstat.h> header file.  */
/* #undef HAVE_KSTAT_H */

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the intl library (-lintl).  */
/* #undef HAVE_LIBINTL */

/* Define if you want to debug IceWM */
/* #undef DEBUG */

/* Define to enable internationalization */
/* #undef I18N */

/* Location of internationalized message */
/* #undef LOCALEDIR */

/* Define to enable internationalized message */
/* #undef ENABLE_NLS */

/* Define to enable GUI events support.  */
/* #undef CONFIG_GUIEVENTS */

/* Define to enable XInternAtoms */
#define HAVE_XINTERNATOMS 1

/* Define to enable X shared memory extension */
#define SM 1

/* Define to enable X shape extension */
#define SHAPE 1

/* Define to disable preferences support.  */
/* #undef NO_CONFIGURE */

/* Define to disable keybinding support.  */
/* #undef NO_KEYBIND */

/* Define to disable configurable menu support.  */
/* #undef NO_CONFIGURE_MENUS */

/* Define to disable configurable window options support.  */
/* #undef NO_WINDOW_OPTIONS */

/* Tooltips */
#define CONFIG_TOOLTIP 1

/* Taskbar */
#define CONFIG_TASKBAR 1

/* Mailbox applet */
#define CONFIG_APPLET_MAILBOX 1

/* CPU status applet */
#define CONFIG_APPLET_CPU_STATUS 1

/* Network status applet */
#define CONFIG_APPLET_NET_STATUS 1

/* PPP status applet */
#define CONFIG_APPLET_PPP_STATUS 1

/* LCD clock applet */
#define CONFIG_APPLET_CLOCK 1

/* Address bar */
#define CONFIG_ADDRESSBAR 1

/* OS/2 like window list */
#define CONFIG_WINLIST 1

/* Window menu */
#define CONFIG_WINMENU 1

/* Define to make IceWM more GNOME-friendly */
/* #undef GNOME */

/* Define to use libXpm for image rendering */
#define XPM 1

/* Define to use Imlib for image rendering */
/* #undef IMLIB */


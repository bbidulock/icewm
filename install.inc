APPLICATIONS = genpref icewm icesh icewmhint icewmbg icehelp

prefix      = /usr
exec_prefix = /usr/X11R6
libdir      = ${exec_prefix}/lib
sysconfdir  = /etc
datadir	    = ${prefix}/share

PREFIX      = /usr
BINDIR      = ${exec_prefix}/bin
LIBDIR      = ${libdir}/X11/icewm
CFGDIR      = ${sysconfdir}/X11/icewm
LOCDIR      = ${datadir}/locale
KDEDIR 	    = /usr/share
DOCDIR	    = ${prefix}/doc

INSTALL     = /usr/bin/install -c
INSTALLDIR  = /usr/bin/install -c -m 755 -d
INSTALLBIN  = ${INSTALL}
INSTALLLIB  = ${INSTALL} -m 644
#INSTALLETC  = ${INSTALL} -m 644

MKFONTDIR   = /usr/X11R6/bin/mkfontdir

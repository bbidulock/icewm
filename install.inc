prefix = /usr
exec_prefix = ${prefix}

PREFIX      = /usr
BINDIR      = ${exec_prefix}/bin
LIBDIR      = ${exec_prefix}/lib/X11/icewm
ETCDIR      = /etc

INSTALL     = /usr/bin/install -c
INSTALLDIR  = /usr/bin/install -c -m 755 -d
INSTALLBIN  = ${INSTALL}
INSTALLLIB  = ${INSTALL} -m 644
#INSTALLETC  = ${INSTALL} -m 644

prefix = /usr/local
exec_prefix = ${prefix}

PREFIX      = /usr/local
BINDIR      = ${exec_prefix}/bin
DATADIR     = ${prefix}/share/$(PNAME)
ETCDIR      = ${prefix}/etc/$(PNAME)

INSTALL     = /usr/bin/install -c
INSTALLDIR  = /usr/bin/install -c -m 755 -d
INSTALLBIN  = ${INSTALL}
INSTALLDATA  = ${INSTALL} -m 644
#INSTALLETC  = ${INSTALL} -m 644

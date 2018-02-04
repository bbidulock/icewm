#!/bin/bash

# need --enable-maintainer-mode to be able to run in place
#      must be disabled to build an installable package

# *FLAGS are what Arch Linux makepkg uses with the exception
#      that -Wall -Werror is added

case "`uname -m`" in
	i686)
		CPPFLAGS="-D_FORTIFY_SOURCE=2"
		CFLAGS="-march=i686 -mtune=generic -O2 -pipe -fstack-protector-strong -fno-plt"
		CXXFLAGS="-march=i686 -mtune=generic -O2 -pipe -fstack-protector-strong -fno-plt"
		LDFLAGS="-Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now"
		DEBUG_CFLAGS="-g -ggdb -fvar-tracking-assignments"
		DEBUG_CXXFLAGS="-g -ggdb -fvar-tracking-assignments"
	;;
	x86_64)
		CPPFLAGS="-D_FORTIFY_SOURCE=2"
		CFLAGS="-march=x86-64 -mtune=generic -O2 -pipe -fstack-protector-strong -fno-plt"
		CXXFLAGS="-march=x86-64 -mtune=generic -O2 -pipe -fstack-protector-strong -fno-plt"
		LDFLAGS="-Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now"
		DEBUG_CFLAGS="-g -ggdb -fvar-tracking-assignments"
		DEBUG_CXXFLAGS="-g -ggdb -fvar-tracking-assignments"
	;;
esac

./configure \
	--enable-maintainer-mode \
	--enable-dependency-tracking \
	--prefix=/usr \
	--sysconfdir=/etc \
	--mandir=/usr/share/man \
	CPPFLAGS="$CPPFLAGS" \
	CFLAGS="$DEBUG_CFLAGS -Wall -Werror $CFLAGS" \
	CXXFLAGS="$DEBUG_CXXFLAGS -Wall -Werror $CXXFLAGS" \
	LDFLAGS="$LDFLAGS" \
	DEBUG_CFLAGS="$DEBUG_CFLAGS" \
	DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS"

# cscope target won't work without this
#
[ -f po/Makefile ] && echo -e '\n%:\n\t@:\n\n' >> po/Makefile

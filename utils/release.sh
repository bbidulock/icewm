#!/bin/sh

#cvs () { echo cvs $* }

CVSROOT=':pserver:anonymous@cvs.icewm.sourceforge.net:/cvsroot/icewm'
MODULE='icewm-1.0'
SRCDIR="$MODULE/$MODULE"

if [ -d "$MODULE" ]; then
  pushd $MODULE > /dev/null
  [ -f Makefile ] && make maintainer-clean				
  echo Updating CVS repository
  cvs -z3 update -d -P
  popd > /dev/null
else
  echo Checking out CVS repository
  cvs -z3 -d$CVSROOT login
  cvs -z3 -d$CVSROOT checkout -P $MODULE
fi

source $SRCDIR/VERSION
DISTDIR="icewm-$VERSION"

echo Copying CVS repository to $DISTDIR
rm -rf $DISTDIR
cp -r $SRCDIR $DISTDIR
pushd $DISTDIR > /dev/null

echo Preparing autoconf
./autogen.sh

echo Running configure
./configure --quiet --prefix=/usr --exec-prefix=/usr/X11R6 --sysconfdir=/etc \
	    --enable-i18n --enable-nls

echo Making distribution information
make -s icewm.lsm icewm.spec docs

echo Cleaning distribution
rm config.{cache,log,status}

RELEASE=`sed -ne 's/^%define\>[[:space:]]*release\>[[:space:]]*\<\(.*\)$/\1/p'\
	< icewm.spec`
TARBALL="icewm-$VERSION.tar"

popd
echo Building tarball $TARBALL
tar -cf $TARBALL --exclude=CVS $DISTDIR
gzip -9 < $TARBALL > "$TARBALL.gz"
bzip2 -9 < $TARBALL > "$TARBALL.bz2"
cp -v "$TARBALL.bz2" "$HOME/rpm/SOURCES/"
ln -sf "$TARBALL.gz" "icewm-$VERSION.tar.gz"
ln -sf "$TARBALL.bz2" "icewm-$VERSION.tar.bz2"

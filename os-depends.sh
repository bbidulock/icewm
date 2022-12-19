#!/usr/bin/env bash
#
# Examine the installed system packages,
# and install missing IceWM dependencies.
# Supported operating systems include:
# Arch, Debian, OpenBSD, OpenSUSE,
# Ubuntu, CentOS, Oracle Linux, Red Hat,
# and their derivatives.
# and finally Void Linux support.
# Run this as root.
# 

fail () {
    echo "$0: $@" >&2
    exit 1
}

usage () {
    echo "Usage: bash -x ./os-depends.sh [ +s|-s ] [ +y|-y ]"
    echo "Options: +s|-s: dis/enable sound; +y|-y: dis/enable interactive"
    exit 0
}

isroot () {
    [ $(id -u) -eq 0 ]
}

sound=1
noask=0

for opt
do
    case $opt in
        (+s) sound=1 ;;
        (-s) sound=0 ;;
        (+y) noask=1 ;;
        (-y) noask=0 ;;
        (*) usage ;;
    esac
done

osarch () {
    type pacman &>/dev/null || fail "pacman is not an executable command"
    t=`mktemp`
    pacman -Q | awk '{print $1}' >$t 2>/dev/null || fail "pacman -Q failed"
    r=" asciidoc asciidoctor autoconf automake binutils cmake fontconfig fribidi gcc gdk-pixbuf2 gdk-pixbuf-xlib imlib2 librsvg gettext git libice libsm libtool libx11 libxext libxft libxinerama libxrandr libxrender libxcomposite libxdamage libxfixes libpng libjpeg libxpm make discount perl ttf-dejavu xdg-utils xterm xorg-xmessage yad "
    [ $sound = 1 ] && snd=" libao libsndfile" || snd=
    [ $noask = 1 ] && ask=y || ask=
    i=
    for p in $r $snd
    do
        grep -q -e ^$p $t || i="$i $p"
    done
    if [ "$i" = "" ]; then
        echo "All required packages are already installed"
    else
        echo "The following packages need to be installed:"
        echo "    $i"
        if isroot; then
            pacman -S$ask $i || fail "pacman failed to install $i"
        else
            echo "Please install these as root"
        fi
    fi
    rm -f -- $t
}

debian () {
    type apt &>/dev/null || fail "apt is not an executable command"
    t=`mktemp`
    apt list --installed >$t || fail "apt list --installed failed"
    sed -i -e '/-base\//d' $t
    r=" asciidoctor autoconf automake autopoint build-essential cmake fonts-dejavu gettext git libfontconfig1-dev libfribidi-dev libgdk-pixbuf2.0-dev libimlib2-dev libtool libsm-dev libx11-dev libxext-dev libxft-dev libxinerama-dev libxrandr-dev libxrender-dev libxcomposite-dev libxdamage-dev libxfixes-dev libpng-dev libjpeg-dev libxpm-dev librsvg2-dev markdown x11-utils xterm xdg-utils yad "
    [ $sound = 1 ] && snd=" libao-dev libasound2-dev libsndfile1-dev libpulse-dev" || snd=
    [ $noask = 1 ] && ask=--yes || ask=
    i=
    for p in $r $snd
    do
        grep -q -e ^$p $t || i="$i $p"
    done
    if [ "$i" = "" ]; then
        echo "All required packages were already installed."
    else
        echo "The following packages need to be installed:"
        echo "    $i"
        if isroot; then
            apt-get install $ask $i || fail "apt-get failed to install $i"
        else
            echo "Please install these as root"
        fi
    fi
    rm -f -- $t
}

ubuntu () {
    type apt &>/dev/null || fail "apt is not an executable command"
    t=`mktemp`
    apt list --installed >$t 2>/dev/null || fail "apt list --installed failed"
    sed -i -e '/-base\//d' $t
    r=" asciidoctor autoconf automake autopoint build-essential cmake fonts-dejavu gettext git libfontconfig1-dev libfribidi-dev libgdk-pixbuf2.0-dev libimlib2-dev libtool libsm-dev libx11-dev libxext-dev libxft-dev libxinerama-dev libxrandr-dev libxrender-dev libxcomposite-dev libxdamage-dev libxfixes-dev libpng-dev libjpeg-dev libxpm-dev librsvg2-dev markdown x11-utils xterm xdg-utils yad "
    [ $sound = 1 ] && snd=" libao-dev libasound2-dev libsndfile1-dev libpulse-dev" || snd=
    [ $noask = 1 ] && ask=--yes || ask=
    i=
    for p in $r $snd
    do
        grep -q -e ^$p $t || i="$i $p"
    done
    if [ "$i" = "" ]; then
        echo "All required packages are already installed"
    else
        echo "The following packages need to be installed:"
        echo "    $i"
        if isroot; then
            apt-get install $ask $i || fail "apt-get failed to install $i"
        else
            echo "Please install these as root"
        fi
    fi
    rm -f -- $t
}

openbsd () {
    type pkg_info &>/dev/null || fail "pkg_info is not an executable command"
    t=`mktemp`
    pkg_info >$t 2>/dev/null || fail "pkg_info failed"
    r=" asciidoc autoconf automake cmake dejavu-ttf freetype fribidi g++ gettext-tools git gmake imlib2 libao libsndfile libtool lxrandr jpeg png py-markdown"
    [ $sound = 1 ] && snd=" libao libsndfile" || snd=
    [ $noask = 1 ] && ask=-I || ask=
    i=
    for p in $r $snd
    do
        grep -q -e ^$p $t || i="$i $p"
    done
    if [ "$i" = "" ]; then
        echo "All required packages are already installed"
    else
        echo "The following packages need to be installed:"
        echo "    $i"
        if isroot; then
            pkg_add -U $ask $i || fail "pkg_add failed to install $i"
        else
            echo "Please install these as root"
        fi
    fi
    rm -f -- $t
}

opensuse () {
    type zypper &>/dev/null || fail "zypper is not an executable command"
    type rpm &>/dev/null || fail "rpm is not an executable command"
    t=`mktemp`
    rpm -qa >$t || fail "rpm -qa failed"
    r=" alsa-devel asciidoc autoconf automake dejavu-fonts fontconfig-devel fribidi-devel gdk-pixbuf-devel gettext gettext-tools git imlib2-devel imlib2-loaders libSM-devel libX11-devel libXext-devel libXft-devel libXinerama-devel libXrandr-devel libXrender-devel libXcomposite-devel libXdamage-devel libXfixes-devel libpng16-devel libjpeg62-devel libXpm-devel librsvg-devel libtool make xmessage xterm xdg-utils zenity "
    [ $sound = 1 ] && snd=" libao-devel libsndfile-devel" || snd=
    [ $noask = 1 ] && ask=-y || ask=
    i=
    for p in $r $snd
    do
        grep -q -e ^$p $t || i="$i $p"
    done
    if [ "$i" = "" ]; then
        echo "All required packages are already installed"
    else
        echo "The following packages need to be installed:"
        echo "    $i"
        if isroot; then
            zypper install $ask --no-recommends $i ||
                fail "zypper failed to install $i"
        else
            echo "Please install these as root"
        fi
    fi
    i=
    if ! type make &>/dev/null || ! type cpp &>/dev/null; then
        i="patterns-openSUSE-devel_C_C++ gcc-c++"
    elif ! type gcc &>/dev/null; then
        i="gcc gcc-c++"
    elif ! type g++ &>/dev/null; then
        i="gcc-c++"
    fi
    if [ "$i" != "" ]; then
        echo "The following packages need to be installed:"
        echo "    $i"
        if isroot; then
            zypper install -y --no-recommends $i ||
                fail "zypper failed to install $i"
        else
            echo "Please install these as root"
        fi
    fi
    rm -f -- $t
}

centos () {
    type yum &>/dev/null || fail "yum is not an executable command"
    t=`mktemp`
    u=`mktemp`
    yum list installed | awk '{print $1}' >$t 2>/dev/null || fail "yum list failed"
    yum list | awk '{print $1}' >$u 2>/dev/null || fail "yum list failed"
    r=" asciidoc alsa-lib-devel autoconf automake cmake dejavu-fonts fontconfig-devel fribidi-devel gcc-c++ gdk-pixbuf2-devel gdk-pixbuf2-xlib-devel librsvg2-devel gettext gettext-devel git glib2-devel imlib2-devel libSM-devel libX11-devel libXext-devel libXft-devel libXinerama-devel libXpm-devel libXrandr-devel libXrender-devel libXcomposite-devel libXdamage-devel libXfixes-devel libjpeg-turbo-devel libpng-devel libtool make markdown perl-Pod-Html xterm xdg-utils xorg-x11-apps zenity "
    [ $sound = 1 ] && snd=" libao-devel libsndfile-devel" || snd=
    [ $noask = 1 ] && ask=-y || ask=
    i=
    for p in $r $snd
    do
        grep -q -e ^$p\. $t || i="$i $p"
    done
    if [ "$i" = "" ]; then
        echo "All required packages are already installed"
    else
        echo "The following packages need to be installed:"
        echo "    $i"
        if isroot; then
            yum install $ask $i || fail "yum failed to install $i"
        else
            echo "Please install these as root"
        fi
    fi
    rm -f -- $t
}

osvoid () {
    type xbps-install &>/dev/null || fail "xbps-install is not an executable command"
    t=`mktemp`
    xbps-query -l | awk '{ print $2 }' | xargs -n1 xbps-uhelper getpkgname >$t 2>/dev/null || fail "xbps-query -l list failed"
    r="asciidoc alsa-lib-devel autoconf automake cmake dejavu-fonts-ttf fontconfig-devel fribidi-devel gcc gdk-pixbuf-devel gdk-pixbuf-xlib-devel librsvg-devel gettext gettext-devel git glib-devel imlib2-devel libSM-devel libX11-devel libXext-devel libXft-devel libXinerama-devel libXpm-devel libXrandr-devel libXrender-devel libXcomposite-devel libXdamage-devel libXfixes-devel libjpeg-turbo-devel libpng-devel libtool make  python3-Markdown xterm xdg-utils xorg-apps zenity "
    [ $sound = 1 ] && snd=" libao-devel libsndfile-devel" || snd=
    [ $noask = 1 ] && ask=-y || ask=
	i=
    for p in $snd $r
    do
        grep -q -e ^$p $t || i="$i $p"
    done
    if [ "$i" = "" ]; then
        echo "All required packages were already installed."
    else
        echo "The following packages need to be installed:"
        echo "    $i"
        if isroot; then
            xbps-install $ask $i || fail "xbps-install failed to install $i"
        else
            echo "Please install these as root"
        fi
    fi
    rm -f -- $t
}

check () {
    grep -q -e "$1" /etc/os-release 2>/dev/null
}

linux () {
    notfound=
    eval $(grep -e ^ID -e NAME= /etc/os-release)
    for name in $ID $ID_LIKE ${NAME%% *} ${PRETTY_NAME%% *}
    do
        case $name in
            (Arch|arch)
                osarch
                return 0
                ;;
            (Manjaro|manjaro)
                osarch
                return 0
                ;;
            (Debian|debian)
                debian
                return 0
                ;;
            (Ubuntu|ubuntu)
                ubuntu
                return 0
                ;;
            (linuxmint)
                ubuntu
                return 0
                ;;
            (openSUSE|SUSE|opensuse|suse)
                opensuse
                return 0
                ;;
            (CentOS|centos)
                centos
                return 0
                ;;
            (Oracle|ol)
                centos
                return 0
                ;;
            (rhel|fedora|Fedora)
                centos
                return 0
                ;;
			(Void|void)
                osvoid
                return 0
                ;;
            (*)
                notfound="$notfound $name"
                ;;
        esac
    done
    fail "Unsupported Linux distro: $notfound!"
}

detect () {
    case $(uname) in
        (Linux) linux ;;
        (OpenBSD) openbsd ;;
        (*) fail "Failed to recognize OS for uname $(uname)!" ;;
    esac
}

detect

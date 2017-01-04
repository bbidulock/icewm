[icewm -- read me first file.  2016-12-28]: #

Ice Window Manager (IceWM)
==========================

This is a fork of the IceWM CVS on [sourceforge][13].  It includes all changes
from the `icewm-1-3-BRANCH` branch, greatly enhanced EWMH/ICCCM compliance, as
well as patches collected from Arch Linux, Debian, pld-linux, the IceWM bug
list, and various other GitHub forks.

> The name was decided on a very hot day... (and Marko started writing it in
> winter ;-)  The aim of IceWM is to have good 'Feel' and decent 'Look'. 'Feel'
> is much more important than 'Look' ...


Release
-------

This is the icewm-1.3.12.140 package, released under LGPL license 2016-12-28.
This release as well as future versions here, can all be obtained from
[GitHub][1] using a command such as:

    git clone https://github.com/bbidulock/icewm.git

When working from `git(1)`, please use this file.  An abbreviated
installation procedure that works for most applications appears below.


Dependencies
------------

Building from tarball requires:

 - gcc or clang (_for compiling_)
 - libxft (_for font rendering_)
 - libxinerama (_for XINERAMA support_)
 - gdk-pixbuf2 (_for pixmap rendering_)
 - libxrandr (_for XRANDR support_)
 - libsndfile (_for sound support_)
 - libsm (_for X11R6 session management_)
 - fribidi (_for i18n text rendering_)

Building from git also requires:

 - git (_for cloning the git repository and updating changelogs_)
 - autoconf/automake or cmake toolchain (_for build scripts_)
 - xorg-mkfontdir (_for installing themes_)
 - linuxdoc-tools (_for creating help html files_)
 - markdown (_for building release package_)

Quick Start
-----------

The quickest and easiest way to get IceWM up and running is to run the
following commands:

    $> git clone http://github.com/bbidulock/icewm.git
    $> cd icewm
    $> ./autogen.sh
    $> ./configure --prefix=/usr --sysconfdir=/etc \
       --enable-shaped-decorations --enable-gradients \
       --enable-guievents --with-icesound=ALSA,OSS \
       --disable-menus-gnome2
    $> make V=0
    $> sudo make DESTDIR="$pkgdir" install

This will configure, compile and install IceWM the quickest.  For those who
would like to customize the installation, use the command:

    ./configure --help

Please see the [INSTALL][3] file for more detailed installation instructions.
The [ChangeLog][4] file contains a detailed history of implementation changes.
The [COMPLIANCE][5] file lists the current state of EWMH/ICCCM compliance.  The
[NEWS][6] file has release notes and history of user visible changes of the
current version.  The [TODO][7] file lists features not yet implemented and
other outstanding items.

This release is published under LGPL license that can be found in the file
[COPYING][8].


Configuring IceWM
-----------------

Documentation for configuring the window manager can be obtained from [IceWM
Website][14] or from the online manual: point your browser at [this file][16].

Unfortunately the documentation is for version 1.2.27 and is incomplete
at that; however, it is for the most part usable.  Also, a rather sparse
`icewm(1)` manual page is available.


Included Utilities
------------------

Currently, the only included utilities are:

 - `icewmbg` (_a background setting program_),
 - `icewmtray` (_a system tray for the IceWM taskbar_), and,
 - `icewm-session` (_a program to launch the window manager, icewmbg and
   icewmtray in an orderly fashion_).


Third-party Utilities
---------------------

Unspecified keyboard shortcuts can be handled with the `bbkeys(1)` utility
available from [sourceforge][17].  For additional utilities see the [IceWM
FAQ][15].


wm-session
----------

`/proc/wm-session` is used to register the process id of an application able to
free resources smoothly when the kernel decides that memory resource have
reached a critical limit. The registered application is notified of this
situation by the signal `SIGUSR1`.

On full featured desktop machines it would make sense to use the session
manager for this purpose. On X window PDAs which have limited memory resources
it makes sense to let the window manager send `WM_DELETE_WINDOW` message to the
last recently used application.


__Requirements to uses this feature in IceWM:__

  - A patched kernel, a patch for Linux 2.4.3 is available in the contrib
    file module.

  - A patched X server assigning the clients process id to each newly
    mapped window. Alternatively you can preload the preice library
    available in the contrib file module.

  - `$ export LD_PRELOAD=$PATH_TO/libpreice.so`

  - IceWM configured to have wm-session support
    (./configure --enable-wm-session ...)

The contrib file module of IceWM is located on [sourceforge][12].


> `/proc/wm-session` was developed by:
>
> Chester Kuo <chester@linux.org.tw> and
> Mathias Hasselman <mathias.hasselman@gmx.de>.


Bug Reports
-----------

Issues can be reported on [GitHub][2].  Please try to submit short patches or
pull requests if you can.  If you would like to perform regular maintenance
activities (e.g. if you are a maintainer of an IceWM package for a
distribution), contact me for push access.

Bug reports, feedback, and suggestions pertaining to the original CVS version
can be sent to: Marko.Macek@gmx.net or icewm-user@lists.sourceforge.net

See also BUGS, TODO and the sites at:

  - http://www.icewm.org/
  - http://www.sourceforge.net/projects/icewm/
  - http://icewm.sourceforge.net/


Development
-----------

If you would like to develop against this fork, the easiest way is to obtain a
[GitHub account][10], fork the [repository][1] and perform your development.
Send me a pull request when you have something stable.


Translations
------------

You can provide translations by patching `.po` files and issuing pull requests,
or you can use the [openSUSE weblate tool][11].



[1]: https://github.com/bbidulock/icewm/
[2]: https://github.com/bbidulock/icewm/issues/
[3]: INSTALL
[4]: ChangeLog
[5]: COMPLIANCE
[6]: NEWS
[7]: TODO
[8]: COPYING
[10]: https://github.com/
[11]: http://i10n.opensuse.org/
[12]: http://sf.net/project/showfiles.php?group_id=31&release_id=31119
[13]: http://icewm.sourceforge.net/
[14]: http://www.icewm.org/
[15]: http://www.icewm.org/FAQ/IceWM-FAQ-11.html
[16]: file:///usr/share/doc/icewm-1.3.12.140/icewm.html
[17]: http://bbkeys.sourceforge.net/

[ vim: set ft=markdown sw=4 tw=80 nocin nosi fo+=tcqlorn: ]: #

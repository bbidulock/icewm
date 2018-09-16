[icewm -- read me first file.  2017-07-30]: #

Ice Window Manager (IceWM)
==========================

This is a fork of the IceWM CVS on [sourceforge][12].  It includes all changes
from the `icewm-1-3-BRANCH` branch, greatly enhanced EWMH/ICCCM compliance, as
well as patches collected from Arch Linux, Debian, pld-linux, the IceWM bug
list, and various other GitHub forks.

> The name was decided on a very hot day... (and Marko started writing it in
> winter ;-)  The aim of IceWM is to have good 'Feel' and decent 'Look'. 'Feel'
> is much more important than 'Look' ...


Release
-------

This is the icewm-1.4.2 package, released under LGPL license 2017-07-30.
This release as well as future versions here, can all be obtained from
[GitHub][1] using a command such as:

    git clone https://github.com/bbidulock/icewm.git

When working from `git(1)`, please use this file.  An abbreviated
installation procedure that works for most applications appears below.


Quick Start
-----------

The quickest and easiest way to get IceWM up and running is to run the
following commands:

    $> git clone http://github.com/bbidulock/icewm.git
    $> cd icewm
    $> ./autogen.sh
    $> ./configure --prefix=/usr --sysconfdir=/etc
    $> make
    $> sudo make DESTDIR="$pkgdir" install

This will configure, compile and install IceWM the quickest.  For those who
would like to customize the installation, use the command:

    ./configure --help

Please see the [INSTALL][3] file for more detailed installation instructions.
An alternative way to build IceWM using CMake is [documented here][19].
The [ChangeLog][4] file contains a detailed history of implementation changes.
The [COMPLIANCE][5] file lists the current state of EWMH/ICCCM compliance.  The
[NEWS][6] file has release notes and history of user visible changes of the
current version.  The [TODO][7] file lists features not yet implemented and
other outstanding items.

This release is published under LGPL license that can be found in the file
[COPYING][8].

Prerequisites
-------------

Building from tarball requires:

 - gcc or clang
 - libxft
 - libxinerama
 - libxpm
 - libjpeg
 - libxrandr

Building from git also requires:

 - complete autoconf or cmake toolchain
 - asciidoctor or asciidoc


Configuring IceWM
-----------------

Documentation for configuring the window manager can be obtained from [IceWM
Website][13] or from the [online manual][15].

Unfortunately the documentation is for version 1.2.27 and is incomplete
at that; however, it is for the most part usable.
The good news is that the `icewm(1)` manual page for version 1.4.1
is complete and fully up-to-date.


Included Utilities
------------------

Currently, the only included utilities are:

 - `icewmbg` (_a background setting program_),
 - `icewmtray` (_a system tray for the IceWM taskbar_), and,
 - `icewm-session` (_a program to launch the window manager, icewmbg and
   icewmtray in an orderly fashion_),
 - `icewm-menu-fdo` (_a utility to genenerate XDG menus_),
 - `icewm-menu-gnome2` (_a utility to generate GNOME menus_),
 - `icewmhint` (_a utility to set IceWM-specific window options hint_).
 - `icesound` (_play audio files when interesting GUI events happen_).


Third-party Utilities
---------------------

Unspecified keyboard shortcuts can be handled with the `bbkeys(1)` utility
available from [GitHub][16].

XDG compliant menus may be generated using the `xde-menu(1)` utility
available from [GitHub][20].

For additional utilities see the [IceWM FAQ][14].

Bug Reports
-----------

Issues can be reported on [GitHub][2].  Please try to submit short patches or
pull requests if you can.  If you would like to perform regular maintenance
activities (e.g. if you are a maintainer of an IceWM package for a
distribution), contact me for push access.

I normally like to have the issuers of problem reports close the report once
it has been resolved.  I do not want you to think that we are being dismissive,
because I welcome all reports.

Bug reports, feedback, and suggestions pertaining to the original CVS version
can be sent to: Marko.Macek@gmx.net or icewm-user@lists.sourceforge.net

See also [BUGS][9], [TODO][7] and the sites at:

  - https://ice-wm.org/
  - https://sourceforge.net/projects/icewm/


Development
-----------

If you would like to develop against this fork, the easiest way is to obtain a
[GitHub account][10], fork the [repository][1] and perform your development.
Send me a pull request when you have something stable.  If you submit regular
pull requests that get accepted, I will just give to push access to save time.


Translations
------------

You can provide translations by patching `.po` files and issuing pull requests,
or you can use the [openSUSE weblate tool][11].  There are two XDG files,
[icewm.desktop][17] and [icewm-session.desktop][18] than may need manual
translations.  If you have difficulties using the tools, just send me the updated
`.po` file or a patch to apply.


[1]: https://github.com/bbidulock/icewm/
[2]: https://github.com/bbidulock/icewm/issues/
[3]: https://github.com/bbidulock/icewm/blob/1.4.2/INSTALL
[4]: https://github.com/bbidulock/icewm/blob/1.4.2/ChangeLog
[5]: https://github.com/bbidulock/icewm/blob/1.4.2/COMPLIANCE
[6]: https://github.com/bbidulock/icewm/blob/1.4.2/NEWS
[7]: https://github.com/bbidulock/icewm/blob/1.4.2/TODO
[8]: https://github.com/bbidulock/icewm/blob/1.4.2/COPYING
[9]: https://github.com/bbidulock/icewm/blob/1.4.2/BUGS
[10]: https://github.com/
[11]: https://l10n.opensuse.org/
[12]: https://sourceforge.net/projects/icewm/
[13]: https://ice-wm.org/
[14]: https://ice-wm.org/FAQ/
[15]: https://github.com/bbidulock/icewm/blob/1.4.2/doc/icewm.adoc
[16]: https://github.com/bbidulock/bbkeys/
[17]: https://github.com/bbidulock/icewm/blob/1.4.2/lib/icewm.desktop
[18]: https://github.com/bbidulock/icewm/blob/1.4.2/lib/icewm-session.desktop
[19]: https://github.com/bbidulock/icewm/blob/1.4.2/INSTALL-cmakebuild.md
[20]: https://github.com/bbidulock/xde-menu/

[ vim: set ft=markdown sw=4 tw=80 nocin nosi fo+=tcqlorn: ]: #

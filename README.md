[icewm -- read me first file.  2022-11-02]: #

Ice Window Manager (IceWM)
==========================

IceWM is a window manager for the X Window System. The features of IceWM are
speed, simplicity, and not getting in the user's way.

> The name was decided on a very hot day... (and Marko started writing it in
> winter ;-)  The aim of IceWM is to have good 'Feel' and decent 'Look'. 'Feel'
> is much more important than 'Look' ...

This is a fork of the IceWM CVS on [sourceforge][12].  It includes all changes
from the `icewm-1-3-BRANCH` branch, greatly enhanced EWMH/ICCCM compliance, as
well as patches collected from Arch Linux, Debian, pld-linux, the IceWM bug
list, and various other GitHub forks.


Release
-------

This is the `icewm-3.2.0` package, released 2022-11-02.  This release, and
the latest version, can be obtained from [GitHub][1], using a command such as:

    $> git clone https://github.com/bbidulock/icewm.git

Please see the [NEWS][3] file for release notes and history of user visible
changes for the current version, and the [ChangeLog][4] file for a more
detailed history of implementation changes.  The [TODO][5] file lists features
not yet implemented and other outstanding items.

Please see the [INSTALL][7] file for installation instructions.

When working from `git(1)`, please use this file.  An abbreviated
installation procedure that works for most applications appears below.

This release is published under LGPL.  Please see the license
in the file [COPYING][9].


Quick Start
-----------

The quickest and easiest way to get icewm up and running is to run the
following commands:

    $> git clone https://github.com/bbidulock/icewm.git
    $> cd icewm
    $> ./autogen.sh
    $> ./configure
    $> make
    $> make DESTDIR="$pkgdir" install

This will configure, compile and install icewm the quickest.  For those who
like to spend the extra 15 seconds reading `./configure --help`, some compile
time options can be turned on and off before the build.

For general information on GNU's `./configure`, see the file [INSTALL][7].

Please see the [INSTALL][7] file for more detailed installation instructions.
An alternative way to build IceWM using CMake is [documented here][19].
The [ChangeLog][4] file contains a detailed history of implementation changes.
The [COMPLIANCE][6] file lists the current state of EWMH/ICCCM compliance.  The
[NEWS][3] file has release notes and history of user visible changes of the
current version.  The [TODO][5] file lists features not yet implemented and
other outstanding items.

This release is published under LGPL license that can be found in the file
[COPYING][9].

Prerequisites
-------------

Building from tarball requires:

 - gcc or clang
 - imlib2
 - libxcomposite
 - libxdamage
 - libxfixes
 - libxft
 - libxinerama
 - libxpm
 - libxrandr
 - libxrender

Building from git also requires:

 - complete autoconf or cmake toolchain
 - either markdown or asciidoctor


Configuring IceWM
-----------------

Documentation for configuring the window manager can be obtained from [IceWM
Website][13] or from the [online manual][15].
Since version 1.4.3 a complete and up-to-date set of manual pages is provided.
Use [__icewm__(1)][26] as a starting point.


Included Utilities
------------------

Currently, the only included utilities are:

 - [__icesh__(1)][25] (_a versatile window manipulation tool_),
 - [__icewmbg__(1)][22] (_a background setting program_),
 - [__icewm-session__(1)][27] (_a program to launch the window manager, icewmbg and
   icewmtray in an orderly fashion_),
 - [__icewm-menu-fdo__(1)][24] (_a utility to genenerate XDG menus_),
 - [__icewmhint__(1)][23] (_a utility to set IceWM-specific window options hint_).
 - [__icesound__(1)][21] (_play audio files when interesting GUI events happen_).


Third-party Utilities
---------------------

Unspecified keyboard shortcuts can be handled with the __bbkeys__(1) utility
available from [GitHub][16].

XDG compliant menus may be generated using the __xde-menu__(1) utility
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

See also [BUGS][8], [TODO][5] and the sites at:

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


[1]: https://github.com/bbidulock/icewm
[2]: https://github.com/bbidulock/icewm/issues
[3]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/NEWS
[4]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/ChangeLog
[5]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/TODO
[6]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/COMPLIANCE
[7]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/INSTALL
[8]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/BUGS
[9]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/COPYING
[10]: https://github.com/
[11]: https://l10n.opensuse.org/
[12]: https://sourceforge.net/projects/icewm/
[13]: https://ice-wm.org/
[14]: https://ice-wm.org/FAQ/
[15]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/doc/icewm.adoc
[16]: https://github.com/bbidulock/bbkeys/
[17]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/lib/icewm.desktop
[18]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/lib/icewm-session.desktop
[19]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/INSTALL-cmakebuild.md
[20]: https://github.com/bbidulock/xde-menu/
[21]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/man/icesound.pod
[22]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/man/icewmbg.pod
[23]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/man/icewmhint.pod
[24]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/man/icewm-menu-fdo.pod
[25]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/man/icesh.pod
[26]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/man/icewm.pod
[27]: https://github.com/bbidulock/icewm/blob/icewm-1-4-BRANCH/man/icewm-session.pod

[ vim: set ft=markdown sw=4 tw=80 nocin nosi fo+=tcqlorn spell: ]: #

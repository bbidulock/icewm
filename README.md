# Ice Window Manager (IceWM)

This is a fork of the original IceWM CVS that is on Sourceforge:
[http://icewm.sourceforge.net](http://icewm.sourceforge.net) <br>
What this includes is all changes made on the official IceWM CVS repository on the 'icewm-1-3-BRANCH' branch, greatly enhanced EWMH/ICCCM compliance, as well as patches collected from Arch Linux, Debian, pld-linux, the IceWM bug list, and other various GitHub forks.
<br>

The name was decided on a very hot day... (and Marko started writing 
it in winter ;-)  The aim of IceWM is to have good 'Feel' and decent
'Look'. 'Feel' is much more important than 'Look' ...  
<br>


---
**_Quick Start_**
---

This is the icewm-1.3.8 package, released under LGPL license 2013-10-10.  This release as well as future versions here, can all be obtained from the GitHub repository using a command such as:

    git clone http://github.com/bbidulock/icewm.git

<br> <br>
The quickest and easiest way to get IceWM up and running is to run the
following commands:

    git clone http://github.com/bbidulock/icewm.git
    autoreconf -fiv
    ./configure --prefix=/usr --sysconfdir=/etc --enable-shaped-decorations --enable-gradients
    make all docs nls
    sudo make DESTDIR="$pkgdir" install install-man install-docs install-nls  install-desktop

This will configure, compile and install IceWM the quickest.  For those
who would like to customize the installation, use the command:

    ./configure --help

When working from git(1), please see the `README-git' file.  An
abbreviated installation procedure that works for most applications
appears below.  

Please see the "INSTALL" file for more detailed installation instructions.  
The "ChangeLog" file contains a detailed history of implementation changes.  
The "COMPLIANCE" file lists the current state of EWMH/ICCCM compliance. 
The "NEWS" file has release notes and history of user visible changes of the current version.  
The "TODO" file lists features not yet implemented and other outstanding items.  

This release is published under LGPL license that can be found in the
file `COPYING'.  
<br>

---
**_Configuring IceWM_**
---

Documentation for configuring the window manager can be obtained from
http://www.icewm.org/ or from the online manual: point your browser at
file:///usr/share/doc/icewm-1.3.8/icewm.html.

Unfortunately the documentation is for version 1.2.27 and is incomplete
at that; however, it is for the most part usable.  Also, a rather sparse
`icewm(1)' manual page is available.  
<br>

---
**_Included Utilities_**
---

Currently, the only included utilities are **icewmbg** (_a background setting program_), **icewmtray** (_a system tray for the IceWM taskbar_), and **icewm-session** (_a program to launch the window manager, icewmbg and icewmtray in an orderly fashion_).  
<br>


---
**_Third-party Utilities_**
---

Unspecified keyboard shortcuts can be handled with the **bbkeys(1)**
utility available from: http://bbkeys.sorceforge.net  <br>
For additional utilities see: [http://www.icewm.org/FAQ/IceWM-FAQ-11.html](http://www.icewm.org/FAQ/IceWM-FAQ-11.html)  
<br>

---
**_wm-session_**
---

/proc/wm-session is used to register the process id of an application able to free resources smoothly when the kernel decides that memory resource have reached a critical limit. The registered application is notified of this situation by the signal SIGUSR1.<br>

On full featured desktop machines it would make sense to use the session manager for this purpose. On X window PDAs which have limited memory resources it makes sense to let the window manager send WM_DELETE_WINDOW message to the last recently used application.<br>
<br>

**Requirements to uses this feature in IceWM:**
  
  - A patched kernel, a patch for Linux 2.4.3 is available in the contrib
    file module.

  - A patched X server assigning the clients process id to each newly
    mapped window. Alternatively you can preload the preice library
    available in the contrib file module.
    
  - $ export LD_PRELOAD=$PATH_TO_libpreice.so).

  - IceWM configured to have wm-session support
    (./configure --enable-wm-session ...)

The contrib file module of IceWM is located at:  
[http://sf.net/project/showfiles.php?group_id=31&release_id=31119](http://sf.net/project/showfiles.php?group_id=31&release_id=31119)  
<br>


/proc/wm-session was developed by

    Chester Kuo <chester@linux.org.tw> and Mathias Hasselman <mathias.hasselman@gmx.de>.

<br>

---
**_Bug Reports_**
---

Issues can be reported using the Issues' utility here on GitHub.  Please try to submit short patches if you can.  If you would like to perform regular maintenance activities (e.g. if you are a maintainer of an IceWM package for a distribution), contact me for push access.

Bug reports, feedback, and suggestions pertaining to the original CVS version can be sent to:

    Marko.Macek@gmx.net or
    icewm-user@lists.sourceforge.net

See also BUGS, TODO and the sites at:

    http://www.icewm.org/
    http://www.sourceforge.net/projects/icewm/
	http://icewm.sourceforge.net/

<br>


---
**_Development_**
---

If you would like to develop against this fork, the easiest way is to
obtain a GitHub account from (http://github.com), fork the repository
at (http://github.com/bbidulock/icewm) and perform your development.
Send me a pull request when you have something stable.

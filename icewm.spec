
%define ver 1.0.0
%define prefix /usr/X11R6

Summary: X11 Window Manager
Name: icewm 
Version: %ver
Release: 1
Copyright: LGPL
Group: X11/Window Managers
Source: icewm-%ver.src.tar.gz
URL: http://www.kiss.uni-lj.si/~k4fr0235/icewm/
Packager: Marko Macek <Marko.Macek@gmx.net>
BuildRoot: /tmp/build-icewm-%ver

%description
Window Manager for X Window System. Can emulate the look of Windows'95,
OS/2 Warp 3,4, Motif. Tries to take the best features of the above systems.
Features multiple workspaces, opaque move/resize, task bar, window list,
mailbox status, digital clock. Fast and small.

%prep

%setup

%build
CXXFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{prefix} --without-gnome-menus
make

%install
make prefix=$RPM_BUILD_ROOT%{prefix} ETCDIR=$RPM_BUILD_ROOT/etc/X11/icewm install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README COPYING CHANGES TODO BUGS icewm.lsm FAQ doc/*.html doc/icewm.sgml
%{prefix}/lib/X11/icewm/
%{prefix}/bin/icewm
%{prefix}/bin/icewmhint
%{prefix}/bin/icewmbg

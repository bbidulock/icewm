include ../VERSION
-include ../install.inc
-include ../sysdep.inc

# executable extenstion
EXEEXT=

CXXFLAGS    = \
	$(I18N) $(DEBUG) $(OS2)\
	-DLIBDIR='"$(LIBDIR)"' \
	-DCFGDIR='"$(CFGDIR)"' \
	-DLOCDIR='"$(LOCDIR)"' \
	-DKDEDIR='"$(KDEDIR)"' \
	-DPACKAGE='"icewm"' \
	-DVERSION='"$(VERSION)"' \
	-DEXEEXT='"$(EXEEXT)"' \
	-DICEWMEXE='"icewm"EXEEXT' \
	-DICEHELPEXE='"icehelp"EXEEXT' \
	-DICEHELPIDX='"$(DOCDIR)/icewm-$(VERSION)/doc/icewm.html"' \
        $(SYS_CFLAGS) $(SYS_INCDIRS)
LFLAGS      = 
LIBS        = $(SYS_LIBDIRS) $(SYS_LIBS)

COREOBJS    = \
        yapp.o ytimer.o ywindow.o ypaint.o ytooltip.o ypopup.o \
        ymenu.o ymenuitem.o yscrollbar.o ybutton.o ylistbox.o yinput.o \
        misc.o icons.o ycursor.o ylabel.o yscrollview.o ysocket.o \
        wmconfig.o

ICEWMOBJS        = \
        ymsgbox.o ydialog.o yurl.o \
        wmsession.o wmwinlist.o wmtaskbar.o wmwinmenu.o \
        wmdialog.o wmabout.o wmswitch.o wmstatus.o \
        wmoption.o wmaction.o \
        wmcontainer.o wmclient.o \
        wmmgr.o wmapp.o \
        wmframe.o wmbutton.o wmminiicon.o wmtitle.o movesize.o \
        themes.o decorate.o gnome.o browse.o \
        wmprog.o \
        atasks.o aworkspaces.o amailbox.o aclock.o acpustatus.o \
	apppstatus.o aaddressbar.o objbar.o aapm.o \
        $(COREOBJS)

GENPREFOBJS = genpref.o

ICEWMHINTOBJS = icewmhint.o

ICEWMBGOBJS = icewmbg.o

ICESOUNDOBJS = icesound.o misc.o 

ICECLOCKOBJS = $(COREOBJS) \
          iceclock.o aclock.o

ICEBAROBJS = $(COREOBJS) \
          wmtaskbar.o \
          wmprog.o gnome.o browse.o themes.o wmaction.o \
	  amailbox.o aclock.o acpustatus.o apppstatus.o aaddressbar.o objbar.o

ICESAMEOBJS = $(COREOBJS) \
          icesame.o

ICELISTOBJS = $(COREOBJS) \
             icelist.o

ICEVIEWOBJS = $(COREOBJS) \
             iceview.o

ICEICONOBJS = $(COREOBJS) \
             iceicon.o

ICEHELPOBJS = $(COREOBJS) \
             icehelp.o

ICERUNOBJS = $(COREOBJS) \
             icerun.o

ICESKTOBJS = $(COREOBJS) \
             iceskt.o

PROGRAMS    = icewm$(EXEEXT) genpref$(EXEEXT) icewmhint$(EXEEXT) icewmbg$(EXEEXT) icehelp$(EXEEXT) #icelist$(EXEEXT) #iceclock$(EXEEXT) #icebar$(EXEEXT) #icesound$(EXEEXT) #icerun$(EXEEXT)

.SUFFIXES:
.SUFFIXES: .cc .o .h

.cc.o:
	$(CXX) $(CXXFLAGS) -c $<

all: $(PROGRAMS) ../lib/preferences

clean:
	rm -f $(PROGRAMS) *.o *~

../lib/preferences: genpref
	./genpref >../lib/preferences

icewm$(EXEEXT): $(ICEWMOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEWMOBJS) $(LIBS)

genpref$(EXEEXT): $(GENPREFOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(GENPREFOBJS)

icewmhint$(EXEEXT): $(ICEWMHINTOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEWMHINTOBJS) $(LIBS)

icewmbg$(EXEEXT): $(ICEWMBGOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEWMBGOBJS) $(LIBS)

icesound$(EXEEXT): $(ICESOUNDOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICESOUNDOBJS) $(LIBS) -lesd

iceclock$(EXEEXT): $(ICECLOCKOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICECLOCKOBJS) $(LIBS)

icebar$(EXEEXT): $(ICEBAROBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEBAROBJS) $(LIBS)

icesame$(EXEEXT): $(ICESAMEOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICESAMEOBJS) $(LIBS)

icelist$(EXEEXT): $(ICELISTOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICELISTOBJS) $(LIBS)

iceview$(EXEEXT): $(ICEVIEWOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEVIEWOBJS) $(LIBS)

iceicon$(EXEEXT): $(ICEICONOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEICONOBJS) $(LIBS)

icehelp$(EXEEXT): $(ICEHELPOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEHELPOBJS) $(LIBS)

icerun$(EXEEXT): $(ICERUNOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICERUNOBJS) $(LIBS)

iceskt$(EXEEXT): $(ICESKTOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICESKTOBJS) $(LIBS)

libice.so: $(COREOBJS)
	-@rm -f $@
	ld -shared -o $@ $(COREOBJS)

# for best results patch gccmakedep to use -MM instead of -M
depend:
	gccmakedep *.cc

# DO NOT DELETE
aaddressbar.o: aaddressbar.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h aaddressbar.h \
 yinputline.h ywindow.h ypaint.h ycursor.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h yaction.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 default.h
aapm.o: aapm.cc config.h ylib.h base.h debug.h /usr/include/X11/X.h \
 /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h sysdep.h \
 /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/time.h \
 /usr/include/bits/time.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/bits/siginfo.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 aapm.h ywindow.h ypaint.h ycursor.h ytimer.h aclock.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h prefs.h \
 default.h bindkey.h intl.h /usr/include/libintl.h
aclock.o: aclock.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h aclock.h \
 ywindow.h ypaint.h ycursor.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h prefs.h \
 default.h bindkey.h /usr/include/string.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h
acpustatus.o: acpustatus.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ywindow.h ypaint.h ycursor.h \
 acpustatus.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 default.h /usr/include/sys/sysinfo.h /usr/include/linux/kernel.h \
 intl.h /usr/include/libintl.h
amailbox.o: amailbox.cc config.h intl.h /usr/include/libintl.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h ylib.h base.h debug.h /usr/include/X11/X.h \
 /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h amailbox.h \
 ywindow.h ypaint.h ycursor.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h ysocket.h /usr/include/netinet/in.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/stdint.h /usr/include/bits/wordsize.h \
 /usr/include/bits/socket.h /usr/include/bits/sockaddr.h \
 /usr/include/asm/socket.h /usr/include/asm/sockios.h \
 /usr/include/bits/in.h /usr/include/bits/byteswap.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 prefs.h default.h bindkey.h /usr/include/sys/socket.h \
 /usr/include/netdb.h /usr/include/rpc/netdb.h
apppstatus.o: apppstatus.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ywindow.h ypaint.h ycursor.h \
 apppstatus.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h /usr/include/sys/socket.h \
 /usr/include/bits/socket.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/bits/sockaddr.h /usr/include/asm/socket.h \
 /usr/include/asm/sockios.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 prefs.h default.h bindkey.h intl.h /usr/include/libintl.h \
 /usr/include/net/if.h /usr/include/sys/ioctl.h \
 /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
 /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
 /usr/include/sys/ttydefaults.h
atasks.o: atasks.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h atasks.h \
 ywindow.h ypaint.h ycursor.h wmclient.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h MwmUtil.h \
 /usr/include/X11/Xmd.h wmtaskbar.h yaction.h ybutton.h apppstatus.h \
 /usr/include/sys/socket.h /usr/include/bits/socket.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/bits/sockaddr.h /usr/include/asm/socket.h \
 /usr/include/asm/sockios.h prefs.h default.h bindkey.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h wmmgr.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xprotostr.h WinMgr.h wmframe.h ymsgbox.h ydialog.h \
 ylabel.h yactionbutton.h wmbutton.h wmoption.h wmwinlist.h ylistbox.h \
 yscrollbar.h yscrollview.h /usr/include/string.h
aworkspaces.o: aworkspaces.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 aworkspaces.h ywindow.h ypaint.h ycursor.h ybutton.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h wmtaskbar.h \
 yaction.h ymenu.h ypopup.h wmclient.h MwmUtil.h \
 /usr/include/X11/Xmd.h apppstatus.h /usr/include/sys/socket.h \
 /usr/include/bits/socket.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/bits/sockaddr.h /usr/include/asm/socket.h \
 /usr/include/asm/sockios.h prefs.h default.h bindkey.h wmmgr.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xprotostr.h WinMgr.h \
 wmapp.h yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ymsgbox.h ydialog.h ylabel.h \
 yactionbutton.h guievent.h wmframe.h wmbutton.h wmoption.h
browse.o: browse.cc config.h obj.h ypaint.h /usr/include/X11/X.h \
 objmenu.h ymenu.h ypopup.h ywindow.h ycursor.h ytimer.h base.h \
 debug.h /usr/include/sys/time.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/time.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/bits/time.h /usr/include/bits/types.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h ymenuitem.h yaction.h browse.h wmmgr.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h WinMgr.h wmprog.h sysdep.h \
 /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/endian.h \
 /usr/include/bits/endian.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/errno.h /usr/include/bits/errno.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h \
 /usr/include/sys/stat.h /usr/include/bits/stat.h \
 /usr/include/sys/wait.h /usr/include/bits/waitflags.h \
 /usr/include/bits/waitstatus.h /usr/include/bits/siginfo.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 /usr/include/dirent.h /usr/include/bits/dirent.h
decorate.o: decorate.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 wmframe.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h ymsgbox.h ydialog.h \
 wmclient.h MwmUtil.h ylabel.h yactionbutton.h ybutton.h yaction.h \
 wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h wmmgr.h \
 wmaction.h wmtitle.h wmapp.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h guievent.h \
 wmcontainer.h ymenuitem.h
genpref.o: genpref.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h sysdep.h \
 /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/time.h \
 /usr/include/bits/time.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/bits/siginfo.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 bindkey.h default.h intl.h /usr/include/libintl.h
iceclock.o: iceclock.cc config.h yapp.h /usr/include/signal.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/sigset.h \
 /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/bits/signum.h /usr/include/time.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ywindow.h ypaint.h /usr/include/X11/X.h \
 ycursor.h aclock.h ytimer.h base.h debug.h /usr/include/sys/time.h \
 /usr/include/bits/time.h /usr/include/sys/select.h \
 /usr/include/bits/select.h
icehelp.o: icehelp.cc /usr/include/stdio.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
 /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/X11/Xlocale.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/locale.h config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/X11/Xatom.h \
 ylistbox.h ywindow.h ypaint.h ycursor.h yscrollbar.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h yscrollview.h \
 ymenu.h ypopup.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h sysdep.h \
 /usr/include/assert.h /usr/include/string.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 yaction.h ymenuitem.h prefs.h default.h bindkey.h intl.h \
 /usr/include/libintl.h
iceicon.o: iceicon.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/Xatom.h ywindow.h ypaint.h ycursor.h yscrollbar.h \
 ytimer.h /usr/include/sys/time.h /usr/include/bits/time.h \
 yscrollview.h ymenu.h ypopup.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h yaction.h \
 wmmgr.h /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h WinMgr.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 /usr/include/dirent.h /usr/include/bits/dirent.h
icelist.o: icelist.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/Xatom.h ylistbox.h ywindow.h ypaint.h ycursor.h \
 yscrollbar.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h yscrollview.h ymenu.h ypopup.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h yaction.h yinputline.h wmmgr.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h WinMgr.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 /usr/include/dirent.h /usr/include/bits/dirent.h intl.h \
 /usr/include/libintl.h
icerun.o: icerun.cc config.h yapp.h /usr/include/signal.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/sigset.h \
 /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/bits/signum.h /usr/include/time.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ywindow.h ypaint.h /usr/include/X11/X.h \
 ycursor.h yinputline.h ytimer.h base.h debug.h \
 /usr/include/sys/time.h /usr/include/bits/time.h \
 /usr/include/sys/select.h /usr/include/bits/select.h yaction.h \
 ylabel.h ybutton.h prefs.h default.h bindkey.h
icesame.o: icesame.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 ywindow.h ypaint.h ycursor.h ylabel.h ymenuitem.h ymenu.h ypopup.h \
 ytimer.h /usr/include/sys/time.h /usr/include/bits/time.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h yaction.h MwmUtil.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/string.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h intl.h /usr/include/libintl.h
iceskt.o: iceskt.cc config.h yapp.h /usr/include/signal.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/sigset.h \
 /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/bits/signum.h /usr/include/time.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ywindow.h ypaint.h /usr/include/X11/X.h \
 ycursor.h ysocket.h /usr/include/sys/types.h /usr/include/endian.h \
 /usr/include/bits/endian.h /usr/include/sys/select.h \
 /usr/include/bits/select.h /usr/include/sys/sysmacros.h \
 /usr/include/netinet/in.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/stdint.h /usr/include/bits/wordsize.h \
 /usr/include/bits/socket.h /usr/include/bits/sockaddr.h \
 /usr/include/asm/socket.h /usr/include/asm/sockios.h \
 /usr/include/bits/in.h /usr/include/bits/byteswap.h debug.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h
icesound.o: icesound.cc config.h intl.h /usr/include/libintl.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/string.h /usr/include/stdlib.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/errno.h /usr/include/bits/errno.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h \
 /usr/include/sys/time.h /usr/include/bits/time.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h /usr/include/X11/Xatom.h \
 /usr/include/X11/Xlib.h /usr/include/X11/X.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/keysym.h \
 /usr/include/X11/keysymdef.h /usr/include/X11/xpm.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h /usr/include/audiofile.h \
 /usr/include/aupvlist.h /usr/include/esd.h guievent.h
icesound2.o: icesound2.cc config.h intl.h /usr/include/libintl.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/ctype.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/string.h /usr/include/stdlib.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/sys/select.h \
 /usr/include/bits/select.h /usr/include/bits/sigset.h \
 /usr/include/sys/sysmacros.h /usr/include/alloca.h \
 /usr/include/errno.h /usr/include/bits/errno.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h \
 /usr/include/sys/time.h /usr/include/bits/time.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h /usr/include/X11/Xatom.h \
 /usr/include/X11/Xlib.h /usr/include/X11/X.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/keysym.h \
 /usr/include/X11/keysymdef.h /usr/include/X11/xpm.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h /usr/include/Y2/Y.h \
 /usr/include/Y2/Ylib.h /usr/include/audiofile.h \
 /usr/include/aupvlist.h /usr/include/esd.h guievent.h
icesound3.o: icesound3.cc config.h intl.h /usr/include/libintl.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/ctype.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/string.h /usr/include/stdlib.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/sys/select.h \
 /usr/include/bits/select.h /usr/include/bits/sigset.h \
 /usr/include/sys/sysmacros.h /usr/include/alloca.h \
 /usr/include/errno.h /usr/include/bits/errno.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h \
 /usr/include/sys/time.h /usr/include/bits/time.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h /usr/include/X11/Xatom.h \
 /usr/include/X11/Xlib.h /usr/include/X11/X.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/keysym.h \
 /usr/include/X11/keysymdef.h /usr/include/X11/xpm.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h /usr/include/Y2/Y.h \
 /usr/include/Y2/Ylib.h /usr/include/audiofile.h \
 /usr/include/aupvlist.h /usr/include/esd.h guievent.h
icetail.o: icetail.cc config.h /usr/include/assert.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/string.h /usr/include/stdlib.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/errno.h /usr/include/bits/errno.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h \
 /usr/include/sys/time.h /usr/include/bits/time.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h /usr/include/X11/Xatom.h \
 /usr/include/X11/Xlib.h /usr/include/X11/X.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/keysym.h \
 /usr/include/X11/keysymdef.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h intl.h \
 /usr/include/libintl.h base.h debug.h WinMgr.h
iceview.o: iceview.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/Xatom.h ylistbox.h ywindow.h ypaint.h ycursor.h \
 yscrollbar.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h yscrollview.h ymenu.h ypopup.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 yaction.h /usr/include/sys/mman.h /usr/include/bits/mman.h intl.h \
 /usr/include/libintl.h
icewmbg.o: icewmbg.cc config.h /usr/include/assert.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/string.h /usr/include/stdlib.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/errno.h /usr/include/bits/errno.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h \
 /usr/include/sys/time.h /usr/include/bits/time.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h /usr/include/X11/Xatom.h \
 /usr/include/X11/Xlib.h /usr/include/X11/X.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/keysym.h \
 /usr/include/X11/keysymdef.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h intl.h \
 /usr/include/libintl.h /usr/include/Imlib.h \
 /usr/include/Imlib_types.h /usr/include/X11/Xos.h \
 /usr/include/X11/Xarch.h /usr/include/X11/extensions/shape.h \
 /usr/include/math.h /usr/include/bits/huge_val.h \
 /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/float.h \
 /usr/include/ctype.h /usr/include/netinet/in.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/stdint.h /usr/include/bits/wordsize.h \
 /usr/include/bits/socket.h /usr/include/bits/sockaddr.h \
 /usr/include/asm/socket.h /usr/include/asm/sockios.h \
 /usr/include/bits/in.h /usr/include/bits/byteswap.h base.h debug.h \
 WinMgr.h
icewmhint.o: icewmhint.cc config.h /usr/include/assert.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/string.h /usr/include/stdlib.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/errno.h /usr/include/bits/errno.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h \
 /usr/include/sys/time.h /usr/include/bits/time.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h /usr/include/X11/Xatom.h \
 /usr/include/X11/Xlib.h /usr/include/X11/X.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/keysym.h \
 /usr/include/X11/keysymdef.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h guievent.h \
 intl.h /usr/include/libintl.h
icons.o: icons.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 ypaint.h yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ywindow.h ycursor.h sysdep.h \
 /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/time.h \
 /usr/include/bits/time.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 prefs.h default.h bindkey.h wmprog.h objmenu.h ymenu.h ypopup.h \
 ytimer.h ymenuitem.h yaction.h obj.h /usr/include/Imlib.h \
 /usr/include/Imlib_types.h /usr/include/X11/Xos.h \
 /usr/include/X11/Xarch.h /usr/include/math.h \
 /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
 /usr/include/bits/mathcalls.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/float.h \
 /usr/include/netinet/in.h /usr/include/stdint.h \
 /usr/include/bits/wordsize.h /usr/include/bits/socket.h \
 /usr/include/bits/sockaddr.h /usr/include/asm/socket.h \
 /usr/include/asm/sockios.h /usr/include/bits/in.h \
 /usr/include/bits/byteswap.h intl.h /usr/include/libintl.h
misc.o: misc.cc config.h sysdep.h /usr/include/assert.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/string.h /usr/include/stdlib.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/errno.h /usr/include/bits/errno.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h \
 /usr/include/sys/time.h /usr/include/bits/time.h \
 /usr/include/sys/stat.h /usr/include/bits/stat.h \
 /usr/include/sys/wait.h /usr/include/bits/waitflags.h \
 /usr/include/bits/waitstatus.h /usr/include/bits/siginfo.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 ylib.h base.h debug.h /usr/include/X11/X.h /usr/include/X11/Xlib.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h intl.h \
 /usr/include/libintl.h
movesize.o: movesize.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 wmframe.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h ymsgbox.h ydialog.h \
 wmclient.h MwmUtil.h ylabel.h yactionbutton.h ybutton.h yaction.h \
 wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h wmmgr.h \
 wmstatus.h yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h wmapp.h guievent.h
objbar.o: objbar.cc config.h objbar.h ywindow.h ypaint.h \
 /usr/include/X11/X.h ycursor.h ybutton.h obj.h objmenu.h ymenu.h \
 ypopup.h ytimer.h base.h debug.h /usr/include/sys/time.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/time.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/bits/time.h /usr/include/bits/types.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h ymenuitem.h yaction.h objbutton.h prefs.h \
 default.h bindkey.h wmtaskbar.h wmclient.h MwmUtil.h \
 /usr/include/X11/Xmd.h apppstatus.h /usr/include/sys/socket.h \
 /usr/include/bits/socket.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/sys/types.h /usr/include/endian.h \
 /usr/include/bits/endian.h /usr/include/sys/sysmacros.h \
 /usr/include/bits/sockaddr.h /usr/include/asm/socket.h \
 /usr/include/asm/sockios.h
testwinhints.o: testwinhints.cc /usr/include/stdio.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
 /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/string.h /usr/include/assert.h \
 /usr/include/sys/time.h /usr/include/bits/time.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h /usr/include/X11/Xatom.h \
 /usr/include/X11/Xlib.h /usr/include/X11/X.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xresource.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h WinMgr.h
themes.o: themes.cc config.h themes.h objmenu.h ymenu.h ypopup.h \
 ywindow.h ypaint.h /usr/include/X11/X.h ycursor.h ytimer.h base.h \
 debug.h /usr/include/sys/time.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/time.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/bits/time.h /usr/include/bits/types.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h ymenuitem.h yaction.h obj.h wmmgr.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h WinMgr.h wmprog.h sysdep.h \
 /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/endian.h \
 /usr/include/bits/endian.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/errno.h /usr/include/bits/errno.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h \
 /usr/include/sys/stat.h /usr/include/bits/stat.h \
 /usr/include/sys/wait.h /usr/include/bits/waitflags.h \
 /usr/include/bits/waitstatus.h /usr/include/bits/siginfo.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 prefs.h default.h bindkey.h /usr/include/dirent.h \
 /usr/include/bits/dirent.h intl.h /usr/include/libintl.h
wmabout.o: wmabout.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h wmabout.h \
 ydialog.h wmclient.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h \
 ytimer.h /usr/include/sys/time.h /usr/include/bits/time.h MwmUtil.h \
 /usr/include/X11/Xmd.h ylabel.h prefs.h default.h bindkey.h wmapp.h \
 yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ymsgbox.h yactionbutton.h ybutton.h \
 yaction.h guievent.h wmframe.h wmbutton.h wmoption.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xprotostr.h WinMgr.h \
 wmmgr.h sysdep.h /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 intl.h /usr/include/libintl.h
wmaction.o: wmaction.cc config.h wmaction.h yaction.h
wmbutton.o: wmbutton.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h wmbutton.h \
 yactionbutton.h ybutton.h ywindow.h ypaint.h ycursor.h yaction.h \
 ymenu.h ypopup.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h wmaction.h wmframe.h ymsgbox.h ydialog.h \
 wmclient.h MwmUtil.h /usr/include/X11/Xmd.h ylabel.h wmoption.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xprotostr.h prefs.h \
 default.h bindkey.h WinMgr.h wmmgr.h wmtitle.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h
wmclient.o: wmclient.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 wmclient.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h MwmUtil.h wmframe.h \
 ymsgbox.h ydialog.h ylabel.h yactionbutton.h ybutton.h yaction.h \
 wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h wmmgr.h \
 wmapp.h yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h guievent.h sysdep.h \
 /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h
wmconfig.o: wmconfig.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h wmoption.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h ypaint.h wmmgr.h ywindow.h ycursor.h \
 ymenu.h ypopup.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h WinMgr.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/bits/siginfo.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 prefs.h default.h bindkey.h yaction.h intl.h /usr/include/libintl.h
wmcontainer.o: wmcontainer.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 wmcontainer.h ywindow.h ypaint.h ycursor.h wmframe.h ymenu.h ypopup.h \
 ytimer.h /usr/include/sys/time.h /usr/include/bits/time.h ymsgbox.h \
 ydialog.h wmclient.h MwmUtil.h /usr/include/X11/Xmd.h ylabel.h \
 yactionbutton.h ybutton.h yaction.h wmbutton.h wmoption.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xprotostr.h prefs.h \
 default.h bindkey.h WinMgr.h wmmgr.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h
wmdialog.o: wmdialog.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h wmdialog.h \
 ywindow.h ypaint.h ycursor.h yaction.h wmaction.h yactionbutton.h \
 ybutton.h prefs.h default.h bindkey.h wmapp.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h ymsgbox.h ydialog.h \
 wmclient.h MwmUtil.h /usr/include/X11/Xmd.h ylabel.h guievent.h \
 wmmgr.h /usr/include/X11/Xproto.h /usr/include/X11/Xprotostr.h \
 WinMgr.h intl.h /usr/include/libintl.h
wmframe.o: wmframe.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 wmframe.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h ymsgbox.h ydialog.h \
 wmclient.h MwmUtil.h ylabel.h yactionbutton.h ybutton.h yaction.h \
 wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h wmmgr.h \
 atasks.h wmaction.h wmcontainer.h wmtitle.h wmminiicon.h wmswitch.h \
 wmtaskbar.h apppstatus.h /usr/include/sys/socket.h \
 /usr/include/bits/socket.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/bits/sockaddr.h /usr/include/asm/socket.h \
 /usr/include/asm/sockios.h wmwinlist.h ylistbox.h yscrollbar.h \
 yscrollview.h wmapp.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h guievent.h \
 sysdep.h /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 intl.h /usr/include/libintl.h
wmmgr.o: wmmgr.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 wmmgr.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h WinMgr.h \
 aaddressbar.h yinputline.h yaction.h atasks.h wmclient.h MwmUtil.h \
 aworkspaces.h ybutton.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/bits/siginfo.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 wmtaskbar.h apppstatus.h /usr/include/sys/socket.h \
 /usr/include/bits/socket.h /usr/include/bits/sockaddr.h \
 /usr/include/asm/socket.h /usr/include/asm/sockios.h wmwinlist.h \
 ylistbox.h yscrollbar.h yscrollview.h objmenu.h ymenuitem.h obj.h \
 wmswitch.h wmstatus.h wmminiicon.h wmcontainer.h wmframe.h ymsgbox.h \
 ydialog.h ylabel.h yactionbutton.h wmbutton.h wmoption.h prefs.h \
 default.h bindkey.h wmdialog.h wmsession.h wmapp.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h guievent.h \
 wmaction.h wmprog.h
wmminiicon.o: wmminiicon.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h wmminiicon.h \
 ywindow.h ypaint.h ycursor.h wmframe.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h ymsgbox.h ydialog.h \
 wmclient.h MwmUtil.h /usr/include/X11/Xmd.h ylabel.h yactionbutton.h \
 ybutton.h yaction.h wmbutton.h wmoption.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xprotostr.h prefs.h default.h bindkey.h WinMgr.h \
 wmmgr.h yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h /usr/include/string.h
wmoption.o: wmoption.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 wmoption.h wmframe.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h \
 ytimer.h /usr/include/sys/time.h /usr/include/bits/time.h ymsgbox.h \
 ydialog.h wmclient.h MwmUtil.h ylabel.h yactionbutton.h ybutton.h \
 yaction.h wmbutton.h prefs.h default.h bindkey.h WinMgr.h wmmgr.h \
 sysdep.h /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/bits/siginfo.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 intl.h /usr/include/libintl.h
wmsession.o: wmsession.cc config.h yfull.h ykey.h ylib.h base.h \
 debug.h /usr/include/X11/X.h /usr/include/X11/Xlib.h \
 /usr/include/sys/types.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 wmframe.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h ymsgbox.h ydialog.h \
 wmclient.h MwmUtil.h ylabel.h yactionbutton.h ybutton.h yaction.h \
 wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h wmmgr.h \
 wmsession.h wmapp.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h guievent.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h intl.h \
 /usr/include/libintl.h
wmstatus.o: wmstatus.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 wmstatus.h ywindow.h ypaint.h ycursor.h wmswitch.h ymenu.h ypopup.h \
 ytimer.h /usr/include/sys/time.h /usr/include/bits/time.h wmframe.h \
 ymsgbox.h ydialog.h wmclient.h MwmUtil.h ylabel.h yactionbutton.h \
 ybutton.h yaction.h wmbutton.h wmoption.h prefs.h default.h bindkey.h \
 WinMgr.h wmmgr.h intl.h /usr/include/libintl.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h
wmswitch.o: wmswitch.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h wmswitch.h \
 ymenu.h ypopup.h ywindow.h ypaint.h ycursor.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h wmmgr.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xmd.h \
 /usr/include/X11/Xprotostr.h WinMgr.h wmframe.h ymsgbox.h ydialog.h \
 wmclient.h MwmUtil.h ylabel.h yactionbutton.h ybutton.h yaction.h \
 wmbutton.h wmoption.h prefs.h default.h bindkey.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h /usr/include/string.h
wmtaskbar.o: wmtaskbar.cc config.h yfull.h ykey.h ylib.h base.h \
 debug.h /usr/include/X11/X.h /usr/include/X11/Xlib.h \
 /usr/include/sys/types.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 wmtaskbar.h yaction.h ybutton.h ywindow.h ypaint.h ycursor.h ymenu.h \
 ypopup.h ytimer.h /usr/include/sys/time.h /usr/include/bits/time.h \
 wmclient.h MwmUtil.h apppstatus.h /usr/include/sys/socket.h \
 /usr/include/bits/socket.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/bits/sockaddr.h /usr/include/asm/socket.h \
 /usr/include/asm/sockios.h ymenuitem.h wmmgr.h WinMgr.h wmframe.h \
 ymsgbox.h ydialog.h ylabel.h yactionbutton.h wmbutton.h wmoption.h \
 prefs.h default.h bindkey.h wmapp.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h guievent.h \
 wmaction.h wmprog.h objmenu.h obj.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 wmwinlist.h ylistbox.h yscrollbar.h yscrollview.h aaddressbar.h \
 yinputline.h aclock.h acpustatus.h amailbox.h ysocket.h \
 /usr/include/netinet/in.h /usr/include/stdint.h \
 /usr/include/bits/wordsize.h /usr/include/bits/in.h \
 /usr/include/bits/byteswap.h objbar.h atasks.h aworkspaces.h aapm.h \
 intl.h /usr/include/libintl.h
wmtitle.o: wmtitle.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h wmtitle.h \
 ywindow.h ypaint.h ycursor.h wmframe.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h ymsgbox.h ydialog.h \
 wmclient.h MwmUtil.h /usr/include/X11/Xmd.h ylabel.h yactionbutton.h \
 ybutton.h yaction.h wmbutton.h wmoption.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xprotostr.h prefs.h default.h bindkey.h WinMgr.h \
 wmmgr.h wmwinlist.h ylistbox.h yscrollbar.h yscrollview.h wmapp.h \
 yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h guievent.h /usr/include/string.h
wmwinlist.o: wmwinlist.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h wmwinlist.h \
 wmclient.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h MwmUtil.h \
 /usr/include/X11/Xmd.h ylistbox.h yscrollbar.h yscrollview.h \
 yaction.h ymenuitem.h wmaction.h wmframe.h ymsgbox.h ydialog.h \
 ylabel.h yactionbutton.h ybutton.h wmbutton.h wmoption.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xprotostr.h prefs.h \
 default.h bindkey.h WinMgr.h wmmgr.h wmapp.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h guievent.h sysdep.h \
 /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 intl.h /usr/include/libintl.h
wmwinmenu.o: wmwinmenu.cc config.h wmaction.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h ymenu.h \
 ypopup.h ywindow.h ypaint.h ycursor.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h ymenuitem.h \
 yaction.h sysdep.h /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/bits/siginfo.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 prefs.h default.h bindkey.h wmmgr.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h WinMgr.h \
 wmframe.h ymsgbox.h ydialog.h wmclient.h MwmUtil.h ylabel.h \
 yactionbutton.h ybutton.h wmbutton.h wmoption.h wmwinmenu.h intl.h \
 /usr/include/libintl.h
yapp.o: yapp.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ywindow.h ypaint.h ycursor.h ysocket.h \
 /usr/include/netinet/in.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/stdint.h /usr/include/bits/wordsize.h \
 /usr/include/bits/socket.h /usr/include/bits/sockaddr.h \
 /usr/include/asm/socket.h /usr/include/asm/sockios.h \
 /usr/include/bits/in.h /usr/include/bits/byteswap.h sysdep.h \
 /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/time.h \
 /usr/include/bits/time.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 wmprog.h objmenu.h ymenu.h ypopup.h ytimer.h ymenuitem.h yaction.h \
 obj.h wmmgr.h WinMgr.h MwmUtil.h prefs.h default.h bindkey.h intl.h \
 /usr/include/libintl.h /usr/include/X11/SM/SMlib.h \
 /usr/include/X11/SM/SM.h /usr/include/X11/ICE/ICElib.h \
 /usr/include/X11/ICE/ICE.h
ybutton.o: ybutton.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h ybutton.h \
 ywindow.h ypaint.h ycursor.h yaction.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h prefs.h default.h bindkey.h wmtaskbar.h \
 wmclient.h MwmUtil.h /usr/include/X11/Xmd.h apppstatus.h \
 /usr/include/sys/socket.h /usr/include/bits/socket.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/bits/sockaddr.h /usr/include/asm/socket.h \
 /usr/include/asm/sockios.h /usr/include/string.h intl.h \
 /usr/include/libintl.h
ycursor.o: ycursor.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ywindow.h ypaint.h ycursor.h wmprog.h \
 objmenu.h ymenu.h ypopup.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h ymenuitem.h yaction.h obj.h \
 /usr/include/Imlib.h /usr/include/Imlib_types.h \
 /usr/include/X11/Xos.h /usr/include/string.h /usr/include/fcntl.h \
 /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/X11/Xarch.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/math.h \
 /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
 /usr/include/bits/mathcalls.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/float.h \
 /usr/include/ctype.h /usr/include/netinet/in.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/stdint.h /usr/include/bits/wordsize.h \
 /usr/include/bits/socket.h /usr/include/bits/sockaddr.h \
 /usr/include/asm/socket.h /usr/include/asm/sockios.h \
 /usr/include/bits/in.h /usr/include/bits/byteswap.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/../../../../include/g++-3/cstdio \
 intl.h /usr/include/libintl.h
ydialog.o: ydialog.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h ydialog.h \
 wmclient.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h MwmUtil.h \
 /usr/include/X11/Xmd.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h prefs.h \
 default.h bindkey.h WinMgr.h wmframe.h ymsgbox.h ylabel.h \
 yactionbutton.h ybutton.h yaction.h wmbutton.h wmoption.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xprotostr.h wmmgr.h \
 wmdialog.h wmabout.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h
yinput.o: yinput.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h yinputline.h \
 ywindow.h ypaint.h ycursor.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h yaction.h ymenu.h ypopup.h ymenuitem.h \
 yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h prefs.h default.h bindkey.h \
 /usr/include/string.h intl.h /usr/include/libintl.h
ylabel.o: ylabel.cc config.h ylabel.h ywindow.h ypaint.h \
 /usr/include/X11/X.h ycursor.h ymenu.h ypopup.h ytimer.h base.h \
 debug.h /usr/include/sys/time.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/time.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/bits/time.h /usr/include/bits/types.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h prefs.h default.h bindkey.h
ylistbox.o: ylistbox.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h ylistbox.h \
 ywindow.h ypaint.h ycursor.h yscrollbar.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h yscrollview.h \
 ymenu.h ypopup.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h prefs.h \
 default.h bindkey.h /usr/include/string.h
ymenu.o: ymenu.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h ymenu.h \
 ypopup.h ywindow.h ypaint.h ycursor.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h yaction.h \
 ymenuitem.h yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h prefs.h default.h bindkey.h \
 /usr/include/string.h
ymenuitem.o: ymenuitem.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h ymenuitem.h \
 ymenu.h ypopup.h ywindow.h ypaint.h ycursor.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h yaction.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h prefs.h default.h bindkey.h \
 /usr/include/string.h
ymsgbox.o: ymsgbox.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h ymsgbox.h \
 ydialog.h wmclient.h ywindow.h ypaint.h ycursor.h ymenu.h ypopup.h \
 ytimer.h /usr/include/sys/time.h /usr/include/bits/time.h MwmUtil.h \
 /usr/include/X11/Xmd.h ylabel.h yactionbutton.h ybutton.h yaction.h \
 WinMgr.h yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h wmframe.h wmbutton.h wmoption.h \
 /usr/include/X11/Xproto.h /usr/include/X11/Xprotostr.h prefs.h \
 default.h bindkey.h wmmgr.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 intl.h /usr/include/libintl.h
ypaint.o: ypaint.cc config.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h ypaint.h \
 yapp.h /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ywindow.h ycursor.h sysdep.h \
 /usr/include/assert.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/time.h \
 /usr/include/bits/time.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 prefs.h default.h bindkey.h intl.h /usr/include/libintl.h
ypopup.o: ypopup.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h ymenu.h \
 ypopup.h ywindow.h ypaint.h ycursor.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h prefs.h default.h bindkey.h
yscrollbar.o: yscrollbar.cc config.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h yscrollbar.h \
 ywindow.h ypaint.h ycursor.h ytimer.h /usr/include/sys/time.h \
 /usr/include/bits/time.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h prefs.h \
 default.h bindkey.h
yscrollview.o: yscrollview.cc config.h yscrollview.h ywindow.h \
 ypaint.h /usr/include/X11/X.h ycursor.h ylistbox.h yscrollbar.h \
 ytimer.h base.h debug.h /usr/include/sys/time.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/time.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/bits/time.h /usr/include/bits/types.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h prefs.h \
 default.h bindkey.h
ysocket.o: ysocket.cc config.h base.h debug.h ysocket.h \
 /usr/include/sys/types.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/netinet/in.h /usr/include/limits.h \
 /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h \
 /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/stdint.h /usr/include/bits/wordsize.h \
 /usr/include/bits/socket.h /usr/include/bits/sockaddr.h \
 /usr/include/asm/socket.h /usr/include/asm/sockios.h \
 /usr/include/bits/in.h /usr/include/bits/byteswap.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h ywindow.h ypaint.h /usr/include/X11/X.h \
 ycursor.h /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/errno.h /usr/include/bits/errno.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h \
 /usr/include/sys/socket.h /usr/include/fcntl.h \
 /usr/include/bits/fcntl.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h
ytimer.o: ytimer.cc config.h ytimer.h base.h debug.h \
 /usr/include/sys/time.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/time.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/bits/time.h /usr/include/bits/types.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h yapp.h /usr/include/signal.h \
 /usr/include/bits/signum.h /usr/include/bits/siginfo.h \
 /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h \
 /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h ywindow.h \
 ypaint.h /usr/include/X11/X.h ycursor.h
ytooltip.o: ytooltip.cc config.h ytooltip.h ywindow.h ypaint.h \
 /usr/include/X11/X.h ycursor.h ytimer.h base.h debug.h \
 /usr/include/sys/time.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h /usr/include/time.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/bits/time.h /usr/include/bits/types.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h prefs.h default.h bindkey.h \
 /usr/include/string.h
ywindow.o: ywindow.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 /usr/include/X11/X.h /usr/include/X11/Xlib.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h \
 /usr/include/X11/keysym.h /usr/include/X11/keysymdef.h \
 /usr/include/X11/Xutil.h /usr/include/X11/Xproto.h \
 /usr/include/X11/Xmd.h /usr/include/X11/Xprotostr.h \
 /usr/include/X11/Xatom.h /usr/include/X11/Xresource.h \
 /usr/include/X11/cursorfont.h /usr/include/X11/extensions/shape.h \
 ywindow.h ypaint.h ycursor.h ytooltip.h ytimer.h \
 /usr/include/sys/time.h /usr/include/bits/time.h yapp.h \
 /usr/include/signal.h /usr/include/bits/signum.h \
 /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h \
 /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h \
 /usr/include/bits/sigstack.h sysdep.h /usr/include/assert.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/string.h \
 /usr/include/stdlib.h /usr/include/alloca.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/sys/wait.h \
 /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h \
 /usr/include/limits.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h \
 /usr/lib/gcc-lib/i586-pc-linux-gnu/2.95.2/include/limits.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 prefs.h default.h bindkey.h

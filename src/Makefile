include ../VERSION
-include ../install.inc
-include ../sysdep.inc

CXXFLAGS    = \
	$(I18N) $(DEBUG) $(OS2)\
	-DLIBDIR='"$(LIBDIR)"' \
	-DCONFIGDIR='"/etc/X11/icewm"' \
	-DVERSION='"$(VERSION)"' \
	-DEXEEXT='"$(EXEEXT)"' \
        $(SYS_CFLAGS) $(SYS_INCDIRS)
LFLAGS      = 
LIBS        = $(SYS_LIBDIRS) $(SYS_LIBS)

#CCM_OBJ=/usr/local/lib/ccmalloc.o
#CCM_DL=-lccmalloc -ldl

#CCM_DL=-lefence

TOP=$(shell pwd)

include base/base.mk
include wm/wm.mk
include dock/dock.mk
include root/root.mk
include utils/utils.mk

TARGETS=libbase.a \
	icewm icedock \
        iceview icelist icehelp iceicon \
        icesame \
        iceclock iceabout icesysdlg iceroot

.cc.o:
	(cd $(dir $<) && $(CXX) -o $@ -I$(TOP)/include $(CXXFLAGS) -c $<)

all: $(TARGETS)

libbase.a: $(BASE_OBJS)
	ar cr $@ $(BASE_OBJS)

icewm: $(WM_OBJS) libbase.a
	$(LD) -o icewm $(LFLAGS) $(WM_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

icedock: $(DOCK_OBJS) libbase.a
	$(LD) -o icedock $(LFLAGS) $(DOCK_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

iceroot: $(ROOT_OBJS) libbase.a
	$(LD) -o iceroot $(LFLAGS) $(ROOT_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

icesame: $(SAME_OBJS) libbase.a
	$(LD) -o icesame $(LFLAGS) $(SAME_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)
        
iceview: $(VIEW_OBJS) libbase.a
	$(LD) -o iceview $(LFLAGS) $(VIEW_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

icelist: $(LIST_OBJS) libbase.a
	$(LD) -o icelist $(LFLAGS) $(LIST_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

icehelp: $(HELP_OBJS) libbase.a
	$(LD) -o icehelp $(LFLAGS) $(HELP_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

iceicon: $(ICON_OBJS) libbase.a
	$(LD) -o iceicon $(LFLAGS) $(ICON_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

iceclock: $(CLOCK_OBJS) libbase.a
	$(LD) -o iceclock $(LFLAGS) $(CLOCK_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

iceabout: $(ABOUT_OBJS) libbase.a
	$(LD) -o iceabout $(LFLAGS) $(ABOUT_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

icesysdlg: $(SYSDLG_OBJS) libbase.a
	$(LD) -o icesysdlg $(LFLAGS) $(SYSDLG_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

clean:
	find . -name "*.o" -exec rm -f {} \;	
	rm -f $(TARGETS)

depend:

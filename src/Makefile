include ../VERSION
-include ../install.inc
-include ../sysdep.inc

.SUFFIXES:
.SUFFIXES: .cc .o .d

#PROFILER=-finstrument-functions
#CCM_DL=-lprofiler

CXXFLAGS    = $(PROFILER) \
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

TARGETS=libbase.a \
	icewm icedock \
        iceview icelist icehelp iceicon \
        icesame \
        iceclock iceabout icesysdlg iceroot \
        dndtest nop

all: $(TARGETS)

.cc.o:
	@echo ---  $<:
	@(cd $(dir $<) && $(CXX) -o $@ -I$(TOP)/include $(CXXFLAGS) -c $<)

.cc.d:
	@(cd $(dir $<) && echo $@ $(dir $<) | tr -d '\n' && gcc -MM -I$(TOP)/include $< ) >$@  || rm -vf $@

include base/base.mk
include wm/wm.mk
include dock/dock.mk
include root/root.mk
include utils/utils.mk


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

dndtest: $(DND_OBJS) libbase.a
	$(LD) -o dndtest $(LFLAGS) $(DND_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

nop: $(NOP_OBJS) libbase.a
	$(LD) -o nop $(LFLAGS) $(NOP_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

wmmark: $(WMMARK_OBJS) libbase.a
	$(LD) -o wmmark $(LFLAGS) $(WMMARK_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

clean:
	rm -f $(TARGETS)
	find . -name "*.o" -exec rm -f {} \;	

depclean: clean
	find . -name "*.d" -exec rm -f {} \;	

depend:

#depend:
#	>.depend
#	for a in `find . -type d` ; do for b in `cd $$a ; find . -name '*.cc'` ; do gcc -o$$a/$${b%o} -MM -MG -I../include $$a/$$b ; done ; done >>.depend

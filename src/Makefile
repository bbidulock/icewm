include ../VERSION
-include ../install.inc
-include ../sysdep.inc

PNAME=icewm-cvs

.SUFFIXES:
.SUFFIXES: .cc .o .d

#PROFILER=-finstrument-functions
#CCM_DL=-lprofiler

#CCM_OBJ=/usr/local/lib/ccmalloc.o
#CCM_DL=-lccmalloc -ldl

#CCM_DL=-lefence

CXXFLAGS    = $(PROFILER) \
	$(I18N) $(DEBUG) $(OS2)\
        -DPNAME='"$(PNAME)"' \
	-DVERSION='"$(VERSION)"' \
	-DLIBDIR='"$(LIBDIR)"' \
	-DCONFIGDIR='"/etc/X11/$(PNAME)"' \
	-DEXEEXT='"$(EXEEXT)"' \
        $(SYS_CFLAGS) $(SYS_INCDIRS)
LFLAGS      = 
LIBS        = $(SYS_LIBDIRS) $(SYS_LIBS)

TOP=$(shell pwd)

TARGETS=libbase.a \
	$(PNAME) \
        $(PNAME)-hint \
        $(PNAME)-bg \
        $(PNAME)-root \
        $(PNAME)-about \
        $(PNAME)-help \
        icedock \
        iceview icelist iceicon \
        icesame \
        iceclock icesysdlg

all: $(TARGETS)

tests: dndtest nop wmmark

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

$(PNAME): $(WM_OBJS) libbase.a
	$(LD) -o $(PNAME) $(LFLAGS) $(WM_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

$(PNAME)-hint: $(HINT_OBJS) libbase.a
	$(LD) -o $(PNAME)-hint $(LFLAGS) $(HINT_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

$(PNAME)-bg: $(BG_OBJS) libbase.a
	$(LD) -o $(PNAME)-bg $(LFLAGS) $(BG_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

$(PNAME)-root: $(ROOT_OBJS) libbase.a
	$(LD) -o $(PNAME)-root $(LFLAGS) $(ROOT_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

$(PNAME)-about: $(ABOUT_OBJS) libbase.a
	$(LD) -o $(PNAME)-about $(LFLAGS) $(ABOUT_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

$(PNAME)-help: $(HELP_OBJS) libbase.a
	$(LD) -o $(PNAME)-help $(LFLAGS) $(HELP_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

icesysdlg: $(SYSDLG_OBJS) libbase.a
	$(LD) -o icesysdlg $(LFLAGS) $(SYSDLG_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

icedock: $(DOCK_OBJS) libbase.a
	$(LD) -o icedock $(LFLAGS) $(DOCK_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

icesame: $(SAME_OBJS) libbase.a
	$(LD) -o icesame $(LFLAGS) $(SAME_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)
        
iceview: $(VIEW_OBJS) libbase.a
	$(LD) -o iceview $(LFLAGS) $(VIEW_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

icelist: $(LIST_OBJS) libbase.a
	$(LD) -o icelist $(LFLAGS) $(LIST_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

iceicon: $(ICON_OBJS) libbase.a
	$(LD) -o iceicon $(LFLAGS) $(ICON_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

iceclock: $(CLOCK_OBJS) libbase.a
	$(LD) -o iceclock $(LFLAGS) $(CLOCK_OBJS) $(CCM_OBJ) -L. -lbase $(LIBS) $(CCM_DL)

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

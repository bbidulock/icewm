include ../VERSION
-include ../install.mk
-include ../sysdep.mk

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

PROGRAMS= \
	icewm \
        icewm_hint \
        icewm_bg \
        icewm_root \
        icewm_about \
        icewm_help \
	icewm_dock \
        icewm_sysdlg \
        icewm_sound \
        iceview \
        iceicon \
        icelist \
        icesame \
        test_clock \
	test_dnd \
        test_nop \
        test_wmmark \
        test_skt
#        icewm_menu_kde \
#        icewm_menu_gnome
        
TARGETS=$(PROGRAMS) libbase.a
        
all: $(TARGETS)

include base/base.mk
include wm/wm.mk
include dock/dock.mk
include root/root.mk
include utils/utils.mk

libbase.a: $(BASE_OBJS)
	@echo A $@...
	@ar cr $@ $(BASE_OBJS)

$(PROGRAMS):
	@echo L $@...
	@$(LD) -o $@ $($@_OBJS) $(LFLAGS) $($@_LFLAGS) $($@_ALIBS) $(LIBS)

.cc.o:
	@echo C $<...
	@(cd $(dir $<) && $(CXX) -o $@ -I$(TOP)/include $(CXXFLAGS) -c $<)

.cc.d:
	@(cd $(dir $<) && echo $@ $(dir $<) | tr -d '\n' && gcc -MM -I$(TOP)/include $< ) >$@  || rm -vf $@

clean:
	rm -f $(TARGETS)
	find . -name "*.o" -exec rm -f {} \;

depend:

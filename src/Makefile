include ../VERSION
-include ../install.mk
-include ../sysdep.mk

.SUFFIXES:
.SUFFIXES: .cc .o .d

#PROFILER	:= -finstrument-functions
#CCM_DL		:= -lprofiler

#CCM_OBJ	:= /usr/local/lib/ccmalloc.o
#CCM_DL		:= -lccmalloc -ldl
CCM_DL = -lmcheck

GCCFL = -mpreferred-stack-boundary=2

CXXFLAGS    := $(GCCFL) -I/usr/local/include -I/usr/include/glib-1.2 -I/usr/include/gtk-1.2 \
        $(SYS_CXXFLAGS) \
        $(SYS_INCDIRS) \
	$(PROFILER) \
        -DPNAME='"icewm"' \
	-DVERSION='"$(VERSION)"' \
	-DDATADIR='"$(DATADIR)"' \
	-DCONFDIR='"$(CONFDIR)"' \
	-DEXEEXT='"$(EXEEXT)"'

LFLAGS      := 

LIBS        := $(SYS_LIBDIRS) $(SYS_LIBS)

TOP	    := $(shell pwd)

PROGRAMS    := \
	icewm \
        icewm_hint \
        icewm_bg \
        icewm_root \
        icewm_about \
        icewm_help \
	icewm_dock \
        icewm_sysdlg \
        iceview \
        iceicon \
        icelist \
        icesame \
        test_clock \
        test_cpu \
	test_dnd \
        test_nop \
        test_wmmark \
        test_skt \
        test_gravity \
        test_mwm_hints
#        icewm_menu_kde \
#        icewm_menu_gnome
#        icewm_sound \
        
TARGETS     := $(PROGRAMS) libbase.a
        
all: $(TARGETS)

DEPS	:=

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
	@$(LD) -o $@ $($@_OBJS) $(CCM_OBJ) $(LFLAGS) $($@_LFLAGS) $($@_ALIBS) $(LIBS) $(CCM_DL)

.cc.o:
	@echo C $<...
	@(cd $(dir $<) && $(CXX) -o $@ -I$(TOP)/include $(CXXFLAGS) -c $< )

clean:
	rm -f $(TARGETS)
	find . -name "*.o" -exec rm -f {} \;

AUTODEP=0
ifeq ($(AUTODEP),1)

-include $(DEPS)

.cc.d:
	@(cd $(dir $<) && echo $@ $(dir $<) | tr -d '\n' && gcc -MM -I$(TOP)/include $< ) >$@ 2>/dev/null || rm -vf $@
endif

depend:

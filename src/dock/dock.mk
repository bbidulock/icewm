icewm_dock_TOP	:= $(TOP)/dock

icewm_dock_SRCS := \
	$(icewm_dock_TOP)/wmtaskbar.cc \
	$(icewm_dock_TOP)/aapm.cc \
	$(icewm_dock_TOP)/aclock.cc \
	$(icewm_dock_TOP)/aaddressbar.cc \
	$(icewm_dock_TOP)/acpustatus.cc \
	$(icewm_dock_TOP)/amailbox.cc \
	$(icewm_dock_TOP)/apppstatus.cc \
	$(icewm_dock_TOP)/aworkspaces.cc \
	$(icewm_dock_TOP)/browse.cc \
	$(icewm_dock_TOP)/gnome.cc \
	$(icewm_dock_TOP)/objbar.cc \
	$(icewm_dock_TOP)/themes.cc \
	$(icewm_dock_TOP)/wmprog.cc \
	$(icewm_dock_TOP)/wmwinlist.cc \
	$(icewm_dock_TOP)/wmwinmenu.cc \
        $(icewm_dock_TOP)/icedock.cc \
        $(icewm_dock_TOP)/dockaction.cc \
	$(icewm_dock_TOP)/atasks.cc \
        $(icewm_dock_TOP)/wmdesktop.cc

icewm_dock_OBJS	:= $(icewm_dock_SRCS:.cc=.o)

icewm_dock_DEPS	:= $(icewm_dock_SRCS:.cc=.d)

icewm_dock_ALIBS := libbase.a

icewm_dock: $(icewm_dock_OBJS) $(icewm_dock_ALIBS)

-include $(icewm_dock_DEPS)

#######################################################################
test_clock_TOP	:= $(TOP)/dock

test_clock_SRCS := \
	$(test_clock_TOP)/iceclock.cc \
        $(test_clock_TOP)/aclock.cc
        
test_clock_OBJS := $(test_clock_SRCS:.cc=.o)

test_clock_DEPS := $(test_clock_SRCS:.cc=.d)

test_clock_ALIBS := libbase.a

test_clock: $(test_clock_OBJS) $(test_clock_ALIBS)

-include $(test_clock_DEPS)


#######################################################################
icewm_about_TOP := $(TOP)/dock

icewm_about_SRCS := \
	$(icewm_about_TOP)/wmabout.cc
        
icewm_about_OBJS := $(icewm_about_SRCS:.cc=.o)

icewm_about_DEPS := $(icewm_about_SRCS:.cc=.d)

icewm_about_ALIBS := libbase.a

icewm_about: $(icewm_about_OBJS) $(icewm_about_ALIBS)

-include $(icewm_about_DEPS)

#######################################################################
icewm_sysdlg_TOP := $(TOP)/dock

icewm_sysdlg_SRCS := \
	$(icewm_sysdlg_TOP)/wmdialog.cc
        
icewm_sysdlg_OBJS := $(icewm_sysdlg_SRCS:.cc=.o)

icewm_sysdlg_DEPS := $(icewm_sysdlg_SRCS:.cc=.d)

icewm_sysdlg_ALIBS := libbase.a

icewm_sysdlg: $(icewm_sysdlg_OBJS) $(icewm_sysdlg_ALIBS)

-include $(icewm_sysdlg_DEPS)

#######################################################################
icewm_root_TOP := $(TOP)/dock

icewm_root_SRCS := \
	$(icewm_root_TOP)/iceroot.cc \
        $(icewm_root_TOP)/wmprog.cc \
        $(icewm_root_TOP)/browse.cc \
        $(icewm_root_TOP)/dockaction.cc \
        $(icewm_root_TOP)/rootmenu.cc

icewm_root_OBJS := $(icewm_root_SRCS:.cc=.o)

icewm_root_DEPS := $(icewm_root_SRCS:.cc=.d)

icewm_root_ALIBS := libbase.a

icewm_root: $(icewm_root_OBJS) $(icewm_root_ALIBS)

-include $(icewm_root_DEPS)

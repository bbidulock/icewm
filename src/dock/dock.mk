DOCK_TOP=$(TOP)/dock

DOCK_SRCS= \
	$(DOCK_TOP)/wmtaskbar.cc \
	$(DOCK_TOP)/aapm.cc \
	$(DOCK_TOP)/aclock.cc \
	$(DOCK_TOP)/aaddressbar.cc \
	$(DOCK_TOP)/acpustatus.cc \
	$(DOCK_TOP)/amailbox.cc \
	$(DOCK_TOP)/apppstatus.cc \
	$(DOCK_TOP)/aworkspaces.cc \
	$(DOCK_TOP)/browse.cc \
	$(DOCK_TOP)/gnome.cc \
	$(DOCK_TOP)/objbar.cc \
	$(DOCK_TOP)/themes.cc \
	$(DOCK_TOP)/wmprog.cc \
	$(DOCK_TOP)/wmwinlist.cc \
	$(DOCK_TOP)/wmwinmenu.cc \
        $(DOCK_TOP)/icedock.cc \
        $(DOCK_TOP)/dockaction.cc \
	$(DOCK_TOP)/atasks.cc \
        $(DOCK_TOP)/wmdesktop.cc

DOCK_OBJS=$(DOCK_SRCS:.cc=.o)

DOCK_DEPS=$(DOCK_SRCS:.cc=.d)

-include $(DOCK_DEPS)


CLOCK_TOP=$(TOP)/dock

CLOCK_SRCS= \
	$(CLOCK_TOP)/iceclock.cc \
        $(CLOCK_TOP)/aclock.cc
        
CLOCK_OBJS=$(CLOCK_SRCS:.cc=.o)

CLOCK_DEPS=$(CLOCK_SRCS:.cc=.d)

-include $(CLOCK_DEPS)


ABOUT_TOP=$(TOP)/dock

ABOUT_SRCS= \
	$(ABOUT_TOP)/wmabout.cc
        
ABOUT_OBJS=$(ABOUT_SRCS:.cc=.o)

ABOUT_DEPS=$(ABOUT_SRCS:.cc=.d)

-include $(ABOUT_DEPS)

SYSDLG_TOP=$(TOP)/dock

SYSDLG_SRCS= \
	$(SYSDLG_TOP)/wmdialog.cc
        
SYSDLG_OBJS=$(SYSDLG_SRCS:.cc=.o)

SYSDLG_DEPS=$(SYSDLG_SRCS:.cc=.d)

-include $(SYSDLG_DEPS)

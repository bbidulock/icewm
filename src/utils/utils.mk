SAME_TOP=$(TOP)/utils

SAME_SRCS= \
	$(SAME_TOP)/icesame.cc
        
SAME_OBJS=$(SAME_SRCS:.cc=.o)

SAME_DEPS=$(SAME_SRCS:.cc=.d)

-include $(SAME_DEPS)

VIEW_TOP=$(TOP)/utils

VIEW_SRCS= \
	$(VIEW_TOP)/iceview.cc
        
VIEW_OBJS=$(VIEW_SRCS:.cc=.o)

VIEW_DEPS=$(VIEW_SRCS:.cc=.d)

-include $(VIEW_DEPS)

LIST_TOP=$(TOP)/utils

LIST_SRCS= \
	$(LIST_TOP)/icelist.cc
        
LIST_OBJS=$(LIST_SRCS:.cc=.o)

LIST_DEPS=$(LIST_SRCS:.cc=.d)

-include $(LIST_DEPS)

HELP_TOP=$(TOP)/utils

HELP_SRCS= \
	$(HELP_TOP)/icehelp.cc
        
HELP_OBJS=$(HELP_SRCS:.cc=.o)

HELP_DEPS=$(HELP_SRCS:.cc=.d)

-include $(HELP_DEPS)

ICON_TOP=$(TOP)/utils

ICON_SRCS= \
	$(ICON_TOP)/iceicon.cc
        
ICON_OBJS=$(ICON_SRCS:.cc=.o)

ICON_DEPS=$(ICON_SRCS:.cc=.d)

-include $(ICON_DEPS)

DND_TOP=$(TOP)/utils

DND_SRCS= \
	$(DND_TOP)/dndtest.cc
        
DND_OBJS=$(DND_SRCS:.cc=.o)

DND_DEPS=$(DND_SRCS:.cc=.d)

-include $(DND_DEPS)

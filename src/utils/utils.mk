SAME_TOP=$(TOP)/utils

SAME_SRCS= \
	$(SAME_TOP)/icesame.cc
        
SAME_OBJS=$(SAME_SRCS:.cc=.o)

VIEW_TOP=$(TOP)/utils

VIEW_SRCS= \
	$(VIEW_TOP)/iceview.cc
        
VIEW_OBJS=$(VIEW_SRCS:.cc=.o)

LIST_TOP=$(TOP)/utils

LIST_SRCS= \
	$(LIST_TOP)/icelist.cc
        
LIST_OBJS=$(LIST_SRCS:.cc=.o)

HELP_TOP=$(TOP)/utils

HELP_SRCS= \
	$(HELP_TOP)/icehelp.cc
        
HELP_OBJS=$(HELP_SRCS:.cc=.o)

ICON_TOP=$(TOP)/utils

ICON_SRCS= \
	$(ICON_TOP)/iceicon.cc
        
ICON_OBJS=$(ICON_SRCS:.cc=.o)

ROOT_TOP=$(TOP)/root

ROOT_SRCS= \
	$(ROOT_TOP)/iceroot.cc \
        $(ROOT_TOP)/rootmenu.cc \

ROOT_OBJS=$(ROOT_SRCS:.cc=.o)

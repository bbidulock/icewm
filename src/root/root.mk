ROOT_TOP=$(TOP)/root

ROOT_SRCS= \
	$(ROOT_TOP)/iceroot.cc \
        $(ROOT_TOP)/rootmenu.cc \

ROOT_OBJS=$(ROOT_SRCS:.cc=.o)

ROOT_DEPS=$(ROOT_SRCS:.cc=.d)

-include $(ROOT_DEPS)

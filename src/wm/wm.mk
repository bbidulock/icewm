icewm_TOP	:= $(TOP)/wm

icewm_SRCS	:= \
	$(icewm_TOP)/wmapp.cc \
        $(icewm_TOP)/wmmgr.cc \
	$(icewm_TOP)/wmframe.cc \
        $(icewm_TOP)/movesize.cc \
        $(icewm_TOP)/decorate.cc \
        $(icewm_TOP)/wmtitle.cc \
        $(icewm_TOP)/wmbutton.cc \
        $(icewm_TOP)/wmcontainer.cc \
        $(icewm_TOP)/wmclient.cc \
        $(icewm_TOP)/wmminiicon.cc \
        $(icewm_TOP)/wmoption.cc \
        $(icewm_TOP)/wmstatus.cc \
        $(icewm_TOP)/wmswitch.cc \
        $(icewm_TOP)/wmaction.cc \
        $(icewm_TOP)/wmconfig.cc \
        $(icewm_TOP)/wmsession.cc
        

icewm_OBJS	:= $(icewm_SRCS:.cc=.o)

icewm_DEPS      := $(icewm_SRCS:.cc=.d)

icewm_ALIBS	:= libbase.a

icewm: $(icewm_OBJS) $(icewm_ALIBS)

-include $(icewm_DEPS)

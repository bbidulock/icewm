WM_TOP=$(TOP)/wm

WM_SRCS= \
	$(WM_TOP)/wmapp.cc \
        $(WM_TOP)/wmmgr.cc \
	$(WM_TOP)/wmframe.cc \
        $(WM_TOP)/movesize.cc \
        $(WM_TOP)/decorate.cc \
        $(WM_TOP)/wmtitle.cc \
        $(WM_TOP)/wmbutton.cc \
        $(WM_TOP)/wmcontainer.cc \
        $(WM_TOP)/wmclient.cc \
        $(WM_TOP)/wmminiicon.cc \
        $(WM_TOP)/wmoption.cc \
        $(WM_TOP)/wmstatus.cc \
        $(WM_TOP)/wmswitch.cc \
        $(WM_TOP)/wmaction.cc \
        $(WM_TOP)/wmconfig.cc \
        $(WM_TOP)/wmsession.cc
        

WM_OBJS=$(WM_SRCS:.cc=.o)

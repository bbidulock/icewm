BASE_TOP=$(TOP)/base

BASE_SRCS= \
	$(BASE_TOP)/yapp.cc $(BASE_TOP)/ytimer.cc \
        $(BASE_TOP)/ywindow.cc \
        $(BASE_TOP)/ypaint.cc \
        $(BASE_TOP)/ypopup.cc \
        $(BASE_TOP)/ysocket.cc \
        $(BASE_TOP)/yscrollview.cc \
        $(BASE_TOP)/yscrollbar.cc \
        $(BASE_TOP)/ymenu.cc \
        $(BASE_TOP)/ymenuitem.cc \
        $(BASE_TOP)/ymsgbox.cc \
        $(BASE_TOP)/ylistbox.cc \
        $(BASE_TOP)/ylabel.cc \
        $(BASE_TOP)/ybutton.cc \
        $(BASE_TOP)/ydialog.cc \
        $(BASE_TOP)/yinput.cc \
        $(BASE_TOP)/ytooltip.cc \
        $(BASE_TOP)/yimage.cc \
        $(BASE_TOP)/yimageio.cc \
        $(BASE_TOP)/yicon.cc \
        $(BASE_TOP)/yiconcache.cc \
        $(BASE_TOP)/yresource.cc \
        $(BASE_TOP)/ytopwindow.cc \
        $(BASE_TOP)/misc.cc

BASE_OBJS=$(BASE_SRCS:.cc=.o)

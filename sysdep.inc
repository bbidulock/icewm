CXX = gcc
LD = gcc

SYS_CFLAGS = \
	 -fpermissive -Wall -Wpointer-arith -Wconversion -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Winline -Woverloaded-virtual -W -fno-exceptions -fno-rtti -pipe -march=i686 -O6 -DHAVE_CONFIG_H \
	 -I/usr/X11R6/include   
SYS_INCDIRS =
SYS_LIBDIRS =
SYS_LIBS = 

CORE_LIBS =  -lSM -lICE  -lX11  -L/usr/X11R6/lib  -lXext
IMAGE_LIBS =  -lXpm
AUDIO_LIBS = 
GNOME_LIBS = 

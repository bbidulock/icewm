CXX = g++ # -ffunction-sections #-fdata-sections
LD = gcc #-Wl,--gc-sections
SYS_CFLAGS =  -Wall -Wpointer-arith -Wconversion -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Winline -Woverloaded-virtual -W -fno-exceptions -fno-rtti -g -O2 -I/usr/X11R6/include  -DHAVE_DIRENT_H=1 -DHAVE_SYS_WAIT_H=1 -DHAVE_FCNTL_H=1 -DHAVE_LIMITS_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_IOCTL_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_UNISTD_H=1 -DSTDC_HEADERS=1 -DTIME_WITH_SYS_TIME=1 -DRETSIGTYPE=void -DHAVE_STRFTIME=1 -DHAVE_VPRINTF=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_PUTENV=1 -DHAVE_SELECT=1 -DHAVE_SOCKET=1 -DHAVE_STRTOL=1 -DHAVE_STRTOUL=1 -DSELECT_TYPE_ARG1=int -DSELECT_TYPE_ARG234=\(fd_set\ \*\) -DSELECT_TYPE_ARG5=\(struct\ timeval\ \*\) -DHAVE_XINTERNATOMS=1 -DSM=1 -DSHAPE=1 -DXPM=1 
SYS_INCDIRS =
SYS_LIBDIRS =
SYS_LIBS =   -L/usr/X11R6/lib -lXext  -lXpm  -lSM -lICE -lX11  

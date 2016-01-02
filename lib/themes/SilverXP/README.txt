--------------------------------------------------------------

                 SilverXP Theme

                for IceWM-1.2.17

            with single height taskbar

               INSTALLATION NOTES

--------------------------------------------------------------


In order to install the theme perform the following actions:



 - first of all - backup your ~/.icewm folder:

   cd && cp -rp .icewm .icewm-backup



 - install Microsoft's TrueType fonts (x11-fonts/webfonts in FreeBSD).
   We need the font'verdana'. You can download the fonts from
   http://corefonts.sf.net



 - extract the archive SilverXP-1.2.17-single-1.tar.bz2 - it
   will create the icewm/ directory

   cd && tar jxvf SilverXP-1.2.17-single-1.tar.bz2


 - hide the directory (do not forget to back up Your ~/.icewm
   before doing it):

   cd && mv icewm/ .icewm/



 - move Your customized files "keys, winoptions, toolbar, menu"
   to the ~/.icewm directory:

   cd ~/.icewm-backup/ && cp keys winoptions toolbar menu ~/.icewm



 - Patch IceWM-1.2.17 to  enable borderless buttons and
   recompile IceWM (this also will enable the support
   of shaped windows and gradients):


   FreeBSD:

   su
   cd /usr/ports/x11-wm/icewm/
   cp ~/.icewm/themes/SilverXP-1.2.17-single-1/FreeBSD/patch-src_ybutton.cc files/
   patch < ~/.icewm/themes/SilverXP-1.2.17-single-1/FreeBSD/Makefile.patch
   make clean reinstall


   Linux:

   tar zxvf icewm-1.2.17.tar.zg
   cd icewm-1.2.17/src
   patch < ~/.icewm/themes/SilverXP-1.2.17-single-1/Linux/ybutton.cc.patch
   cd ..
   ./configure --prefix=/usr/X11R6 --exec-prefix=/usr/X11R6 \
               --with-imlib --without-xpm \
               --enable-gradients --enable-shaped-decorations \
               --enable-xfreetype --disable-corefonts
    make
    su
    make install



  - enjoy the best theme for the best window manager ;-)


-----------------------------------------------------------------------
Theme Home Page - http://icewmsilverxp.sourceforge.net
Theme Author - Alexander Portnoy   http://sourceforge.net/users/alexpor/
-----------------------------------------------------------------------



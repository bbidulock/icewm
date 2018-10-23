#ifndef __WPIXMAPS_H
#define __WPIXMAPS_H

extern ref<YPixmap> closePixmap[3];
extern ref<YPixmap> minimizePixmap[3];
extern ref<YPixmap> maximizePixmap[3];
extern ref<YPixmap> restorePixmap[3];
extern ref<YPixmap> hidePixmap[3];
extern ref<YPixmap> rollupPixmap[3];
extern ref<YPixmap> rolldownPixmap[3];
extern ref<YPixmap> depthPixmap[3];

extern ref<YPixmap> buttonIPixmap;
extern ref<YPixmap> buttonAPixmap;

extern ref<YPixmap> logoutPixmap;
extern ref<YPixmap> switchbackPixmap;
extern ref<YPixmap> listbackPixmap;
extern ref<YPixmap> dialogbackPixmap;

extern ref<YPixmap> frameTL[2][2];
extern ref<YPixmap> frameT[2][2];
extern ref<YPixmap> frameTR[2][2];
extern ref<YPixmap> frameL[2][2];
extern ref<YPixmap> frameR[2][2];
extern ref<YPixmap> frameBL[2][2];
extern ref<YPixmap> frameB[2][2];
extern ref<YPixmap> frameBR[2][2];

extern ref<YPixmap> menuButton[3];

extern ref<YPixmap> taskbuttonPixmap;
extern ref<YPixmap> taskbuttonactivePixmap;
extern ref<YPixmap> taskbuttonminimizedPixmap;
extern ref<YPixmap> taskbuttonLeftPixmap;
extern ref<YPixmap> taskbuttonRightPixmap;
extern ref<YPixmap> taskbuttonactiveLeftPixmap;
extern ref<YPixmap> taskbuttonactiveRightPixmap;
extern ref<YPixmap> taskbuttonminimizedLeftPixmap;
extern ref<YPixmap> taskbuttonminimizedRightPixmap;

extern ref<YPixmap> titleJ[2]; // Frame <=> Left buttons
extern ref<YPixmap> titleL[2]; // Left buttons <=> Left pane
extern ref<YPixmap> titleS[2]; // Left pane
extern ref<YPixmap> titleP[2]; // Left pane <=> Title
extern ref<YPixmap> titleT[2]; // Title
extern ref<YPixmap> titleM[2]; // Title <=> Right pane
extern ref<YPixmap> titleB[2]; // Right pane
extern ref<YPixmap> titleR[2]; // Right pane <=> Right buttons
extern ref<YPixmap> titleQ[2]; // Right buttons <=> Frame

extern ref<YPixmap> taskbackPixmap;
extern ref<YPixmap> toolbuttonPixmap;
extern ref<YPixmap> workspacebuttonPixmap;
extern ref<YPixmap> workspacebuttonactivePixmap;

extern ref<YImage> buttonIPixbuf;
extern ref<YImage> buttonAPixbuf;

extern ref<YImage> logoutPixbuf;
extern ref<YImage> switchbackPixbuf;
extern ref<YImage> listbackPixbuf;
extern ref<YImage> dialogbackPixbuf;

extern ref<YImage> rgbFrameT[2][2];
extern ref<YImage> rgbFrameL[2][2];
extern ref<YImage> rgbFrameR[2][2];
extern ref<YImage> rgbFrameB[2][2];

extern ref<YImage> rgbTitleS[2];
extern ref<YImage> rgbTitleT[2];
extern ref<YImage> rgbTitleB[2];

extern ref<YImage> taskbackPixbuf;
extern ref<YImage> taskbuttonPixbuf;
extern ref<YImage> taskbuttonactivePixbuf;
extern ref<YImage> taskbuttonminimizedPixbuf;
extern ref<YImage> taskbuttonLeftPixbuf;
extern ref<YImage> taskbuttonRightPixbuf;
extern ref<YImage> taskbuttonactiveLeftPixbuf;
extern ref<YImage> taskbuttonactiveRightPixbuf;
extern ref<YImage> taskbuttonminimizedLeftPixbuf;
extern ref<YImage> taskbuttonminimizedRightPixbuf;

extern ref<YImage> toolbuttonPixbuf;

extern ref<YImage> workspacebuttonPixbuf;
extern ref<YImage> workspacebuttonactivePixbuf;

extern ref<YImage> taskbarStartImage;
extern ref<YImage> taskbarLinuxImage;   // deprecated
extern ref<YImage> taskbarWindowsImage;
extern ref<YImage> taskbarShowDesktopImage;
extern ref<YImage> taskbarCollapseImage;
extern ref<YImage> taskbarExpandImage;

extern ref<YPixmap> mailPixmap;
extern ref<YPixmap> noMailPixmap;
extern ref<YPixmap> errMailPixmap;
extern ref<YPixmap> unreadMailPixmap;
extern ref<YPixmap> newMailPixmap;

extern ref<YPixmap> ledPixNum[10];
extern ref<YPixmap> ledPixSpace;
extern ref<YPixmap> ledPixColon;
extern ref<YPixmap> ledPixSlash;
extern ref<YPixmap> ledPixA;
extern ref<YPixmap> ledPixP;
extern ref<YPixmap> ledPixM;
extern ref<YPixmap> ledPixDot;
extern ref<YPixmap> ledPixPercent;

#endif

// vim: set sw=4 ts=4 et:

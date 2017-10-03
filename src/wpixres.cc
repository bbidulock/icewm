#include "config.h"
#include "wpixres.h"
#include "udir.h"
#include "ypixmap.h"
#include "yimage.h"
#include "ref.h"
#include "ypaths.h"
#include "ymenu.h"

#define extern
#include "wpixmaps.h"
#undef extern

class ResourceFlag {
private:
    bool flag;
public:
    ResourceFlag(bool value = false) : flag(value) { }
    operator bool() const { return flag; }
    void set(bool value) { flag = value; }
};

class PixmapResource {
private:
    ref<YPixmap> *pixmapRef;
    ref<YImage>  *imageRef;
    const char   *filename;
    const char   *alternative;

protected:
    ResourceFlag  isGradient;

public:
    PixmapResource(ref<YPixmap>& p, ref<YImage>& i,
            const char *n, const char *a = 0) :
        pixmapRef(&p), imageRef(&i), filename(n), alternative(a)
    { }

    PixmapResource(ref<YPixmap>& p, const char *n, const char *a = 0) :
        pixmapRef(&p), imageRef(0), filename(n), alternative(a)
    { }

    PixmapResource(ref<YImage>& i, const char *n, const char *a = 0) :
        pixmapRef(0), imageRef(&i), filename(n), alternative(a)
    { }

    bool nameEqual(const char *str) const {
        return checkName(filename, str);
    }
    bool altEqual(const char *str) const {
        return alternative && str && checkName(alternative, str);
    }
    bool needPixmap() const { return pixmapRef != 0 && *pixmapRef == null; }
    bool needImage() const { return imageRef != 0 && *imageRef == null; }
    bool needLoad() const {
        return (pixmapRef != 0) ? *pixmapRef == null : needImage();
    }
    void loadFromFile(const upath& file) const;
    void reset() const {
        if (pixmapRef != 0) *pixmapRef = null;
        if (imageRef != 0) *imageRef = null;
    }
    const char* name() const { return filename; }
    const char* altn() const { return alternative; }

private:

    bool checkName(const char *wanted, const char *given) const {
        if (0 == strcmp(wanted, given)) {
#ifdef CONFIG_GRADIENTS
            if (isGradient) {
                extern const char* gradients;
                return gradients && strstr(gradients, wanted);
            }
#endif
            return true;
        }
        return false;
    }

};

void PixmapResource::loadFromFile(const upath& file) const
{
    if (needPixmap()) {
        *pixmapRef = YResourcePaths::loadPixmapFile(file);
    }
    if (needImage()) {
        *imageRef = YResourcePaths::loadImageFile(file);
    }
}

#ifdef CONFIG_GRADIENTS
class GradientResource : public PixmapResource {
public:
    GradientResource(ref<YImage>& i, const char *n, const char *a = 0) :
        PixmapResource(i, n, a)
    {
        isGradient.set(true);
    }
};
#endif

static const PixmapResource themePixRes[] = {
#ifdef CONFIG_GRADIENTS
    GradientResource(rgbTitleS[0], "titleIS.xpm"),
    GradientResource(rgbTitleT[0], "titleIT.xpm"),
    GradientResource(rgbTitleB[0], "titleIB.xpm"),
    GradientResource(rgbTitleS[1], "titleAS.xpm"),
    GradientResource(rgbTitleT[1], "titleAT.xpm"),
    GradientResource(rgbTitleB[1], "titleAB.xpm"),

    GradientResource(rgbFrameT[0][0], "frameIT.xpm"),
    GradientResource(rgbFrameL[0][0], "frameIL.xpm"),
    GradientResource(rgbFrameR[0][0], "frameIR.xpm"),
    GradientResource(rgbFrameB[0][0], "frameIB.xpm"),
    GradientResource(rgbFrameT[0][1], "frameAT.xpm"),
    GradientResource(rgbFrameL[0][1], "frameAL.xpm"),
    GradientResource(rgbFrameR[0][1], "frameAR.xpm"),
    GradientResource(rgbFrameB[0][1], "frameAB.xpm"),

    GradientResource(rgbFrameT[1][0], "dframeIT.xpm"),
    GradientResource(rgbFrameL[1][0], "dframeIL.xpm"),
    GradientResource(rgbFrameR[1][0], "dframeIR.xpm"),
    GradientResource(rgbFrameB[1][0], "dframeIB.xpm"),
    GradientResource(rgbFrameT[1][1], "dframeAT.xpm"),
    GradientResource(rgbFrameL[1][1], "dframeAL.xpm"),
    GradientResource(rgbFrameR[1][1], "dframeAR.xpm"),
    GradientResource(rgbFrameB[1][1], "dframeAB.xpm"),

    GradientResource(buttonIPixbuf, "buttonI.xpm"),
    GradientResource(buttonAPixbuf, "buttonA.xpm"),

    GradientResource(logoutPixbuf, "logoutbg.xpm"),
    GradientResource(switchbackPixbuf, "switchbg.xpm"),
#ifndef LITE
    GradientResource(listbackPixbuf, "listbg.xpm"),
#endif
    GradientResource(dialogbackPixbuf, "dialogbg.xpm"),

    GradientResource(menubackPixbuf, "menubg.xpm"),
    GradientResource(menuselPixbuf, "menusel.xpm"),
    GradientResource(menusepPixbuf, "menusep.xpm"),
#endif /*CONFIG_GRADIENTS*/

    PixmapResource(closePixmap[0], "closeI.xpm", "close.xpm"),
    PixmapResource(depthPixmap[0], "depthI.xpm", "depth.xpm"),
    PixmapResource(maximizePixmap[0], "maximizeI.xpm", "maximize.xpm"),
    PixmapResource(minimizePixmap[0], "minimizeI.xpm", "minimize.xpm"),
    PixmapResource(restorePixmap[0], "restoreI.xpm", "restore.xpm"),
    PixmapResource(hidePixmap[0], "hideI.xpm", "hide.xpm"),
    PixmapResource(rollupPixmap[0], "rollupI.xpm", "rollup.xpm"),
    PixmapResource(rolldownPixmap[0], "rolldownI.xpm", "rolldown.xpm"),

    PixmapResource(closePixmap[1], "closeA.xpm"),
    PixmapResource(depthPixmap[1], "depthA.xpm"),
    PixmapResource(maximizePixmap[1], "maximizeA.xpm"),
    PixmapResource(minimizePixmap[1], "minimizeA.xpm"),
    PixmapResource(restorePixmap[1], "restoreA.xpm"),
    PixmapResource(hidePixmap[1], "hideA.xpm"),
    PixmapResource(rollupPixmap[1], "rollupA.xpm"),
    PixmapResource(rolldownPixmap[1], "rolldownA.xpm"),

    PixmapResource(closePixmap[2], "closeO.xpm"),
    PixmapResource(depthPixmap[2], "depthO.xpm"),
    PixmapResource(maximizePixmap[2], "maximizeO.xpm"),
    PixmapResource(minimizePixmap[2], "minimizeO.xpm"),
    PixmapResource(restorePixmap[2], "restoreO.xpm"),
    PixmapResource(hidePixmap[2], "hideO.xpm"),
    PixmapResource(rollupPixmap[2], "rollupO.xpm"),
    PixmapResource(rolldownPixmap[2], "rolldownO.xpm"),

    PixmapResource(frameTL[0][0], "frameITL.xpm"),
    PixmapResource(frameTR[0][0], "frameITR.xpm"),
    PixmapResource(frameBL[0][0], "frameIBL.xpm"),
    PixmapResource(frameBR[0][0], "frameIBR.xpm"),

    PixmapResource(frameTL[0][1], "frameATL.xpm"),
    PixmapResource(frameTR[0][1], "frameATR.xpm"),
    PixmapResource(frameBL[0][1], "frameABL.xpm"),
    PixmapResource(frameBR[0][1], "frameABR.xpm"),

    PixmapResource(frameTL[1][0], "dframeITL.xpm"),
    PixmapResource(frameTR[1][0], "dframeITR.xpm"),
    PixmapResource(frameBL[1][0], "dframeIBL.xpm"),
    PixmapResource(frameBR[1][0], "dframeIBR.xpm"),

    PixmapResource(frameTL[1][1], "dframeATL.xpm"),
    PixmapResource(frameTR[1][1], "dframeATR.xpm"),
    PixmapResource(frameBL[1][1], "dframeABL.xpm"),
    PixmapResource(frameBR[1][1], "dframeABR.xpm"),

    PixmapResource(frameT[0][0], "frameIT.xpm"),
    PixmapResource(frameL[0][0], "frameIL.xpm"),
    PixmapResource(frameR[0][0], "frameIR.xpm"),
    PixmapResource(frameB[0][0], "frameIB.xpm"),

    PixmapResource(frameT[0][1], "frameAT.xpm"),
    PixmapResource(frameL[0][1], "frameAL.xpm"),
    PixmapResource(frameR[0][1], "frameAR.xpm"),
    PixmapResource(frameB[0][1], "frameAB.xpm"),

    PixmapResource(frameT[1][0], "dframeIT.xpm"),
    PixmapResource(frameL[1][0], "dframeIL.xpm"),
    PixmapResource(frameR[1][0], "dframeIR.xpm"),
    PixmapResource(frameB[1][0], "dframeIB.xpm"),

    PixmapResource(frameT[1][1], "dframeAT.xpm"),
    PixmapResource(frameL[1][1], "dframeAL.xpm"),
    PixmapResource(frameR[1][1], "dframeAR.xpm"),
    PixmapResource(frameB[1][1], "dframeAB.xpm"),

    PixmapResource(titleJ[0], "titleIJ.xpm"),
    PixmapResource(titleL[0], "titleIL.xpm"),
    PixmapResource(titleP[0], "titleIP.xpm"),
    PixmapResource(titleM[0], "titleIM.xpm"),
    PixmapResource(titleR[0], "titleIR.xpm"),
    PixmapResource(titleQ[0], "titleIQ.xpm"),

    PixmapResource(titleJ[1], "titleAJ.xpm"),
    PixmapResource(titleL[1], "titleAL.xpm"),
    PixmapResource(titleP[1], "titleAP.xpm"),
    PixmapResource(titleM[1], "titleAM.xpm"),
    PixmapResource(titleR[1], "titleAR.xpm"),
    PixmapResource(titleQ[1], "titleAQ.xpm"),

    PixmapResource(titleS[0], "titleIS.xpm"),
    PixmapResource(titleT[0], "titleIT.xpm"),
    PixmapResource(titleB[0], "titleIB.xpm"),

    PixmapResource(titleS[1], "titleAS.xpm"),
    PixmapResource(titleT[1], "titleAT.xpm"),
    PixmapResource(titleB[1], "titleAB.xpm"),

    PixmapResource(menuButton[0], "menuButtonI.xpm"),
    PixmapResource(menuButton[1], "menuButtonA.xpm"),
    PixmapResource(menuButton[2], "menuButtonO.xpm"),

    PixmapResource(logoutPixmap, "logoutbg.xpm"),
    PixmapResource(switchbackPixmap, "switchbg.xpm"),
    PixmapResource(menubackPixmap, "menubg.xpm"),
    PixmapResource(menuselPixmap, "menusel.xpm"),
    PixmapResource(menusepPixmap, "menusep.xpm"),

#ifndef LITE
    PixmapResource(listbackPixmap, "listbg.xpm", "menubg.xpm"),
#endif
    PixmapResource(dialogbackPixmap, "dialogbg.xpm", "menubg.xpm"),
};

static const PixmapResource taskbarPixRes[] = {
    PixmapResource(buttonIPixmap, "buttonI.xpm", "taskbuttonbg.xpm"),
    PixmapResource(buttonAPixmap, "buttonA.xpm", "taskbuttonactive.xpm"),

#ifdef CONFIG_TASKBAR
#ifdef CONFIG_GRADIENTS
    GradientResource(toolbuttonPixbuf, "toolbuttonbg.xpm", "buttonI.xpm"),
    GradientResource(workspacebuttonPixbuf, "workspacebuttonbg.xpm", "buttonI.xpm"),
    GradientResource(workspacebuttonactivePixbuf, "workspacebuttonactive.xpm", "buttonA.xpm"),
#endif

    PixmapResource(toolbuttonPixmap, "toolbuttonbg.xpm", "buttonI.xpm"),
    PixmapResource(workspacebuttonPixmap, "workspacebuttonbg.xpm", "buttonI.xpm"),
    PixmapResource(workspacebuttonactivePixmap, "workspacebuttonactive.xpm", "buttonA.xpm"),

    PixmapResource(taskbarStartImage, "start.xpm", "icewm.xpm"),

#ifdef CONFIG_GRADIENTS
    GradientResource(taskbackPixbuf, "taskbarbg.xpm"),
    GradientResource(taskbuttonPixbuf, "taskbuttonbg.xpm"),
    GradientResource(taskbuttonactivePixbuf, "taskbuttonactive.xpm"),
    GradientResource(taskbuttonminimizedPixbuf, "taskbuttonminimized.xpm"),
#endif
    PixmapResource(taskbackPixmap, "taskbarbg.xpm"),
    PixmapResource(taskbuttonPixmap, "taskbuttonbg.xpm"),
    PixmapResource(taskbuttonactivePixmap, "taskbuttonactive.xpm"),
    PixmapResource(taskbuttonminimizedPixmap, "taskbuttonminimized.xpm"),
#endif
};

static const PixmapResource taskbar2PixRes[] = {
#ifdef CONFIG_TASKBAR
#ifdef CONFIG_GRADIENTS
    GradientResource(taskbackPixbuf, "taskbarbg.xpm"),
    GradientResource(taskbuttonPixbuf, "taskbuttonbg.xpm"),
    GradientResource(taskbuttonactivePixbuf, "taskbuttonactive.xpm"),
    GradientResource(taskbuttonminimizedPixbuf, "taskbuttonminimized.xpm"),
#endif
    PixmapResource(taskbackPixmap, "taskbarbg.xpm"),
    PixmapResource(taskbuttonPixmap, "taskbuttonbg.xpm"),
    PixmapResource(taskbuttonactivePixmap, "taskbuttonactive.xpm"),
    PixmapResource(taskbuttonminimizedPixmap, "taskbuttonminimized.xpm"),

    PixmapResource(taskbarStartImage, "start.xpm", "icewm.xpm"),
    PixmapResource(taskbarLinuxImage, "linux.xpm"),     // deprecated
    PixmapResource(taskbarWindowsImage, "windows.xpm"),
    PixmapResource(taskbarShowDesktopImage, "desktop.xpm"),
    PixmapResource(taskbarCollapseImage, "collapse.xpm"),
    PixmapResource(taskbarExpandImage, "expand.xpm"),
#endif
};

static const PixmapResource mailboxPixRes[] = {
#ifdef CONFIG_APPLET_MAILBOX
    PixmapResource(mailPixmap, "mail.xpm"),
    PixmapResource(noMailPixmap, "nomail.xpm"),
    PixmapResource(errMailPixmap, "errmail.xpm"),
    PixmapResource(unreadMailPixmap, "unreadmail.xpm"),
    PixmapResource(newMailPixmap, "newmail.xpm"),
#endif
};

static const PixmapResource ledclockPixRes[] = {
#ifdef CONFIG_APPLET_CLOCK
    PixmapResource(ledPixNum[0], "n0.xpm"),
    PixmapResource(ledPixNum[1], "n1.xpm"),
    PixmapResource(ledPixNum[2], "n2.xpm"),
    PixmapResource(ledPixNum[3], "n3.xpm"),
    PixmapResource(ledPixNum[4], "n4.xpm"),
    PixmapResource(ledPixNum[5], "n5.xpm"),
    PixmapResource(ledPixNum[6], "n6.xpm"),
    PixmapResource(ledPixNum[7], "n7.xpm"),
    PixmapResource(ledPixNum[8], "n8.xpm"),
    PixmapResource(ledPixNum[9], "n9.xpm"),
    PixmapResource(ledPixSpace, "space.xpm"),
    PixmapResource(ledPixColon, "colon.xpm"),
    PixmapResource(ledPixSlash, "slash.xpm"),
    PixmapResource(ledPixDot, "dot.xpm"),
    PixmapResource(ledPixA, "a.xpm"),
    PixmapResource(ledPixP, "p.xpm"),
    PixmapResource(ledPixM, "m.xpm"),
    PixmapResource(ledPixPercent, "percent.xpm"),
#endif
};

class PixmapsDescription {
public:
    const PixmapResource *pixres;
    size_t size;
    const char *subdir;
    bool themeOnly;

    int count() const { return (int) size; }

    void load(const upath& file, const char *ent);
    void altL(const upath& file, const char *ent);
    void scan(const upath& path);
};

static PixmapsDescription pixdes[] = {
    { themePixRes, ACOUNT(themePixRes), 0, true },
    { taskbarPixRes, ACOUNT(taskbarPixRes), "taskbar", true },
    { taskbar2PixRes, ACOUNT(taskbar2PixRes), "taskbar", false },
    { mailboxPixRes, ACOUNT(mailboxPixRes), "mailbox", false },
    { ledclockPixRes, ACOUNT(ledclockPixRes), "ledclock", false },
};

void PixmapsDescription::load(const upath& file, const char *ent) {
    for (int i = 0; i < count(); ++i) {
        const PixmapResource *res = &pixres[i];
        if (res->needLoad()) {
            if (res->nameEqual(ent)) {
                res->loadFromFile(file);
            }
        }
    }
}

void PixmapsDescription::altL(const upath& file, const char *ent) {
    for (int i = 0; i < count(); ++i) {
        const PixmapResource *res = &pixres[i];
        if (res->needLoad()) {
            if (res->altEqual(ent)) {
                res->loadFromFile(file);
            }
        }
    }
}

void PixmapsDescription::scan(const upath& path) {
    upath subdir(path + this->subdir);
    cdir dir(subdir.string());
    while (dir.nextExt(".xpm")) {
        const char *ent = dir.entry();
        upath file(subdir + ent);
        load(file, ent);
    }
    dir.rewind();
    while (dir.nextExt(".xpm")) {
        const char *ent = dir.entry();
        upath file(subdir + ent);
        altL(file, ent);
    }
}

static void loadPixmapResources() {
    bool themeOnly = true;
    for (int k = 0; k < 2; ++k, themeOnly = !themeOnly) {
        ref<YResourcePaths> paths = YResourcePaths::subdirs(null, themeOnly);
        for (int i = 0; i < (int) ACOUNT(pixdes); ++i) {
            if (themeOnly == pixdes[i].themeOnly) {
                for (int p = 0; p < paths->getCount(); ++p) {
                    pixdes[i].scan(paths->getPath(p));
                }
            }
        }
    }
}

static void freePixmapResources() {
    for (int i = 0; i < (int) ACOUNT(pixdes); ++i) {
        for (int k = 0; k < pixdes[i].count(); ++k) {
            pixdes[i].pixres[k].reset();
        }
    }
}

static void replicatePixmaps() {
    if (logoutPixmap != null) {
        logoutPixmap->replicate(true, false);
        logoutPixmap->replicate(false, false);
    }
    if (switchbackPixmap != null) {
        switchbackPixmap->replicate(true, false);
        switchbackPixmap->replicate(false, false);
    }

    if (menubackPixmap != null) {
        menubackPixmap->replicate(true, false);
        menubackPixmap->replicate(false, false);
    }
    if (menusepPixmap != null)
        menusepPixmap->replicate(true, false);
    if (menuselPixmap != null)
        menuselPixmap->replicate(true, false);

#ifndef LITE
    if (listbackPixmap != null) {
        listbackPixmap->replicate(true, false);
        listbackPixmap->replicate(false, false);
    }
#endif

    if (dialogbackPixmap != null) {
        dialogbackPixmap->replicate(true, false);
        dialogbackPixmap->replicate(false, false);
    }

    if (buttonIPixmap != null)
        buttonIPixmap->replicate(true, false);
    if (buttonAPixmap != null)
        buttonAPixmap->replicate(true, false);
}

static void copyPixmaps() {
#ifndef LITE
    if (TEST_GRADIENT(listbackPixbuf == null) && listbackPixmap == null)
        listbackPixmap = menubackPixmap;
#endif
    if (TEST_GRADIENT(dialogbackPixbuf == null) && dialogbackPixmap == null)
        dialogbackPixmap = menubackPixmap;
#ifdef CONFIG_TASKBAR
    if (TEST_GRADIENT(toolbuttonPixbuf == null) && toolbuttonPixmap == null)
    {
        IF_CONFIG_GRADIENTS (buttonIPixbuf != null,
                             toolbuttonPixbuf = buttonIPixbuf)
        else toolbuttonPixmap = buttonIPixmap;
    }
    if (TEST_GRADIENT(workspacebuttonPixbuf == null) &&
        workspacebuttonPixmap == null)
    {
        IF_CONFIG_GRADIENTS (buttonIPixbuf != null,
                             workspacebuttonPixbuf = buttonIPixbuf)
        else workspacebuttonPixmap = buttonIPixmap;
    }
    if (TEST_GRADIENT(workspacebuttonactivePixbuf == null) &&
        workspacebuttonactivePixmap == null)
    {
        IF_CONFIG_GRADIENTS (buttonAPixbuf != null,
                             workspacebuttonactivePixbuf = buttonAPixbuf)
        else workspacebuttonactivePixmap = buttonAPixmap;
    }
#endif
}

void WPixRes::initPixmaps() {
    loadPixmapResources();
    copyPixmaps();
    replicatePixmaps();
}

void WPixRes::freePixmaps() {
    freePixmapResources();
}

// vim: set sw=4 ts=4 et:

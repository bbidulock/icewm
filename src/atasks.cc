#include "config.h"
#include "atasks.h"
#include "atray.h"
#include "applet.h"
#include "yprefs.h"
#include "default.h"
#include "wmapp.h"
#include "wmmgr.h"
#include "wmclient.h"
#include "wpixmaps.h"
#include "ymenuitem.h"

static YColorName normalTaskBarAppFg(&clrNormalTaskBarAppText);
static YColorName normalTaskBarAppBg(&clrNormalTaskBarApp);
static YColorName activeTaskBarAppFg(&clrActiveTaskBarAppText);
static YColorName activeTaskBarAppBg(&clrActiveTaskBarApp);
static YColorName minimizedTaskBarAppFg(&clrMinimizedTaskBarAppText);
static YColorName minimizedTaskBarAppBg(&clrMinimizedTaskBarApp);
static YColorName invisibleTaskBarAppFg(&clrInvisibleTaskBarAppText);
static YColorName invisibleTaskBarAppBg(&clrInvisibleTaskBarApp);
static YColorName groupingBg(&clrActiveTaskBarAppText);
static YColorName groupingFg(&clrActiveTaskBarApp);
static YFont normalTaskBarFont;
static YFont activeTaskBarFont;

TaskBarApp::TaskBarApp(ClientData* frame, TaskButton* button) :
    fFrame(frame),
    fButton(button),
    fShown(true)
{
    fButton->addApp(this);
}

TaskBarApp::~TaskBarApp() {
    fButton->remove(this);
}

void TaskBarApp::activate() const {
    bool raise = true;
    bool flash = fButton->flashing() && fFrame->isUrgent();
    fFrame->activateWindow(raise, flash);
}

YAction TaskBarApp::action() const {
    return fFrame->current()->action();
}

void TaskBarApp::setShown(bool show) {
    if (fShown != show || (show && fFrame->focused())) {
        fShown = show;
        fButton->setShown(this, show);
    }
}

bool TaskBarApp::getShown() const {
    return fShown || fFrame->isUrgent();
}

int TaskBarApp::getOrder() const {
    return fFrame->getTrayOrder();
}

void TaskBarApp::setFlash(bool flashing) {
    fButton->setFlash(flashing);
}

void TaskBarApp::setToolTip(const mstring& tip) {
    if (fButton->getActive() == this &&
        fButton->toolTipVisible()) {
        fButton->setToolTip(tip);
    }
}

void TaskBarApp::repaint() {
    fButton->repaintApp(this);
}

mstring TaskBarApp::getTitle() {
    mstring str(getFrame()->getTitle());
    if (str.isEmpty())
        str = getFrame()->getIconTitle();
    return str;
}

mstring TaskBarApp::getIconTitle() {
    mstring str(getFrame()->getIconTitle());
    if (str.isEmpty())
        str = getFrame()->getTitle();
    return str;
}

TaskButton::TaskButton(TaskPane* taskPane):
    YWindow(taskPane),
    fTaskPane(taskPane),
    fActive(nullptr),
    fTaskGrouping(taskPane->grouping()),
    fRepainted(false),
    fShown(true),
    fFlashing(false),
    fFlashOn(false),
    fFlashStart(zerotime()),
    selected(0)
{
    addStyle(wsToolTipping);
    setParentRelative();
}

TaskButton::~TaskButton() {
    if (fTaskPane->dragging() == this)
        fTaskPane->endDrag();
}

void TaskButton::activate() const {
    if (fActive) {
        fActive->activate();
    }
}

bool TaskButton::getShown() {
    bool shown(fActive && fActive->getShown());
    if (!shown && grouping()) {
        for (IterGroup iter = fGroup.reverseIterator(); ++iter; ) {
            if (iter->getShown()) {
                fActive = iter;
                shown = true;
                break;
            }
        }
    }
    return shown;
}

bool TaskButton::isFocusTraversable() {
    return true;
}

int TaskButton::getOrder() const {
    return fActive ? fActive->getOrder() : 0;
}

static bool similar(ClassHint* left, ClassHint* right) {
    bool match = false;
    if (nonempty(left->res_class)) {
        if (right->res_class)
            match = !strcmp(right->res_class, left->res_class);
    }
    else if (left->res_name && right->res_name && isEmpty(right->res_class))
        match = !strcmp(right->res_name, left->res_name);
    return match;
}

int TaskButton::getCount() const {
    int count = 0;
    if (grouping()) {
        for (TaskBarApp* app : fGroup) {
            if (taskBarShowAllWindows || app->getShown()) {
                ClassHint* hint = app->getFrame()->classHint();
                for (YFrameClient* tab : app->getFrame()->clients()) {
                    if (similar(hint, tab->classHint()))
                        count += 1;
                }
            }
        }
    } else {
        count = (fActive != nullptr);
    }
    return count;
}

void TaskButton::addApp(TaskBarApp* tapp) {
    if (grouping()) {
        if (find(fGroup, tapp) < 0) {
            fGroup += tapp;
        }
        if ( !fActive ||
             !fActive->getShown() ||
             !fActive->getFrame()->focused())
        {
            if (fActive && fMenu) {
                YMenuItem* item = fMenu->findAction(fActive->action());
                if (item && item->isChecked()) {
                    item->setChecked(false);
                    fMenu->repaintItem(item);
                }
            }
            fActive = tapp;
        }
        if (fMenu && (taskBarShowAllWindows || tapp->getShown())) {
            YMenuItem* item = fMenu->addItem(tapp->getTitle(), -2,
                                             null, tapp->action());
            if (tapp == fActive) {
                item->setChecked(true);
            }
            item->setIcon(tapp->getFrame()->getIcon());
            fMenu->sizePopup(int(fMenu->width()));
            fMenu->repaintItem(item);
        }
    } else {
        fActive = tapp;
    }
    if (toolTipVisible()) {
        updateToolTip();
    }
    fTaskPane->relayout();
    if (visible())
        repaint();
}

void TaskButton::findActive() {
    if (grouping()) {
        TaskBarApp* focus = nullptr, *shown = nullptr;
        for (TaskBarApp* app : fGroup) {
            if (app->getShown()) {
                shown = app;
                if (app->getFrame()->focused())
                    focus = app;
            }
        }
        TaskBarApp* active = focus ? focus : shown ? shown
                           : fGroup.nonempty() ? fGroup.last() : nullptr;
        if (fActive != active) {
            TaskBarApp* previous = fActive;
            fActive = active;
            if (fMenu) {
                if (0 <= find(fGroup, previous))
                    repaintApp(previous);
                if (fActive)
                    repaintApp(fActive);
            }
        }
    }
}

void TaskButton::remove(TaskBarApp* tapp) {
    if (grouping()) {
        if (findRemove(fGroup, tapp)) {
            if (fMenu && fGroup.nonempty()) {
                YMenuItem* item = fMenu->findAction(tapp->action());
                if (item) {
                    item->setChecked(false);
                    item->setEnabled(false);
                    fMenu->repaintItem(item);
                }
            }
        }
    }
    if (fActive == tapp) {
        fActive = nullptr;
        findActive();
    }
    if (fActive == nullptr) {
        fTaskPane->remove(this);
    }
    else if (visible() && manager->isRunning()) {
        if (getShown())
            repaint();
        else
            fTaskPane->relayout();
        if (toolTipVisible())
            updateToolTip();
    }
}

TaskBarApp* TaskButton::getNextShown(TaskBarApp* tapp) const {
    if (grouping()) {
        int k = tapp ? find(fGroup, tapp) : -1;
        if (k >= 0 || tapp == nullptr) {
            while (++k < fGroup.getCount()) {
                if (fGroup[k]->getShown()) {
                    return fGroup[k];
                }
            }
        }
    } else if (tapp == nullptr) {
        return fActive;
    }
    return nullptr;
}

TaskBarApp* TaskButton::getPrevShown(TaskBarApp* tapp) const {
    if (grouping()) {
        int k = tapp ? find(fGroup, tapp) : fGroup.getCount();
        if (k > 0) {
            while (--k >= 0) {
                if (fGroup[k]->getShown()) {
                    return fGroup[k];
                }
            }
        }
    } else if (tapp == nullptr) {
        return fActive;
    }
    return nullptr;
}

void TaskButton::setShown(TaskBarApp* tapp, bool ashow) {
    if (grouping()) {
        bool gdraw = (tapp == fActive);
        bool shown = getShown();
        if (ashow < shown && tapp == fActive) {
            for (IterGroup iter = fGroup.reverseIterator(); ++iter; ) {
                if (iter->getShown()) {
                    fActive = iter;
                    gdraw = true;
                    break;
                }
            }
        }
        else if (ashow > shown || tapp->getFrame()->focused()) {
            fActive = tapp;
            gdraw = true;
        }
        else if (getShown()) {
            gdraw = true;
        }
        fTaskPane->relayout();
        if (visible() && gdraw)
            repaint();
    }
    else if (tapp == fActive) {
        if (ashow != visible())
            fTaskPane->relayout();
    }
}

void TaskButton::setFlash(bool flashing) {
    if (fFlashing != flashing) {
        fFlashing = flashing;

        if (fFlashing && focusRequestFlashInterval > 0) {
            fFlashOn = true;
            fFlashStart = monotime();
            fFlashTimer->setTimer(focusRequestFlashInterval, this, true);
        } else {
            //fFlashTimer->stopTimer();
        }

        if (fShown == false)
            fTaskPane->relayout();
    }
}

void TaskButton::configure(const YRect2& r) {
    if (r.resized()) {
        int dw = r.deltaWidth();
        if (r.deltaHeight() || dw < -2 || dw > 0 || fRepainted == false) {
            fRepainted = false;
            repaint();
        } else {
            fRepainted = false;
        }
    }
}

void TaskButton::repaintApp(TaskBarApp* app) {
    if (app == fActive && visible()) {
        repaint();
    } else {
        fRepainted = false;
    }
    if (fMenu) {
        YMenuItem* item = fMenu->findAction(app->action());
        if (item) {
            item->setIcon(app->getFrame()->getIcon());
            item->getName() = app->getTitle();
            item->setChecked(app == fActive);
            fMenu->repaintItem(item);
        }
    }
}

void TaskButton::repaint() {
    fPaintTimer->setTimer(0L, this, true);
}

void TaskButton::handleExpose(const XExposeEvent& exp) {
    if (fRepainted == false && exp.count == 0) {
        repaint();
    }
}

void TaskButton::paint(Graphics& g, const YRect& r) {
    YColor bg, fg;
    ref<YPixmap> bgPix;
    ref<YPixmap> bgLeft;
    ref<YPixmap> bgRight;
    ref<YImage> bgGrad;
    ref<YImage> bgLeftG;
    ref<YImage> bgRightG;

    int p(0);
    int border_size = 0;
    int left = 0;
    int style = 0;

    if (selected == 3)
        style = 3;
    else if ((fActive && getFrame()->focused()) || selected == 2)
        style = 2;
    else
        style = 1;

    if (fFlashing) {
        if (fFlashOn) {
            bg = activeTaskBarAppBg;
            fg = activeTaskBarAppFg;
            bgPix = taskbuttonactivePixmap;
            bgLeft = taskbuttonactiveLeftPixmap;
            bgRight = taskbuttonactiveRightPixmap;
            bgGrad = taskbuttonactivePixbuf;
            bgLeftG = taskbuttonactiveLeftPixbuf;
            bgRightG = taskbuttonactiveRightPixbuf;
            style = 1;
        } else {
            bg = normalTaskBarAppBg;
            fg = normalTaskBarAppFg;
            bgPix = taskbuttonPixmap;
            bgLeft = taskbuttonLeftPixmap;
            bgRight = taskbuttonRightPixmap;
            bgGrad = taskbuttonPixbuf;
            bgLeftG = taskbuttonLeftPixbuf;
            bgRightG = taskbuttonRightPixbuf;
            style = 1;
        }
    } else if (fActive && !getFrame()->visibleNow()) {
        bg = invisibleTaskBarAppBg;
        fg = invisibleTaskBarAppFg;
        bgPix = taskbackPixmap;
        bgGrad = taskbackPixbuf;
    } else if (fActive && getFrame()->isMinimized()) {
        bg = minimizedTaskBarAppBg;
        fg = minimizedTaskBarAppFg;
        bgPix = taskbuttonminimizedPixmap;
        bgLeft = taskbuttonminimizedLeftPixmap;
        bgRight = taskbuttonminimizedRightPixmap;
        bgGrad = taskbuttonminimizedPixbuf;
        bgLeftG = taskbuttonminimizedLeftPixbuf;
        bgRightG = taskbuttonminimizedRightPixbuf;
    } else if (fActive && getFrame()->focused()) {
        bg = activeTaskBarAppBg;
        fg = activeTaskBarAppFg;
        bgPix = taskbuttonactivePixmap;
        bgLeft = taskbuttonactiveLeftPixmap;
        bgRight = taskbuttonactiveRightPixmap;
        bgGrad = taskbuttonactivePixbuf;
        bgLeftG = taskbuttonactiveLeftPixbuf;
        bgRightG = taskbuttonactiveRightPixbuf;
    } else {
        bg = normalTaskBarAppBg;
        fg = normalTaskBarAppFg;
        bgPix = taskbuttonPixmap;
        bgLeft = taskbuttonLeftPixmap;
        bgRight = taskbuttonRightPixmap;
        bgGrad = taskbuttonPixbuf;
        bgLeftG = taskbuttonLeftPixbuf;
        bgRightG = taskbuttonRightPixbuf;
    }

    if (style == 3) {
        p = 2;
        g.setColor(YColor::black);
        g.drawRect(0, 0, width() - 1, height() - 1);
        g.setColor(bg);
        g.fillRect(1, 1, width() - 2, height() - 2);
    } else {
        g.setColor(bg);

        if (style == 2) {
            p = 2;
            if (wmLook == lookMetal) {
                g.drawBorderM(0, 0, width() - 1, height() - 1, false);
            } else if (wmLook == lookGtk) {
                g.drawBorderG(0, 0, width() - 1, height() - 1, false);
            }
            else if (wmLook != lookFlat)
                g.drawBorderW(0, 0, width() - 1, height() - 1, false);
        } else {
            p = 1;
            if (wmLook == lookMetal) {
                p = 2;
                g.drawBorderM(0, 0, width() - 1, height() - 1, true);
            } else if (wmLook == lookGtk) {
                g.drawBorderG(0, 0, width() - 1, height() - 1, true);
            }
            else if (wmLook != lookFlat)
                g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        }

        int const dp(wmLook == lookFlat ? 0: wmLook == lookMetal ? 2 : p);
        int const ds(wmLook == lookFlat ? 0: wmLook == lookMetal ? 4 : 3);
        border_size = ds;

        if ((int) width() > ds && (int) height() > ds) {
            if (bgGrad != null) {
                int x = dp;
                int y = dp;
                unsigned w = width() - ds;
                unsigned h = height() - ds;
                if (taskbuttonIconOffset &&
                    bgLeftG != null &&
                    bgRightG != null &&
                    3 * taskbuttonIconOffset <= w)
                {
                    g.drawGradient(bgLeftG, x, y,
                                   bgLeftG->width(), h);
                    x += left = bgLeftG->width();
                    g.drawGradient(bgRightG, w - bgRightG->width(), y,
                                   bgRightG->width(), h);
                    w -= bgLeftG->width() + bgRightG->width();
                }
                g.drawGradient(bgGrad, x, y, w, h);
            }
            else if (bgPix != null) {
                int x = dp;
                int y = dp;
                unsigned w = width() - ds;
                unsigned h = height() - ds;
                if (taskbuttonIconOffset &&
                    bgLeft != null &&
                    bgRight != null &&
                    3 * taskbuttonIconOffset <= w)
                {
                    g.fillPixmap(bgLeft, x, y,
                                 bgLeft->width(), h);
                    x += left = bgLeft->width();
                    g.fillPixmap(bgRight, w - bgRight->width(), y,
                                 bgRight->width(), h);
                    w -= bgLeft->width() + bgRight->width();
                }
                g.fillPixmap(bgPix, x, y, w, h);
            }
            else {
                g.fillRect(dp, dp, width() - ds, height() - ds);
            }
        }
    }

    ref<YIcon> icon;
    bool iconDrawn = false;
    int iconSize = YIcon::smallSize();
    int iconX = 0, iconY = 0;
    if (taskBarShowWindowIcons) {
        if (fActive) {
            icon = getFrame()->getIcon();
        }
    }
    if (icon != null) {
        int const y((height() - 3 - iconSize -
                     (wmLook == lookMetal)) / 2);
        iconX = p + max(1, left);
        iconY = p + 1 + y;
        iconDrawn = icon->draw(g, iconX, iconY, iconSize);
        if (iconDrawn && p + max(1, left) + iconSize + 5 >= int(width())) {
            if (bgGrad != null) {
                g.maxOpacity();
            }
            return;
        }
    }

    int groupCount = getCount();
    bool groupings = (groupCount > 1);
    YColor crumbsBg;

    if (groupings) {
        crumbsBg = bg.brighter();
        if (crumbsBg == bg) crumbsBg = bg.darker();
    }

    if (groupings && (grouping() & 2)) {
        // Draw rumbs for grouped tabs before text so that the text
        // can overlap it for those who have really thin taskbar.
        int crumbSize = 7;
        int crumb_gap = 3;
        int crumbStep = crumbSize + crumb_gap;
        g.setColor(crumbsBg);

        int x = width() / 2 - crumbStep * groupCount / 2;
        int y = height() - border_size;
        // please check if border_size is correctly above

        int count = groupCount;
        if (x < 0) {
            x = 0;
            count = (width() + crumbSize) / crumbStep;
        }

        for (int dn = 0; dn < count; dn++) {
            if (wmLook != lookFlat) {
                g.fillArc(x + p + dn * crumbStep, y + p - crumbSize,
                          crumbSize, crumbSize, 0, 360 * 64);
            }
            else {
                // flat people are know to avoid circles
                g.fillRect(x + p + dn * crumbStep, y + p - crumbSize,
                           crumbSize, crumbSize);
            }
        }
    }

    int textX = 0;
    int textY = 0;
    mstring str(fActive && taskBarShowWindowTitles ? fActive->getIconTitle() : null);
    if (str != null) {
        YFont font = getFont();
        if (font != null) {
            g.setColor(fg);
            g.setFont(font);

            int iconSize = 0;
            int pad = max(1, left);
            if (iconDrawn) {
                iconSize = YIcon::smallSize();
                pad += 2;
            }
            int const tx = pad + iconSize;
            int const ty = max(2U,
                               (height() + font->height() -
                               (LOOK(lookMetal | lookFlat) ? 2 : 1)) / 2 -
                               font->descent());
            int const wm = int(width()) - p - pad - iconSize - 2;

            if (0 < wm && p + tx + wm < int(width())) {
                textX = p + tx;
                textY = p + ty;
                int x = 0;
                if (groupings && (grouping() & 1)) {
                    mstring cnt(groupCount);
                    x = font->textWidth(cnt, cnt.length()) + 4;
                    g.setColor(groupingBg);
                    int h = font->ascent();
                    g.fillRect(textX, textY - font->ascent() + 3, x, h, 5);
                    g.setColor(groupingFg);
                    g.drawStringEllipsis(textX + 2, textY + 1, cnt, wm);
                    x += 1;
                }
                g.setColor(fg);
                g.drawStringEllipsis(textX + x, textY, str,
                                     wm - max(0, x - 4));
            }
        }
    }

    if (bgGrad != null) {
        g.maxOpacity();
    }
}

int TaskButton::estimate() {
    int p(0);

    if (taskbuttonLeftPixbuf != null &&
        taskbuttonRightPixbuf != null)
    {
        p += taskbuttonLeftPixbuf->width();
        p += taskbuttonRightPixbuf->width();
    }
    else if (taskbuttonLeftPixmap != null &&
             taskbuttonRightPixmap != null)
    {
        p += taskbuttonLeftPixmap->width();
        p += taskbuttonRightPixmap->width();
    }

    if (taskBarShowWindowIcons) {
        p += YIcon::smallSize();
    }

    mstring str(fActive && taskBarShowWindowTitles ? fActive->getIconTitle() : null);
    if (str != null) {
        YFont font = getFont();
        if (font != null) {
            if (taskBarShowWindowIcons)
                p += 2;
            p += font->textWidth(str);
        }
    }

    return p + 4;
}

unsigned TaskButton::maxHeight() {
    unsigned activeHeight =
        (getActiveFont() != null) ? getActiveFont()->height() : 0;
    unsigned normalHeight =
        (getNormalFont() != null) ? getNormalFont()->height() : 0;
    return 2 + max(activeHeight, normalHeight);
}

YFont TaskButton::getFont() {
    YFont font;
    if (fActive && getFrame()->focused())
        font = getActiveFont();
    if (font == null)
        font = getNormalFont();
    return font;
}

YFont TaskButton::getNormalFont() {
    if (normalTaskBarFont == null)
        normalTaskBarFont = normalTaskBarFontName;
    return normalTaskBarFont;
}

YFont TaskButton::getActiveFont() {
    if (activeTaskBarFont == null)
        activeTaskBarFont = activeTaskBarFontName;
    return activeTaskBarFont;
}

void TaskButton::popupGroup() {
    fMenu->setActionListener(this);
    fMenu->removeAll();
    for (TaskBarApp* app : fGroup) {
        if (taskBarShowAllWindows || app->getShown()) {
            ClassHint* hint = app->getFrame()->classHint();
            for (YFrameClient* tab : app->getFrame()->clients()) {
                if (similar(hint, tab->classHint())) {
                    YMenuItem* item = fMenu->addItem(tab->windowTitle(), -2,
                                                     null, tab->action());
                    if (app == fActive && tab == app->getFrame()->current()) {
                        item->setChecked(true);
                    }
                    item->setIcon(tab->getIcon());
                }
            }
        }
    }
    int x = 0, y = taskBarAtTop * height();
    mapToGlobal(x, y);
    fMenu->popup(this, nullptr, this, x, y,
                 YPopupWindow::pfCanFlipVertical |
                 YPopupWindow::pfCanFlipHorizontal |
                 YPopupWindow::pfPopupMenu);
}

void TaskButton::handleButton(const XButtonEvent& button) {
    YWindow::handleButton(button);

    if (fTaskPane->dragging())
        return;

    if (button.type == ButtonPress) {
        if (button.button == Button1 || button.button == Button2) {
            selected = 2;
            repaint();
        }
    }
    else if (button.type == ButtonRelease) {
        if (button.button == Button1 && selected == 2 && fActive) {
            if (fMenu) {
            }
            else if (getFrame()->focused() && getFrame()->visibleNow() &&
                (!getFrame()->canRaise(true) || (button.state & ControlMask)))
            {
                getFrame()->wmMinimize();
            }
            else {
                if (button.state & ShiftMask)
                    getFrame()->wmOccupyCurrent();
                activate();
            }
        }
        else if (button.button == Button2 && selected == 2 && fActive) {
            if (hasbit(button.state, xapp->AltMask)) {
                if (getFrame()) {
                    activate();
                    if (manager->getFocused()) {
                        manager->getFocused()->wmClose();
                        return;
                    }
                }
            } else
            if (getFrame()->focused() && getFrame()->visibleNow() &&
                (!getFrame()->canRaise() || (button.state & ControlMask)))
            {
                getFrame()->wmLower();
            }
            else {
                if (button.state & ShiftMask)
                    getFrame()->wmOccupyCurrent();
                activate();
            }
        }
        if (button.button == Button1 || button.button == Button2) {
            deselect();
            repaint();
        }
    }
}

void TaskButton::actionPerformed(YAction action, unsigned modifiers) {
    for (TaskBarApp* app : fGroup) {
        for (YFrameClient* tab : app->getFrame()->clients()) {
            if (action == tab->action()) {
                if (fActive != app) {
                    TaskBarApp* old = fActive;
                    fActive = app;
                    if (old)
                        old->repaint();
                    fActive->repaint();
                }
                if (fActive) {
                    if (fActive->getFrame()->current() != tab)
                        fActive->getFrame()->selectTab(tab);
                    if (fActive->getFrame()->current() == tab)
                        fActive->activate();
                }
                break;
            }
        }
    }
}

void TaskButton::handlePopDown(YPopupWindow* popup) {
    fMenu = null;
}

void TaskButton::updateToolTip() {
    if (fActive) {
        YWindow::setToolTip(fActive->getTitle());
    }
}

void TaskButton::handleCrossing(const XCrossingEvent& crossing) {
    if (selected > 0) {
        if (crossing.type == EnterNotify) {
            selected = 2;
            repaint();
        } else if (crossing.type == LeaveNotify) {
            deselect();
            repaint();
        }
    }
    YWindow::handleCrossing(crossing);
}

void TaskButton::handleClick(const XButtonEvent& up, int count) {
    if (up.button == Button3 && fActive) {
        getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
                                    YPopupWindow::pfCanFlipVertical |
                                    YPopupWindow::pfCanFlipHorizontal |
                                    YPopupWindow::pfPopupMenu);
    }
    else if (up.button == Button4 && taskBarUseMouseWheel) {
        fTaskPane->switchToPrev();
    }
    else if (up.button == Button5 && taskBarUseMouseWheel) {
        fTaskPane->switchToNext();
    }
    else if (up.button == Button1 && getCount() > 1) {
        if (count == 1 && KEY_MODMASK(up.state) == 0) {
            popupGroup();
        } else {
            fMenu = null;
        }
    }
}

void TaskButton::handleDNDEnter() {
    fRaiseTimer->setTimer(autoRaiseDelay, this, true);
    selected = 3;
    repaint();
}

void TaskButton::handleDNDLeave() {
    fRaiseTimer = null;
    deselect();
    repaint();
}

bool TaskButton::handleTimer(YTimer* t) {
    if (t == fRaiseTimer) {
        fRaiseTimer = null;
        if (fActive) {
            getFrame()->wmRaise();
        }
    }
    if (t == fFlashTimer) {
        if (!fFlashing) {
            fFlashOn = false;
            fFlashStart = zerotime();
            return false;
        }
        fFlashOn = !fFlashOn;
        if (focusRequestFlashTime > 0) {
            if (fFlashStart + focusRequestFlashTime < monotime())
                fFlashing = false;
        }
        repaint();
        return fFlashing;
    }
    if (t == fPaintTimer) {
        if (width() > 1 && height() > 1 && getShown()) {
            GraphicsBuffer(this).paint();
            fRepainted = true;
        }
    }
    return false;
}

bool TaskButton::handleBeginDrag(const XButtonEvent& down, const XMotionEvent& motion) {
    if (down.button == Button1) {
        raise();
        fTaskPane->startDrag(this, 0, down.x + x(), down.y + y());
        fTaskPane->processDrag(motion.x + x(), motion.y + y());
        return true;
    }
    return false;
}

TaskPane::TaskPane(IAppletContainer* taskBar, YWindow* parent):
    YWindow(parent),
    fTaskBar(taskBar),
    fDragging(nullptr),
    fDragX(0),
    fDragY(0),
    fNeedRelayout(true),
    fForceImmediate(false),
    fRemoveAtWorkspace(-1),
    fTaskGrouping(taskBarTaskGrouping)
{
    if (getGradient() == null && taskbackPixmap == null) {
        setBackground(taskBarBg);
    }
    setParentRelative();
}

TaskPane::~TaskPane() {
    if (fDragging)
        endDrag();

    normalTaskBarFont = null;
    activeTaskBarFont = null;
}

void TaskPane::insert(TaskBarApp* tapp) {
    IterApps it = fApps.reverseIterator();
    while (++it && it->getOrder() > tapp->getOrder());
    (--it).insert(tapp);
    fRemoveAtWorkspace = -1;
}

TaskBarApp* TaskPane::predecessor(TaskBarApp* tapp) {
    TaskButton* button = tapp->button();
    TaskBarApp* prev = button->getPrevShown(tapp);
    if (prev && button->getShown()) {
        return prev;
    } else {
        int k = find(fTasks, button);
        if (k >= 0) {
            int i = k;
            do {
                i = (i - 1 + fTasks.getCount()) % fTasks.getCount();
                if (fTasks[i]->getShown()) {
                    prev = fTasks[i]->getPrevShown(nullptr);
                    if (prev && prev != tapp) {
                        return prev;
                    }
                }
            } while (i != k);
        }
    }
    return nullptr;
}

TaskBarApp* TaskPane::successor(TaskBarApp* tapp) {
    TaskButton* button = tapp->button();
    TaskBarApp* next = button->getNextShown(tapp);
    if (next && button->getShown()) {
        return next;
    } else {
        int k = find(fTasks, button);
        if (k >= 0) {
            int i = k;
            do {
                i = (i + 1) % fTasks.getCount();
                if (fTasks[i]->getShown()) {
                    next = fTasks[i]->getNextShown(nullptr);
                    if (next && next != tapp) {
                        return next;
                    }
                }
            } while (i != k);
        }
    }
    return nullptr;
}

TaskBarApp* TaskPane::findApp(ClientData* frame) {
    IterApps task = fApps.iterator();
    while (++task && task->getFrame() != frame);
    return task ? *task : nullptr;
}

TaskBarApp* TaskPane::getActiveApp() {
    return findApp(manager->getFocused());
}

TaskButton* TaskPane::getActiveButton() {
    TaskBarApp* tapp = getActiveApp();
    return tapp ? tapp->button() : nullptr;
}

TaskBarApp* TaskPane::addApp(ClientData* frame) {
    TaskBarApp* tapp = nullptr;
    TaskButton* task = nullptr;
    TaskButton* make = nullptr;
    if (grouping()) {
        const char* klas = frame->classHint()->res_class;
        const char* name = frame->classHint()->res_name;
        if (nonempty(klas)) {
            for (IterTask it = fTasks.reverseIterator(); ++it; ) {
                const char* hint = it->getFrame()->classHint()->res_class;
                if (nonempty(hint) && !strcmp(hint, klas)) {
                    task = *it;
                    break;
                }
            }
        }
        else if (nonempty(name)) {
            for (IterTask it = fTasks.reverseIterator(); ++it; ) {
                ClassHint* hint = it->getFrame()->classHint();
                if (isEmpty(hint->res_class) &&
                    nonempty(hint->res_name) &&
                    !strcmp(hint->res_name, name))
                {
                    task = *it;
                    break;
                }
            }
        }
    }
    if (task == nullptr) {
        task = make = new TaskButton(this);
    }
    if (task) {
        tapp = new TaskBarApp(frame, task);
        if (tapp) {
            insert(tapp);
        }
        if (make) {
            if (tapp)
                insert(make);
            else
                delete make;
        }
    }
    return tapp;
}

void TaskPane::remove(TaskBarApp* task) {
    findRemove(fApps, task);
}

void TaskPane::insert(TaskButton* task) {
    IterTask it = fTasks.reverseIterator();
    while (++it && it->getOrder() > task->getOrder());
    (--it).insert(task);
    fRemoveAtWorkspace = -1;
}

void TaskPane::remove(TaskButton* button) {
    if (findRemove(fTasks, button)) {
        fRemoveAtWorkspace = manager->activeWorkspace();
        relayout();
    }
}

void TaskPane::relayout(bool force) {
    if (force)
        fForceImmediate = true;
    if (fNeedRelayout == false || !fRelayoutTimer) {
        fNeedRelayout = true;
        fRelayoutTimer->setTimer(15L, this, true);
    }
}

bool TaskPane::handleTimer(YTimer* t) {
    bool repeat = false;
    if (t == fRelayoutTimer) {
        unsigned mask = 0;
        if (fRemoveAtWorkspace == manager->activeWorkspace() &&
            xapp->queryMask(handle(), &mask) && hasbit(mask, xapp->AltMask)) {
            repeat = true;
        } else {
            fRemoveAtWorkspace = -1;
            fRelayoutTimer = null;
            relayoutNow();
        }
    }
    return repeat;
}

void TaskPane::regroup() {
    YArray<ClientData*> frames;
    for (IterApps it = fApps.reverseIterator(); ++it; ) {
        frames += it->getFrame();
        it->getFrame()->removeAppStatus();
    }

    fTaskGrouping = taskBarTaskGrouping;
    fNeedRelayout = true;

    for (auto it = frames.reverseIterator(); ++it; ) {
        it->updateAppStatus();
    }
}

void TaskPane::relayoutNow(bool force) {
    if (fTaskGrouping != taskBarTaskGrouping && !dragging())
        regroup();

    if (!fNeedRelayout && !force)
        return ;

    fNeedRelayout = false;
    if (fRelayoutTimer)
        fRelayoutTimer = null;

    if (width() <= 3)
        return;

    int tc = 0;

    for (TaskButton* task : fTasks) {
        if (task->getShown())
            tc++;
        else
            task->hide();
    }
    if (tc == 0)
        return;
    tc = max(tc, taskBarButtonWidthDivisor);

    const int wid = (width() - 2) / tc;
    const int rem = (width() - 2) % tc;
    int x = 0;
    int lc = 0;

    for (TaskButton* task : fTasks) {
        if (task->getShown()) {
            const int w1 = taskBarShowWindowTitles
                         ? wid + (lc < rem)
                         : task->estimate();
            if (task != dragging()) {
                YRect r(x, 0, unsigned(w1), height());
                if (rightToLeft) {
                    r.xx = width() - r.xx - r.ww - 1;
                }
                task->setGeometry(r);
                task->show();
            }
            x += w1;
            lc++;
        }
    }
    if (dragging())
        dragging()->show();
}

void TaskPane::handleClick(const XButtonEvent& up, int count) {
    if (up.button == 3 && count == 1 && xapp->isButton(up.state, Button3Mask)) {
        fTaskBar->contextMenu(up.x_root, up.y_root);
    }
    else if ((up.button == Button4 || up.button == Button5)
           && taskBarUseMouseWheel)
    {
        TaskButton* button = getActiveButton();
        if (button)
            button->handleClick(up, count);
        else
            fTaskBar->windowTrayPane()->handleClick(up, count);
    }
}

void TaskPane::handleExpose(const XExposeEvent& expose) {
    YRect r(expose.x, expose.y, expose.width, expose.height);
    paint(getGraphics(), r);
}

void TaskPane::paint(Graphics& g, const YRect& r) {
    if (taskbackPixmap == null && getGradient() == null) {
        g.setColor(taskBarBg);
        g.fillRect(r.x(), r.y(), r.width(), r.height());
    } else {
        clearArea(r.xx, r.yy, r.ww, r.hh);
    }
}

unsigned TaskPane::maxHeight() {
    return TaskButton::maxHeight();
}

void TaskPane::handleMotion(const XMotionEvent& motion) {
    if (fDragging) {
        processDrag(motion.x, motion.y);
    }
}

void TaskPane::handleButton(const XButtonEvent& button) {
    if (button.type == ButtonRelease)
        endDrag();
    YWindow::handleButton(button);
}

void TaskPane::configure(const YRect2& r) {
    if (fNeedRelayout || r.resized()) {
        fRemoveAtWorkspace = -1;
        clearWindow();
    }
}

void TaskPane::startDrag(TaskButton* drag, int /*byMouse*/, int sx, int sy) {
    if (fDragging == nullptr) {
        if (xapp->grabEvents(this, YWMApp::movePointer,
                             ButtonPressMask |
                             ButtonReleaseMask |
                             PointerMotionMask))
        {
            fDragging = drag;
            fDragX = sx;
            fDragY = sy;
        }
    }
}

void TaskPane::processDrag(int mx, int my) {
    if (fDragging == nullptr)
        return;

    const int w2 = width() - fDragging->width() - 2;
    const int ox = fDragX - fDragging->x();
    const int oy = fDragY - fDragging->y();
    const int cx = clamp(mx, ox, ox + w2);
    const int cy = clamp(my, oy - 1, oy + 1);
    const int dx = cx - fDragX;
    const int dy = cy - fDragY;
    if (dx == 0 && dy == 0)
        return;

    fDragging->setPosition(fDragging->x() + dx, fDragging->y() + dy);

    int drag = -1;
    int drop = -1;
    for (IterTask task = fTasks.iterator(); ++task; ) {
        if (task->getShown()) {
            if (task == fDragging)
                drag = task.where();
            else if (inrange(cx, task->x(), task->x() + (int) task->width() - 1))
                drop = task.where();
        }
    }
    if (drag >= 0 && drop >= 0) {
        fTasks.swap(drag, drop);
    }
    fDragX += dx;
    fDragY += dy;
    relayout(true);
}

void TaskPane::endDrag() {
    if (fDragging) {
        xapp->releaseEvents();
        fDragging->deselect();
        fDragging = nullptr;
        relayout(true);
    }
}

void TaskPane::movePrev() {
    TaskButton* active = getActiveButton();
    if (active) {
        int index = find(fTasks, active);
        if (index > 0) {
            fTasks.swap(index, index - 1);
            relayout(true);
        }
    }
}

void TaskPane::moveNext() {
    TaskButton* active = getActiveButton();
    if (active) {
        int index = find(fTasks, active);
        if (index + 1 < fTasks.getCount()) {
            fTasks.swap(index, index + 1);
            relayout(true);
        }
    }
}

void TaskPane::switchToPrev() {
    TaskBarApp* act = getActiveApp();
    if (act) {
        TaskBarApp* pre = predecessor(act);
        if (pre && pre != act)
            pre->activate();
    }
}

void TaskPane::switchToNext() {
    TaskBarApp* act = getActiveApp();
    if (act) {
        TaskBarApp* suc = successor(act);
        if (suc && suc != act)
            suc->activate();
    }
}

// vim: set sw=4 ts=4 et:

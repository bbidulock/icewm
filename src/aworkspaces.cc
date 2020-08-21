#include "config.h"
#include "aworkspaces.h"
#include "workspaces.h"
#include "prefs.h"
#include "yxapp.h"
#include "wmframe.h"
#include "udir.h"
#include "ypaths.h"
#include "wpixmaps.h"
#include "intl.h"
#include <math.h>

YColorName WorkspaceButton::normalButtonBg(&clrWorkspaceNormalButton);
YColorName WorkspaceButton::normalBackupBg(&clrNormalButton);
YColorName WorkspaceButton::normalButtonFg(&clrWorkspaceNormalButtonText);

YColorName WorkspaceButton::activeButtonBg(&clrWorkspaceActiveButton);
YColorName WorkspaceButton::activeBackupBg(&clrActiveButton);
YColorName WorkspaceButton::activeButtonFg(&clrWorkspaceActiveButtonText);

ref<YFont> WorkspaceButton::normalButtonFont;
ref<YFont> WorkspaceButton::activeButtonFont;

WorkspaceButton::WorkspaceButton(int ws, YWindow *parent, WorkspaceDragger* d):
    super(parent, YAction()),
    fWorkspace(ws),
    fDelta(0),
    fDownX(0),
    fDragging(false),
    fGraphics(this, true),
    fPane(d)
{
    addStyle(wsNoExpose);
    setParentRelative();
    //setDND(true);
    setTitle(name());
}

void WorkspaceButton::configure(const YRect2& r) {
    if (r.resized() || !fGraphics) {
        repaint();
    }
}

void WorkspaceButton::repaint() {
    fGraphics.paint();
}

void WorkspaceButton::handleButton(const XButtonEvent &button) {
    if (fDragging &&
        button.type == ButtonPress &&
        button.button == Button1)
    {
        fDragging = false;
        fPane->stopDrag();
    }
    super::handleButton(button);
}

void WorkspaceButton::handleClick(const XButtonEvent &up, int count) {
    switch (up.button) {
        case 1:
            if (count == 2) {
                if (fInput == nullptr) {
                    fInput = new YInputLine(this, this);
                }
                if (fInput) {
                    fInput->setSize(width(), height());
                    fInput->setText(name(), false);
                    fInput->show();
                    fInput->setWindowFocus();
                }
                return;
            }
            break;
        case 2:
            manager->doWMAction(ICEWM_ACTION_WINDOWLIST);
            break;
        case 3:
            manager->popupWindowListMenu(this, up.x_root, up.y_root);
            break;
        case 4:
            if (taskBarUseMouseWheel)
                manager->switchToPrevWorkspace(false);
            break;
        case 5:
            if (taskBarUseMouseWheel)
                manager->switchToNextWorkspace(false);
            break;
    }
    super::handleClick(up, count);
}

void WorkspaceButton::handleCrossing(const XCrossingEvent &e) {
    if (fDragging &&
        e.type == LeaveNotify &&
        e.mode == NotifyUngrab &&
        hasbit(e.state, Button1Mask))
    {
        fDragging = false;
        fDelta = e.x_root - fDownX;
        fPane->drag(fWorkspace, fDelta, false, true);
    }

    if (false == pagerShowPreview) {
        super::handleCrossing(e);
    }
}

void WorkspaceButton::handleDNDEnter() {
    fRaiseTimer->setTimer(autoRaiseDelay, this, true);
    repaint();
}

void WorkspaceButton::handleDNDLeave() {
    if (fRaiseTimer)
        fRaiseTimer->disableTimerListener(this);
    repaint();
}

void WorkspaceButton::handleBeginDrag(const XButtonEvent& d, const XMotionEvent& m)
{
    if (d.button == Button1) {
        fDragging = true;
        fDownX = d.x_root;
        fDelta = m.x_root - d.x_root;
        fPane->drag(fWorkspace, fDelta, true, false);
    }
}

void WorkspaceButton::handleDrag(const XButtonEvent& d, const XMotionEvent& m)
{
    if (d.button == Button1 && fDragging) {
        fDelta = m.x_root - d.x_root;
        PRECONDITION(fDownX == d.x_root);
        fPane->drag(fWorkspace, fDelta, false, false);
    }
}

void WorkspaceButton::handleEndDrag(const XButtonEvent& d, const XButtonEvent& u)
{
    if (d.button == Button1 && fDragging) {
        fDragging = false;
        fDelta = u.x_root - d.x_root;
        PRECONDITION(fDownX == d.x_root);
        fPane->drag(fWorkspace, fDelta, false, true);
    }
}

bool WorkspaceButton::handleTimer(YTimer *t) {
    if (t == fRaiseTimer) {
        manager->activateWorkspace(fWorkspace);
    }
    return false;
}

void WorkspaceButton::inputReturn(YInputLine* input) {
    if (input == fInput) {
        mstring text(input->getText());
        if (text != name()) {
            csmart str(newstr(text));
            swap(*&str, *workspaces[fWorkspace]);
            manager->setDesktopNames();
            fPane->relabel(fWorkspace);
        }
        fInput = nullptr;
    }
}

void WorkspaceButton::inputEscape(YInputLine* input) {
    if (input == fInput) {
        fInput = nullptr;
    }
}

void WorkspaceButton::inputLostFocus(YInputLine* input) {
    if (input == fInput) {
        fInput = nullptr;
    }
}

void WorkspaceButton::actionPerformed(YAction /*action*/, unsigned int modifiers) {
    if (modifiers & ShiftMask) {
        manager->switchToWorkspace(fWorkspace, true);
    } else if (modifiers & xapp->AltMask) {
        if (manager->getFocus())
            manager->getFocus()->wmOccupyWorkspace(fWorkspace);
    } else {
        manager->activateWorkspace(fWorkspace);
        return;
    }
}

void WorkspaceButton::setPosition(int x, int y) {
    bool left = (x >= -int(width()));
    bool right = x <= int((fPane->width() >= max(10U, width()) ?
                           fPane->width() : desktop->width()));
    int change((left && right) - visible());
    if (change < 0)
        hide();
    YWindow::setPosition(x, y);
    if (0 < change)
        show();
}

WorkspacesPane::WorkspacesPane(YWindow *parent):
    super(parent),
    fActive(0),
    fDelta(0),
    fMoved(0),
    fSpeed(0),
    fTime(zerotime()),
    fMillis(16L),
    fRepositioning(false),
    fReconfiguring(false)
{
    addStyle(wsNoExpose);
    setParentRelative();
}

void WorkspacesPane::resize(unsigned width, unsigned height) {
    bool save(fReconfiguring);
    fReconfiguring = true;
    long limit = limitWidth(width);
    setSize(unsigned(limit), height);
    fReconfiguring = save;

    int excess = int(this->width()) - extent();
    if (excess > 0) {
        fMoved = min(0, fMoved + excess);
        repositionButtons();
    }
}

long WorkspacesPane::limitWidth(long paneWidth) {
    const char* str = taskBarWorkspacesLimit;
    long taskBarWidth = desktop->getScreenGeometry().width()
                      * taskBarWidthPercentage / 100;
    long reserved = 200L;
    long maxPixels = 0;
    long maxButtons = 0;
    long maxPercent = 0;
    if (nonempty(str)) {
        char* end = nullptr;
        long num = strtol(str, &end, 0);
        if (end && str < end && inrange(num, 0L, long(SHRT_MAX))) {
            maxPixels = max(50L, taskBarWidth - x() - reserved);
            maxButtons = maxPixels * count() / non_zero(paneWidth);
            maxPercent = 100L * maxPixels / taskBarWidth;
            if (*end == ' ')
                ++end;
            if (*end == 'p' || *end == 'P') {
                maxPixels = clamp(num, 0L, maxPixels);
            }
            if (*end == 0 || *end == 'b' || *end == 'B') {
                maxButtons = clamp(num, 0L, maxButtons);
            }
            if (*end == '%' && inrange(num, 0L, 100L)) {
                maxPercent = num;
                maxPixels = maxPercent * taskBarWidth / 100L;
                maxButtons = maxPixels * count() / non_zero(paneWidth);
            }
        }
    }
    if (isEmpty(str) || 0 == (maxPixels | maxButtons)) {
        if (taskBarDoubleHeight && taskBarWorkspacesTop) {
            maxPixels = taskBarWidth - x() - reserved;
        } else {
            maxPixels = (taskBarWidth - x() - reserved) / 2;
            maxButtons = 20;    // old limit
        }
    }


    long limit = 0;
    long activ = 0;
    for (int k = fActive, pass = 0, third = 0; true; ) {
        if (limit >= maxPixels && maxPixels > 0)
            break;
        if (activ >= maxButtons && maxButtons > 0)
            break;

        limit += fButtons[k]->width();
        activ += 1;

        if (pass == 0) {
            if (--k < 0 || fButtons[k]->extent() <= 0) {
                ++pass;
                third = k + 1;
                k = fActive;
            }
        }
        if (pass == 1) {
            if (++k >= count()) {
                ++pass;
                k = third;
            }
        }
        if (pass == 2) {
            if (--k < 0)
                break;
        }
    }

    return min(paneWidth, limit);
}

void WorkspacesPane::repositionButtons() {
    MSG(("WorkspacesPane::repositionButtons()"));
    fRepositioning = true;
    int width = 0;
    asmart<int> xs(new int[count()]);
    IterType wk = iterator();
    for (; ++wk; width += wk->width()) {
        if (wk->height() != height())
            wk->setSize(wk->width(), height());
        xs[wk.where()] = width + fMoved;
        if (xs[wk.where()] < wk->x()) {
            wk->setPosition(xs[wk.where()], 0);
        }
    }
    while (--wk) {
        if (wk->x() < xs[wk.where()]) {
            wk->setPosition(xs[wk.where()], 0);
        }
    }
    if (fReconfiguring == false)
        resize(width, height());
    fRepositioning = false;
}

void WorkspacesPane::relabelButtons() {
    for (IterType wk = iterator(); ++wk; )
        label(*wk);

    if ( !pagerShowPreview)
        repositionButtons();

    paths = null;
    repaint();
}

void WorkspacesPane::configure(const YRect2& r) {
    if ((fReconfiguring | fRepositioning) == false && r.resized()) {
        fReconfiguring = true;
        if (count() == 0) {
            unsigned width = 0, height = max(smallIconSize + 8, r.height());
            for (int i = 0, n = workspaceCount; i < n; ++i)
                width += create(i, height)->width();
            resize(width, height);
            setPressed(manager->activeWorkspace(), true);
            paths = null;
        }
        else {
            repositionButtons();
        }
        fReconfiguring = false;
    }
}

void WorkspacesPane::updateButtons() {
    MSG(("WorkspacesPane::udpateButtons(): updating %d -> %d",
         count(), int(workspaceCount)));

    if (count() > workspaceCount)
        fButtons.shrink(max<int>(1, workspaceCount));

    int width = extent() - fMoved;
    for (int i = count(), n = workspaceCount; i < n; ++i) {
        WorkspaceButton* wk = create(i, height());
        wk->setPosition(width + fMoved, 0);
        width += wk->width();
        if (wk->extent() > 0 && wk->x() < max(width, int(this->width())))
            wk->show();
    }
    resize(width, height());
    int limit = int(this->width());
    if (fMoved + width < limit) {
        fMoved = limit - width;
        repositionButtons();
    }

    paths = null;
}

WorkspaceButton* WorkspacesPane::create(int workspace, unsigned height) {
    WorkspaceButton *wk = new WorkspaceButton(workspace, this, this);
    fButtons += wk;
    if (pagerShowPreview) {
        unsigned dw = desktop->width();
        unsigned dh = desktop->height();
        unsigned scaled = (height * dw + (dh / 2)) / dh;
        wk->setSize(scaled, height);
        wk->updateName();
    } else {
        label(wk);
    }
    if (workspace == 0)
        wk->show();
    return wk;
}

WorkspaceIcons::WorkspaceIcons() {
    ref<YResourcePaths> dirs(YResourcePaths::subdirs("workspace", false));
    for (int i = 0; i < dirs->getCount(); ++i) {
        upath path(dirs->getPath(i) + "/workspace/");
        for (adir dir(path.string()); dir.next(); ) {
            files += (path + dir.entry()).string();
        }
    }
}

ref<YImage> WorkspaceIcons::load(const char* name) {
    int len = int(strlen(name));
    if (len < 1)
        return null;
    for (YStringArray::IterType iter = files.iterator(); ++iter; ) {
        const char* file = 1 + strrchr(*iter, '/');
        if (0 == strncmp(file, name, len)) {
            if (file[len] == 0 || file[len] == '.' || name[len - 1] == '.') {
                ref<YImage> image(YImage::load(*iter));
                if (image != null)
                    return image;
            }
        }
    }
    mstring trim(mstring(name).trim());
    return (trim.length() && trim != name) ? load(trim) : null;
}

void WorkspacesPane::label(WorkspaceButton* wk) {
    if ( !pagerShowPreview) {
        wk->setImage(paths->load(wk->name()));
        wk->setText(wk->name());
    }
    wk->updateName();
}

void WorkspacesPane::relabel(int ws) {
    label(fButtons[ws]);
    repositionButtons();
    paths = null;
}

void WorkspacesPane::setPressed(long ws, bool set) {
    if (inrange<long>(1+ws, 1, count())) {
        WorkspaceButton* wk = fButtons[int(ws)];
        wk->setPressed(set);
        if (set) {
            fActive = int(ws);
            if (wk->extent() > int(width())) {
                fMoved -= wk->extent() - int(width());
                repositionButtons();
            }
            else if (wk->x() < 0) {
                fMoved -= wk->x();
                repositionButtons();
            }
        }
    }
}

void WorkspacesPane::drag(int ws, int dx, bool start, bool end) {
    int previous = fMoved;
    int farthest = extent() - fButtons[0]->x();
    int lowerBound = min(0, int(width()) - farthest);
    int upperBound = 0;
    if (start) {
        fDelta = dx;
        fMoved = clamp(fMoved + dx, lowerBound, upperBound);
        fSpeed = 0;
        fTime = monotime();
    }
    else {
        if (fDelta != dx) {
            fMoved = clamp(fMoved + dx - fDelta, lowerBound, upperBound);
            fDelta = dx;
        }
        timeval now(monotime());
        if (fTime < now) {
            double speed = (fMoved - previous) / toDouble(now - fTime);
            fSpeed = (fSpeed + speed) * 0.5;
            fTime = now;
        }
    }
    if (fMoved != previous) {
        repositionButtons();
    }
    if (end) {
        fDragTimer->setTimer(fMillis, this, true);
    }
    else if (fDragTimer) {
        fDragTimer = null;
    }
}

bool WorkspacesPane::handleTimer(YTimer *t) {
    if (t == fDragTimer) {
        fDragTimer = null;
        int delta = int(trunc(fSpeed * fMillis / 1000.0));
        if (delta) {
            drag(0, fDelta + delta, false, true);
        }
    }
    else if (t == fRepaintTimer) {
        fRepaintTimer = null;
        if (fRepaintSpaces) {
            fRepaintSpaces = false;
            if (pagerShowPreview) {
                for (IterType wk = iterator(); ++wk; ) {
                    if (wk->visible() &&
                        wk->extent() > 0 &&
                        wk->x() < int(width()))
                    {
                        wk->repaint();
                    }
                }
            }
        }
    }
    return false;
}

void WorkspacesPane::stopDrag() {
    if (fDragTimer) {
        fDragTimer = null;
        fSpeed = 0;
    }
}

ref<YFont> WorkspaceButton::getActiveFont() {
    if (activeButtonFont == null) {
        if (*activeWorkspaceFontName || *activeWorkspaceFontNameXft) {
            activeButtonFont = YFont::getFont(XFA(activeWorkspaceFontName));
        }
        if (activeButtonFont == null) {
            activeButtonFont = YButton::getActiveFont();
        }
    }
    return activeButtonFont;
}

ref<YFont> WorkspaceButton::getFont() {
    if (isPressed()) {
        return getActiveFont();
    }
    else if (normalButtonFont == null) {
        if (*normalWorkspaceFontName || *normalWorkspaceFontNameXft) {
            normalButtonFont = YFont::getFont(XFA(normalWorkspaceFontName));
        }
        if (normalButtonFont == null) {
            normalButtonFont = YButton::getFont();
        }
    }
    return normalButtonFont;
}

YColor WorkspaceButton::getColor() {
    return isPressed()
        ? activeButtonFg ? activeButtonFg : YButton::getColor()
        : normalButtonFg ? normalButtonFg : YButton::getColor();
}

YSurface WorkspaceButton::getSurface() {
    return (isPressed()
            ? YSurface(activeButtonBg ? activeButtonBg : activeBackupBg,
                                   workspacebuttonactivePixmap,
                                   workspacebuttonactivePixbuf)
            : YSurface(normalButtonBg ? normalButtonBg : normalBackupBg,
                       workspacebuttonPixmap,
                       workspacebuttonPixbuf));
}

YDimension WorkspaceButton::getTextSize() {
    ref<YFont> font(getActiveFont());
    return YDimension(font->textWidth(name()), font->height());
}

const char* WorkspaceButton::name() const {
    return workspaces[fWorkspace];
}

mstring WorkspaceButton::baseName() {
    mstring name(my_basename(this->name()));
    name = name.trim();
    int dot = name.lastIndexOf('.');
    if (inrange(dot, 1, (int) name.length() - 2))
        name = name.substring(0, dot);
    return name;
}

void WorkspaceButton::updateName() {
    setToolTip(_("Workspace: ") + baseName());
}

void WorkspacesPane::repaint() {
    if (!pagerShowPreview) return;

    fRepaintSpaces = true;
    fRepaintTimer->setTimer(20L, this, true);
}

void WorkspaceButton::paint(Graphics &g, const YRect& r) {
    if (!pagerShowPreview) {
        YButton::paint(g, r);
        return;
    }

    int x(0), y(0), w(width()), h(height());

    if (w > 1 && h > 1) {
        YSurface surface(getSurface());
        g.setColor(surface.color);
        g.drawSurface(surface, x, y, w, h);

        if (pagerShowBorders) {
            x += 1; y += 1; w -= 2; h -= 2;
        }

        ref<YIcon> icon;
        YColor colors[] = {
            surface.color,
            surface.color.brighter(),
            surface.color.darker(),
            getColor(),
            YColor(), // getColor().brighter(),
            getColor().darker()
        };

        for (YFrameWindow *yfw = manager->bottomLayer(WinLayerBelow);
                yfw && yfw->getActiveLayer() <= WinLayerDock;
                yfw = yfw->prevLayer()) {
            if (yfw->isHidden() ||
                hasbit(yfw->frameOptions(),
                       YFrameWindow::foIgnoreWinList |
                       YFrameWindow::foIgnorePagerPreview)) {
                continue;
            }
            if (yfw->isAllWorkspaces() ?
                fWorkspace != manager->activeWorkspace() :
                fWorkspace != yfw->getWorkspace()) {
                continue;
            }
            int dw = int(desktop->width());
            int wx = x + (yfw->x() * w + (dw / 2)) / dw;
            int wy = y + (yfw->y() * w + (dw / 2)) / dw;
            int ww = (int(yfw->width()) * w + (dw / 2)) / dw;
            int wh = (int(yfw->height()) * w + (dw / 2)) / dw;
            if (ww <= 1 || wh <= 1)
                continue;
            if (yfw->isMaximizedVert()) { // !!! hack
                wy = y; wh = h;
            }
            if (yfw->isMinimized()) {
                if (!pagerShowMinimized)
                    continue;
                g.setColor(colors[2]);
            } else {
                if (ww > 2 && wh > 2) {
                    if (yfw->focused())
                        g.setColor(colors[1]);
                    else
                        g.setColor(colors[2]);
                    g.fillRect(wx+1, wy+1, ww-2, wh-2);

                    if (pagerShowWindowIcons &&
                        ww > 1 + int(smallIconSize) &&
                        wh > 1 + int(smallIconSize) &&
                        (icon = yfw->clientIcon()) != null &&
                        icon->small() != null)
                    {
                        g.drawImage(icon->small(),
                                    wx + (ww-smallIconSize)/2,
                                    wy + (wh-smallIconSize)/2);
                    }
                }
                g.setColor(colors[5]);
            }
            if (ww == 1 && wh == 1)
                g.drawPoint(wx, wy);
            else
                g.drawRect(wx, wy, ww-1, wh-1);
        }

        if (pagerShowBorders) {
            g.setColor(surface.color);
            g.draw3DRect(x-1, y-1, w+1, h+1, !isPressed());
        }

        char label[12] = {};
        if (pagerShowNumbers) {
            snprintf(label, sizeof label, "%d", int(fWorkspace+1) % 100);
        }
        else if (pagerShowLabels) {
            strlcpy(label, baseName(), min(5, int(sizeof label)));
        }
        if (label[0] != 0) {
            ref<YFont> font = getFont();

            int wx = (w - font->textWidth(label)) / 2 + x;
            int wy = (h - font->height()) / 2 + font->ascent() + y;

            g.setFont(font);
            g.setColor(colors[0]);
            g.drawChars(label, 0, strlen(label), wx+1, wy+1);
            g.setColor(colors[3]);
            g.drawChars(label, 0, strlen(label), wx, wy);
        }
        if (surface.gradient != null) {
            g.maxOpacity();
        }
    }
}

// vim: set sw=4 ts=4 et:

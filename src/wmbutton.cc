/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "wmframe.h"
#include "wmbutton.h"
#include "wmtitle.h"
#include "yxapp.h"
#include "yprefs.h"
#include "prefs.h"
#include "wpixmaps.h"
#include "intl.h"

static YColorName titleButtonBg(&clrNormalTitleButton);
static YColorName titleButtonFg(&clrNormalTitleButtonText);

inline YColor YFrameButton::background(bool active) {
    return YFrameTitleBar::background(active);
}

YFrameButton::YFrameButton(YFrameTitleBar* parent, char kind) :
    YButton(parent, actionNull),
    fKind(kind),
    fVisible(false)
{
    if (onRight())
        setWinGravity(NorthEastGravity);

    addStyle(wsNoExpose);
    addEventMask(VisibilityChangeMask);
    setParentRelative();

    setKind(kind);
}

YFrameButton::~YFrameButton() {
}

YFrameWindow *YFrameButton::getFrame() const {
    return reinterpret_cast<YFrameTitleBar*>(parent())->getFrame();
};

bool YFrameButton::focused() const {
    return getFrame()->focused();
}

bool YFrameButton::onRight() const {
    return (strchr(titleButtonsRight, fKind) != nullptr);
}

void YFrameButton::setKind(char kind) {
    const unsigned height = wsTitleBar;
    unsigned width;
    fKind = kind;
    if (kind == YFrameTitleBar::Menu) {
        if (LOOK(lookPixmap | lookMetal | lookGtk | lookFlat | lookMotif)
            && showFrameIcon)
            width = height;
        else {
            ref<YPixmap> pixmap(getPixmap(0));
            width = (pixmap == null) ? height : pixmap->width();
        }
    }
    else if (LOOK(lookPixmap | lookMetal | lookGtk | lookFlat)) {
        ref<YPixmap> pixmap(getPixmap(0));
        width = (pixmap == null) ? height : pixmap->width();
    }
    else if (wmLook == lookWin95) {
        width = height - 3;
    }
    else {
        width = height;
    }
    setSize(width, height);

    switch (kind) {
    case YFrameTitleBar::Depth:
        setAction(actionDepth);
        setToolTip(_("Raise/Lower"));
        setTitle("Lower");
        break;
    case YFrameTitleBar::Hide:
        setAction(actionHide);
        setToolTip(_("Hide"));
        setTitle("Hide");
        break;
    case YFrameTitleBar::Maxi:
        if (getFrame()->isMaximized()) {
            setAction(actionRestore);
            setToolTip(_("Restore"));
            setTitle("Restore");
        } else {
            setAction(actionMaximize);
            setToolTip(_("Maximize"));
            setTitle("Maximize");
        }
        break;
    case YFrameTitleBar::Mini:
        setAction(actionMinimize);
        setToolTip(_("Minimize"));
        setTitle("Minimize");
        break;
    case YFrameTitleBar::Roll:
        setAction(actionRollup);
        if (getFrame()->isRollup()) {
            setToolTip(_("Rolldown"));
            setTitle("Rolldown");
        } else {
            setToolTip(_("Rollup"));
            setTitle("Rollup");
        }
        break;
    case YFrameTitleBar::Menu:
        setAction(actionNull);
        setActionListener(getFrame());
        setPopup(getFrame()->windowMenu());
        setTitle("SysMenu");
        break;
    case YFrameTitleBar::Close:
        setAction(actionClose);
        setToolTip(_("Close"));
        setTitle("Close");
        break;
    }
}

void YFrameButton::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress &&
        getAction() != actionRollup &&
        (buttonRaiseMask & (1 << (button.button - Button1))))
    {
        if (!(button.state & ControlMask) && raiseOnClickButton) {
            getFrame()->activate();
            if (raiseOnClickButton && getAction() != actionDepth)
                getFrame()->wmRaise();
        }
    }
    YButton::handleButton(button);
}

void YFrameButton::handleClick(const XButtonEvent &up, int count) {
    if (getAction() == actionNull && up.button == Button1 && !(count % 2)) {
        setArmed(false, false);
        getFrame()->wmClose();
    }
    else if (up.button == Button3 && notbit(up.state, xapp->AltMask)) {
        if (!isPopupActive())
            getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
                                        YPopupWindow::pfCanFlipVertical |
                                        YPopupWindow::pfCanFlipHorizontal);
    }
    else if (getAction() == actionRollup && up.button == Button1) {
        actionPerformed(getAction(), up.state);
    }
    else {
        YButton::handleClick(up, count);
    }
}

void YFrameButton::handleBeginDrag(const XButtonEvent &down, const XMotionEvent &/*motion*/) {
    bool dragButton = inrange(int(down.button), Button2, Button3);
    if (dragButton && getFrame()->canMove()) {
        if (!isPopupActive()) {
            YFrameTitleBar* tbar = getFrame()->titlebar();
            getFrame()->startMoveSize(true, true,
                                      0, 0,
                                      down.x + x() + (tbar ? tbar->x() : 0),
                                      down.y + y() + (tbar ? tbar->y() : 0));
        }
    }
}

void YFrameButton::handleVisibility(const XVisibilityEvent& visib) {
    bool prev = fVisible;
    fVisible = (visib.state != VisibilityFullyObscured);
    if (prev < fVisible)
        repaint();
}

void YFrameButton::updatePopup() {
    getFrame()->updateMenu();
}

void YFrameButton::actionPerformed(YAction action, unsigned int modifiers) {
    if (hasbit(modifiers, ShiftMask)) {
        switch (action.ident()) {
            case actionMaximize: action = actionMaximizeVert; break;
            case actionMinimize: action = actionHide; break;
            case actionClose: action = actionKill; break;
            default: break;
        }
    }
    getFrame()->actionPerformed(action, modifiers);
}

ref<YPixmap> YFrameButton::getPixmap(int pn) const {
    if (fKind == YFrameTitleBar::Maxi) {
        if (getFrame()->isMaximized() && restorePixmap[pn] != null)
            return restorePixmap[pn];
        else
            return maximizePixmap[pn];
    }
    else if (fKind == YFrameTitleBar::Mini)
        return minimizePixmap[pn];
    else if (fKind == YFrameTitleBar::Close)
        return closePixmap[pn];
    else if (fKind == YFrameTitleBar::Hide)
        return hidePixmap[pn];
    else if (fKind == YFrameTitleBar::Roll)
        return getFrame()->isRollup() ? rolldownPixmap[pn] : rollupPixmap[pn];
    else if (fKind == YFrameTitleBar::Depth)
        return depthPixmap[pn];
    else if (fKind == YFrameTitleBar::Menu &&
             LOOK(lookPixmap | lookMetal | lookGtk | lookFlat | lookMotif))
        return menuButton[pn];
    else
        return null;
}

void YFrameButton::configure(const YRect2& r) {
    if (r.resized()) {
        repaint();
    }
}

void YFrameButton::repaint() {
    if (fVisible && visible() && width() > 1 && height() > 1) {
        GraphicsBuffer(this).paint();
    }
}

void YFrameButton::paint(Graphics &g, const YRect &/*r*/) {
    int xPos = 1, yPos = 1;
    int pn = LOOK(lookPixmap | lookMetal | lookGtk | lookFlat) && focused();
    const bool armed(isArmed());

    g.setColor(titleButtonBg);

    if (fOver && rolloverTitleButtons) {
        if (pn == 1) {
            pn = 2;
        } else {
            pn = 0;
        }
    }

    int iconSize = YIcon::smallSize();
    ref<YIcon> icon = (getAction() == actionNull)
                    ? getFrame()->clientIcon() : null;

    ref<YPixmap> pixmap = getPixmap(pn);
    if (pixmap == null && pn) {
        pixmap = getPixmap(0);
    }

    if (pixmap != null && pixmap->depth() != g.rdepth()) {
        tlog("YFrameButton::%s: attempt to use pixmap 0x%lx of depth %d with gc of depth %d\n",
                __func__, pixmap->pixmap(), pixmap->depth(), g.rdepth());
    }

    if (wmLook == lookWarp4) {
        g.fillRect(0, 0, width(), height());

        if (armed) {
            g.setColor(background(true));
            g.fillRect(1, 1, width() - 2, height() - 2);
        }

        if (getAction() == actionNull) {
            if (icon != null && showFrameIcon) {
                icon->draw(g,
                           (int(width()) - iconSize) / 2,
                           (int(height()) - iconSize
                            + MenuButtonIconVertOffset) / 2,
                           iconSize);
            }
        } else {
            if (pixmap != null)
                g.copyPixmap(pixmap, 0, armed ? 20 : 0,
                             pixmap->width(), pixmap->height() / 2,
                             (int(width()) - int(pixmap->width())) / 2,
                             (int(height()) - int(pixmap->height()) / 2));
        }
    }
    else if (LOOK(lookMotif | lookWarp3 | lookNice)) {
        g.draw3DRect(0, 0, width() - 1, height() - 1, !armed);

        if (wmLook != lookMotif) {
            if (armed) {
                xPos = 3;
                yPos = 3;
                g.drawLine(1, 1, width() - 2, 1);
                g.drawLine(1, 1, 1, height() - 2);
                g.drawLine(2, 2, width() - 3, 2);
                g.drawLine(2, 2, 2, height() - 3);
            } else {
                xPos = 2;
                yPos = 2;
                g.drawRect(1, 1, width() - 3, width() - 3);
            }
        }

        unsigned const w(LOOK(lookMotif) ? width() - 2 : width() - 4);
        unsigned const h(LOOK(lookMotif) ? height() - 2 : height() - 4);

        g.fillRect(xPos, yPos, w, h);
        if (getAction() == actionNull) {
            if (icon != null && showFrameIcon) {
                icon->draw(g,
                           xPos + (int(w) - iconSize) / 2,
                           yPos + (int(h) - iconSize
                               + MenuButtonIconVertOffset) / 2,
                           iconSize);
            }
            else if (pixmap != null) {
                g.drawCenteredPixmap(xPos, yPos, w, h, pixmap);
            }
        } else {
            if (pixmap != null)
                g.drawCenteredPixmap(xPos, yPos, w, h, pixmap);
        }
    }
    else if (wmLook == lookWin95) {
        if (getAction() == actionNull) {
            g.setColor(background(focused()));
            g.fillRect(0, 0, width(), height());
            if (icon != null && showFrameIcon) {
                icon->draw(g,
                           (int(width()) - iconSize) / 2,
                           (int(height()) - iconSize
                            + MenuButtonIconVertOffset) / 2,
                           iconSize);
            }
        } else {
            g.fillRect(0, 0, width(), height());
            g.drawBorderW(0, 0, width() - 1, height() - 1, !armed);

            if (armed)
                xPos = yPos = 2;

            if (pixmap != null)
                g.drawCenteredPixmap(xPos, yPos, width() - 3, height() - 3,
                                     pixmap);
        }
    }
    else if (LOOK(lookPixmap | lookMetal | lookFlat | lookGtk)) {
        if (pixmap != null && getPixmap(1) != null) {
            int const h(pixmap->height() / 2);
            g.copyPixmap(pixmap, 0, armed ? h : 0, pixmap->width(), h, 0, 0);
        } else {
            // If we have only an image we change
            // the over or armed color and paint it.
           g.fillRect(0, 0, width(), height());
           if (armed)
               g.setColor(background(true).darker());
           else if (rolloverTitleButtons && fOver)
               g.setColor(background(true).brighter());
           g.fillRect(1, 1, width()-2, height()-3);
           if (pixmap != null) {
               int x((int(width())  - int(pixmap->width()))  / 2);
               int y((int(height()) - int(pixmap->height())) / 2);
               g.drawPixmap(pixmap, x, y);
            }
        }

        if (getAction() == actionNull && icon != null && showFrameIcon) {
            icon->draw(g,
                       (int(width()) - iconSize) / 2,
                       (int(height()) - iconSize
                        + MenuButtonIconVertOffset) / 2,
                       iconSize);
        }
    }
}


void YFrameButton::paintFocus(Graphics &/*g*/, const YRect &/*r*/) {
}

// vim: set sw=4 ts=4 et:

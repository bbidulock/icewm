/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Windows/OS2 like Alt{+Shift}+Tab window switching
 */
#include "config.h"

#ifndef LITE
#include "ypixbuf.h"
#include "ykey.h"
#include "wmswitch.h"

#include "wmmgr.h"
#include "wmframe.h"
#include "yapp.h"
#include "prefs.h"

#include <string.h>

YColor *SwitchWindow::switchFg = 0;
YColor *SwitchWindow::switchBg = 0;
YColor *SwitchWindow::switchHl = 0;

YFont *SwitchWindow::switchFont = 0;

SwitchWindow * switchWindow = 0;

SwitchWindow::SwitchWindow(YWindow *parent):
    YPopupWindow(parent), fGradient(NULL) {
    if (switchBg == 0)
        switchBg = new YColor(clrQuickSwitch);
    if (switchFg == 0)
        switchFg = new YColor(clrQuickSwitchText);
    if (switchHl == 0 && clrQuickSwitchActive)
        switchHl = new YColor(clrQuickSwitchActive);
    if (switchFont == 0)
        switchFont = YFont::getFont(switchFontName);

    fActiveWindow = 0;
    fLastWindow = 0;
    modsDown = 0;
    isUp = false;

    resize();

    setStyle(wsSaveUnder | wsOverrideRedirect);
}

SwitchWindow::~SwitchWindow() {
    if (isUp) {
        cancelPopup();
        isUp = false;
    }
    
    delete fGradient;
}

void SwitchWindow::resize() {
    const char *cTitle(fActiveWindow ? fActiveWindow->client()->windowTitle()
				     : 0);

    int const iWidth
	(max(quickSwitchSmallWindow ? manager->width() * 1/3
				    : manager->width() * 3/5,
         max(cTitle ? switchFont->textWidth(cTitle) : 0,
	     fIconCount * (ICON_LARGE + 2 * quickSwitchIMargin) +
	    (quickSwitchHugeIcon ? ICON_HUGE - ICON_LARGE : 0))));
    int const mWidth(manager->width() * 6/7);
    int const iHeight((quickSwitchHugeIcon ? ICON_HUGE : ICON_LARGE) +
		       quickSwitchIMargin * 2);

    int const w((quickSwitchAllIcons && iWidth < mWidth
		? iWidth : mWidth) + quickSwitchHMargin * 2);
    int const h((quickSwitchAllIcons
		? iHeight + switchFont->height() + quickSwitchSepSize
		: max(iHeight, (int)switchFont->height()))
		+ quickSwitchVMargin * 2);

    setGeometry((manager->width() - w) >> 1,
    		(manager->height() - h) >> 1, w, h);
}

void SwitchWindow::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    if (switchbackPixbuf && !(fGradient &&
			      fGradient->width() == width() - 2 &&
			      fGradient->height() == height() - 2)) {
	delete fGradient;
	fGradient = new YPixbuf(*switchbackPixbuf, width() - 2, height() - 2);
    }

    g.setColor(switchBg);
    g.drawBorderW(0, 0, width() - 1, height() - 1, true);
    
    if (fGradient)
        g.copyPixbuf(*fGradient, 1, 1, width() - 2, height() - 2, 1, 1);
    else if (switchbackPixmap)
        g.fillPixmap(switchbackPixmap, 1, 1, width() - 3, height() - 3);
    else
        g.fillRect(1, 1, width() - 3, height() - 3);

    if (fActiveWindow) {
        int tOfs(0);

	const int ih(quickSwitchHugeIcon ? ICON_HUGE : ICON_LARGE);

        if (!quickSwitchAllIcons &&
	    fActiveWindow->clientIcon()) {
	    YPixmap * icon(quickSwitchHugeIcon
		? fActiveWindow->clientIcon()->huge()
		: fActiveWindow->clientIcon()->large());

	    if (icon)
		if (quickSwitchTextFirst) {
		    g.drawPixmap(icon,
				 width() - icon->width() - quickSwitchIMargin, 
				 (height() - icon->height() - 
				  quickSwitchIMargin) / 2);
		} else {
		    g.drawPixmap(icon, quickSwitchIMargin,
				 (height() - icon->height() - 
				  quickSwitchIMargin) / 2);

		    tOfs = icon->width() + quickSwitchIMargin
		         + quickSwitchSepSize;
		}
		
		if (quickSwitchSepSize) {
		    const int ip(icon->width() + 2 * quickSwitchIMargin +
		    		 quickSwitchSepSize/2);
		    const int x(quickSwitchTextFirst ? width() - ip : ip);
		    
		    g.setColor(switchBg->darker());
		    g.drawLine(x + 0, 1, x + 0, width() - 2);
		    g.setColor(switchBg->brighter());
		    g.drawLine(x + 1, 1, x + 1, width() - 2);
		}
	}

        g.setColor(switchFg);
        g.setFont(switchFont);

        const char *cTitle(fActiveWindow->client()->windowTitle());

        if (cTitle) {
	    const int x(max((width() - tOfs - 
			     switchFont->textWidth(cTitle)) >> 1, 0U) + tOfs);
	    const int y(quickSwitchAllIcons 	
	    	      ? quickSwitchTextFirst
		      ? quickSwitchVMargin + switchFont->ascent()
		      : height() - quickSwitchVMargin - switchFont->descent()
		      : ((height() + switchFont->height()) >> 1) -
		        switchFont->descent());

            g.drawChars(cTitle, 0, strlen(cTitle), x, y);
	    
	    if (quickSwitchAllIcons && quickSwitchSepSize) {
		int const h(quickSwitchVMargin + ih + 
			    quickSwitchIMargin * 2 + 
			    quickSwitchSepSize / 2);
		int const y(quickSwitchTextFirst ? height() - h : h);

		g.setColor(switchBg->darker());
		g.drawLine(1, y + 0, width() - 2, y + 0);
		g.setColor(switchBg->brighter());
		g.drawLine(1, y + 1, width() - 2, y + 1);
	    }
        }
	
	if (quickSwitchAllIcons) {
	    int const ds(quickSwitchHugeIcon ? ICON_HUGE - ICON_LARGE : 0);
	    int const dx(ICON_LARGE + 2 * quickSwitchIMargin);

	    const int visIcons((width() - 2 * quickSwitchHMargin) / dx);
	    int curIcon(-1); fIconCount = 0;

	    YFrameWindow * first(nextWindow(NULL, true, false));
	    YFrameWindow * frame(first);
	    
	    do {
		if (frame == fActiveWindow) curIcon = fIconCount;
		++fIconCount;
	    } while ((frame = nextWindow(frame, true, true)) != first);


	    int const y(quickSwitchTextFirst
		? height() - quickSwitchVMargin - ih - quickSwitchIMargin + ds / 2
		: quickSwitchVMargin + ds + quickSwitchIMargin - ds / 2);

	    g.setColor(switchHl ? switchHl : switchBg->brighter());

	    const int off(max(1 + curIcon - visIcons, 0));
	    const int end(off + visIcons);

	    int x((width() - min(visIcons, fIconCount) * dx - ds) /  2 + 
	    	   quickSwitchIMargin);
	    int i(0);
	    
	    do {
	    	if (frame->clientIcon()) {
		    if (i >= off && i < end) {
			if (frame == fActiveWindow) {
			    if (quickSwitchFillSelection)
				g.fillRect(x - quickSwitchIBorder,
					   y - quickSwitchIBorder - ds/2, 
					   ICON_HUGE + 2 * quickSwitchIBorder,
					   ICON_HUGE + 2 * quickSwitchIBorder);
			    else
				g.drawRect(x - quickSwitchIBorder,
					   y - quickSwitchIBorder - ds/2, 
					   ICON_HUGE + 2 * quickSwitchIBorder,
					   ICON_HUGE + 2 * quickSwitchIBorder);

			    YPixmap const * icon(frame->clientIcon()->huge());
			    if (icon) g.drawPixmap(icon, x, y - ds/2);
			    x+= ds;
			} else {
			    YPixmap const * icon(frame->clientIcon()->large());
			    if (icon) g.drawPixmap(icon, x, y);
			}

			x+= dx;
		    }

		    ++i;
		}
	    } while ((frame = nextWindow(frame, true, true)) != first);
	}
    }
}

YFrameWindow *SwitchWindow::nextWindow(YFrameWindow *from, bool zdown, bool next) {
    if (from == 0) {
        next = false;
        from = zdown ? manager->topLayer() : manager->bottomLayer();
    }
    int flags =
        YFrameWindow::fwfFocusable |
        (quickSwitchToAllWorkspaces ? 0 : YFrameWindow::fwfWorkspace) |
        YFrameWindow::fwfLayers |
        YFrameWindow::fwfSwitchable |
        (next ? YFrameWindow::fwfNext: 0) |
        (zdown ? 0 : YFrameWindow::fwfBackward);

    YFrameWindow *n = 0;

    if (from == 0)
        return 0;

    if (zdown) {
        n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
        if (n == 0 && quickSwitchToMinimized)
            n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
        if (n == 0 && quickSwitchToHidden)
            n = from->findWindow(flags | YFrameWindow::fwfHidden);
        if (n == 0) {
            flags |= YFrameWindow::fwfCycle;
            n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
            if (n == 0 && quickSwitchToMinimized)
                n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
            if (n == 0 && quickSwitchToHidden)
                n = from->findWindow(flags | YFrameWindow::fwfHidden);
        }
    } else {
        if (n == 0 && quickSwitchToHidden)
            n = from->findWindow(flags | YFrameWindow::fwfHidden);
        if (n == 0 && quickSwitchToMinimized)
            n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
        if (n == 0)
            n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
        if (n == 0) {
            flags |= YFrameWindow::fwfCycle;
            if (n == 0 && quickSwitchToHidden)
                n = from->findWindow(flags | YFrameWindow::fwfHidden);
            if (n == 0 && quickSwitchToMinimized)
                n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
            if (n == 0)
                n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
        }
    }
    if (n == 0)
        n = from->findWindow(flags |
                             YFrameWindow::fwfVisible |
                             (quickSwitchToMinimized) ? 0 : YFrameWindow::fwfUnminimized |
                             YFrameWindow::fwfSame);
    if (n == 0)
        n = fLastWindow;

    return n;
}

void SwitchWindow::begin(bool zdown, int mods) {
    modsDown = mods & (app->AltMask | app->MetaMask | 
    		       app->HyperMask | app->SuperMask | 
		       ControlMask);

    if (isUp) {
        cancelPopup();
        isUp = false;
    } else {
	fLastWindow = fActiveWindow = manager->getFocus();
	fActiveWindow = nextWindow(fLastWindow, zdown, true);

	if (fActiveWindow &&
	   (!fActiveWindow->isFocusable() ||
	    !(quickSwitchToAllWorkspaces || fActiveWindow->visibleNow()) ||
	    (fActiveWindow->frameOptions() & YFrameWindow::foIgnoreQSwitch) ||
	    (!quickSwitchToMinimized && fActiveWindow->isMinimized()) ||
	    (!quickSwitchToHidden && fActiveWindow->isHidden()))) {
	    fActiveWindow = NULL;
	    app->alert();
	}

	fIconCount = fIconOffset = 0;

	if (quickSwitchAllIcons && fActiveWindow) {
	    YFrameWindow * frame(fActiveWindow); do {
	    	if (fActiveWindow->clientIcon() && 
		    fActiveWindow->clientIcon()->large())
		    ++fIconCount;
	    } while ((frame = nextWindow(frame, zdown, true)) != fActiveWindow);
	}

	MSG(("fIconCount: %d, fIconOffset: %d", fIconCount, fIconOffset));

	resize();

	if (fActiveWindow) {
	    displayFocus(fActiveWindow);
	    isUp = popup(0, 0, YPopupWindow::pfNoPointerChange);
	}
    }
}

void SwitchWindow::activatePopup() {
}

void SwitchWindow::deactivatePopup() {
}

void SwitchWindow::cancel() {
    if (isUp) {
        cancelPopup();
        isUp = false;
    }
    if (fLastWindow) {
        displayFocus(fLastWindow);
    } else if (fActiveWindow) {
        if (!quickSwitchToMinimized &&
            fActiveWindow->isMinimized())
        {
            // !!! workaround, because the window is actually focused...
        } else {
            manager->activate(fActiveWindow, true);
            fActiveWindow->wmRaise();
        }
    }
    fLastWindow = fActiveWindow = 0;
}

void SwitchWindow::accept() {
    if (fActiveWindow == 0)
        cancel();
    else {
        if (!quickSwitchToMinimized &&
            fActiveWindow->isMinimized())
        {
            // !!! workaround, because the window is actually focused...
        } else {
            manager->activate(fActiveWindow, true);
            fActiveWindow->wmRaise();
        }
        if (isUp) {
            cancelPopup();
            isUp = false;
        }
        //manager->activate(fActiveWindow, true);
    }
    fLastWindow = fActiveWindow = 0;
}

void SwitchWindow::displayFocus(YFrameWindow *frame) {
    manager->switchFocusTo(frame);
    repaint();
}

void SwitchWindow::destroyedFrame(YFrameWindow *frame) {
    if (frame == fLastWindow)
        fLastWindow = 0;
    if (frame == fActiveWindow) {
        fActiveWindow = nextWindow(0, true, false);
        displayFocus(fActiveWindow);
    }
}

bool SwitchWindow::handleKey(const XKeyEvent &key) {
    KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    unsigned int m = KEY_MODMASK(key.state);
    unsigned int vm = VMod(m);

    if (key.type == KeyPress) {
        if ((IS_WMKEY(k, vm, gKeySysSwitchNext))) {
            fActiveWindow = nextWindow(fActiveWindow, true, true);
            displayFocus(fActiveWindow);
            return true;
        } else if ((IS_WMKEY(k, vm, gKeySysSwitchLast))) {
            fActiveWindow = nextWindow(fActiveWindow, false, true);
            displayFocus(fActiveWindow);
            return true;
        } else if (k == XK_Escape) {
            cancel();
            return true;
        }
        if ((IS_WMKEY(k, vm, gKeySysSwitchNext)) && !modDown(m)) {
            accept();
            return true;
        }
    } else if (key.type == KeyRelease) {
        if ((IS_WMKEY(k, vm, gKeySysSwitchNext)) && !modDown(m)) {
            accept();
            return true;
        } else if (isModKey(key.keycode)) {
            accept();
            return true;
        }
    }
    return YPopupWindow::handleKey(key);
}

bool SwitchWindow::isModKey(KeyCode c) {
    KeySym k = XKeycodeToKeysym(app->display(), c, 0);

    if (k == XK_Control_L || k == XK_Control_R ||
        k == XK_Alt_L     || k == XK_Alt_R     ||
        k == XK_Meta_L    || k == XK_Meta_R    ||
        k == XK_Super_L   || k == XK_Super_R   ||
        k == XK_Hyper_L   || k == XK_Hyper_R)
        return true;

    return false;
}

bool SwitchWindow::modDown(int mod) {
    int m = mod & (app->AltMask | app->MetaMask | app->HyperMask | app->SuperMask | ControlMask);

    if ((m & modsDown) != modsDown)
        return false;
    return true;
}

void SwitchWindow::handleButton(const XButtonEvent &button) {
    YPopupWindow::handleButton(button);
}
#endif

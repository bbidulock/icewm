#include "config.h"

#ifdef CONFIG_TASKBAR

#include "ylib.h"
#include "aworkspaces.h"
#include "wmtaskbar.h"
#include "prefs.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "wmframe.h"

YColor * WorkspaceButton::activeButtonBg(NULL);
YColor * WorkspaceButton::normalButtonBg(NULL);

YPixmap *workspacebuttonPixmap(NULL);
YPixmap *workspacebuttonactivePixmap(NULL);

#ifdef CONFIG_GRADIENTS
class YPixbuf *workspacebuttonPixbuf(NULL);
class YPixbuf *workspacebuttonactivePixbuf(NULL);
#endif

WorkspaceButton::WorkspaceButton(long ws, YWindow *parent): YButton(parent, 0)
{
    fWorkspace = ws;
    //setDND(true);
}

void WorkspaceButton::handleClick(const XButtonEvent &/*up*/, int /*count*/) {
}

void WorkspaceButton::handleDNDEnter() {
    if (fRaiseTimer == 0)
        fRaiseTimer = new YTimer(autoRaiseDelay);
    if (fRaiseTimer) {
        fRaiseTimer->setTimerListener(this);
        fRaiseTimer->startTimer();
    }
    repaint();
}

void WorkspaceButton::handleDNDLeave() {
    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
    repaint();
}

bool WorkspaceButton::handleTimer(YTimer *t) {
    if (t == fRaiseTimer) {
        manager->activateWorkspace(fWorkspace);
    }
    return false;
}

void WorkspaceButton::actionPerformed(YAction */*action*/, unsigned int modifiers) {
    if (modifiers & ShiftMask) {
        manager->switchToWorkspace(fWorkspace, true);
    } else if (modifiers & app->AltMask) {
        if (manager->getFocus())
            manager->getFocus()->wmOccupyWorkspace(fWorkspace);
    } else {
        manager->activateWorkspace(fWorkspace);
        return;
    }
}

WorkspacesPane::WorkspacesPane(YWindow *parent): YWindow(parent) {
    long w;

    if (workspaceCount > 1)
        fWorkspaceButton = new WorkspaceButton *[workspaceCount];
    else
        fWorkspaceButton = 0;

    if (fWorkspaceButton) {
        int ht = 0;
        int leftX = 0;
        for (w = 0; w < workspaceCount; w++) {
            WorkspaceButton *wk = new WorkspaceButton(w, this);
            if (wk) {
                wk->setText(workspaceNames[w]);
                if ((int)wk->height() + 1 > ht) ht = wk->height() + 1;
            }
            fWorkspaceButton[w] = wk;
        }
        for (w = 0; w < workspaceCount; w++) {
            YButton *wk = fWorkspaceButton[w];
            //leftX += 2;
            if (wk) {
                wk->setSize(wk->width(), ht);
                wk->setPosition(leftX, 0); // + (ht - ADD - wk->height()) / 2);
                wk->show();
                leftX += wk->width();
            }
        }
        setSize(leftX, ht);
    }
}

WorkspacesPane::~WorkspacesPane() {
    if (fWorkspaceButton) {
        for (long w = 0; w < workspaceCount; w++)
            delete fWorkspaceButton[w];
        delete [] fWorkspaceButton;
    }
}

WorkspaceButton *WorkspacesPane::workspaceButton(long n) {
    if (!fWorkspaceButton)
        return 0;
    return fWorkspaceButton[n];
}

YSurface WorkspaceButton::getSurface() {
    if (activeButtonBg == 0)
        activeButtonBg = new YColor(*clrWorkspaceActiveButton
			? clrWorkspaceActiveButton : clrActiveButton);
    if (normalButtonBg == 0)
        normalButtonBg = new YColor(*clrWorkspaceNormalButton
			? clrWorkspaceNormalButton : clrNormalButton);

#ifdef CONFIG_GRADIENTS    
    return (isPressed() ? YSurface(activeButtonBg, 
				   workspacebuttonactivePixmap, 
				   workspacebuttonactivePixbuf)
    		        : YSurface(normalButtonBg,
		     		   workspacebuttonPixmap, 
				   workspacebuttonPixbuf));
#else		     
    return (isPressed() ? YSurface(activeButtonBg, workspacebuttonactivePixmap)
    		        : YSurface(normalButtonBg, workspacebuttonPixmap));
#endif
}

#endif

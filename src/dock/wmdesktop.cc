#include "config.h"
#include "yfull.h"
#include "yapp.h"
#include "ywindow.h"
#include "atasks.h"

#include "default.h"
#include "wmdesktop.h"
#include "ycstring.h"
#include <stdio.h>

WindowInfo::WindowInfo(Window w) {
    fHandle = w;
    fMarked = false;
    fOwner = 0;
    fWinListItem = 0;

    fWindowTitle = 0;
    fIconTitle = 0;
}

const CStr *WindowInfo::getTitle() {
    getNameHint();
    return fWindowTitle;
}

const CStr *WindowInfo::getIconTitle() {
    getIconNameHint();
    return fIconTitle;
}

void WindowInfo::setWindowTitle(const char *aWindowTitle) {
    delete fWindowTitle;
    fWindowTitle = CStr::newstr(aWindowTitle);
    //if (getFrame())
    //    getFrame()->updateTitle();
}

#ifdef CONFIG_I18N
void WindowInfo::setWindowTitle(XTextProperty  *prop) {
    Status status;
    char **cl;
    int n;
    if (!prop->value || prop->encoding == XA_STRING) {
        setWindowTitle((const char *)prop->value);
        return;
    }
    status = XmbTextPropertyToTextList (app->display(), prop, &cl, &n);
    if (status >= Success && n > 0 && cl[0]) {
        setWindowTitle((const char *)cl[0]);
        XFreeStringList(cl);
    } else {
        setWindowTitle((const char *)prop->value);
    }
}
#endif

void WindowInfo::setIconTitle(const char *aIconTitle) {
    delete fIconTitle;
    fIconTitle = CStr::newstr(aIconTitle);
    //if (getFrame())
    //    getFrame()->updateIconTitle();
}

#ifdef CONFIG_I18N
void WindowInfo::setIconTitle(XTextProperty  *prop) {
    Status status;
    char **cl;
    int n;
    if (!prop->value || prop->encoding == XA_STRING) {
        setIconTitle((const char *)prop->value);
        return;
    }
    status = XmbTextPropertyToTextList (app->display(), prop, &cl, &n);
    if (status >= Success && n > 0 && cl[0]) {
        setIconTitle((const char *)cl[0]);
        XFreeStringList(cl);
    } else {
        setIconTitle((const char *)prop->value);
    }
}
#endif

void WindowInfo::getNameHint() {
    XTextProperty prop;

    if (XGetWMName(app->display(), handle(), &prop)) {
#ifdef CONFIG_I18N
        if (true /*multiByte*/) {
            setWindowTitle(&prop);
        } else
#endif
        {
            setWindowTitle((char *)prop.value);
        }
        if (prop.value) XFree(prop.value);
    } else {
        setWindowTitle((const char*)0);
    }
}

void WindowInfo::getIconNameHint() {
    XTextProperty prop;

    if (XGetWMIconName(app->display(), handle(), &prop)) {
#ifdef CONFIG_I18N
        if (true /*multiByte*/) {
            setIconTitle(&prop);
        } else
#endif
        {
            setIconTitle((char *)prop.value);
        }
        if (prop.value) XFree(prop.value);
    } else {
        setIconTitle((const char *)0);
    }
}

DesktopInfo::DesktopInfo(YWindow *parent, Window win): YDesktop(parent, win) {
    wmContext = XUniqueContext();
    fTasks = 0;
}

DesktopInfo::~DesktopInfo() {
}

void DesktopInfo::setTaskPane(TaskPane *tasks) {
    if (fTasks && tasks)
        return ;
    fTasks = tasks;
    if (fTasks) {
        updateTasks();
    }
}

void DesktopInfo::updateTasks() {
#ifdef GNOME_HINTS
    Atom type;
    int format;
    unsigned long nitems, lbytes;
    unsigned char *propdata;

    printf("_XA_WIN_CLIENT_LIST: %ld\n", _XA_WIN_CLIENT_LIST);
    // unmark all here
    if (XGetWindowProperty(app->display(), desktop->handle(),
                           _XA_WIN_CLIENT_LIST, 0, 4096, False, None,
                           &type, &format, &nitems, &lbytes,
                           &propdata) == Success && propdata)
    {
        long *w = (long *)propdata;

        printf("%ld\n", nitems);
        for (unsigned int i = 0; i < nitems; i++) {
            WindowInfo *wi = getInfo(w[i]);
            wi->mark(true);


        }
    }
    // delete unmarked here
    fTasks->relayoutNow();
#endif
}

WindowInfo *DesktopInfo::getInfo(Window w) {
    WindowInfo *wi;

    if (XFindContext(app->display(), w, wmContext,
                     (XPointer *)&wi) != 0)
    {
        wi = new WindowInfo(w);
        if (wi == 0)
            return 0;
        XSaveContext(app->display(), w, wmContext, (XPointer)wi);

        /*!!!???TaskBarApp *ta =*/ fTasks->addApp(wi);
        fTasks->relayout();
    }
    return wi;
}

void DesktopInfo::handleProperty(const XPropertyEvent &property) {
#ifdef GNOME_HINTS
    if (property.atom == _XA_WIN_CLIENT_LIST) {
        updateTasks();
    }
#endif
}

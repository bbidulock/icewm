#ifndef __YXCONTEXT_H
#define __YXCONTEXT_H

#ifndef NULLQUARK
#include <X11/Xresource.h>
#endif

class YAnyContext {
protected:
    typedef void* AnyPointer;

private:
    XContext unique;
    const char* title;
    const bool verbose;

    XContext context() {
        if (unique == 0) {
            unique = XUniqueContext();
            if (verbose) {
                tlog("%s: created", title);
            }
        }
        return unique;
    }

    Display* dpy() const { return xapp->display(); }

public:
    YAnyContext(const char* title = 0, bool verbose = false) :
        unique(0),
        title(title),
        verbose(verbose)
    {
    }

    ~YAnyContext() {
        if (verbose) {
            tlog("%s: destroyed", title);
        }
    }

    // store mapping of window to pointer
    void save(Window w, AnyPointer p) {
        const char* q = (const char *) p;
        XSaveContext(dpy(), w, context(), q);
        if (verbose) {
            tlog("%s: save 0x%lx to %p", title, w, p);
        }
    }

    // lookup pointer by window
    bool find(Window w, AnyPointer* p) {
        char* q = 0;
        int rc = XFindContext(dpy(), w, context(), &q);
        if (verbose) {
            if (rc == 0)
                tlog("%s: find 0x%lx found %p", title, w, p);
            else
                tlog("%s: find 0x%lx not found", title, w);
        }
        *p = q;
        return rc == 0;
    }

    // remove mapping of window to pointer
    bool remove(Window w) {
        int rc = XDeleteContext(dpy(), w, context());
        if (verbose) {
            if (rc == 0)
                tlog("%s: remove for 0x%lx", title, w);
            else
                tlog("%s: remove for 0x%lx failed", title, w);
        }
        return rc == 0;
    }
};

template <typename T>
class YContext : private YAnyContext {
    typedef T* TPtr;
public:
    YContext(const char* title = 0, bool verbose = false) :
        YAnyContext(title, verbose) { }

    // store mapping of window to pointer
    void save(Window w, TPtr p) {
        YAnyContext::save(w, AnyPointer(p));
    }

    // lookup pointer by window
    bool find(Window w, TPtr* ptr) {
        AnyPointer p = 0;
        if (YAnyContext::find(w, &p)) {
            *ptr = TPtr(p);
            return true;
        }
        return false;
    }

    // lookup pointer by window
    TPtr find(Window w) {
        AnyPointer p = 0;
        YAnyContext::find(w, &p);
        return TPtr(p);
    }

    // remove mapping of window to pointer
    bool remove(Window w) {
        return YAnyContext::remove(w);
    }
};

class YFrameClient;
class YFrameWindow;
class YWindow;

extern YContext<YFrameClient> clientContext;
extern YContext<YFrameWindow> frameContext;
extern YContext<YWindow> windowContext;

#endif

// vim: set sw=4 ts=4 et:

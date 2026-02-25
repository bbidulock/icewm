#include "config.h"
#include "wmframe.h"
#include "preview.h"
#include "wmmgr.h"
#include "yxapp.h"
#include "prefs.h"
#include "yprefs.h"
#include "keysyms.h"
#include <X11/Xatom.h>
#include <ctype.h>

namespace {
   int NoteSize = 256;
}
#define NoteHeight  NoteSize
#define NoteWidth   NoteSize

Preview::Preview(YFrameClient* client, YFrameWindow* frame,
                 SwitchPreview* parent) :
    client(client),
    frame(frame),
    parent(parent),
    picture(None),
    source(None),
    x(0), y(0),
    priority(0),
    letter(0),
    damaged(false),
    updated(false)
{
    int leading = parent->font()->height();
    p = quickSwitchHMargin;
    w = NoteWidth + 2 * p;
    q = quickSwitchVMargin + 2 * leading + quickSwitchIMargin;
    h = q + NoteHeight + quickSwitchVMargin;
    pixmap = xapp->createPixmap(w, h, xapp->depth());
    damage = XDamageCreate(xapp->display(), client->handle(),
                           XDamageReportNonEmpty);

    picture = xapp->createPicture(pixmap, xapp->format());
    XRenderColor color = parent->background.color();
    XRenderFillRectangle(xapp->display(), PictOpSrc, picture,
                         &color, 0, 0, w, h);
}

Preview::~Preview() {
    if (picture)
        xapp->freePicture(picture);
    if (source)
        xapp->freePicture(source);
    if (pixmap)
        xapp->freePixmap(pixmap);
    if (damage)
        XDamageDestroy(xapp->display(), damage);
}

void Preview::draw(SwitchPreview* parent, bool active) {
    Drawable dest = parent->handle();
    YFont font = parent->switchFont;
    Graphics g(pixmap, w, h, xapp->depth());
    g.setFont(font);

    XRenderColor color = active ? parent->activeFlat
                       ? parent->activeFlat.color()
                       : parent->background.color().brighter()
                       : parent->background.color();
    if (updated) {
        XRenderFillRectangle(xapp->display(), PictOpSrc, picture, &color,
                             0, 0, w, q);
        XRenderFillRectangle(xapp->display(), PictOpSrc, picture, &color,
                             0, q, p, NoteHeight);
        XRenderFillRectangle(xapp->display(), PictOpSrc, picture, &color,
                             p + NoteWidth, q, w - p - NoteWidth, NoteHeight);
        XRenderFillRectangle(xapp->display(), PictOpSrc, picture, &color,
                             0, q + NoteHeight, w, h - q - NoteHeight);
    } else {
        XRenderFillRectangle(xapp->display(), PictOpSrc, picture, &color,
                             0, 0, w, h);
        ref<YIcon> icon = client->getIcon();
        unsigned meta = 128;
        if (icon != null && hugeIconSize < meta && client->haveMetaIcon()) {
            const unsigned sizes[] = { 3 * meta, 2 * meta, 3 * meta / 2, meta };
            unsigned size = meta;
            for (unsigned k : sizes) {
                if (k < unsigned(NoteSize)) {
                    size = k;
                    break;
                }
            }
            if (size != icon->getOtherSize()) {
                ref<YImage> image = client->getMetaIcon();
                if (image != null && size != image->width()) {
                    image = image->scale(size, size);
                }
                if (image != null && size == image->width()) {
                    ref<YPixmap> pixmap = image->renderToPixmap(32, true);
                    if (pixmap != null) {
                        Picture picture = pixmap->picture();
                        if (picture) {
                            icon->setOther(picture, size);
                            pixmap->forgetPicture();
                        }
                    }
                }
            }
            if (size == icon->getOtherSize()) {
                icon->draw(g, p + (NoteSize - size) / 2,
                           q + (NoteSize - size) / 2, size);
                icon = null;
            }
        }
        if (icon != null) {
            const int hu = hugeIconSize, la = largeIconSize, sm = smallIconSize;
            const int sz[] = { 3 * hu, 2 * hu, hu, la, sm, NoteSize / 2 };
            for (int k : sz) {
                if (k < NoteSize) {
                    icon->draw(g, p + (NoteSize - k) / 2,
                               q + (NoteSize - k) / 2, k);
                    break;
                }
            }
        }
    }
    g.setColor(active ? parent->activeText ? parent->activeText :
               parent->foreground.brighter() : parent->foreground);

    int y = quickSwitchVMargin + font->ascent();
    ClassHint* ch = client->classHint();
    char title[80] = "";
    if (ch && nonempty(ch->res_class))
        strlcpy(title, ch->res_class, sizeof(title));
    else if (ch && nonempty(ch->res_name))
        strlcpy(title, ch->res_name, sizeof(title));
    else {
        YProperty cmd(client, _XA_WM_COMMAND, F8, 80, XA_STRING);
        if (cmd) {
            strlcpy(title, my_basename(cmd.data<char>()), sizeof(title));
        }
    }
    for (int i = 0; title[i] && isupper((unsigned char) title[i]); ++i) {
        title[i] = tolower((unsigned char) title[i]);
    }
    letter = title[0];

    if (nonempty(title))
        g.drawStringEllipsis(p, y, title, NoteWidth);
    y += font->height();

    mstring copy(client->windowTitle());
    strlcpy(title, copy, sizeof(title));
    if (nonempty(title))
        g.drawStringEllipsis(p, y, title, NoteWidth);

    XCopyArea(xapp->display(), pixmap, dest, g.handleX(),
              0, 0, w, h, x, this->y);
}

void Preview::notify() {
    damaged = true;
}

void Preview::subtract() {
    if (damaged) {
        damaged = false;
        XDamageSubtract(xapp->display(), damage, None, None);
    }
}

void Preview::update() {
    if (client->visible() && frame->visible() && frame->client() == client) {
        if (source == None) {
            XRenderPictureAttributes attr;
            attr.repeat = RepeatNormal;
            unsigned long mask = CPRepeat;
            source = XRenderCreatePicture(xapp->display(), frame->handle(),
                                          frame->format(), mask, &attr);
        }
        if (picture == None)
            picture = xapp->createPicture(pixmap, xapp->format());

        XRenderColor color = parent->background.color();
        XRenderFillRectangle(xapp->display(), PictOpSrc, picture,
                             &color, p, q, NoteWidth, NoteHeight);

        unsigned dim = max(frame->width(), frame->height());
        double factor = double(dim) / double(NoteSize);
        XTransform transform = { {
            { XDoubleToFixed(factor), 0, 0 },
            { 0, XDoubleToFixed(factor), 0 },
            { 0, 0, XDoubleToFixed(1) }
        } };
        XRenderSetPictureTransform(xapp->display(), source, &transform);
        XRenderSetPictureFilter(xapp->display(), source, FilterBilinear,
                                nullptr, 0);

        int dw = frame->width() * NoteSize / dim;
        int dh = frame->height() * NoteSize / dim;
        int dx = p + (NoteSize - dw) / 2;
        int dy = q + (NoteSize - dh) / 2;
        XRenderComposite(xapp->display(), PictOpSrc, source, None, picture,
                         0, 0, 0, 0, dx, dy, dw, dh);
        updated = true;
        subtract();
    }
    // else tlog("hiding when invisible");
}

void Preview::prioritize(const char* wmclass) {
    priority = 0;

    if (hasbit(client->winHints(), WinHintsSkipFocus))
        return;

    if (!client->adopted() && !frame->visible())
        return;

    if (!quickSwitchToAllWorkspaces &&
        !frame->visibleOn(manager->activeWorkspace()) &&
        !frame->hasState(WinStateUrgent) &&
        !client->urgencyHint())
        return;

    if (frame->frameOption(YFrameWindow::foIgnoreQSwitch) ||
        client->frameOption(YFrameWindow::foIgnoreQSwitch))
        return;

    if (nonempty(wmclass)) {
        if (client->classHint()->match(wmclass) == false)
            return;
    }

    if (frame == manager->getFocus()) {
        if (client == manager->getFocus()->client())
            priority = 1;
        else
            priority = 2;
    }
    else if (quickSwitchToUrgent && frame->isUrgent()) {
        priority = 3;
    }
    else if (frame->isMinimized()) {
        if (quickSwitchToMinimized)
            priority = 6;
    }
    else if (frame->isHidden()) {
        if (quickSwitchToHidden)
            priority = 7;
    }
    else if (frame->avoidFocus()) {
        priority = 8;
    }
    else {
        priority = 4;
    }
}

int SwitchPreview::compare(const void* v1, const void* v2) {
    const Preview* p1 = *static_cast<Preview* const*>(v1);
    const Preview* p2 = *static_cast<Preview* const*>(v2);

    if (p1->priority == 0 || p2->priority == 0)
        return p2->priority - p1->priority;

    if (quickSwitchGroupWorkspaces) {
        int act = manager->activeWorkspace();
        int w1 = p1->frame->getWorkspace();
        if (w1 == AllWorkspaces)
            w1 = act;
        int w2 = p2->frame->getWorkspace();
        if (w2 == AllWorkspaces)
            w2 = act;
        if (w1 != w2) {
            return (w1 == act) ? -1 : (w2 == act) ? +1 : (w1 - w2);
        }
    }

    if (p1->priority != p2->priority)
        return p1->priority - p2->priority;

    if (quickSwitchPersistence) {
        const WindowOption* wo1 = p1->client->getWindowOption();
        const WindowOption* wo2 = p1->client->getWindowOption();
        int order = (wo1 ? wo1->order : 0) - (wo2 ? wo2->order : 0);
        if (order)
            return order;

        const char* c1 = p1->client->classHint()->res_class;
        const char* c2 = p2->client->classHint()->res_class;
        if (nonempty(c1)) {
            if (nonempty(c2)) {
                int sc = strcmp(c1, c2);
                if (sc)
                    return sc;
            } else {
                return -1;
            }
        } else if (nonempty(c2)) {
            return +1;
        }

        const char* n1 = p1->client->classHint()->res_name;
        const char* n2 = p2->client->classHint()->res_name;
        if (nonempty(n1)) {
            if (nonempty(n2)) {
                int sc = strcmp(n1, n2);
                if (sc)
                    return sc;
            } else {
                return -1;
            }
        } else if (nonempty(n2)) {
            return +1;
        }
    }

    int dw = p1->frame->getWorkspace() - p2->frame->getWorkspace();
    if (dw)
        return dw;

    int dx = p1->frame->x() - p2->frame->x();
    if (dx)
        return dx;

    int dy = p1->frame->y() - p2->frame->y();
    if (dy)
        return dy;

    return 0;
}

Switcher* Switcher::newSwitchPreview(YWindow* parent) {
    return new SwitchPreview(parent);
}

SwitchPreview::SwitchPreview(YWindow* parent) :
    Switcher(parent),
    rootPixmap(None),
    backPixmap(None),
    windowPicture(None),
    rootPixel(-1L),
    foreground(&clrQuickSwitchText),
    background(&clrQuickSwitch),
    bordercolor(&clrQuickSwitchBorder),
    highlighter(&clrQuickSwitchActive),
    activeText(&clrActiveTitleBarText),
    switchFont(switchFontName),
    active(-1),
    viewables(0),
    exposed(false),
    keyPressed(0),
    modsDown(0)
{
    if (clrQuickSwitchActive)
        activeFlat = &clrQuickSwitchActive;
    else if (strcmp(clrNormalMenu, clrQuickSwitch) == 0)
        activeFlat = &clrActiveMenuItem;
    else
        activeFlat = &clrNormalMenu;

    setStyle(wsSaveUnder | wsOverrideRedirect | wsPointerMotion); // | wsNoExpose);
    setTitle("IcePreview");
    setClassHint("preview", "IceWM");
    setNetWindowType(_XA_NET_WM_WINDOW_TYPE_DIALOG);

    if (backcolor() == false && background) {
        setBackground(background.pixel());
    }

    NoteSize = scale(256);

    for (YFrameIter iter = manager->focusedReverseIterator(); ++iter; ) {
        createdFrame(iter);
    }
}

SwitchPreview::~SwitchPreview() {
    if (isUp())
        close();
    for (PreviewIterator iter = views.iterator(); ++iter; ) {
        context.remove(iter->client->handle());
        delete *iter;
    }
    discard();
}

int SwitchPreview::numerator() {
    int size = int(min(width(), height()) / 10);
    return clamp(size, 120, 200);
}

int SwitchPreview::scale(int value) {
    return value * numerator() / 120;
}

void SwitchPreview::discard() {
    if (backPixmap) {
        if (backPixmap != true) {
            xapp->freePixmap(backPixmap);
        }
        backPixmap = None;
    }
    if (windowPicture) {
        xapp->freePicture(windowPicture);
        windowPicture = None;
    }
}

bool SwitchPreview::backcolor() {
    YProperty rootcolor(desktop, _XA_XROOTCOLOR_PIXEL, F32, 1, XA_CARDINAL);
    if (rootcolor && (unsigned long) *rootcolor != rootPixel) {
        rootPixel = (unsigned long) *rootcolor;
        setBackground(rootPixel);
    }
    return rootcolor;
}

void SwitchPreview::backdrop() {
    manager->grabServer();
    YProperty rootprop(desktop, _XA_XROOTPMAP_ID, F32, 1, XA_PIXMAP);
    if (rootprop == false) {
        printf("no XROOTPMAP\n");
    }
    else if (rootPixmap != (unsigned long) *rootprop) {
        discard();
        rootPixmap = (unsigned long) *rootprop;
        Picture rootPict = xapp->createPicture(rootPixmap, desktop->format());
        backPixmap = createPixmap(width(), height());
        Picture backPict = xapp->createPicture(backPixmap, format());
        XRenderComposite(xapp->display(), PictOpSrc, rootPict, None, backPict,
                         x(), y(), 0, 0, 0, 0, width(), height());

        setBackgroundPixmap(backPixmap);
        clearWindow();
        xapp->sync();
        xapp->freePicture(rootPict);
        xapp->freePicture(backPict);
    }
    manager->ungrabServer();

    int prevWidth = previewWidth();
    int prevHeight = previewHeight();
    int horiMarg = scale(100), vertMarg = horiMarg;
    if (width() > height() * 2) {
        horiMarg = (width() - 2*(height() - 2*vertMarg)) / 2;
    }
    else if (height() > width() * 2) {
        vertMarg = (height() - 2*(width() - 2*horiMarg)) / 2;
    }
    if (horiMarg * 2 + prevWidth > int(width())) {
        horiMarg = max(0, (int(width()) - prevWidth) / 2);
    }
    if (vertMarg * 2 + prevHeight > int(height())) {
        vertMarg = max(0, (int(height()) - prevHeight) / 2);
    }
    if ((int(height()) - 2*vertMarg - 10) / prevHeight == 1 &&
        2*prevHeight + 30 <= int(height()) && height() < width()) {
        vertMarg = (int(height()) - 10 - 2*prevHeight) / 2;
    }
    rect.setRect(horiMarg, vertMarg, width() - 2*horiMarg, height() - 2*vertMarg);
    backfill(rect.xx, rect.yy, rect.ww, rect.hh);
}

void SwitchPreview::backfill(int x, int y, unsigned w, unsigned h) {
    if (windowPicture == None)
        windowPicture = createPicture();
    XRenderColor color = background.color();
    color.alpha /= 2;
    color.red /= 2;
    color.green /= 2;
    color.blue /= 2;

    XRenderFillRectangle(xapp->display(), PictOpOver, windowPicture, &color,
                         x, y, w, h);
}

void SwitchPreview::deactivatePopup() {
    if (isUp() == false) {
        discard();
    }
}

void SwitchPreview::begin(bool down, unsigned mods, char* wm_class) {
    mods = KEY_MODMASK(mods);
    modsDown = mods;
    wmclass = wm_class;
    exposed = false;

    int screen = manager->getSwitchScreen();
    const YRect geom(desktop->getScreenGeometry(screen));
    // const int dx = geom.xx, dy = geom.yy;
    // const int dw = int(geom.ww), dh = int(geom.hh);
    setGeometry(geom);
    setPointerMotion(true);

    if (popup(nullptr, nullptr, nullptr, screen, pfNoPointerChange) && isUp()) {
        backdrop();
        prioritize();
        sortViews();
        placeViews();
        fUpdateTimer->setTimer(345L, this, true);
        active = findActive();
        if (down ? active + 1 < viewables : 0 < active)
            target(down ? +1 : -1);
    }
}

int SwitchPreview::findActive() {
    YFrameWindow* focus = manager->getFocus();
    for (PreviewIterator iter = views.iterator(); ++iter; ) {
        if (iter->frame == focus && iter->client == focus->client()) {
            return iter.where();
        }
    }
    for (PreviewIterator iter = views.iterator(); ++iter; ) {
        if (iter->placed()) {
            return iter.where();
        }
    }
    return -1;
}

void SwitchPreview::draw(Preview* prev) {
    if (prev->placed()) {
        if (prev->damaged) {
            prev->update();
        }
        bool act = (active >= 0 && views[active] == prev);
        prev->draw(this, act);
    }
}

void SwitchPreview::eraseAt(int x, int y, int w, int h) {
    if (isUp() && windowPicture) {
        clearArea(x, y, w, h, false);
        backfill(x, y, w, h);
    }
}

void SwitchPreview::erase(Preview* prev) {
    if (prev->placed()) {
        eraseAt(prev->x, prev->y, prev->w, prev->h);
    }
}

void SwitchPreview::drawViews() {
    for (PreviewIterator iter = views.iterator(); ++iter; ) {
        draw(iter);
    }
}

void SwitchPreview::placeViews(int skip) {
    place.reset();
    place.top(rect.xx, rect.yy);
    place.size(previewWidth(), previewHeight());
    int margin = min(rect.ww / place.wide, rect.hh / place.high) < 3
                 ? scale(2) : scale(10);
    place.hori = max(1, int(rect.ww - margin) / (place.wide + margin));
    place.hgap = max(1, int(rect.ww - (place.hori * place.wide))
                            / (place.hori + 1));
    place.vert = max(1, int(rect.hh - margin) / (place.high + margin));
    place.vgap = max(1, int(rect.hh - (place.vert * place.high))
                            / (place.vert + 1));
    int skipped = 0;
    for (PreviewIterator iter = views.iterator(); ++iter; ) {
        if (iter->priority == 0 || place.done()) {
            iter->place(0, 0);
        }
        else if (skipped < skip) {
            iter->place(0, 0);
            ++skipped;
        }
        else {
            place.place(iter);
            place.next();
        }
    }
}

void SwitchPreview::prioritize() {
    viewables = 0;
    int n = views.getCount();
    for (int i = 0; i < n; ++i) {
        Preview* prev = views[i];
        prev->prioritize(wmclass);
        if (prev->priority)
            ++viewables;
        else if (i < n - 1) {
            views.swap(i, n - 1);
            --i;
            --n;
        }
    }
}

void SwitchPreview::sortViews() {
    if (viewables > 1)
        qsort(&*views, viewables, sizeof(Preview*), compare);
}

Preview* SwitchPreview::createPreview(YFrameClient* client, YFrameWindow* frame) {
    if (client->adopted()) {
        Window window = client->handle();
        Preview* prev = context.find(window);
        if (prev == nullptr) {
            prev = new Preview(client, frame, this);
            if (prev) {
                context.save(window, prev);
                return prev;
            }
        }
    }
    return nullptr;
}

int SwitchPreview::previewWidth() const {
    return NoteWidth + 2 * quickSwitchHMargin + quickSwitchIMargin;
}

int SwitchPreview::previewHeight() const {
    int leading = font()->height();
    return 2 * leading + NoteHeight
         + 2 * quickSwitchIMargin + 2 * quickSwitchVMargin;
}

void SwitchPreview::destroyedClient(YFrameClient* client) {
    bool redraw = false;
    context.remove(client->handle());
    for (int i = views.getCount(); 0 < i--; ) {
        Preview* prev = views[i];
        if (prev->client == client) {
            redraw |= (i == active);
            erase(prev);
            prev->damage = None;
            prev->source = None;
            if (prev->priority)
                --viewables;
            delete prev;
            views.remove(i);
            if (i < active)
                --active;
            break;
        }
    }
    if (redraw && isUp() && active >= 0 && active < viewables)
        draw(views[active]);
}

void SwitchPreview::destroyedFrame(YFrameWindow* frame) {
    bool redraw = false;
    for (int i = views.getCount(); 0 < i--; ) {
        Preview* prev = views[i];
        if (prev->frame == frame) {
            redraw |= (i == active);
            context.remove(prev->client->handle());
            erase(prev);
            prev->damage = None;
            prev->source = None;
            if (prev->priority)
                --viewables;
            delete prev;
            views.remove(i);
            if (i < active)
                --active;
        }
    }
    if (redraw && isUp() && active >= 0 && active < viewables)
        draw(views[active]);
}

void SwitchPreview::createdFrame(YFrameWindow* frame) {
    for (YFrameClient* client : frame->clients()) {
        createdClient(frame, client);
    }
}

void SwitchPreview::createdClient(YFrameWindow* frame, YFrameClient* client) {
    Preview* prev = createPreview(client, frame);
    if (prev) {
        if (isUp() && (prev->prioritize(wmclass), prev->priority > 0)) {
            views.insert(viewables, prev);
            ++viewables;
            if (place.have()) {
                prev->update();
                place.place(prev);
                place.next();
                draw(prev);
            }
        } else {
            views.append(prev);
        }
    }
}

void SwitchPreview::transfer(YFrameClient* client, YFrameWindow* frame) {
    for (int i = 0; i < views.getCount(); ++i) {
        if (views[i]->client == client) {
            views[i]->frame = frame;
            break;
        }
    }
}

void SwitchPreview::hiding(YFrameClient* client) {
    Window window = client->handle();
    Preview* prev = context.find(window);
    if (prev) {
        prev->update();
    }
}

YFrameWindow* SwitchPreview::current() {
    return nullptr;
}

void SwitchPreview::accept() {
    YRestackLock restack;
    close();
    if (active >= 0 && active < viewables &&
        views[active]->priority) {
        Preview* v = views[active];
        if (v->client != v->frame->client())
            v->frame->selectTab(v->client);
        v->frame->activateWindow(true, false);
        if (v->frame->isFullscreen())
            v->frame->updateLayer();
    }
    manager->restackWindows();
}

void SwitchPreview::close() {
    cancelPopup();
    fUpdateTimer = null;
}

void SwitchPreview::handleDamageNotify(const XDamageNotifyEvent& damage) {
    Preview* prev = context.find(damage.drawable);
    if (prev) {
        prev->notify();
    }
}

bool SwitchPreview::point(int x, int y) {
    for (PreviewIterator iter = views.iterator(); ++iter; ) {
        if (iter->placed() && iter->inside(x, y)) {
            if (iter.where() != active) {
                target(iter.where() - active);
            }
            return true;
        }
    }
    return false;
}

void SwitchPreview::target(int incr) {
    int nonp = 0;
    int n = viewables;
    int a = max(0, active);

    if (incr > 0) {
        for (int i = 0; i < incr; ++i) {
            if (a + 1 < n && views[a + 1]->priority) {
                ++a;
                if (views[a]->placed() == false)
                    ++nonp;
            }
        }
    }
    else if (incr < 0) {
        for (int i = 0; i < -incr; ++i) {
            if (a > 0) {
                --a;
                if (views[a]->placed() == false)
                    ++nonp;
            }
        }
    }

    if (a != active) {
        if (nonp == 0) {
            int old = active;
            active = a;
            if (old >= 0 && old < n) {
                draw(views[old]);
            }
            draw(views[active]);
        }
        else if (a < active) {
            int skip = (a / place.hori) * place.hori;
            placeViews(skip);
            active = a;
            drawViews();
        }
        else if (a > active) {
            int need = ((a + place.hori) / place.hori);
            int have = place.vert;
            int skip = (need - have) * place.hori;
            placeViews(skip);
            active = a;
            drawViews();
            if (place.have()) {
                Placer copy = place;
                int w = views[a]->w, h = views[a]->h;
                while (copy.xind && copy.xind < copy.hori) {
                    eraseAt(copy.getx(), copy.gety(), w, h);
                    copy.next();
                }
            }
        }
    }
}

bool SwitchPreview::handleKey(const XKeyEvent& key) {
    if (isUp() == false)
        return true;

    KeySym k = mapKeypad(keyCodeToKeySym(key.keycode));
    if (key.type == KeyPress) {
        keyPressed = k;
        if (isKey(key)) {
            if (active + 1 < viewables)
                target(+1);
            else if (active > 0)
                target(-active);
        }
        else if (gKeySysSwitchLast == key) {
            if (active > 0)
                target(-1);
            else if (viewables > 1 && active + 1 < viewables)
                target(viewables - 1 - active);
        }
        else if (gKeyWinClose == key) {
            if (active >= 0 && active < viewables) {
                Preview* prev = views[active];
                bool confirm = false;
                if (prev->frame->hasTab(prev->client)) 
                    prev->frame->wmCloseClient(prev->client, &confirm);
            }
        }
        else if (k == XK_Return) {
            accept();
        }
        else if (manager->handleSwitchWorkspaceKey(key)) {
            if (views.isEmpty())
                close();
            else {
                prioritize();
                sortViews();
                placeViews();
                backdrop();
                drawViews();
            }
        }
        else if (k == XK_Left) {
            if (active > 0)
                target(-1);
        }
        else if (k == XK_Right) {
            if (active + 1 < viewables)
                target(+1);
        }
        else if (k == XK_Up) {
            if (active >= place.hori)
                target(-place.hori);
        }
        else if (k == XK_Down) {
            if (active + place.hori < viewables)
                target(+place.hori);
        }
        else if (k == XK_End) {
            if (active + 1 < viewables)
                target(viewables - 1 - active);
        }
        else if (k == XK_Home) {
            if (active > 0)
                target(-active);
        }
        else if (k == XK_Next) {
            if (active + place.capacity() < viewables)
                target(+place.capacity());
        }
        else if (k == XK_Prior) {
            if (active >= place.capacity())
                target(-place.capacity());
        }
        else if (k == XK_Delete) {
            if (active >= 0 && active < viewables) {
                Preview* prev = views[active];
                bool confirm = false;
                if (prev->frame->hasTab(prev->client)) 
                    prev->frame->wmCloseClient(prev->client, &confirm);
            }
        }
        else if (k >= '1' && k <= '9') {
            int index = int(k - '0');
            for (PreviewIterator iter = views.iterator(); ++iter; ) {
                if (iter->placed() && --index == 0) {
                    if (active != iter.where()) {
                        target(iter.where() - active);
                    }
                    break;
                }
            }
        }
        else if (isKey(key) && !modDown(key.state)) {
            accept();
        }
        else if ((k & 0xFF) == k && isalpha((unsigned char) k)) {
            char c = char(tolower((unsigned char) k));
            for (int i = 1; i < viewables; ++i) {
                int n = (active + i) % viewables;
                if (n >= 0 && views[n]->letter == c && views[n]->placed()) {
                    target(n - active);
                    break;
                }
            }
        }
    }
    else if (key.type == KeyRelease) {
        if ((isKey(key) && !modDown(key.state)) || isModKey(key.keycode)) {
            accept();
        }
        else if (k == XK_Escape && k == keyPressed) {
            close();
        }
    }
    return true;
}

bool SwitchPreview::isKey(const XKeyEvent& x) {
    return gKeySysSwitchNext == x
        || (wmclass.nonempty() && gKeySysSwitchClass == x);
}

unsigned SwitchPreview::modifiers() {
    return gKeySysSwitchNext.mod
         | gKeySysSwitchLast.mod
         | gKeySysSwitchClass.mod;
}

bool SwitchPreview::modDown(unsigned mod) {
    return modsDown == ShiftMask
        ? hasbit(KEY_MODMASK(mod), ShiftMask)
        : hasbits(KEY_MODMASK(mod), modsDown & ~ShiftMask); 
}

bool SwitchPreview::isModKey(KeyCode c) {
    KeySym k = keyCodeToKeySym(c);

    if (k == XK_Shift_L || k == XK_Shift_R)
        return hasbit(modsDown, ShiftMask)
            && hasbit(modifiers(), kfShift)
            && modsDown == ShiftMask;

    if (k == XK_Control_L || k == XK_Control_R)
        return hasbit(modsDown, ControlMask)
            && hasbit(modifiers(), kfCtrl);

    if (k == XK_Alt_L     || k == XK_Alt_R)
        return hasbit(modsDown, xapp->AltMask)
            && hasbit(modifiers(), kfAlt);

    if (k == XK_Meta_L    || k == XK_Meta_R)
        return hasbit(modsDown, xapp->MetaMask)
            && hasbit(modifiers(), kfMeta);

    if (k == XK_Super_L   || k == XK_Super_R)
        return hasbit(modsDown, xapp->SuperMask)
            && (modSuperIsCtrlAlt
                ? hasbits(modifiers(), kfCtrl + kfAlt)
                : hasbit(modifiers(), kfSuper));

    if (k == XK_Hyper_L   || k == XK_Hyper_R)
        return hasbit(modsDown, xapp->HyperMask)
            && hasbit(modifiers(), kfHyper);

    if (k == XK_ISO_Level3_Shift || k == XK_Mode_switch)
        return hasbit(modsDown, xapp->ModeSwitchMask)
            && hasbit(modifiers(), kfAltGr);

    return false;
}

void SwitchPreview::handleMotion(const XMotionEvent& motion) {
    if (isUp() == false)
        return;

    point(motion.x, motion.y);
}

void SwitchPreview::handleButton(const XButtonEvent& button) {
    if (isUp() == false)
        return;

    if (button.type == ButtonPress) {
        switch (button.button) {
            case Button1:
                if (point(button.x, button.y)) {
                    accept();
                } else {
                    close();
                }
                break;
            case Button2:
                if (point(button.x, button.y)) {
                    Preview* prev = views[active];
                    bool confirm = false;
                    if (prev->frame->hasTab(prev->client)) 
                        prev->frame->wmCloseClient(prev->client, &confirm);
                }
                break;
            case Button3:
                if (point(button.x, button.y)) {
                    YFrameWindow* frame = views[active]->frame;
                    frame->popupSystemMenu(this, button.x_root, button.y_root,
                                           YPopupWindow::pfCanFlipVertical |
                                           YPopupWindow::pfCanFlipHorizontal);
                }
                break;
            case Button4:
                if (active > 0)
                    target(-1);
                break;
            case Button5:
                if (active + 1 < viewables)
                    target(+1);
                break;
            default:
                break;
        }
    }
}

void SwitchPreview::handleExpose(const XExposeEvent& expose) {
    if (expose.count == 0 && isUp()) {
        exposed = true;
        drawViews();
    }
}

void SwitchPreview::handleProperty(const XPropertyEvent& property) {
    if (property.state == PropertyNewValue) {
        if (property.atom == _XA_XROOTCOLOR_PIXEL) {
            backcolor();
        }
        else if (property.atom == _XA_XROOTPMAP_ID) {
            if (isUp()) {
                YProperty rootprop(desktop, _XA_XROOTPMAP_ID, F32, 1, XA_PIXMAP);
                if (rootprop && (unsigned long) *rootprop != rootPixmap) {
                    backdrop();
                    drawViews();
                }
            }
        }
    }
}

bool SwitchPreview::handleTimer(YTimer* timer) {
    if (timer == fUpdateTimer && isUp()) {
        for (PreviewIterator iter = views.iterator(); ++iter; ) {
            if (iter->damaged && iter->placed() && iter->client->visible()) {
                iter->update();
                if (iter->damaged == false) {
                    draw(iter);
                }
            }
        }
        return true;
    }
    return false;
}


#ifndef PREVIEW_H
#define PREVIEW_H
#include "yxapp.h"
#include "switcher.h"
#include "yxcontext.h"

class Preview {
public:
    Preview(YFrameClient* client, YFrameWindow* frame, SwitchPreview* parent);
    ~Preview();
    void draw(SwitchPreview* parent, bool active);
    void notify();
    void update();
    void subtract();
    void prioritize(const char* wmclass);
    bool placed() const { return x > 0 && y > 0 && priority; }
    void place(int p, int q) { x = p; y = q; }
    bool inside(int p, int q) const {
        return p >= x && p < x + int(w) && q >= y && q < y + int(h);
    }

    YFrameClient* client;
    YFrameWindow* frame;
    SwitchPreview* parent;
    Pixmap pixmap;
    Damage damage;
    Picture picture, source;
    int x, y;
    unsigned w, h;
    int p, q;
    int priority;
    char letter;
    bool damaged;
    bool updated;
};

class Placer {
public:
    int xtop, ytop;
    int hori, vert;
    int hgap, vgap;
    int wide, high;
    int xind, yind;
    Placer() : xtop(0), ytop(0), hori(0), vert(0), hgap(0), vgap(0),
               wide(0), high(0), xind(0), yind(0) { }
    int getx() const {
        return (yind >= vert) ? 0 : xtop + (xind + 1) * hgap + xind * wide;
    }
    int gety() const {
        return (yind >= vert) ? 0 : ytop + (yind + 1) * vgap + yind * high;
    }
    bool have() const {
        return yind < vert;
    }
    bool done() const {
        return yind >= vert;
    }
    bool next() {
        return have() && ((++xind < hori) || (xind = 0, ++yind < vert));
    }
    void reset() {
        xind = yind = 0;
    }
    void top(int x, int y) {
        xtop = x; ytop = y;
    }
    void size(int width, int height) {
        wide = width; high = height;
    }
    void geometry(int x, int y, int w, int h) {
        top(x, y); size(w, h);
    }
    int capacity() const {
        return hori * vert;
    }
    int placed() const {
        return xind + (yind * hori);
    }
    void place(Preview* prev) { prev->place(getx(), gety()); }
};

class SwitchPreview : public Switcher, private YTimerListener {
public:
    SwitchPreview(YWindow* parent);
    ~SwitchPreview();

    void deactivatePopup() override;

    void begin(bool down, unsigned mods, char* wmclass) override;
    void destroyedClient(YFrameClient* client) override;
    void destroyedFrame(YFrameWindow* frame) override;
    void createdFrame(YFrameWindow* frame) override;
    void createdClient(YFrameWindow* frame, YFrameClient* client) override;
    void transfer(YFrameClient* client, YFrameWindow* frame) override;
    void hiding(YFrameClient* client) override;
    YFrameWindow* current() override;

    void handleExpose(const XExposeEvent& expose) override;
    bool handleKey(const XKeyEvent& key) override;
    void handleButton(const XButtonEvent& button) override;
    void handleMotion(const XMotionEvent& motion) override;
    void handleProperty(const XPropertyEvent& property) override;
    void handleDamageNotify(const XDamageNotifyEvent& damage) override;
    bool handleTimer(YTimer* timer) override;

    bool previews() const override { return true; }
    void accept();
    void close();
    YFont font() const { return switchFont; }

private:

    void draw(Preview* prev);
    void erase(Preview* prev);
    void eraseAt(int x, int y, int w, int h);
    void drawViews();
    void placeViews(int skip = 0);
    void prioritize();
    void sortViews();
    static int compare(const void* p1, const void* p2);
    bool backcolor();
    void backdrop();
    void backfill(int x, int y, unsigned w, unsigned h);
    void discard();
    Preview* createPreview(YFrameClient* client, YFrameWindow* frame);
    int previewWidth() const;
    int previewHeight() const;
    int findActive();
    bool isKey(const XKeyEvent& x);
    bool isModKey(KeyCode c);
    unsigned modifiers();
    bool modDown(unsigned mod);
    void target(int incr);
    bool point(int x, int y);
    int numerator();
    int scale(int value);

    Pixmap rootPixmap;
    Pixmap backPixmap;
    Picture windowPicture;
    unsigned long rootPixel;

public:
    YColorName foreground;
    YColorName background;
    YColorName bordercolor;
    YColorName highlighter;
    YColorName activeFlat;
    YColorName activeText;
    YFont switchFont;
    mstring wmclass;

private:
    int active;
    int viewables;
    bool exposed;
    unsigned keyPressed;
    unsigned modsDown;

    YContext<Preview> context;
    YArray<Preview*> views;
    YRect rect;
    Placer place;
    lazy<YTimer> fUpdateTimer;

    typedef YArrayIterator<Preview*> PreviewIterator;
};

#endif

// vim: set sw=4 ts=4 et:

#ifndef __YMENU_H
#define __YMENU_H

#include "ypopup.h"
#include "ytimer.h"

class YAction;
class YActionListener;
class YMenuItem;

class YMenu: public YPopupWindow, public YTimerListener {
public:
    YMenu(YWindow *parent = 0);
    virtual ~YMenu();

    virtual void sizePopup(int hspace);
    virtual void activatePopup(int flags);
    virtual void deactivatePopup();
    virtual void donePopup(YPopupWindow *popup);

    virtual void paint(Graphics &g, const YRect &r);

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleMotionOutside(bool top, const XMotionEvent &motion);
    virtual bool handleAutoScroll(const XMotionEvent &mouse);

    void trackMotion(const int x_root, const int y_root, const unsigned state, bool submenu);

    YMenuItem *add(YMenuItem *item, const char *icons);
    YMenuItem *addItem(const ustring &name, int hotCharPos, const ustring &param, YAction action, const char *icons);
    YMenuItem *addItem(const ustring &name, int hotCharPos, YAction action, YMenu *submenu, const char *icons);
    YMenuItem *addSubmenu(const ustring &name, int hotCharPos, YMenu *submenu, const char *icons);

    YMenuItem *add(YMenuItem *item);
    YMenuItem *addSorted(YMenuItem *item, bool duplicates, bool ignoreCase = false);
    YMenuItem *addItem(const ustring &name, int hotCharPos, const ustring &param, YAction action);
    YMenuItem *addItem(const ustring &name, int hotCharPos, YAction action, YMenu *submenu);
    YMenuItem *addSubmenu(const ustring &name, int hotCharPos, YMenu *submenu);
    YMenuItem *addSeparator();
    YMenuItem *addLabel(const ustring &name);
    void removeAll();
    YMenuItem *findAction(YAction action);
    YMenuItem *findSubmenu(const YMenu *sub);
    YMenuItem *findName(const ustring &name, const int first = 0);
    int findFirstLetRef(char firstLet, const int first, const int ignCase = 1);

    void enableCommand(YAction action); // 0 == All
    void disableCommand(YAction action); // 0 == All

    int itemCount() const { return fItems.getCount(); }
    YMenuItem *getItem(int n) const { return fItems[n]; }
    void setItem(int n, YMenuItem *ref) { fItems[n] = ref; return; }

    bool isShared() const { return fShared; }
    void setShared(bool shared) { fShared = shared; }

    void setActionListener(YActionListener *actionListener);
    YActionListener *getActionListener() const { return fActionListener; }

    virtual bool handleTimer(YTimer *timer);

private:
    YObjectArray<YMenuItem> fItems;
    int selectedItem;
    int paintedItem;
    int paramPos;
    int namePos;
    YPopupWindow *fPopup;
    YPopupWindow *fPopupActive;
    bool fShared;
    YActionListener *fActionListener;
    int activatedX, activatedY;
    int submenuItem;

    ref<YImage> fGradient;

    static YMenu *fPointedMenu;
    static lazy<YTimer> fMenuTimer;
    int fTimerX, fTimerY;
    int fTimerSubmenuItem;
    static int fAutoScrollDeltaX, fAutoScrollDeltaY;
    static int fAutoScrollMouseX, fAutoScrollMouseY;

    void getOffsets(int &left, int &top, int &right, int &bottom);
    void getArea(int &x, int &y, unsigned &w, unsigned &h);

    void drawBackground(Graphics &g, int x, int y, unsigned w, unsigned h);
    void drawSeparator(Graphics &g, int x, int y, unsigned w);

    void drawSubmenuArrow(Graphics &g, YMenuItem *mitem,
                          int left, int top);
    void paintItem(Graphics &g, const int i, const int l, const int t, const int r,
                   const int minY, const int maxY, bool draw);

    void repaintItem(int item);
    void paintItems();
    int findItemPos(int item, int &x, int &y, unsigned &h);
    int findItem(int x, int y);
    int findActiveItem(int cur, int direction);
    int findHotItem(char k);
    void focusItem(int item);
    void activateSubMenu(int item, bool byMouse);

    int activateItem(int modifiers, bool byMouse = false);
    bool isCondCascade(int selectedItem);
    int onCascadeButton(int selectedItem, int x, int y, bool checkPopup);

    void autoScroll(int deltaX, int deltaY, int mx, int my, const XMotionEvent *motion);
    void finishPopup(YMenuItem *item, YAction action, unsigned int modifiers);
    void hideSubmenu();
};

extern ref<YPixmap> menubackPixmap;
extern ref<YPixmap> menuselPixmap;
extern ref<YPixmap> menusepPixmap;

//class YPixbuf;

extern ref<YImage> menubackPixbuf;
extern ref<YImage> menuselPixbuf;
extern ref<YImage> menusepPixbuf;

#endif

// vim: set sw=4 ts=4 et:

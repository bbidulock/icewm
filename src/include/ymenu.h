#ifndef __YMENU_H
#define __YMENU_H

#include "ypopup.h"
#include "ytimer.h"
#include "yconfig.h"

#pragma interface

class YAction;
class YActionListener;
class YMenuItem;

class YMenu: public YPopupWindow, public YTimerListener {
public:
    YMenu(YWindow *parent = 0);
    virtual ~YMenu();

    virtual void sizePopup();
    virtual void activatePopup();
    virtual void deactivatePopup();
    virtual void donePopup(YPopupWindow *popup);

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual bool handleAutoScroll(const XMotionEvent &mouse);

    YMenuItem *add(YMenuItem *item);
    YMenuItem *addItem(const char *name, int hotCharPos, const char *param, YAction *action);
    YMenuItem *addItem(const char *name, int hotCharPos, YAction *action, YMenu *submenu);
    YMenuItem *addSubmenu(const char *name, int hotCharPos, YMenu *submenu);
    YMenuItem *addSeparator();
    void removeAll();
    YMenuItem *findAction(YAction *action);
    YMenuItem *findSubmenu(YMenu *sub);

    void enableCommand(YAction *action); // 0 == All
    void disableCommand(YAction *action); // 0 == All

    int itemCount() const { return fItemCount; }
    YMenuItem *item(int n) const { return fItems[n]; }

    bool isShared() const { return fShared; }
    void setShared(bool shared) { fShared = shared; }

    void setActionListener(YActionListener *actionListener);
    YActionListener *getActionListener() const { return fActionListener; }

    virtual bool handleTimer(YTimer *timer);

    enum MenuStyle {
        msMetal = 1,
        msWindows = 2,
        msMotif = 3,
        msGtk = 4
    };

private:
    int fItemCount;
    YMenuItem **fItems;
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

    static YMenu *fPointedMenu;
    static YTimer *fMenuTimer;
    static int fTimerX, fTimerY, fTimerSubmenu; //, fTimerItem;
    static bool fTimerSlow;
    static int fAutoScrollDeltaX, fAutoScrollDeltaY;

    int getItemHeight(int itemNo, int &h, int &top, int &bottom, int &pad);
    void getItemWidth(int i, int &iw, int &nw, int &pw);
    void getOffsets(int &left, int &top, int &right, int &bottom);
    void getArea(int &x, int &y, int &w, int &h);

    void drawSeparator(Graphics &g, int x, int y, int w);

    void paintItem(Graphics &g, int i, int &l, int &t, int &r, int paint);
    void paintItems();
    int findItemPos(int item, int &x, int &y);
    int findItem(int x, int y);
    int findActiveItem(int cur, int direction);
    int findHotItem(char k);
    void focusItem(int item);
    void activateSubMenu(bool submenu, bool byMouse);
    //void focusItem(int item, int submenu, int byMouse);

    int activateItem(int no, int byMouse, unsigned int modifiers);
    bool isCondCascade(int selectedItem);
    int onCascadeButton(int selectedItem, int x, int y, bool checkPopup);

    void autoScroll(int deltaX, int deltaY, const XMotionEvent *motion);
    void finishPopup(YMenuItem *item, YAction *action, unsigned int modifiers);

    static YNumPrefProperty gSubmenuActivateDelay;
    static YNumPrefProperty gMenuActivateDelay;
    static YBoolPrefProperty gMenuMouseTracking;
    static YColorPrefProperty gMenuBg;
    static YColorPrefProperty gMenuItemFg;
    static YColorPrefProperty gActiveMenuItemBg;
    static YColorPrefProperty gActiveMenuItemFg;
    static YColorPrefProperty gDisabledMenuItemFg;
    static YFontPrefProperty gMenuFont;
    static YPixmapPrefProperty gPixmapBackground;
private: // not-used
    YMenu(const YMenu &);
    YMenu &operator=(const YMenu &);
};

#endif

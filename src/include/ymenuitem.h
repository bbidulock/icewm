#ifndef __YMENUITEM_H
#define __YMENUITEM_H

class YMenu;
class YAction;
class YPixmap;
class YActionListener;

class YMenuItem {
public:
    YMenuItem(const char *name, int hotCharPos, const char *param, YAction *action, YMenu *submenu);
    YMenuItem() { fName = 0; fHotCharPos = -1; fParam = 0; fAction = 0; fEnabled = 0; fSubmenu = 0; }
    virtual ~YMenuItem();
    const char *name() const { return fName; }
    const char *param() const { return fParam; }
    YAction *action() const { return fAction; }
    YMenu *submenu() const { return fSubmenu; }
    int hotChar() const { return (fName && fHotCharPos >= 0) ? fName[fHotCharPos] : -1; }
    int hotCharPos() const { return fHotCharPos; }

    YPixmap *getPixmap() const { return fPixmap; }
    void setPixmap(YPixmap *pixmap);
    void setChecked(bool c);
    int isChecked() const { return fChecked; }
    int isEnabled() const { return fEnabled; }
    void setEnabled(bool e) { fEnabled = e; }
    void setSubmenu(YMenu *submenu) { fSubmenu = submenu; }

    virtual void actionPerformed(YActionListener *listener, YAction *action, unsigned int modifiers);
private:
    char *fName;
    char *fParam;
    YAction *fAction;
    int fHotCharPos;
    YMenu *fSubmenu;
    YPixmap *fPixmap;
    bool fChecked;
    bool fEnabled;
};

#endif

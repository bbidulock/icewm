#ifndef __YMENUITEM_H
#define __YMENUITEM_H

#include "ypaint.h"
#include "ypixbuf.h"

class YMenu;
class YAction;
class YActionListener;

class YMenuItem {
public:
    YMenuItem(const ustring &name, int hotCharPos, const ustring &param, YAction *action, YMenu *submenu);
    YMenuItem(const ustring &name);
    YMenuItem();
    virtual ~YMenuItem();

    ustring getName() const { return fName; }
    ustring getParam() const { return fParam; }
    YAction *getAction() const { return fAction; }
    YMenu *getSubmenu() const { return fSubmenu; }

    int getHotChar() const { 
        return (fName != null && fHotCharPos >= 0) ? fName.charAt(fHotCharPos) : -1;
    }

    int getHotCharPos() const { 
        return fHotCharPos;
    }

    ref<YIconImage> getIcon() const { return fIcon; }
    void setChecked(bool c);
    int isChecked() const { return fChecked; }
    int isEnabled() const { return fEnabled; }
    void setEnabled(bool e) { fEnabled = e; }
    void setSubmenu(YMenu *submenu) { fSubmenu = submenu; }

    virtual void actionPerformed(YActionListener *listener, YAction *action, unsigned int modifiers);

    int queryHeight(int &top, int &bottom, int &pad) const;

    int getIconWidth() const;
    int getNameWidth() const;
    int getParamWidth() const;

    bool isSeparator() { return getName() == null && getSubmenu() == 0; }
    
    void setIcon(YIcon *icon);
private:
    ustring fName;
    ustring fParam;
    YAction *fAction;
    int fHotCharPos;
    YMenu *fSubmenu;
    ref<YIconImage> fIcon;
    bool fChecked;
    bool fEnabled;

    void setIcon(ref<YIconImage> icon);
};

#endif

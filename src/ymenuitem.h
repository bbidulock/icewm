#ifndef __YMENUITEM_H
#define __YMENUITEM_H

class YMenu;
class YAction;
class YActionListener;

class YMenuItem {
public:
    YMenuItem(const char *name, int hotCharPos, const char *param, YAction *action, YMenu *submenu);
    YMenuItem(const char *name);
    YMenuItem();
    virtual ~YMenuItem();

    char *getName() const { return fName; }
    const char *getParam() const { return fParam; }
    YAction *getAction() const { return fAction; }
    YMenu *getSubmenu() const { return fSubmenu; }

    int getHotChar() const { 
    	return (fName && fHotCharPos >= 0) ? fName[fHotCharPos] : -1; 
    }

    int getHotCharPos() const { 
    	return fHotCharPos; 
    }

    YIcon::Image *getIcon() const { return fIcon; }
    void setIcon(YIcon::Image *icon);
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

    bool isSeparator() { return !getName() && !getSubmenu(); }
    
private:
    char *fName;
    char *fParam;
    YAction *fAction;
    int fHotCharPos;
    YMenu *fSubmenu;
    YIcon::Image *fIcon;
    bool fChecked;
    bool fEnabled;
};

#endif

#ifndef __YBUTTON_H
#define __YBUTTON_H

#include "ywindow.h"
#include "yconfig.h"

#pragma interface

class YAction;
class YActionListener;
class YMenu;

class YButton: public YWindow {
public:
    YButton(YWindow *parent, YAction *action, YMenu *popup = 0);
    virtual ~YButton();

    virtual void paint(Graphics &g, const YRect &er);
    virtual void paintFocus(Graphics &g, const YRect &er);
    virtual bool eventKey(const YKeyEvent &key);
    virtual bool eventButton(const YButtonEvent &button);
    virtual bool eventCrossing(const YCrossingEvent &crossing);

    void setAction(YAction *action);
    void setPopup(YMenu *popup);
    void setPixmap(YPixmap *pixmap);
    void setText(const char *str, int hot = -1);

    void setPressed(int pressed);
    virtual bool isFocusTraversable();

    virtual void donePopup(YPopupWindow *popup);

    void popupMenu();
    virtual void updatePopup();

    void setActionListener(YActionListener *listener) { fListener = listener; }
    YActionListener *getActionListener() const { return fListener; }

    void setSelected(bool selected);
    void setArmed(bool armed, bool mousedown);
    bool isSelected() const { return fSelected; }
    bool isArmed() const { return fArmed; }
    bool isPopupActive() const { return fPopupActive; }

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    
private:
    YAction *fAction;
    YMenu *fPopup;
    YPixmap *fPixmap;
    CStr *fText;
    int fPressed;
    int fHotCharPos;
    int hotKey;

    YActionListener *fListener;

    bool fSelected;
    bool fArmed;
    bool wasPopupActive;
    bool fPopupActive;

    void popup(bool mouseDown);
    void popdown();

    static YColorPrefProperty gNormalButtonBg;
    static YColorPrefProperty gNormalButtonFg;
    static YColorPrefProperty gActiveButtonBg;
    static YColorPrefProperty gActiveButtonFg;
    static YFontPrefProperty gNormalButtonFont;
    static YFontPrefProperty gActiveButtonFont;
    static YPixmapPrefProperty gPixmapNormalButton;
    static YPixmapPrefProperty gPixmapActiveButton;
    static YNumPrefProperty gBorderStyle;
private: // not-used
    YButton(const YButton &);
    YButton &operator=(const YButton &);
};

#endif

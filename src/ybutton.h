#ifndef __YBUTTON_H
#define __YBUTTON_H

#include "ywindow.h"
#include "yaction.h"

class YMenu;

class YButton: public YWindow {
public:
    YButton(YWindow *parent, YAction *action, YMenu *popup = 0);
    virtual ~YButton();

    virtual void paint(Graphics &g, int x, int y, unsigned w, unsigned h);
    virtual void paintFocus(Graphics &g, int x, int y, unsigned w, unsigned h);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleCrossing(const XCrossingEvent &crossing);

    void setAction(YAction * action);
    void setPopup(YMenu * popup);
    void setImage(YIcon::Image * image);
    void setText(const char * str, int hot = -1);

    void setPressed(int pressed);
    virtual bool isFocusTraversable();

    virtual void donePopup(YPopupWindow *popup);

    void popupMenu();
    virtual void updatePopup();

    void actionListener(YAction::Listener *listener) { fListener = listener; }
    YAction::Listener *actionListener() const { return fListener; }

    void setSelected(bool selected);
    void setArmed(bool armed, bool mousedown);
    bool isPressed() const { return fPressed; } 
    bool isSelected() const { return fSelected; }
    bool isArmed() const { return fArmed; }
    bool isPopupActive() const { return fPopupActive; }

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual YFont * getFont();
    virtual YColor * getColor();
    virtual YSurface getSurface();
    
private:
    void paint(Graphics &g, int const d, int const x, int const y,
    			    unsigned const w, unsigned const h);

    YAction *fAction;
    YMenu *fPopup;
    YIcon::Image *fImage;
    char *fText;
    int fPressed;
    int fHotCharPos;
    int hotKey;

    YAction::Listener *fListener;

    bool fSelected;
    bool fArmed;
    bool wasPopupActive;
    bool fPopupActive;

    void popup(bool mouseDown);
    void popdown();

    static YColor *normalButtonBg;
    static YColor *normalButtonFg;
    
    static YColor *activeButtonBg;
    static YColor *activeButtonFg;
    
    static YFont *normalButtonFont;
    static YFont *activeButtonFont;
};

extern YPixmap *buttonIPixmap;
extern YPixmap *buttonAPixmap;

#ifdef CONFIG_GRADIENTS
extern class YPixbuf *buttonIPixbuf;
extern class YPixbuf *buttonAPixbuf;
#endif

#endif

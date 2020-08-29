#ifndef __YBUTTON_H
#define __YBUTTON_H

#include "ywindow.h"
#include "yaction.h"

class YMenu;
class YIcon;

class YButton: public YWindow {
public:
    YButton(YWindow *parent, YAction action, YMenu *popup = nullptr);
    virtual ~YButton();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void paintFocus(Graphics &g, const YRect &r);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &button, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);

    YAction getAction() const { return fAction; }
    void setAction(YAction action);
    void setPopup(YMenu * popup);
    void setIcon(ref<YIcon> image, int size);
    void setImage(ref<YImage> image);
    void setText(const mstring &str, int hot = -1);
    mstring getText() const { return fText; }
    bool hasImage() const { return fImage != null; }
    bool hasText() const { return fText.nonempty(); }
    bool hasPopup() const { return fPopup; }

    void setPressed(int pressed);
    virtual bool isFocusTraversable();

    void updateSize();
    virtual void donePopup(YPopupWindow *popup);

    virtual void popupMenu();
    virtual void updatePopup();

    void setActionListener(YActionListener *listener) { fListener = listener; }
    YActionListener *getActionListener() const { return fListener; }

    void setSelected(bool selected);
    void setOver(bool over);

    void setArmed(bool armed, bool mousedown);
    bool isPressed() const { return fPressed; }
    bool isSelected() const { return fSelected; }
    bool isArmed() const { return fArmed; }
    bool isPopupActive() const { return fPopupActive; }

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual ref<YFont> getFont();
    virtual ref<YFont> getActiveFont();
    virtual ref<YFont> getNormalFont();
    virtual YColor   getColor();
    virtual YSurface getSurface();
    virtual YDimension getTextSize();

    void setEnabled(bool enabled);

protected:
    bool fOver;

private:
    void paint(Graphics &g, int const d, const YRect &r);

    YAction fAction;
    YMenu *fPopup;
    ref<YIcon> fIcon;
    int fIconSize;
    ref<YImage> fImage;
    mstring fText;
    int fPressed;
    bool fEnabled;
    int fHotCharPos;
    int hotKey;

    YActionListener *fListener;

    bool fSelected;
    bool fArmed;
    bool wasPopupActive;
    bool fPopupActive;

    void popup(bool mouseDown);
    void popdown();

protected:
    static YColorName normalButtonBg;
    static YColorName normalButtonFg;

    static YColorName activeButtonBg;
    static YColorName activeButtonFg;

private:
    static ref<YFont> normalButtonFont;
    static ref<YFont> activeButtonFont;
    static int buttonObjectCount;
};

#endif

// vim: set sw=4 ts=4 et:

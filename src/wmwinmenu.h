#ifndef WMWINMENU_H
#define WMWINMENU_H

class WindowListMenu:
    public YMenu,
    private YActionListener
{
    typedef YMenu super;

public:
    WindowListMenu(YActionListener *app, YWindow *parent = nullptr);

private:
    void actionPerformed(YAction action, unsigned modifiers) override;
    void activatePopup(int flags) override;
    void updatePopup() override;

    YActionListener* defer;
};

#endif

// vim: set sw=4 ts=4 et:

#ifndef SWITCHER_H
#define SWITCHER_H

class YFrameClient;
class YFrameWindow;
class YWindow;
class ISwitchItems;
class SwitchWindow;
class SwitchPreview;

class Switcher: public YPopupWindow {
public:
    Switcher(YWindow* parent) : YPopupWindow(parent) { }
    virtual void begin(bool down, unsigned mods, char* wmclass = nullptr) = 0;
    virtual void destroyedClient(YFrameClient* client) = 0;
    virtual void destroyedFrame(YFrameWindow* frame) = 0;
    virtual void createdFrame(YFrameWindow* frame) = 0;
    virtual void createdClient(YFrameWindow* frame, YFrameClient* client) = 0;
    virtual void transfer(YFrameClient* client, YFrameWindow* frame) = 0;
    virtual void hiding(YFrameClient* client) { }
    virtual bool previews() const { return false; }
    virtual YFrameWindow* current() = 0;
    void handleDamageNotify(const XDamageNotifyEvent& damage) override { }

    static Switcher* newSwitchWindow(YWindow* parent, ISwitchItems* items,
                                     bool verticalStyle);
    static Switcher* newSwitchPreview(YWindow* parent);
};

#endif

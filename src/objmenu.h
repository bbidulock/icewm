#ifndef __OBJMENU_H
#define __OBJMENU_H

#include "ymenu.h"
#include "ymenuitem.h"
#include "yaction.h"
#include "obj.h"

class DObject;

class ObjectMenu:
    public YMenu,
    public ObjectContainer,
    public YActionListener
{
public:
    ObjectMenu(YActionListener *wmActionListener, YWindow *parent = nullptr);
    virtual ~ObjectMenu();

    virtual void actionPerformed(YAction action, unsigned modifiers) override;
    virtual void addObject(DObject* object) override;
    virtual void addSeparator() override;
    virtual void addContainer(mstring name, ref<YIcon> icon, ObjectMenu *container) override;

    YMenuItem* addObject(DObject* object, const char* icon,
                         ObjectMenu* sub = nullptr, bool check = false);
    YMenuItem* getObjectItem(DObject* object);

    void setActionListener(YActionListener* al) override {
        wmActionListener = al;
    }
    YActionListener* getActionListener() const override {
        return const_cast<ObjectMenu*>(this);
    }

private:
    struct ObjectAction {
        DObject* object;
        YAction action;
        ObjectAction(DObject* obj, YAction act) : object(obj), action(act) { }
    };
    typedef YArray<ObjectAction> ArrayType;
    ArrayType fArray;

    YActionListener* wmActionListener;
};

#endif

// vim: set sw=4 ts=4 et:

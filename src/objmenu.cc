#include "config.h"
#include "objmenu.h"

ObjectMenu::ObjectMenu(YActionListener* actionListener, YWindow* parent):
    YMenu(parent),
    wmActionListener(actionListener)
{
    YMenu::setActionListener(this);
}

ObjectMenu::~ObjectMenu() {
}

void ObjectMenu::actionPerformed(YAction action, unsigned modifiers) {
    for (const ObjectAction& obj : fArray) {
        if (obj.action == action) {
            obj.object->open();
            return;
        }
    }
    if (wmActionListener) {
        wmActionListener->actionPerformed(action, modifiers);
    }
}

void ObjectMenu::addObject(DObject* object) {
    addObject(object, nullptr, nullptr, false);
}

YMenuItem* ObjectMenu::getObjectItem(DObject* object) {
    YAction action;
    YMenuItem* item = new YMenuItem(object->getName(), -3, null, action, nullptr);
    if (item) {
        fArray += ObjectAction(object, action);
    }
    return item;
}

YMenuItem* ObjectMenu::addObject(DObject* object, const char* icon,
                                 ObjectMenu* sub, bool checked) {
    YMenuItem* item = getObjectItem(object);
    if (item) {
        if (icon == nullptr) {
            item->setIcon(object->getIcon());
        } else if (*icon) {
            item->setIcon(YIcon::getIcon(icon));
        }
        if (checked) {
            item->setChecked(true);
        }
        add(item, icon);
    }
    return item;
}

void ObjectMenu::addSeparator() {
    YMenu::addSeparator();
}

void ObjectMenu::addContainer(mstring name, ref<YIcon> icon, ObjectMenu* container) {
    if (container) {
        YMenuItem* item =
            addSubmenu(name, -3, container);

        if (item && icon != null)
            item->setIcon(icon);
    }
}


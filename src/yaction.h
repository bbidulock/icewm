#ifndef __YACTION_H
#define __YACTION_H

#ifndef __WMACTION_H
#include "wmaction.h"
#endif

class YAction {
public:
    YAction() : id(next()) {}
    YAction(EAction id) : id(id) {}
    YAction(const YAction& copy) : id(copy.id) {}
    void operator=(const YAction& copy) { id = copy.id; }

    static int next() {
        static int gen(1001);
        return gen += 2;
    }

    bool operator==(const YAction& rhs) const {
        return id == rhs.id;
    }
    bool operator!=(const YAction& rhs) const {
        return id != rhs.id;
    }
    bool operator==(EAction rhs) const {
        return id == rhs;
    }
    bool operator!=(EAction rhs) const {
        return id != rhs;
    }
    bool operator<(const YAction& rhs) const {
        return id < rhs.id;
    }
    bool operator>(const YAction& rhs) const {
        return id > rhs.id;
    }
    int ident() const { return id; }

private:
    int id;
};

class YActionListener {
public:
    virtual void actionPerformed(YAction action, unsigned int modifiers) = 0;
protected:
    virtual ~YActionListener() {}
};

#endif

// vim: set sw=4 ts=4 et:

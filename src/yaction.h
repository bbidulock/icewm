#ifndef __YACTION_H
#define __YACTION_H

// Create a unique id for actions which are not handled by global EAction pool
typedef size_t tActionId;
tActionId genActionId();

class YActionListener {
public:
    virtual void actionPerformed(tActionId action, unsigned int modifiers) = 0;
protected:
    virtual ~YActionListener() {};
};

#endif

// vim: set sw=4 ts=4 et:

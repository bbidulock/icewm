#ifndef __YACTION_H
#define __YACTION_H

class YAction {
public:
};

class YActionListener {
public:
    virtual void actionPerformed(YAction *action, unsigned int modifiers) = 0;
protected:
    virtual ~YActionListener() {};
};

#endif

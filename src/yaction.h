#ifndef __YACTION_H
#define __YACTION_H

class YAction {
public:
    class Listener {
    public:
        virtual void actionPerformed(YAction *action,
                                     unsigned int modifiers) = 0;
    };
};

#endif

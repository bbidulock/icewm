#ifndef __YPOINTER_H
#define __YPOINTER_H

class YPointer {
public:
    YPointer(int aPointer) { pointer = aPointer; }

    static YPointer *left();
    static YPointer *right();
    static YPointer *move();

    static YPointer *sb_left_arrow();
    static YPointer *sb_right_arrow();


    static YPointer *resize_top_left();
    static YPointer *resize_top_right();
    static YPointer *resize_bottom_left();
    static YPointer *resize_bottom_right();
    static YPointer *resize_left();
    static YPointer *resize_right();
    static YPointer *resize_top();
    static YPointer *resize_bottom();

    int handle() { return pointer; }

private:
    int pointer;
};

#endif

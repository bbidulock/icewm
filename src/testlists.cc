/*
 *  IceWM - Test linked lists
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU General Public License
 *
 *  2001/10/05: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */

#include "config.h"
#undef DEBUG

#include "ylists.h"
#include <stdio.h>

#define newline() \
    putchar('\n')
#define validate(Expr) \
    if (Expr) { puts("   - "#Expr": succeed"); } \
    else { puts("   - "#Expr": failed"); return 1; }

class SType:
public YSingleList<SType>::Item {
public:
    SType(int value): fValue(value) {}
    int value() const { return fValue; }

private:
    int fValue;
};

class DType:
public YDoubleList<DType>::Item {
public:
    DType(int value): fValue(value) {}
    int value() const { return fValue; }

private:
    int fValue;
};

template <class IteratorType>
void dump(IteratorType iterator) {
    printf("   -");
    
    while (iterator) {
        printf(" {%d}", iterator->value());
        ++iterator;
    }
    
    newline();        
}

int main() {
    puts("Testing single linked lists"); {
        SType::List list;
        SType v1(7), v2(13), v3(23), v4(42), v5(66);

        puts(" - an empty list");
            validate(NULL == list.head());
            validate(NULL == list.tail());

        puts(" - appending one item");
            list.append(&v1);
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v1 == list.tail());

        puts(" - appending another item");
            list.append(&v2);
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v2 == list.head()->next());
            validate(&v2 == list.tail());

        puts(" - appending third item");
            list.append(&v3);
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v2 == list.head()->next());
            validate(&v3 == list.head()->next()->next());
            validate(&v3 == list.tail());

        puts(" - appending second last item");
            list.append(&v4);
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v2 == list.head()->next());
            validate(&v3 == list.head()->next()->next());
            validate(&v4 == list.head()->next()->next()->next());
            validate(&v4 == list.tail());

        puts(" - appending last item");
            list.append(&v5);
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v2 == list.head()->next());
            validate(&v3 == list.head()->next()->next());
            validate(&v4 == list.head()->next()->next()->next());
            validate(&v5 == list.head()->next()->next()->next()->next());
            validate(&v5 == list.tail());

        printf(" - content of the list:\n   - ");
            for (SType::Iterator i(list.head()); i; ++i)
                printf("%p->{%d}%s", &*i, i->value(), i->next() ? ", " : "");
            newline();

        printf(" - content of the list with counting:\n   - ");
            for (SType::CountingIterator i(list.head()); i; ++i)
                printf("%p->{%d,%d}%s", &*i, i.count(), i->value(),
                                        i->next() ? ", " : "");
            newline();

        puts(" - removing first item");
            list.remove(list.head());
            validate(NULL == list.tail()->next());
            validate(&v2 == list.head());
            validate(&v3 == list.head()->next());
            validate(&v4 == list.head()->next()->next());
            validate(&v5 == list.head()->next()->next()->next());
            validate(&v5 == list.tail());

        puts(" - prepending one item");
            list.prepend(&v1);
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v2 == list.head()->next());
            validate(&v3 == list.head()->next()->next());
            validate(&v4 == list.head()->next()->next()->next());
            validate(&v5 == list.head()->next()->next()->next()->next());
            validate(&v5 == list.tail());

        puts(" - removing second item");
            list.remove(list.head()->next());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v3 == list.head()->next());
            validate(&v4 == list.head()->next()->next());
            validate(&v5 == list.head()->next()->next()->next());
            validate(&v5 == list.tail());

        puts(" - removing second last item");
            list.remove(&v4);
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v3 == list.head()->next());
            validate(&v5 == list.head()->next()->next());
            validate(&v5 == list.tail());

        puts(" - removing last item");
            list.remove(list.tail());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v3 == list.head()->next());
            validate(&v3 == list.tail());

        puts(" - removing last item");
            list.remove(list.tail());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v1 == list.tail());

        puts(" - prepending one item");
            list.prepend(&v2);
            validate(NULL == list.tail()->next());
            validate(&v2 == list.head());
            validate(&v1 == list.head()->next());
            validate(&v1 == list.tail());

        puts(" - removing first item");
            list.remove(list.head());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v1 == list.tail());

        puts(" - removing first item");
            list.remove(list.head());
            validate(NULL == list.head());
            validate(NULL == list.tail());

        puts(" - prepending one item");
            list.prepend(&v1);
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v1 == list.tail());

        puts(" - all tests passed");
    }

    puts("Testing double linked lists"); {
        DType::List list;
        DType v1(7), v2(13), v3(23), v4(42), v5(66);

        puts(" - an empty list");
            validate(NULL == list.head());
            validate(NULL == list.tail());

        puts(" - appending one item");
            list.append(&v1);
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v1 == list.tail());

        puts(" - appending another item");
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            list.append(&v2);
            validate(&v1 == list.head());
            validate(&v2 == list.head()->next());
            validate(&v2 == list.tail());
            validate(&v1 == list.tail()->prev());

        puts(" - appending third item");
            list.append(&v3);
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v2 == list.head()->next());
            validate(&v3 == list.head()->next()->next());
            validate(&v3 == list.tail());
            validate(&v2 == list.tail()->prev());
            validate(&v1 == list.tail()->prev()->prev());

        puts(" - appending second last item");
            list.append(&v4);
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v2 == list.head()->next());
            validate(&v3 == list.head()->next()->next());
            validate(&v4 == list.head()->next()->next()->next());
            validate(&v4 == list.tail());
            validate(&v3 == list.tail()->prev());
            validate(&v2 == list.tail()->prev()->prev());
            validate(&v1 == list.tail()->prev()->prev()->prev());

        puts(" - appending last item");
            list.append(&v5);
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v2 == list.head()->next());
            validate(&v3 == list.head()->next()->next());
            validate(&v4 == list.head()->next()->next()->next());
            validate(&v5 == list.head()->next()->next()->next()->next());
            validate(&v5 == list.tail());
            validate(&v4 == list.tail()->prev());
            validate(&v3 == list.tail()->prev()->prev());
            validate(&v2 == list.tail()->prev()->prev()->prev());
            validate(&v1 == list.tail()->prev()->prev()->prev()->prev());

        printf(" - content of the list:\n   - ");
        for (DType::Iterator i(list.head()); i; ++i)
            printf("%p->{%d}%s", &*i, i->value(), i->next() ? ", " : "");
        newline();

        printf(" - content of the list with counting:\n   - ");
        for (DType::CountingIterator i(list.head()); i; ++i)
            printf("%p->{%d,%d}%s", &*i, i.count(), i->value(),
                                    i->next() ? ", " : "");
        newline();

        puts(" - removing first item");
            list.remove(list.head());
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v2 == list.head());
            validate(&v3 == list.head()->next());
            validate(&v4 == list.head()->next()->next());
            validate(&v5 == list.head()->next()->next()->next());
            validate(&v5 == list.tail());
            validate(&v4 == list.tail()->prev());
            validate(&v3 == list.tail()->prev()->prev());
            validate(&v2 == list.tail()->prev()->prev()->prev());

        puts(" - prepending one item");
            list.prepend(&v1);
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v2 == list.head()->next());
            validate(&v3 == list.head()->next()->next());
            validate(&v4 == list.head()->next()->next()->next());
            validate(&v5 == list.head()->next()->next()->next()->next());
            validate(&v5 == list.tail());
            validate(&v4 == list.tail()->prev());
            validate(&v3 == list.tail()->prev()->prev());
            validate(&v2 == list.tail()->prev()->prev()->prev());
            validate(&v1 == list.tail()->prev()->prev()->prev()->prev());

        puts(" - removing second item");
            list.remove(list.head()->next());
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v3 == list.head()->next());
            validate(&v4 == list.head()->next()->next());
            validate(&v5 == list.head()->next()->next()->next());
            validate(&v5 == list.tail());
            validate(&v4 == list.tail()->prev());
            validate(&v3 == list.tail()->prev()->prev());
            validate(&v1 == list.tail()->prev()->prev()->prev());

        puts(" - removing second last item");
            list.remove(&v4);
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v3 == list.head()->next());
            validate(&v5 == list.head()->next()->next());
            validate(&v5 == list.tail());
            validate(&v3 == list.tail()->prev());
            validate(&v1 == list.tail()->prev()->prev());

        puts(" - removing last item");
            list.remove(list.tail());
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v3 == list.head()->next());
            validate(&v3 == list.tail());
            validate(&v1 == list.tail()->prev());

        puts(" - removing last item");
            list.remove(list.tail());
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v1 == list.tail());

        puts(" - prepending one item");
            list.prepend(&v2);
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v2 == list.head());
            validate(&v1 == list.head()->next());
            validate(&v1 == list.tail());
            validate(&v2 == list.tail()->prev());

        puts(" - removing first item");
            list.remove(list.head());
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v1 == list.tail());

        puts(" - removing first item");
            list.remove(list.head());
            validate(NULL == list.head());
            validate(NULL == list.tail());

        puts(" - prepending one item");
            list.prepend(&v1);
            validate(NULL == list.head()->prev());
            validate(NULL == list.tail()->next());
            validate(&v1 == list.head());
            validate(&v1 == list.tail());

        puts(" - all tests passed");
    }

    puts("Testing iterators"); {
        DType::List list;
        DType v1(7), v2(13), v3(23), v4(42), v5(66);
        
        puts(" - filling list");
            list.append(&v1);
            list.append(&v2);
            list.append(&v3);
            list.append(&v4);
            list.append(&v5);

        puts(" - testing forward iterator");
            DType::ForwardIterator i1(list);
            validate(i1); validate(&v1 == &*i1);
            validate(i1); validate(&v2 == &*++i1);
            validate(i1); validate(&v3 == &*++i1);
            validate(i1); validate(&v4 == &*++i1);
            validate(i1); validate(&v5 == &*++i1);
            validate(i1); validate(NULL == &*++i1);
            validate(i1 == false);
            
        puts(" - testing backward iterator");
            DType::BackwardIterator i2(list);
            validate(i2); validate(&v5 == &*i2);
            validate(i2); validate(&v4 == &*++i2);
            validate(i2); validate(&v3 == &*++i2);
            validate(i2); validate(&v2 == &*++i2);
            validate(i2); validate(&v1 == &*++i2);
            validate(i2); validate(NULL == &*++i2);
            validate(i2 == false);
            
        puts(" - testing counting forward iterator");
            DType::CountingIterator i3(list);
            validate(i3); validate(0 == i3.count()); validate(&v1 == &*i3);
            validate(i3); validate(0 == i3.count()); validate(&v2 == &*++i3);
            validate(i3); validate(1 == i3.count()); validate(&v3 == &*++i3);
            validate(i3); validate(2 == i3.count()); validate(&v4 == &*++i3);
            validate(i3); validate(3 == i3.count()); validate(&v5 == &*++i3);
            validate(i3); validate(4 == i3.count()); validate(NULL == &*++i3);
            validate(i3 == false); validate(5 == i3.count()); 

        puts(" - testing counting backward iterator");
            DType::CountingBackwardIterator i4(list);
            validate(i4); validate(0 == i4.count()); validate(&v5 == &*i4);
            validate(i4); validate(0 == i4.count()); validate(&v4 == &*++i4);
            validate(i4); validate(1 == i4.count()); validate(&v3 == &*++i4);
            validate(i4); validate(2 == i4.count()); validate(&v2 == &*++i4);
            validate(i4); validate(3 == i4.count()); validate(&v1 == &*++i4);
            validate(i4); validate(4 == i4.count()); validate(NULL == &*++i4);
            validate(i4 == false); validate(5 == i4.count()); 
            
        puts(" - testing forward ring iterator, starting at head");
            DType::ForwardRingIterator i5(list);
            validate(i5); validate(&v1 == &*i5);
            validate(i5); validate(&v2 == &*++i5);
            validate(i5); validate(&v3 == &*++i5);
            validate(i5); validate(&v4 == &*++i5);
            validate(i5); validate(&v5 == &*++i5);
            validate(i5); validate(&v1 == &*++i5);
            validate(i5 == false);
            
        puts(" - testing forward ring iterator, starting at 3rd element");
            DType::ForwardRingIterator i6(list, &v3);
            validate(i6); validate(&v3 == &*i6);
            validate(i6); validate(&v4 == &*++i6);
            validate(i6); validate(&v5 == &*++i6);
            validate(i6); validate(&v1 == &*++i6);
            validate(i6); validate(&v2 == &*++i6);
            validate(i6); validate(&v3 == &*++i6);
            validate(i6 == false);
            
        puts(" - testing forward ring iterator, starting at last element");
            DType::ForwardRingIterator i7(list, list.tail());
            validate(i7); validate(&v5 == &*i7);
            validate(i7); validate(&v1 == &*++i7);
            validate(i7); validate(&v2 == &*++i7);
            validate(i7); validate(&v3 == &*++i7);
            validate(i7); validate(&v4 == &*++i7);
            validate(i7); validate(&v5 == &*++i7);
            validate(i7 == false);
            
        puts(" - testing backward ring iterator, starting at last element");
            DType::BackwardRingIterator i8(list);
            validate(i8); validate(&v5 == &*i8);
            validate(i8); validate(&v4 == &*++i8);
            validate(i8); validate(&v3 == &*++i8);
            validate(i8); validate(&v2 == &*++i8);
            validate(i8); validate(&v1 == &*++i8);
            validate(i8); validate(&v5 == &*++i8);
            validate(i8 == false);
            
        puts(" - testing backward ring iterator, starting at 3rd element");
            DType::BackwardRingIterator i9(list, &v3);
            validate(i9); validate(&v3 == &*i9);
            validate(i9); validate(&v2 == &*++i9);
            validate(i9); validate(&v1 == &*++i9);
            validate(i9); validate(&v5 == &*++i9);
            validate(i9); validate(&v4 == &*++i9);
            validate(i9); validate(&v3 == &*++i9);
            validate(i9 == false);
            
        puts(" - testing backward ring iterator, starting at head");
            DType::BackwardRingIterator i10(list, list.head());
            validate(i10); validate(&v1 == &*i10);
            validate(i10); validate(&v5 == &*++i10);
            validate(i10); validate(&v4 == &*++i10);
            validate(i10); validate(&v3 == &*++i10);
            validate(i10); validate(&v2 == &*++i10);
            validate(i10); validate(&v1 == &*++i10);
            validate(i10 == false);

        dump(DType::ForwardRingIterator(list, &v3));
        dump(DType::BackwardRingIterator(list, &v3));
    }

    return 0;        
}

/*
 *  IceWM - Linked lists
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU General Public License
 *
 *  2001/10/05: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */

#ifndef __YLISTS_H
#define __YLISTS_H

#include "base.h"
#include <stddef.h>

/*******************************************************************************
 * Iterators
 ******************************************************************************/

template <class ListType, class ContentType>
class YForwardIterator {
public:
    YForwardIterator(ListType & list, ContentType * end = NULL):
        fPos(list.head()), fEnd(end) {}
    YForwardIterator(ContentType * begin, ContentType * end = NULL):
        fPos(begin), fEnd(end) {}

    operator bool () const { return fPos != fEnd; }
    operator ContentType * () const { return fPos; }
    ContentType & operator * () const { return *fPos; }
    ContentType * operator -> () const { return fPos; }
    YForwardIterator & operator ++ () { return extend(); }
    
    YForwardIterator & extend (void) {
        if (fPos && fPos != fEnd) fPos = fPos->next();
        return *this;
    }

private:
    ContentType * fPos, * fEnd;
};

template <class ListType, class ContentType>
class YBackwardIterator {
public:
    YBackwardIterator(ListType & list, ContentType * end = NULL):
        fPos(list.tail()), fEnd(end) {}
    YBackwardIterator(ContentType * begin, ContentType * end = NULL):
        fPos(begin), fEnd(end) {}

    operator bool () const { return fPos != fEnd; }
    operator ContentType * () const { return fPos; }
    ContentType & operator * () const { return *fPos; }
    ContentType * operator -> () const { return fPos; }
    YBackwardIterator & operator -- () { return extend(); }

    YBackwardIterator & extend (void) {
        if (fPos && fPos != fEnd) fPos = fPos->prev();
        return *this;
    }

private:
    ContentType * fPos, * fEnd;
};

/******************************************************************************/

template <class IteratorBase, class ListType, class ContentType>
class YCountingIterator:
public IteratorBase {
public:
    YCountingIterator(ListType & list, ContentType * end = NULL):
        IteratorBase(list, end), fCount(0) {}
    YCountingIterator(ContentType * begin, ContentType * end = NULL):
        IteratorBase(begin, end), fCount(0) {}

    unsigned count() const { return fCount; }
    YCountingIterator & extend() { IteratorBase::extend(); return *this; }

private:
    unsigned fCount;
};

template <class ListType, class ContentType>
class YCountingForwardIterator:
public YCountingIterator <YForwardIterator <ListType, ContentType>,
                          ListType, ContentType> {
public:                 
    YCountingForwardIterator(ListType & list, ContentType * end = NULL):
        YCountingIterator <YForwardIterator <ListType, ContentType>,
                           ListType, ContentType>(list, end) {}
    YCountingForwardIterator(ContentType * begin, ContentType * end = NULL):
        YCountingIterator <YForwardIterator <ListType, ContentType>,
                           ListType, ContentType>(begin, end) {}

    YCountingForwardIterator & operator ++ () { extend(); return *this; }
};

template <class ListType, class ContentType>
class YCountingBackwardIterator:
public YCountingIterator <YBackwardIterator <ListType, ContentType>,
                          ListType, ContentType> {
public:                 
    YCountingBackwardIterator(ListType & list, ContentType * end = NULL):
        YCountingIterator <YBackwardIterator <ListType, ContentType>,
                           ListType, ContentType>(list, end) {}
    YCountingBackwardIterator(ContentType * begin, ContentType * end = NULL):
        YCountingIterator <YBackwardIterator <ListType, ContentType>,
                           ListType, ContentType>(begin, end) {}

    YCountingBackwardIterator & operator ++ () { extend(); return *this; }
};

/*******************************************************************************
 * A single linked list
 ******************************************************************************/

template <class ContentType>
class YSingleList {
public:
    typedef YForwardIterator<YSingleList, ContentType> Iterator;
    typedef YCountingForwardIterator<YSingleList, ContentType> CountingIterator;

    class Item {
    public:
        typedef YSingleList List;
        typedef YSingleList::Iterator Iterator;
        typedef YSingleList::CountingIterator CountingIterator;

        Item(ContentType * next = NULL): fNext(next) {}
    	ContentType * next() const { return fNext; }

    protected:
        friend class YSingleList;
        void next(ContentType * next) { fNext = next; }

    private:
        ContentType * fNext;
    };
    
    YSingleList(): fHead(NULL), fTail(NULL) {}

    ContentType * head() const { return fHead; }
    ContentType * tail() const { return fTail; }
    
    bool empty() const { return NULL == fHead; }
    bool filled() const { return NULL != fHead; }
    
    void append(ContentType * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        if (NULL != fTail) fTail->next(item);
        else fHead = fTail = item;

        while (NULL != fTail->next()) { fTail = fTail->next(); }
    }

    void prepend(ContentType * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        item->next(fHead);

        if (NULL != fTail) fHead = item;
        else fHead = fTail = item;
    }

    void remove(ContentType * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        if (item != fHead) {
            ContentType * predecessor(fHead), * next;
            while ((next = predecessor->next()) != item) predecessor = next;
            predecessor->next(item->next());
            if (item == fTail) fTail = predecessor;
        } else {
            fHead = item->next();
            if (item == fTail) fTail = NULL;
        }
    }

    void destroy(ContentType * item) {
        remove(item);
        delete item;
    }

private:
    ContentType * fHead;
    ContentType * fTail;
};

/*******************************************************************************
 * A double linked list
 ******************************************************************************/

template <class ContentType>
class YDoubleList {
public:
    typedef YForwardIterator<YDoubleList, ContentType> Iterator;
    typedef YForwardIterator<YDoubleList, ContentType> ForwardIterator;
    typedef YBackwardIterator<YDoubleList, ContentType> BackwardIterator;

    typedef YCountingForwardIterator<YDoubleList, ContentType> CountingIterator;

    class Item {
    public:
        typedef YDoubleList List;
        typedef YDoubleList::Iterator Iterator;
        typedef YDoubleList::CountingIterator CountingIterator;

        Item(ContentType * prev = NULL, ContentType * next = NULL): 
            fPrev(prev), fNext(next) {}

    	ContentType * prev() const { return fPrev; }
    	ContentType * next() const { return fNext; }

    protected:
        friend class YDoubleList;
        void prev(ContentType * prev) { fPrev = prev; }
        void next(ContentType * next) { fNext = next; }

    private:
        ContentType * fPrev, * fNext;
    };
    
    YDoubleList(): fHead(NULL), fTail(NULL) {}

    ContentType * head() const { return fHead; }
    ContentType * tail() const { return fTail; }
    
    bool empty() const { return NULL == fHead; }
    bool filled() const { return NULL != fHead; }
    
    void append(ContentType * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fHead || NULL == fHead->prev());
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        item->prev(fTail);

        if (NULL != fTail) fTail->next(item);
        else fHead = fTail = item;

        while (NULL != fTail->next()) { fTail = fTail->next(); }
    }

    void prepend(ContentType * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fHead || NULL == fHead->prev());
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        item->next(fHead);

        if (NULL != fHead) fHead->prev(item);
        if (NULL != fTail) fHead = item;
        else fHead = fTail = item;
    }

    void remove(ContentType * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        if (item->next())
            item->next()->prev(item->prev());
        else
            fTail = item->prev();

        if (item->prev())
            item->prev()->next(item->next());
        else
            fHead = item->next();

        item->prev(NULL);
        item->next(NULL);
    }

    void destroy(ContentType * item) {
        remove(item);
        delete item;
    }

private:
    ContentType * fHead;
    ContentType * fTail;
};

#endif

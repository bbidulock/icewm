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

template <class IteratorType, class ListType, class ContentType>
class YIterator {
protected:
    typedef YIterator Iterator;
    typedef typename ListType::Item Item;

protected:
    YIterator(ContentType * begin): fPos(begin) {}

public:
    operator bool () const { return valid(); }
    operator ContentType * () const { return fPos; }
    ContentType & operator * () const { return *fPos; }
    ContentType * operator -> () const { return fPos; }

    IteratorType & operator ++ () {
        extend(); return * static_cast<IteratorType *>(this);
    }

    virtual inline bool valid() const = 0;
    virtual inline void extend() = 0;

protected:
    ContentType * fPos;
};

/******************************************************************************/

template <class ListType, class ContentType>
class YForwardIterator:
public YIterator<YForwardIterator<ListType, ContentType>,
                 ListType, ContentType> {
public:
    YForwardIterator(ListType const & list, ContentType * end = NULL):
        Iterator(list.head()), fEnd(end) {}
    YForwardIterator(ContentType * begin, ContentType * end = NULL):
        Iterator(begin), fEnd(end) {}

    virtual bool valid() const {
        return fPos != fEnd;
    }

    virtual void extend() {
        if (fPos && fPos != fEnd) fPos = fPos->next();
    }

private:
    ContentType * fEnd;
};

template <class ListType, class ContentType>
class YBackwardIterator:
public YIterator<YBackwardIterator<ListType, ContentType>,
                 ListType, ContentType> {
public:
    YBackwardIterator(ListType const & list, ContentType * end = NULL):
        Iterator(list.tail()), fEnd(end) {}
    YBackwardIterator(ContentType * begin, ContentType * end = NULL):
        Iterator(begin), fEnd(end) {}

    virtual bool valid() const {
        return fPos != fEnd;
    }

    virtual void extend() {
        if (fPos && fPos != fEnd) fPos = fPos->prev();
    }

private:
    ContentType * fEnd;
};

/******************************************************************************/

template <class ListType, class ContentType>
class YForwardRingIterator:
public YIterator<YForwardRingIterator<ListType, ContentType>,
                 ListType, ContentType> {
public:
    YForwardRingIterator(ListType const & list):
        Iterator(list.head()), fList(list), fBegin(NULL) {}
    YForwardRingIterator(ListType const & list, ContentType * begin):
        Iterator(begin), fList(list), fBegin(NULL) {}

    virtual bool valid () const {
        return fPos != fBegin;
    }

    virtual void extend() {
        if (fPos && fPos != fBegin) {
            if (NULL == fBegin) fBegin = fPos;
            fPos = fPos->next();
            if (NULL == fPos) fPos = fList.head();
        }
    }

private:
    ListType const & fList;
    ContentType * fBegin;
};

template <class ListType, class ContentType>
class YBackwardRingIterator:
public YIterator<YBackwardRingIterator<ListType, ContentType>,
                 ListType, ContentType> {
public:
    YBackwardRingIterator(ListType const & list):
        Iterator(list.tail()), fList(list), fBegin(NULL) {}
    YBackwardRingIterator(ListType const & list, ContentType * begin):
        Iterator(begin), fList(list), fBegin(NULL) {}

    virtual bool valid () const {
        return fPos != fBegin;
    }

    virtual void extend() {
        if (fPos && fPos != fBegin) {
            if (NULL == fBegin) fBegin = fPos;
            fPos = fPos->prev();
            if (NULL == fPos) fPos = fList.tail();
        }
    }

private:
    ListType const & fList;
    ContentType * fBegin;
};

/******************************************************************************/

template <class IteratorBase, class ListType, class ContentType>
class YCountingIterator:
public IteratorBase {
protected:
    typedef YCountingIterator CountingIterator;

public:
    YCountingIterator(ListType const & list, ContentType * end = NULL):
        IteratorBase(list, end), fCount(0) {}
    YCountingIterator(ContentType * begin, ContentType * end = NULL):
        IteratorBase(begin, end), fCount(0) {}

    unsigned count() const { return fCount; }
    virtual void extend() { IteratorBase::extend(); ++fCount; }

private:
    unsigned fCount;
};

template <class ListType, class ContentType>
class YCountingForwardIterator:
public YCountingIterator <YForwardIterator <ListType, ContentType>,
                          ListType, ContentType> {
public:
    YCountingForwardIterator(ListType const & list, ContentType * end = NULL):
        CountingIterator(list, end) {}
    YCountingForwardIterator(ContentType * begin, ContentType * end = NULL):
        CountingIterator(begin, end) {}

    YCountingForwardIterator & operator ++ () { extend(); return *this; }
};

template <class ListType, class ContentType>
class YCountingBackwardIterator:
public YCountingIterator <YBackwardIterator <ListType, ContentType>,
                          ListType, ContentType> {
public:
    YCountingBackwardIterator(ListType const & list, ContentType * end = NULL):
        CountingIterator(list, end) {}
    YCountingBackwardIterator(ContentType * begin, ContentType * end = NULL):
        CountingIterator(begin, end) {}

    YCountingBackwardIterator & operator ++ () { extend(); return *this; }
};

/*******************************************************************************
 * A single linked list
 ******************************************************************************/

template <class ContentType>
class YSingleList {
public:
    typedef YForwardIterator<YSingleList, ContentType> Iterator;
    typedef YForwardRingIterator<YSingleList, ContentType> RingIterator;
    typedef YCountingForwardIterator<YSingleList, ContentType> CountingIterator;

    class Item {
    public:
        typedef YSingleList List;
        typedef YSingleList::Iterator Iterator;
        typedef YSingleList::RingIterator RingIterator;
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
    ContentType * fHead, * fTail;
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

    typedef YForwardRingIterator<YDoubleList, ContentType> RingIterator;
    typedef YForwardRingIterator<YDoubleList, ContentType> ForwardRingIterator;
    typedef YBackwardRingIterator<YDoubleList, ContentType> BackwardRingIterator;

    typedef YCountingForwardIterator<YDoubleList, ContentType>
                                     CountingIterator;
    typedef YCountingForwardIterator<YDoubleList, ContentType>
                                     CountingForwardIterator;
    typedef YCountingBackwardIterator<YDoubleList, ContentType>
                                      CountingBackwardIterator;

    class Item {
    public:
        typedef YDoubleList List;
        typedef YDoubleList::Iterator Iterator;
        typedef YDoubleList::ForwardIterator ForwardIterator;
        typedef YDoubleList::BackwardIterator BackwardIterator;

        typedef YDoubleList::RingIterator RingIterator;
        typedef YDoubleList::ForwardRingIterator ForwardRingIterator;
        typedef YDoubleList::BackwardRingIterator BackwardRingIterator;

        typedef YDoubleList::CountingIterator CountingIterator;
        typedef YDoubleList::CountingForwardIterator CountingForwardIterator;
        typedef YDoubleList::CountingBackwardIterator CountingBackwardIterator;

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

    ContentType * head() const { return static_cast<ContentType *>(fHead); }
    ContentType * tail() const { return static_cast<ContentType *>(fTail); }

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

        static_cast<Item *>(item)->next(static_cast<ContentType *>(fHead));

        if (NULL != fHead) fHead->prev(item);
        if (NULL != fTail) fHead = item;
        else fHead = fTail = item;
    }

    void remove(ContentType * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        if (item->next()) item->next()->prev(item->prev());
        else fTail = item->prev();

        if (item->prev()) item->prev()->next(item->next());
        else fHead = item->next();

        item->prev(NULL);
        item->next(NULL);
    }

    void destroy(ContentType * item) {
        remove(item);
        delete item;
    }
    
    unsigned count() { return count(head()); }

    unsigned count(ContentType * begin) {
        CountingForwardIterator i(begin);
        while (i) ++i; return i.count();
    }

    unsigned position(ContentType * item) {
        CountingBackwardIterator i(item);
        while (i) ++i; return i.count();
    }

private:
    ContentType * fHead, * fTail;
};

#endif

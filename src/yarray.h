/*
 *  IceWM - Simple dynamic array
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/04/14: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 *  2002/07/31: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - major rewrite of the code
 *  - introduced YBaseArray to reduce overhead caused by templates
 *  - introduced YObjectArray for easy memory management
 *  - introduced YStringArray
 */

#ifndef __YARRAY_H
#define __YARRAY_H

#include "base.h"
#include "ref.h"

template <class DataType>
class YArray;
template <class DataType, class ArrayType = YArray<DataType> >
class YArrayIterator;

/*******************************************************************************
 * A dynamic array for anonymous data
 ******************************************************************************/

class YBaseArray {
public:
    typedef int SizeType;
    typedef unsigned char StorageType;

    explicit YBaseArray(SizeType elementSize):
        fElementSize(elementSize), fCapacity(0), fCount(0), fElements(0) {}
    YBaseArray(YBaseArray &other);
    virtual ~YBaseArray() { clear(); }

    void append(const void *item);
    void insert(const SizeType index, const void *item);
    virtual void remove(const SizeType index);
    virtual void clear();
    virtual void shrink(const SizeType reducedCount);

    SizeType getCapacity() const { return fCapacity; }
    SizeType getCount() const { return fCount; }
    bool isEmpty() const { return 0 == getCount(); }

    void setCapacity(SizeType nCapacity);

    static const SizeType npos = (SizeType) -1;

protected:
    const StorageType *getElement(const SizeType index) const {
        return fElements + (index * fElementSize);
    }
    StorageType *getElement(const SizeType index) {
        return fElements + (index * fElementSize);
    }

    const void *getBegin() const { return getElement(0); }
    const void *getEnd() const { return getElement(getCount()); }

    void release();
    void swap(YBaseArray& other);

public:
    SizeType getIndex(void const * ptr) const {
        PRECONDITION(ptr >= getBegin() && ptr < getEnd());
        return (ptr >= getBegin() && ptr < getEnd()
                ? ((StorageType *) ptr - fElements) / fElementSize : npos);
    }
    const void *getItem(const SizeType index) const {
        PRECONDITION(index < getCount());
        return (index < getCount() ? getElement(index) : 0);
    }
    void *getItem(const SizeType index) {
        PRECONDITION(index < getCount());
        return (index < getCount() ? getElement(index) : 0);
    }

    const void *operator[](const SizeType index) const {
        return getItem(index);
    }
    void *operator[](const SizeType index) {
        return getItem(index);
    }


private:
    YBaseArray(const YBaseArray &); // not implemented
    void operator=(const YBaseArray&); // not implemented

    const SizeType fElementSize;
    SizeType fCapacity, fCount;
    StorageType *fElements;
};

/*******************************************************************************
 * A dynamic array for typed data
 ******************************************************************************/

template <class DataType>
class YArray: public YBaseArray {
public:
    typedef YArrayIterator<DataType> IterType;

    YArray(): YBaseArray(sizeof(DataType)) {}
    YArray(YArray &other): YBaseArray((YBaseArray&)other) {}
    explicit YArray(SizeType capacity) { setCapacity(capacity); }

    void append(const DataType &item) {
        YBaseArray::append(&item);
    }
    void insert(const SizeType index, const DataType &item) {
        YBaseArray::insert(index, &item);
    }

    const DataType *getItemPtr(const SizeType index) const {
        return (const DataType *) YBaseArray::getItem(index);
    }
    const DataType &getItem(const SizeType index) const {
        return *getItemPtr(index);
    }
    const DataType &operator[](const SizeType index) const {
        return getItem(index);
    }
    const DataType &operator*() const {
        return getItem(0);
    }

    DataType *getItemPtr(const SizeType index) {
        return (DataType *) YBaseArray::getItem(index);
    }
    DataType &getItem(const SizeType index) {
        return *getItemPtr(index);
    }
    DataType &operator[](const SizeType index) {
        return getItem(index);
    }
    DataType &operator*() {
        return getItem(0);
    }
    void swap(const SizeType index1, const SizeType index2) {
        ::swap(getItem(index1), getItem(index2));
    }
    void swap(YArray<DataType>& other) {
        YBaseArray::swap(other);
    }
    IterType iterator();
    IterType reverseIterator();
};

/*******************************************************************************
 * An array of objects
 ******************************************************************************/

template <class DataType>
class YObjectArray: public YArray<DataType *> {
public:
    typedef YArray<DataType *> BaseType;
    typedef typename BaseType::SizeType SizeType;
    using BaseType::getCount;
    using BaseType::getItem;

    virtual ~YObjectArray() {
        clear();
    }

    virtual void remove(const SizeType index) {
        if (index < getCount()) {
             delete getItem(index);
             BaseType::remove(index);
        }
    }

    virtual void clear() {
        for (SizeType n = getCount(); n > 0; )
            delete getItem(--n);
        BaseType::clear();
    }

    virtual void shrink(int reducedCount) {
        for (SizeType n = getCount(); n > reducedCount; )
            delete getItem(--n);
        BaseType::shrink(reducedCount);
    }
};

template <class DataType>
class YRefArray: public YBaseArray {
public:
    YRefArray(): YBaseArray(sizeof(ref<DataType>)) {}
    ~YRefArray() { clear(); }

    void append(ref<DataType> item) {
        item.__ref();
        YBaseArray::append(&item);
    }
    void insert(const SizeType index, ref<DataType> item) {
        item.__ref();
        YBaseArray::insert(index, &item);
    }

    ref<DataType> getItem(const SizeType index) const {
        return *getItemPtr(index);
    }
    ref<DataType> operator[](const SizeType index) const {
        return getItem(index);
    }

    virtual void remove(const SizeType index) {
        if (index < getCount()) {
            getItemPtr(index)->__unref();
            YBaseArray::remove(index);
        }
    }

    virtual void clear() {
        for (SizeType i = 0; i < getCount(); ++i)
            getItemPtr(i)->__unref();
        YBaseArray::clear();
    }

    virtual void shrink(int reducedCount) {
        for (SizeType n = getCount(); n > reducedCount; )
            getItemPtr(--n)->__unref();
        YBaseArray::shrink(reducedCount);
    }

private:
    ref<DataType>* getItemPtr(const SizeType index) const {
        return (ref<DataType> *) YBaseArray::getItem(index);
    }
};

/*******************************************************************************
 * An array of strings
 ******************************************************************************/

class YStringArray: public YArray<const char *> {
public:
    typedef YArray<const char *> BaseType;
    typedef BaseType::IterType IterType;

    YStringArray(YStringArray &other): BaseType((BaseType&)other) { }
    YStringArray(const YStringArray &other);

    explicit YStringArray(SizeType capacity = 0) {
        setCapacity(capacity);
    }

    virtual ~YStringArray() {
        clear();
    }

    void append(const char *str) {
        char *s = newstr(str);
        YBaseArray::append(&s);
    }
    void insert(const SizeType index, const char *str) {
        char *s = newstr(str);
        YBaseArray::insert(index, &s);
    }

    const char *getString(const SizeType index) const {
        return *(const char **) YBaseArray::getItem(index);
    }
    const char *operator[](const SizeType index) const {
        return getString(index);
    }
    const char *operator*() const {
        return getString(0);
    }

    virtual void remove(const SizeType index);
    virtual void clear();
    virtual void shrink(int reducedSize);

    virtual SizeType find(const char *str);

    char *const *getCArray() const;
    char **release();
};

/*******************************************************************************
 * A stack emulated by a dynamic array
 ******************************************************************************/

template <class DataType>
class YStack: public YArray<DataType> {
public:
    using YArray<DataType>::getCount;

    const DataType &getTop() const {
        PRECONDITION(getCount() > 0);
        return getItem(getCount() - 1);
    }
    const DataType &operator*() const { return getTop(); }

    virtual void push(const DataType &item) { append(item); }
    void pop() {
        PRECONDITION(getCount() > 0);
        remove(getCount() - 1);
    }
};

/*******************************************************************************
 * A set emulated by a stack
 ******************************************************************************/

template <class DataType>
class YStackSet: public YStack<DataType> {
public:
    virtual void push(const DataType &item) {
        findRemove(*this, item);
        YStack<DataType>::push(item);
    }
};

/*******************************************************************************
 * An array of mstrings
 ******************************************************************************/

#ifdef __MSTRING_H
class MStringArray: public YArray<mstring> {
public:
    virtual ~MStringArray() { clear(); }

    void append(mstring& item) {
        item.acquire();
        YBaseArray::append(&item);
    }
    void insert(const SizeType index, mstring& item) {
        item.acquire();
        YBaseArray::insert(index, &item);
    }

    mstring& getItem(const SizeType index) const {
        return *getItemPtr(index);
    }
    mstring& operator[](const SizeType index) const {
        return getItem(index);
    }

    virtual void remove(const SizeType index) {
        if (index < getCount()) {
            getItemPtr(index)->release();
            YBaseArray::remove(index);
        }
    }

    virtual void clear() {
        for (SizeType i = 0; i < getCount(); ++i)
            getItemPtr(i)->release();
        YBaseArray::clear();
    }

    virtual void shrink(int reducedCount) {
        for (SizeType n = getCount(); n > reducedCount; )
            getItemPtr(--n)->release();
        YBaseArray::shrink(reducedCount);
    }

private:
    mstring* getItemPtr(const SizeType index) const {
        return (mstring *) YBaseArray::getItem(index);
    }
};
#endif  /*__MSTRING_H*/

/*******************************************************************************
 * An array iterator
 ******************************************************************************/

template <class DataType, class ArrayType>
class YArrayIterator {
public:
    typedef YArrayIterator<DataType, ArrayType> IterType;

private:
    ArrayType *array;
    int step;
    int index;

    bool validate(int extra) const {
        return inrange(index + extra, 0, (int) array->getCount() - 1);
    }
    IterType& move(int amount) {
        index += amount;
        return *this;
    }

public:
    // initially the iterator is invalid until next() is called.
    YArrayIterator(ArrayType *arr, bool reverse):
        array(arr),
        step(reverse ? -1 : 1),
        index(reverse ? arr->getCount() : -1)
    {}
    bool hasNext() const {
        return validate(step);
    }
    bool hasPrev() const {
        return validate(-step);
    }
    bool isValid() const {
        return validate(0);
    }
    DataType& get() const {
        return (*array)[index];
    }
    DataType& operator*() const {
        return get();
    }
    IterType& operator++() {
        return move(step);
    }
    IterType& operator--() {
        return move(-step);
    }
    DataType& next() {
        return move(step).get();
    }
    DataType& prev() {
        return move(-step).get();
    }
    operator bool() const {
        return isValid();
    }
    operator DataType&() const {
        return get();
    }
    DataType& operator->() const {
        return get();
    }
    int where() const {
        return index;
    }
    IterType& remove() {
        if (isValid())
            array->remove(where());
        return *this;
    }
};

template<class DataType>
YArrayIterator<DataType> YArray<DataType>::iterator() {
    return YArrayIterator<DataType>(this, false);
}

template<class DataType>
YArrayIterator<DataType> YArray<DataType>::reverseIterator() {
    return YArrayIterator<DataType>(this, true);
}

template<class DataType>
int find(YArray<DataType>& array, DataType& data) {
    YArrayIterator<DataType> iter = array.iterator();
    while (++iter)
        if (*iter == data) return iter.where();
    return -1;
}

template<class DataType>
int find(const YArray<DataType>& array, const DataType& data) {
    for (int i = 0; i < array.getCount(); ++i)
        if (array[i] == data) return i;
    return -1;
}

template<class DataType>
void findRemove(YArray<DataType>& array, DataType& data) {
    int k = find(array, data);
    if (k >= 0)
        array.remove(k);
}
#endif

// vim: set sw=4 ts=4 et:

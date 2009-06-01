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

#include "config.h"
#include "base.h"
#include "ref.h"

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
    YBaseArray(const YBaseArray &) {} // not implemented

    SizeType fElementSize, fCapacity, fCount;
    StorageType *fElements;
};

/*******************************************************************************
 * A dynamic array for typed data
 ******************************************************************************/

template <class DataType>
class YArray: public YBaseArray {
public:
    YArray(): YBaseArray(sizeof(DataType)) {}

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
#if 0
    virtual SizeType find(const DataType &item) {
        for (SizeType i = 0; i < getCount(); ++i)
            if (getItem(i) == item) return i;

        return npos;
    }
#endif
};

/*******************************************************************************
 * An array of objects
 ******************************************************************************/

template <class DataType>
class YObjectArray: public YArray<DataType *> {
public:
    virtual ~YObjectArray() {
        clear();
    }

    virtual void remove(const typename YArray<DataType *>::SizeType index) {
        if (index < YArray<DataType *>::getCount()) delete getItem(index);
        YArray<DataType *>::remove(index);
    }
    
    virtual void clear() {
        for (typename YArray<DataType *>::SizeType i = 0; i < YArray<DataType *>::getCount(); ++i)
            delete YArray<DataType *>::getItem(i);
        YArray<DataType *>::clear();
    }
};

template <class DataType>
class YRefArray: public YBaseArray {
public:
    YRefArray(): YBaseArray(sizeof(ref<DataType>)) {}

    void append(ref<DataType> &item) {
        ref<DataType> r = item;
        r.__ref();
        YBaseArray::append(&r);
    }
    void insert(const SizeType index, ref<DataType> &item) {
        ref<DataType> r = item;
        r.__ref();
        YBaseArray::insert(index, &r);
    }

    ref<DataType> getItem(const SizeType index) const {
        ref<DataType> r = *(ref<DataType> *)YBaseArray::getItem(index);
        return r;
    }
    ref<DataType> operator[](const SizeType index) const {
        return getItem(index);
    }

    virtual void remove(const typename YArray<ref<DataType> *>::SizeType index) {
        if (index < getCount())
            ((ref<DataType> *)YBaseArray::getItem(index))->__unref();
        YBaseArray::remove(index);
    }
    
    virtual void clear() {
        for (typename YArray<ref<DataType> *>::SizeType i = 0; i < getCount(); ++i)
            ((ref<DataType> *)YBaseArray::getItem(i))->__unref();
        YBaseArray::clear();
    }
};
/*******************************************************************************
 * An array of strings
 ******************************************************************************/

#if 1
class YStringArray: public YBaseArray {
public:
    YStringArray(YStringArray &other): YBaseArray((YBaseArray&)other) {}
    YStringArray(const YStringArray &other);

    explicit YStringArray(SizeType capacity = 0): 
        YBaseArray(sizeof(char *)) {
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

    virtual SizeType find(const char *str);
    
    char *const *getCArray() const;
    char **release();
};
#endif

/*******************************************************************************
 * A stack emulated by a dynamic array
 ******************************************************************************/

template <class DataType>
class YStack: public YArray<DataType> {
public:
    const DataType &getTop() const {
        return getItem(YArray<DataType>::getCount() - 1);
    }
    const DataType &operator*() const { return getTop(); }

    virtual void push(const DataType &item) { append(item); }
    void pop() {
        remove(YArray<DataType>::getCount() - 1);
    }
};

/*******************************************************************************
 * A set emulated by a stack
 ******************************************************************************/

template <class DataType>
class YStackSet: public YStack<DataType> {
public:
    virtual void push(const DataType &item) {
        const typename YArray<DataType *>::SizeType index = find(item);

        remove(index);
        YStack<DataType>::push(item);
    }
};

#endif

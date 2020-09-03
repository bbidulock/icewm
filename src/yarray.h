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

#ifndef YARRAY_H
#define YARRAY_H

#include <string.h>
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
        fElementSize(elementSize), fCapacity(0), fCount(0), fElements(nullptr) {}
    YBaseArray(YBaseArray &other);
    YBaseArray(const YBaseArray& other);
    virtual ~YBaseArray() { clear(); }

    void append(const void *item);
    void insert(const SizeType index, const void *item);
    virtual void remove(const SizeType index);
    virtual void clear();
    virtual void shrink(const SizeType reducedCount);
    virtual void extend(const SizeType extendedCount);

    SizeType getCapacity() const { return fCapacity; }
    SizeType getCount() const { return fCount; }
    bool isEmpty() const { return 0 == getCount(); }
    bool nonempty() const { return 0 < getCount(); }

    void setCapacity(SizeType nCapacity);

    static const SizeType npos = SizeType(-1);

protected:
    const StorageType *getElement(const SizeType index) const {
        return fElements + (index * fElementSize);
    }
    StorageType *getElement(const SizeType index) {
        return fElements + (index * fElementSize);
    }

    const void* begin() const { return getElement(0); }
    const void* end() const { return getElement(getCount()); }

    void release();
    void swap(YBaseArray& other);

public:
    SizeType getIndex(void const * ptr) const {
        PRECONDITION(ptr >= begin() && ptr < end());
        return (ptr >= begin() && ptr < end()
             ? SizeType(static_cast<const StorageType *>(ptr) - fElements)
                 / fElementSize
             : npos);
    }
    const void *getItem(const SizeType index) const {
        PRECONDITION(index < getCount());
        return (index < getCount() ? getElement(index) : nullptr);
    }
    void *getItem(const SizeType index) {
        PRECONDITION(index < getCount());
        return (index < getCount() ? getElement(index) : nullptr);
    }

    const void *operator[](const SizeType index) const {
        return getItem(index);
    }
    void *operator[](const SizeType index) {
        return getItem(index);
    }


private:
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
    YArray(YArray &other):
        YBaseArray(static_cast<YBaseArray&>(other)) {}
    YArray(const YArray& other):
        YBaseArray(static_cast<const YBaseArray&>(other)) {}
    explicit YArray(SizeType capacity): YBaseArray(sizeof(DataType)) {
        setCapacity(capacity);
    }

    void append(const DataType &item) {
        YBaseArray::append(&item);
    }
    void insert(const SizeType index, const DataType &item) {
        YBaseArray::insert(index, &item);
    }

    const DataType *getItemPtr(const SizeType index) const {
        return static_cast<const DataType *>(YBaseArray::getItem(index));
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
        return static_cast<DataType *>(YBaseArray::getItem(index));
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
    YArray<DataType>& operator+=(const DataType& item) {
        append(item); return *this;
    }
    void swap(const SizeType index1, const SizeType index2) {
        ::swap(getItem(index1), getItem(index2));
    }
    void swap(YArray<DataType>& other) {
        YBaseArray::swap(other);
    }
    const DataType* begin() const {
        return (const DataType*) YBaseArray::begin();
    }
    const DataType* end() const {
        return (const DataType*) YBaseArray::end();
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

    YObjectArray() {}
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

    YObjectArray<DataType>& operator+=(DataType* item) {
        BaseType::append(item); return *this;
    }

private:
    YObjectArray(const YObjectArray&);
    YObjectArray& operator=(const YObjectArray&);
};

template <class DataType>
class YRefArray: public YBaseArray {
public:
    YRefArray(): YBaseArray(sizeof(ref<DataType>)) {}
    ~YRefArray() { clear(); }

    YRefArray(const YRefArray& other): YBaseArray(other) {
        for (SizeType i = 0; i < getCount(); ++i)
            getItemPtr(i)->__ref();
    }

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

    const ref<DataType>* begin() const {
        return (const ref<DataType>*) YBaseArray::begin();
    }
    const ref<DataType>* end() const {
        return (const ref<DataType>*) YBaseArray::end();
    }

    int find(DataType* data) {
        for (SizeType i = 0; i < getCount(); ++i) {
            if (data == getItemPtr(i)->_ptr()) {
                return i;
            }
        }
        return npos;
    }

private:
    ref<DataType>* getItemPtr(const SizeType index) const {
        return static_cast<ref<DataType> *>(
                const_cast<void *>(YBaseArray::getItem(index)));
    }
};

/*******************************************************************************
 * An array of strings
 ******************************************************************************/

class YStringArray: public YArray<const char *> {
public:
    typedef YArray<const char *> BaseType;
    typedef BaseType::IterType IterType;

    YStringArray(YStringArray &other):
        BaseType(static_cast<BaseType&>(other)) { }
    YStringArray(const YStringArray &other);
    YStringArray(const char* cstr[], SizeType count = npos, SizeType cap = 0);

    YStringArray() { }
    explicit YStringArray(SizeType capacity) : YArray(capacity) { }

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
        return *getItemPtr(index);
    }
    const char *operator[](const SizeType index) const {
        return getString(index);
    }
    YStringArray& operator+=(const char* item) {
        append(item); return *this;
    }

    virtual void replace(const SizeType index, const char *str);
    virtual void remove(const SizeType index);
    virtual void clear();
    virtual void shrink(int reducedSize);

    virtual SizeType find(const char *str);

    void sort();

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
    using YArray<DataType>::nonempty;

    const DataType &getTop() const {
        PRECONDITION(nonempty());
        return getItem(getCount() - 1);
    }
    const DataType &operator*() const { return getTop(); }

    virtual void push(const DataType &item) { append(item); }
    void pop() {
        PRECONDITION(nonempty());
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

#ifdef MSTRING_H
class MStringArray: public YArray<mstring> {
public:
    typedef YArray<mstring> BaseType;
    typedef BaseType::IterType IterType;

    MStringArray() { }
    MStringArray(MStringArray& other) :
        YArray<mstring>(static_cast<BaseType&>(other))
    { }

    MStringArray(const MStringArray& other) :
        YArray<mstring>(static_cast<const BaseType&>(other))
    {
        for (SizeType i = 0; i < getCount(); ++i)
            getItemPtr(i)->acquire();
    }

    MStringArray(const YStringArray& other): YArray<mstring>(other.getCount())
    {
        for (SizeType i = 0; i < getCount(); ++i) {
            mstring copy(other[i]);
            append(copy);
        }
    }

    virtual ~MStringArray() {
        clear();
    }

    void append(mstring item) {
        item.acquire();
        YBaseArray::append(&item);
    }
    void insert(const SizeType index, mstring item) {
        item.acquire();
        YBaseArray::insert(index, &item);
    }

    mstring& getItem(const SizeType index) const {
        return *getItemPtr(index);
    }
    mstring& operator[](const SizeType index) const {
        return getItem(index);
    }
    MStringArray& operator+=(const mstring& item) {
        append(item); return *this;
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

    void sort();

private:
    mstring* getItemPtr(const SizeType index) const {
        return static_cast<mstring *>(
                const_cast<void *>(YBaseArray::getItem(index)));
    }
};
#endif /* MSTRING_H */

/*******************************************************************************
 * An associative array
 ******************************************************************************/

template <class DataType>
class YAssoc {
public:
    unsigned long hash;
    const char* key;
    DataType value;

    YAssoc(unsigned long h, const char* k, DataType& v) :
        hash(h), key(k), value(v) { }
    YAssoc(unsigned long h, const char* k) :
        hash(h), key(k), value(0) { }

    DataType& operator=(DataType& v) { value = v; }
    DataType& operator=(const DataType& v) { value = v; }

    int compare(unsigned long hash2, const char* key2) const {
        return hash < hash2 ? -1 :
               hash > hash2 ? +1 :
               key == key2 ? 0 :
               key == nullptr && key2 != nullptr ? -1 :
               key != nullptr && key2 == nullptr ? +1 :
               strcmp(key, key2);
    }

    operator DataType() const { return value; }
    DataType operator->() const { return value; }
};

template <class DataType>
class YAssocArray : private YArray<YAssoc<DataType> > {
public:
    typedef YAssoc<DataType> AssocType;
    typedef YArray<AssocType> BaseType;
    typedef YArray<const char*> KeysType;
    typedef typename BaseType::SizeType SizeType;
    using BaseType::getCount;
    using BaseType::getItem;
    using BaseType::npos;
    using typename BaseType::IterType;
    using BaseType::iterator;
    using BaseType::reverseIterator;

    YAssocArray() : BaseType() { }
    explicit YAssocArray(SizeType capacity) : BaseType(capacity) { }

    bool has(const char* key) const {
        return find(key) != npos;
    }

    SizeType find(const char* key) const {
        SizeType index;
        return find(key, &index) ? index : npos;
    }

    bool find(const char* key, SizeType* index) const {
        return find(strhash(key), key, index);
    }

    bool find(unsigned long hash, const char* key, SizeType* index) const {
        SizeType lo = 0, hi = getCount();
        while (lo < hi) {
            SizeType pv = (lo + hi) / 2;
            int cmp = getItem(pv).compare(hash, key);
            if (cmp < 0)
                hi = pv;
            else if (cmp > 0)
                lo = pv + 1;
            else
                return *index = pv, true;
        }
        return *index = lo, false;
    }

    const AssocType& operator[](SizeType index) const {
        return getItem(index);
    }

    AssocType& operator[](SizeType index) {
        return getItem(index);
    }

    DataType& operator[](const char* key) {
        unsigned long hash = strhash(key);
        SizeType index;
        if (find(hash, key, &index) == false) {
            AssocType assoc(hash, newstr(key));
            BaseType::insert(index, assoc);
        }
        return getItem(index).value;
    }

    void keys(KeysType& keys) {
        keys.clear();
        keys.setCapacity(getCount());
        for (SizeType i = 0; i < getCount(); ++i)
            keys.append(getItem(i).key);
    }

    virtual void remove(const SizeType index) {
        if (index < getCount()) {
            delete[] getItem(index).key;
            BaseType::remove(index);
        }
    }

    void remove(const char* key) {
        SizeType index = find(key);
        if (index >= 0) remove(index);
    }

    virtual void clear() {
        for (SizeType i = 0; i < getCount(); ++i)
            delete[] getItem(i).key;
        BaseType::clear();
    }

    virtual ~YAssocArray() {
        clear();
    }
};

/*******************************************************************************
 * A fixed multi-dimension array
 ******************************************************************************/

template <class DataType>
class YMulti {
private:
    typedef DataType* BaseType;
    BaseType* base;
    DataType* data;
    int rows, cols;

public:
    YMulti(int rows, int cols) :
        base(new BaseType[rows]),
        data(new DataType[rows * cols]),
        rows(rows), cols(cols)
    {
        for (int i = 0; i < rows; ++i)
            base[i] = data + i * cols;
    }
    ~YMulti() {
        delete[] base;
        delete[] data;
    }

    BaseType operator[](int index) const {
        return base[index];
    }

    void clear() const {
        memset(data, 0, sizeof(DataType) * rows * cols);
    }

    void clear(int index) const {
        memset(base[index], 0, sizeof(DataType) * cols);
    }

    int compare(int left, int right) const {
        int i = -1;
        while (++i < cols && base[left][i] == base[right][i]);
        return i < cols ? base[left][i] < base[right][i] ? -1 : +1 : 0;
    }

    void copyTo(int from, int dest) const {
        memcpy(base[dest], base[from], sizeof(DataType) * cols);
    }

    void copyFrom(int dest, BaseType from) const {
        memcpy(base[dest], from, sizeof(DataType) * cols);
    }

    DataType sum(int row) const {
        DataType *ptr(base[row]), *end(ptr + cols), total(*ptr);
        while (++ptr < end) total += *ptr;
        return total;
    }
};

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
        return inrange(index + extra, 0, int(array->getCount()) - 1);
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
    IterType& insert(DataType& elem) {
        array->insert(where(), elem);
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
    for (YBaseArray::SizeType i = 0; i < array.getCount(); ++i)
        if (array[i] == data) return i;
    return -1;
}

template<class DataType>
bool findRemove(YArray<DataType>& array, DataType& data) {
    int k = find(array, data);
    if (k >= 0)
        array.remove(k);
    return k >= 0;
}
#endif

// vim: set sw=4 ts=4 et:

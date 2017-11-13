/*
 * Copyright (c) 2017 Eduard Bloch
 * License: WTFPL
 *
 * Collection of various data structures used to support certain IceWM functionality.
 * All of them are made for simple data structures which must be trivially copyable and mem-movable!
 */
#ifndef __YCOLLECTIONS_H
#define __YCOLLECTIONS_H

#include "yarray.h"

/**
 * The templates around YBaseArray above are created
 * for handling with void pointers for storage.
 * This has sometimes practical disadvantages in
 * cases where really basic storage of value typed
 * members is needed.
 * This is an alternative class, which is supposed
 * to be used like a swiss knife. Intentionally
 * very primitive, doing little memory
 * management and that's it. No access
 * protection, no hacking around pointer copies.
 * The data members are expected to be default
 * contructible/copyable/deletable, persistent
 * memory location is not guaranteed after adding
 * members or preserving more space.
 */
template<typename DataType, typename SizeType = size_t>
class YVec
{
protected:
    SizeType capa;
    inline void resize(SizeType newSize)
    {
        DataType *old = data;
        data = new DataType[newSize];
        for(SizeType i=0;i<size;++i) data[i] = old[i];
        delete[] old;
        capa = newSize;
    }
    inline void inflate() {
        resize(capa == 0 ? 2 : capa*2);
    }
    // shall not be copyable
    YVec(const YVec& other);
    YVec operator=(const YVec& other);

public:
    SizeType size;
    DataType *data;
    inline YVec(): capa(0), size(0), data(0) {}
    inline YVec(SizeType initialCapa):  capa(initialCapa), size(0), data(new DataType[initialCapa]) { }
    inline void reset() {
        if (!size) return;
        delete[] data; data = 0;
        capa = size = 0;
    }
    inline void preserve(SizeType wanted) { if(wanted > capa) resize(wanted); }
    inline SizeType remainingCapa() { return capa - size; }
    inline ~YVec() { reset(); }
    DataType& add(const DataType& element) {
        if (size >= capa)
            inflate();
        return data[size++] = element;
    }
    DataType& insert(const DataType& element, SizeType destPos)
    {
        if (size >= capa)
            inflate();
        if (size > destPos)
            memmove(&data[destPos+1], &data[destPos],
                    sizeof(element) * (size - destPos));
        size++;
        return (data[destPos] = element);
    }
    const DataType& operator[](const SizeType index) const { return data[index]; }
    DataType& operator[](const SizeType index) { return data[index]; }
    SizeType getCount() const { return size; }

    typedef YArrayIterator<DataType, YVec<DataType, SizeType> > iterator;
    inline iterator getIterator(bool reverse = false) {
        return iterator(this, reverse);
    }
};
/**
 * Simple container based on YVec but made only for raw pointers of the particular type.
 * Ownership of add pointers is transfered to here!
 */
template<typename DataType>
struct YPointVec : public YVec<DataType*>
{
    inline void reset() {
        for(DataType **p = this->data, **e = this->data + this->size; p<e; ++p)
            delete *p;
        YVec<DataType*>::reset();
    }
    inline ~YPointVec() { reset(); }
};

template<typename KeyType, typename ValueType>
struct YKeyValuePair
{
    KeyType key;
    ValueType value;
    YKeyValuePair(KeyType key, ValueType value) : key(key), value(value) {}
    YKeyValuePair() : key(), value() {}
    YKeyValuePair(const YKeyValuePair& src) : key(src.key), value(src.value) {}
};

template<typename KeyType>
bool lessThan(KeyType left, KeyType right);

/*
 * Very basic implementation of lookup-friendly container.
 * No guarantees WRT non-unique values!
 * Sort order is guaranteed.
 * Complexities:
 * Find: O(logN)
 * Add: O(logN) // mind the memmove
 * Erase: later...
 */
template<typename KeyType, typename ValueType>
class YSortedMap
{
public:
    typedef YKeyValuePair<KeyType,ValueType> kvp;
private:
    YVec<kvp, int> store;
    bool binsearch(KeyType key, int& pos) {
        size_t leftPos(0), rightPos(store.size - 1), splitPos(0);
        while (leftPos <= rightPos) {
            splitPos = leftPos + (rightPos - leftPos) / 2;
            // single-stepping due to rounding
            if (lessThan(key, store[splitPos].key))
                rightPos = splitPos - 1;
            else if (lessThan(store[splitPos].key, key))
                leftPos = splitPos + 1;
            else if (key == store[splitPos].key) {
                pos = splitPos;
                return true;
            }
        }
        pos = splitPos;
        return false;
    }

public:
    const ValueType& find(const KeyType& key, const ValueType &notFoundRetValue)
    {
        int pos;
        if(binsearch(key, pos))
            return store[pos].value;
        return notFoundRetValue;
    }
    // FIXME: YArrayIterator<kvp> find(KeyType key);
    // FIXME: iterator? Just wrap the one from store?
    inline void add(KeyType key, ValueType value)
    {
        if(store.size == 0)
        {
            store.add(kvp(key,value));
            return;
        }
        store.preserve(store.size+1);
        int destPos=0;

        binsearch(key, destPos);
        // cursor ended on the left side?
        destPos += (destPos > 0 && lessThan(store[destPos].key, key));
        store.insert(kvp(key, value), destPos);
    }

};

#endif // __YCOLLECTIONS_H

// vim: set sw=4 ts=4 et:

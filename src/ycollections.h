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
template<typename DataType>
class YVec
{
protected:
    size_t capa;
    inline void resize(size_t newSize)
    {
        DataType *old = data;
        data = new DataType[newSize];
        for(size_t i=0;i<size;++i) data[i] = old[i];
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
    size_t size;
    DataType *data;
    inline YVec(): capa(0), size(0), data(0) {}
    inline YVec(size_t initialCapa):  capa(initialCapa), size(0), data(new DataType[initialCapa]) { }
    inline void reset() { if(!size) return; delete[] data; data = 0; size = 0; }
    inline void preserve(size_t wanted) { if(wanted > capa) resize(wanted); }
    inline size_t remainingCapa() { return capa - size; }
    inline ~YVec() { reset(); }
    DataType& add(const DataType& element) {
        if (size >= capa)
            inflate();
        return data[size++] = element;
    }
    const DataType& operator[](const size_t index) const { return data[index]; }
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
};
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
    YVec<kvp> store;
    bool binsearch(KeyType key, int& pos) {
        size_t leftPos(0), rightPos(store.size - 1), splitPos(0);
        while (leftPos <= rightPos) {
            splitPos = leftPos + (rightPos - leftPos) / 2;
            // single-stepping due to rounding
            if (lessThan(key, store[splitPos]))
                rightPos = splitPos - 1;
            else if (lessThan(store[splitPos], key))
                leftPos = splitPos + 1;
            else if (key == store[splitPos]) {
                pos = splitPos;
                return true;
            }
         }
        return false;
    }

public:
    const ValueType& find(const KeyType& key, const ValueType &notFoundRetValue)
    {
        size_t pos;
        if(binsearch(key, pos))
            return store[pos];
        return notFoundRetValue;
    }
    // FIXME: YArrayIterator<kvp> find(KeyType key);
    inline void add(KeyType key, ValueType value)
    {
        if(store.size == 0)
        {
            store.add(kvp(key,value));
            return;
        }
        store.preserve(store.size+1);
        size_t destPos=0;

        // found or not, either it's close, so can be the next bigger or lesser, or can be inside of a sequence of multiple non-equal
        binsearch(key, destPos);
        for (; !lessThan(key, store[destPos]); --destPos)
            ;
        destPos++; // now on the first bigger-or-equal key position
        memmove(&store[destPos+1], &store[destPos], store.size-destPos);
        store.size++;
        store.data[destPos] = value;
    }

    inline YArrayIterator<kvp> iterator(bool reverse = false) {
        return YArrayIterator<kvp>(store.data, reverse);
    }
    size_t getCount() const {
        return store.size;
    }

};

#endif // __YCOLLECTIONS_H

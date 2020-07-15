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
        for (SizeType i=0;i<size;++i) data[i] = old[i];
        delete[] old;
        capa = newSize;
    }
    inline void inflate() {
        resize(capa == 0 ? 2 : capa*2);
    }
    // shall not be copyable
    YVec(const YVec& other);
    YVec operator=(const YVec& other);
    YVec& operator=(YVec&& other) { other.swap(*this); return *this; }

public:
    SizeType size;
    DataType *data;
    inline YVec(): capa(0), size(0), data(nullptr) {}
    inline YVec(SizeType initialCapa):  capa(initialCapa), size(0), data(new DataType[initialCapa]) { }
    inline YVec(YVec&& src) { src.swap(*this); }

    inline void reset() {
        if (!size) return;
        delete[] data; data = nullptr;
        capa = size = 0;
    }
    inline void preserve(SizeType wanted) { if (wanted > capa) resize(wanted); }
    inline SizeType remainingCapa() { return capa - size; }
    inline ~YVec() { reset(); }
    void swap(YVec& other) { if(this == &other) return; ::swap(data, other.data); ::swap(size, other.size); }
    DataType& add(const DataType& element) {
        if (size >= capa)
            inflate();
        return data[size++] = element;
    }
    DataType& emplace_back(DataType&& element) {
        if (size >= capa)
            inflate();
        return data[size++] = static_cast<DataType&&>(element);
    }
    DataType& insert(const DataType& element, SizeType destPos)
    {
        if (size >= capa)
            inflate();
        if (size > destPos) {
            DataType *old = data;
            data = new DataType[size + 1];
            for (SizeType i=0; i < destPos; ++i)
                data[i] = old[i];
            for (SizeType i=destPos; i < size; ++i)
                data[i+1] = old[i];
            delete[] old;
            capa = size + 1;
        }
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
    // STL-friendly iterator
    DataType* begin() { return data; }
    DataType* end() { return data + size; }
};
/**
 * Simple container based on YVec but made only for raw pointers of the particular type.
 * Ownership of add pointers is transfered to here!
 */
template<typename DataType>
struct YPointVec : public YVec<DataType*>
{
    inline void reset() {
        for (DataType **p = this->data, **e = this->data + this->size; p<e; ++p)
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
    YKeyValuePair<KeyType, ValueType>& operator=(const YKeyValuePair<KeyType, ValueType>& src)
    {
        key = src.key;
        value = src.value;
        return *this;
    }
};

template<typename KeyType>
bool lessThan(KeyType left, KeyType right);

/*
 * Very basic implementation of lookup-friendly container.
 * No guarantees on uniqueness, it's actually a multimap.
 * Sort order is guaranteed (although not stable WRT multi-entries)
 * Complexities:
 * Find: O(logN)
 * Add: O(N+logN)
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
        int leftPos(0), rightPos(store.size - 1), splitPos(0);
        while (leftPos <= rightPos) {
            splitPos = (leftPos + rightPos) / 2;
            // single-stepping due to rounding
            if (lessThan(key, store[splitPos].key))
                rightPos = splitPos - 1;
            else if (lessThan(store[splitPos].key, key))
                leftPos = splitPos + 1;
            else
            {
                pos = splitPos;
                return true;
            }
        }
        pos = splitPos;
            return false;
        }

        inline bool
        values_equal (size_t v, size_t w) {
            return !lessThan (store[v].value, store[w].value)
                    && !lessThan (store[w].value, store[v].value);
        }

public:
    const ValueType& find(const KeyType& key, const ValueType &notFoundRetValue)
    {
        int pos;
        if (binsearch(key, pos))
            return store[pos].value;
        return notFoundRetValue;
    }
    /**!
     * @return A range of values if found (pointer to the first matching alement
     * and the first non-matching after the matches); if not found, returns a
     * pair of same values (might be invalid pointers).
     */
    const YKeyValuePair<kvp*,kvp*> multifind(const KeyType& key)
    {
        int pos;
        YKeyValuePair<kvp*,kvp*> ret(NULL, NULL);
        if (!binsearch(key, pos))
            return ret;
        for(int i=pos;;++i)
        {
            if(i==store.size || !values_equal(i, pos))
            {
                ret.value = &store[i];
                break;
            }
        }
        for(int i=pos;;--i)
        {
            if(i==0 || !values_equal(i-1, pos))
            {
                ret.key = &store[i];
                break;
            }
        }
        return ret;
    }

    // FIXME: YArrayIterator<kvp> find(KeyType key);
    // FIXME: iterator? Just wrap the one from store?
    inline void add(KeyType key, ValueType value)
    {
        if (store.size == 0)
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

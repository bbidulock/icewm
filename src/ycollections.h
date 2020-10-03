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
        for (SizeType i = 0;i<size;++i) data[i] = old[i];
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
            for (SizeType i = 0; i < destPos; ++i)
                data[i] = old[i];
            for (SizeType i = destPos; i < size; ++i)
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
        for (int i = pos;;++i)
        {
            if (i == store.size || !values_equal(i, pos))
            {
                ret.value = &store[i];
                break;
            }
        }
        for (int i = pos;;--i)
        {
            if (i == 0 || !values_equal(i-1, pos))
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
        int destPos = 0;

        binsearch(key, destPos);
        // cursor ended on the left side?
        destPos += (destPos > 0 && lessThan(store[destPos].key, key));
        store.insert(kvp(key, value), destPos);
    }

};


class mstring;
/**
 * @brief Shortcut for data type which converts directly to mstring (as key).
 *
 */
template<typename TElement>
struct YSparseHTKey {
    const mstring& operator()(const TElement &el) { return el; }
};
/**
 * Simple associative array for string keys, element-provided key storage,
 * open addressing collision handling with linear probing.
 *
 * The user of this structure must make some promises:
 * - default-constructed elements are considered invalid
 * - after getting an element (by reference), the reference will be initialized
 *   with a non-default value (i.e. comparing to TElement() must become false)
 * - the element returned from TKeyGetter functor (i.e. its operator() ) must
 *   be convertible to const char * and remain valid.
 * - the element returned by TKeyGetter must be value-comparable (so probably
 *   NOT plain const char* or something which implicitly converts to it but
 *   preferably mstring or std::string reference)
 *
 * There are two ways of inserting elements:
 * a) via operator[] - using this, the caller MUST assign a non-default
 *   value immediately afterwards
 * b) using find() returned iterator helper - if a value is assigned via iter,
 *   it will be commited in lazy fashion, when the iterator goes out of scope.
 *   NO other iterator object or [] access must interfere in the meantime!
 */
template<typename TElement,
typename TKeyGetter = YSparseHTKey<TElement>,
int Power = 7>
class YSparseHashTable {
    unsigned count;
    // this is almost POD apart from ctor, for basic swapping purpose
    struct HTpool {
#ifdef DEBUG
        unsigned compCount = 0;
        unsigned lookupCount = 0;
#endif
        unsigned dSize, mask, highMark;
        TElement *data;
        TElement inval;
        /**
         * Locate cached item, return either the item itself or a reference to position
         * where such element can be stored for later lookup.
         */
        TElement& hashFind(const char *name) {
#ifdef DEBUG
                lookupCount++;
#endif
            auto hval = strhash(name);
            // hash function is good enough to avoid clustering
            auto hpos = hval & mask;

            for (auto i = hpos; i < dSize; ++i) {
                if (inval == data[i])
                    return data[i];
#ifdef DEBUG
                    compCount++;
#endif
                if (TKeyGetter()(data[i]) == name)
                    return data[i];
            }

            for (auto i = int(hpos) - 1; i >= 0; --i) {
                if (inval == data[i])
                    return data[i];
#ifdef DEBUG
                    compCount++;
#endif
                if (TKeyGetter()(data[i]) == name)
                    return data[i];
            }
            // not accessible - always enough spare slots
            throw std::exception();
        }
        HTpool(unsigned size) :
                dSize(size), mask(size - 1), highMark(size * 3 / 4),
                data(new TElement[size]), inval(TElement()) {
        }
        ~HTpool() {
            delete[] data;
        }
        HTpool& operator=(HTpool &&other) {
            delete [] data;
            data = other.data;
            other.data = nullptr;
            dSize = other.dSize;
            mask = other.mask;
            highMark = other.highMark;
            return *this;
        }

    } pool;

    void inflate()
    {
        decltype(pool) next(pool.dSize * 2);
        count = 0;
        // like using iterator but cheaper
        for (auto p = pool.data, pe = pool.data + pool.dSize; p < pe; ++p) {
            if (pool.inval != *p) {
                next.hashFind(TKeyGetter()(*p)) = std::move(*p);
                count++;
            }
        }
        pool = std::move(next);
    }
    // not permitted because of explicit POD actions
    YSparseHashTable(const YSparseHashTable&) = delete;
    YSparseHashTable& operator=(const YSparseHashTable&) = delete;

public:
    YSparseHashTable(unsigned power = Power) : count(0), pool(1 << power) {}
    ~YSparseHashTable() {
#ifdef DEBUG
        auto it = begin();
        auto someName = it != end() ? TKeyGetter()(*it).c_str() : "??";
        ++it;
        auto other = it != end() ? TKeyGetter()(*it).c_str() : "??";
        MSG(("HT(%s, %s, ...): lookups: %u, compcount: %u, "
                "used/reserved: %u/%u AKA LF: %F",
                        someName, other,
                        pool.lookupCount, pool.compCount,
                        count, unsigned(pool.dSize),
                        double(count)/pool.dSize));

        unsigned scheck = 0;
        for(auto p = pool.data, pe = pool.data + pool.dSize; p < pe; ++p )
            scheck += (pool.inval != *p);
        if (scheck != count) {
            MSG(("Hash contents mismatch, some content"
                    " illegally invalidated or added"));
        }

#endif
    }

    unsigned long getCount() { return count; }

    /**
     * WARNING:
     * after getting an element reference, the internal counter will be
     * adjusted by assumption that user will add an element after that if the
     * value was not set before. For plain checks of existence, use of has() or
     * find() is advisable.
     */
    TElement& operator[](const char *key) {
        if (count >= pool.highMark)
            inflate();
        auto &res = pool.hashFind(key);
        if (pool.inval == res) // was not set, will be, so resize later
            count++;
        return res;
    }
    /**
     * STL-friendly iterators, although just good enough for basic actions.
     * Validity rules (life cycle) are similar to std::unordere_map::iterator.
     */
    class TIterator
    {
        TElement* p;
        YSparseHashTable& parent;
        bool fromFind = false, hadValue = false;
        friend class YSparseHashTable;
    public:
        TIterator(TElement *ap, YSparseHashTable &par) :
                p(ap), parent(par) {
        }
        ~TIterator() {
            if (!fromFind)
                return;
            auto gotValue = p && (TElement() != *p);
            if (gotValue && !hadValue) {
                // okay, newly added. Pending resize?
                if (parent.count >= parent.pool.highMark)
                    parent.inflate();
                parent.count++;
            }
        }
        friend class YSparseHashTable;
    public:
        TElement& operator*() { return *p; }
        bool operator==(const TIterator &b) const {
            return p == b.p;
        }
        bool operator!=(const TIterator &b) const {
            return p != b.p;
        }
        TIterator& operator++() {
            for (++p; p < parent.pool.data + parent.pool.dSize; ++p) {
                if (TElement() != *p)
                    break;
            }
            return *this;
        }
    };
    TIterator begin() {
        TIterator it(pool.data, *this), eit(end());
        // jump to first valid slot or end?
        if (*it == pool.inval) {
            ++it;
        }
        return it;
    }
    TIterator end() {
        return TIterator(pool.data + pool.dSize, *this);
    }
    TIterator find(const char *key) {
        auto& it = pool.hashFind(key);
        TIterator ret(&it, *this);
        ret.fromFind = true;
        ret.hadValue = it != pool.inval;
        return ret;
    }
    bool has(const char* key) {
        return pool.inval != * find(key);
    }
    bool add(const TElement& val) {
        auto & cur = (*this)[val];
        if (pool.inval != cur) return false;
        cur = val;
        return true;
    }
};

#endif // __YCOLLECTIONS_H

// vim: set sw=4 ts=4 et:

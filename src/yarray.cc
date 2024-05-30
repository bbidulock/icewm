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

#include "config.h"
#include "mstring.h"
#include "yarray.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

YBaseArray::YBaseArray(YBaseArray &other):
    fElementSize(other.fElementSize),
    fCapacity(other.fCapacity),
    fCount(other.fCount),
    fElements(other.fElements) {
    other.release();
}

YBaseArray::YBaseArray(const YBaseArray& other):
    fElementSize(other.fElementSize),
    fCapacity(other.fCount),
    fCount(other.fCount),
    fElements(nullptr)
{
    const unsigned size = unsigned(fCount) * fElementSize;
    if (size) {
        fElements = new StorageType[size];
        if (fElements)
            memcpy(fElements, other.fElements, size);
        else
            fCount = fCapacity = 0;
    }
}

void YBaseArray::setCapacity(SizeType nCapacity) {
    if (nCapacity != fCapacity) {
        const unsigned newSize = unsigned(nCapacity) * fElementSize;
        StorageType* nElements = new StorageType[newSize];
        if (nElements == nullptr)
            return;
        if (fElements) {
            if (0 < fCount) {
                const unsigned oldSize = unsigned(fCount) * fElementSize;
                memcpy(nElements, fElements, min(newSize, oldSize));
            }
            delete[] fElements;
        }
        fElements = nElements;
        fCapacity = nCapacity;
        fCount = min(fCount, fCapacity);
    }
}

void YBaseArray::append(const void *item) {
    if (fCapacity < fCount + 1) {
        setCapacity(max(fCapacity * 2, fCount + 1, 4));
    }
    if (fCount < fCapacity) {
        memcpy(getElement(fCount), item, fElementSize);
        fCount++;
    }
}

void YBaseArray::insert(const SizeType index, const void *item) {
    PRECONDITION(index <= fCount);

    const SizeType nCount = max(index, fCount) + 1;
    const SizeType nCapacity(nCount <= fCapacity ? fCapacity :
                             max(nCount, fCapacity * 2, 4));
    StorageType* nElements = fElements;
    if (fCapacity < nCapacity) {
        const unsigned newSize = unsigned(nCapacity) * fElementSize;
        nElements = new StorageType[newSize];
        if (nElements == nullptr)
            return;
        const SizeType head = min(index, fCount);
        if (0 < head)
            memcpy(nElements, fElements, unsigned(head) * fElementSize);
    }

    if (index < fCount && fElements)
        memmove(nElements + unsigned(index + 1) * fElementSize,
                fElements + unsigned(index) * fElementSize,
                unsigned(fCount - index) * fElementSize);
    else if (fCount < index && nElements)
        memset(nElements + unsigned(fCount) * fElementSize,
               0, unsigned(index - fCount) * fElementSize);

    if (nElements != fElements) {
        delete[] fElements;
        fElements = nElements;
        fCapacity = nCapacity;
    }

    memcpy(fElements + unsigned(index) * fElementSize, item, fElementSize);
    fCount = nCount;

    PRECONDITION(fCount <= fCapacity);
    PRECONDITION(index < fCount);
}

void YBaseArray::extend(const SizeType extendedCount) {
    if (fCapacity < extendedCount)
        setCapacity(extendedCount);
    if (fCount < extendedCount && extendedCount <= fCapacity) {
        memset(fElements + unsigned(fCount) * fElementSize, 0,
               unsigned(extendedCount - fCount) * fElementSize);
        fCount = extendedCount;
    }
}

void YBaseArray::moveto(const SizeType index, const SizeType place) {
    PRECONDITION(index < fCount);
    PRECONDITION(place < fCount);
    unsigned char copy[fElementSize];
    memcpy(copy, getElement(index), fElementSize);
    if (index < place) {
        memmove(getElement(index), getElement(index + 1),
                (place - index) * fElementSize);
    }
    else if (index > place) {
        memmove(getElement(place + 1), getElement(place),
                (index - place) * fElementSize);
    }
    memcpy(getElement(place), copy, fElementSize);
}

void YBaseArray::remove(const SizeType index) {
    PRECONDITION(index < getCount());
    if (0 <= index) {
        int tail = fCount - index - 1;
        if (tail > 0) {
            memmove(getElement(index), getElement(index + 1),
                    unsigned(tail) * fElementSize);
            fCount--;
        }
        else if (tail == 0 && --fCount == 0)
            clear();
    }
}

void YBaseArray::shrink(const SizeType reducedCount) {
    if (reducedCount >= 0 && reducedCount <= fCount)
        fCount = reducedCount;
    else { PRECONDITION(false); }
}

void YBaseArray::clear() {
    if (fElements) {
        delete[] fElements;
        release();
    }
}

void YBaseArray::release() {
    fElements = nullptr;
    fCapacity = 0;
    fCount = 0;
}

void YBaseArray::swap(YBaseArray& other) {
    PRECONDITION(fElementSize == other.fElementSize);
    ::swap(fCapacity, other.fCapacity);
    ::swap(fCount,    other.fCount);
    ::swap(fElements, other.fElements);
}

void YBaseArray::operator=(const YBaseArray& other) {
    if (this != &other) {
        clear();
        if (other.nonempty()) {
            setCapacity(other.getCount());
            memcpy(fElements, other.fElements,
                   unsigned(fCount) * fElementSize);
            fCount = other.getCount();
        }
    }
}

YStringArray::YStringArray(const YStringArray &other) :
    BaseType(other.getCount())
{
    for (SizeType i = 0; i < other.getCount(); ++i)
        append(other.getString(i));
}

YStringArray::YStringArray(const char* cstr[], SizeType num, SizeType cap) :
    BaseType(max(num, cap))
{
    if (cstr) {
        if (num == npos) {
            for (SizeType i = 0; cstr[i]; ++i)
                append(cstr[i]);
            append(nullptr);
        }
        else {
            for (SizeType i = 0; i < num; ++i)
                append(cstr[i]);
        }
    }
}

bool strequal(const char* a, const char* b) {
    return a ? b && !strcmp(a, b) : !b;
}

YStringArray::SizeType YStringArray::find(const char *str) {
    for (SizeType i = 0; i < getCount(); ++i)
        if (strequal(getString(i), str)) return i;

    return npos;
}

void YStringArray::remove(const SizeType index) {
    if (inrange(index, 0, getCount() - 1)) {
        delete[] getString(index);
        YBaseArray::remove(index);
    }
}

void YStringArray::replace(const SizeType index, const char *str) {
    if (inrange(index, 0, getCount() - 1)) {
        const char *copy = newstr(str);
        ::swap(copy, *getItemPtr(index));
        delete[] copy;
    }
    else if (index == getCount())
        append(str);
}

void YStringArray::clear() {
    for (int i = 0; i < getCount(); ++i)
        delete[] getString(i);
    YBaseArray::clear();
}

void YStringArray::shrink(int reducedSize) {
    for (int n = getCount(); n > reducedSize; )
        delete[] getString(--n);
    BaseType::shrink(reducedSize);
}

static int ystring_compare(const void *p1, const void *p2) {
    return strcoll(*(char *const *)p1, *(char *const *)p2);
}

void YStringArray::sort() {
    if (1 < getCount())
        qsort(getItemPtr(0), getCount(), sizeof(char *), ystring_compare);
}

char * const *YStringArray::getCArray() const {
    return (char * const*) begin();
}

char **YStringArray::release() {
    char **strings = (char **) begin();
    YBaseArray::release();
    return strings;
}

static int mstring_compare(const void *p1, const void *p2)
{
    const mstring* s1 = static_cast<const mstring*>(p1);
    const mstring* s2 = static_cast<const mstring*>(p2);
    return const_cast<mstring*>(s1)->collate(*const_cast<mstring*>(s2));
}

void MStringArray::sort() {
    if (1 < getCount())
        qsort(getItemPtr(0), getCount(), sizeof(mstring), mstring_compare);
}

bool testOnce(const char* file, const int line) {
    int esave = errno;
    bool once = false;
    if (file) {
        static YArray<unsigned long> list;
        unsigned long hash = strhash(file) * 0x10001 ^ line;
        if (find(list, hash) < 0) {
            list.append(hash);
            once = true;
        }
    }
    errno = esave;
    return once;
}

// vim: set sw=4 ts=4 et:

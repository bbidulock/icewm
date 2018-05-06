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

#include "mstring.h"
#include "yarray.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

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
    fElements(new StorageType[fCapacity * fElementSize])
{
    memcpy(fElements, other.fElements, fCount * fElementSize);
}

void YBaseArray::setCapacity(SizeType nCapacity) {
    if (nCapacity != fCapacity) {
        StorageType *nElements = new StorageType[nCapacity * fElementSize];
        size_t how_much = min(nCapacity, fCapacity) * fElementSize;
        if (how_much)
        {
            assert(fElements);
            memcpy(nElements, fElements, how_much);
        }

        delete[] fElements;
        fElements = nElements;
        fCapacity = nCapacity;
        fCount = min(fCount, fCapacity);
    }
}

void YBaseArray::append(const void *item) {
    if (fCount >= fCapacity) {
        setCapacity(max(fCapacity * 2, 4));
    }

    memcpy(getElement(fCount++), item, fElementSize);

    assert(fCount <= fCapacity);
}

void YBaseArray::insert(const SizeType index, const void *item) {
    assert(index <= fCount);

    const SizeType nCount = max(fCount + 1, index + 1);
    const SizeType nCapacity(nCount <= fCapacity ? fCapacity :
                             max(nCount, fCapacity * 2));
    StorageType *nElements(nCount <= fCapacity ? fElements :
                           new StorageType[nCapacity * fElementSize]);

    if (nElements != fElements && fElements)
        memcpy(nElements, fElements, min(index, fCount) * fElementSize);

    if (index < fCount)
        memmove(nElements + (index + 1) * fElementSize,
                fElements + (index) * fElementSize,
                (fCount - index) * fElementSize);
    else if (fCount < index)
        memset(nElements + fCount * fElementSize,
               0, (index - fCount) * fElementSize);

    if (nElements != fElements) {
        delete[] fElements;
        fElements = nElements;
        fCapacity = nCapacity;
    }

    memcpy(getElement(index), item, fElementSize);
    fCount = nCount;

    assert(fCount <= fCapacity);
    assert(index < fCount);
}

void YBaseArray::remove(const SizeType index) {
    MSG(("remove %d %d", index, fCount));
    PRECONDITION(index < getCount());
    if (fCount > 0)
        memmove(getElement(index), getElement(index + 1),
                fElementSize * (--fCount - index));
    else
        clear();
}

void YBaseArray::shrink(const SizeType reducedCount) {
    MSG(("shrink %d %d", reducedCount, fCount));
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
    fElements = 0;
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
        setCapacity(other.getCount());
        memcpy(fElements, other.fElements, fCount * fElementSize);
    }
}

YStringArray::YStringArray(const YStringArray &other) :
    YArray<const char*>(other.getCount())
{
    for (SizeType i = 0; i < other.getCount(); ++i)
        append(other.getString(i));
}

static bool strequal(const char *a, const char *b) {
    return a ? b && !strcmp(a, b) : !b;
}

YStringArray::SizeType YStringArray::find(const char *str) {
    for (SizeType i = 0; i < getCount(); ++i)
        if (strequal(getString(i), str)) return i;

    return npos;
}

void YStringArray::remove(const SizeType index) {
    if (index < getCount()) {
        delete[] getString(index);
        YBaseArray::remove(index);
    }
}

void YStringArray::clear() {
    for (int i = 0; i < getCount(); ++i) delete[] getString(i);
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
    return (char * const*) getBegin();
}

char **YStringArray::release() {
    char **strings = (char **) getBegin();
    YBaseArray::release();
    return strings;
}

static int mstring_compare(const void *p1, const void *p2)
{
    const mstring *s1 = (const mstring *) p1;
    const mstring *s2 = (const mstring *) p2;
    return s1->collate(*s2);
}

void MStringArray::sort() {
    if (1 < getCount())
        qsort(getItemPtr(0), getCount(), sizeof(mstring), mstring_compare);
}

// vim: set sw=4 ts=4 et:

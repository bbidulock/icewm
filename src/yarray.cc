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

#include "yarray.h"
#include <string.h>
#include <assert.h>

YBaseArray::YBaseArray(YBaseArray &other):
    fElementSize(other.fElementSize),
    fCapacity(other.fCapacity),
    fCount(other.fCount),
    fElements(other.fElements) {
    other.release();
}

void YBaseArray::setCapacity(SizeType nCapacity) {
    if (nCapacity != fCapacity) {
        StorageType *nElements = new StorageType[nCapacity * fElementSize];
        memcpy(nElements, fElements, min(nCapacity, fCapacity) * fElementSize);

        delete[] fElements;
        fElements = nElements;
        fCapacity = nCapacity;
        fCount = min(fCount, fCapacity);
    }
}

void YBaseArray::append(const void *item) {
    if (fCount >= fCapacity) {
        const SizeType nCapacity = (fCapacity ? fCapacity * 2 : 4);
        StorageType *nElements = new StorageType[nCapacity * fElementSize];

        memcpy(nElements, fElements, fCapacity * fElementSize);

        delete[] fElements;
        fElements = nElements;
        fCapacity = nCapacity;
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

    if (nElements != fElements)
        memcpy(nElements, fElements, min(index, fCount) * fElementSize);

    if (index < fCount)
        memmove(nElements + (index + 1) * fElementSize,
                fElements + (index) * fElementSize,
                (fCount - index) * fElementSize);

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

void YBaseArray::clear() {
    delete[] fElements;

    fElements = 0;
    fCapacity = 0;
    fCount = 0;
}

void YBaseArray::release() {
    fElements = 0;
    fCapacity = 0;
    fCount = 0;
}

YStringArray::YStringArray(const YStringArray &other):
YBaseArray(sizeof(char *)) {
    setCapacity(other.getCapacity());

    for (SizeType i = 0; i < other.getCount(); ++i)
        append(other.getString(i));
}

const YStringArray::SizeType YStringArray::find(const char *str) {
    for (SizeType i = 0; i < getCount(); ++i)
        if (strequal(getString(i), str)) return i;

    return npos;
}

void YStringArray::remove(const SizeType index) {
    if (index < getCount()) delete[] getString(index);
    YBaseArray::remove(index);
}

void YStringArray::clear() {
    for (unsigned i = 0; i < getCount(); ++i) delete[] getString(i);
    YBaseArray::clear();
}

char * const *YStringArray::getCArray() const {
    return (char * const*) getBegin();
}

char **YStringArray::release() {
    char **strings = (char **) getBegin();
    YBaseArray::release();
    return strings;
}

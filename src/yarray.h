/*
 *  IceWM - Simple dynamic array
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU General Public License
 *
 *  2001/04/14: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */

#ifndef __YARRAY_H
#define __YARRAY_H

#include <string.h>

/*******************************************************************************
 * A dynamic array
 ******************************************************************************/

template <class DataType, DataType const Null = 0>
class YArray {
public:
    YArray() : fSize(0), fCount(0), fElements(NULL) {}
    virtual ~YArray() { delete[] fElements; }

    unsigned const size() const { return fSize; }
    unsigned const count() const { return fCount; }

    DataType const & element(unsigned const index) const {
	static DataType const & null(Null);
	return (index < fCount ? fElements[index] : null);
    }

    virtual bool insert(unsigned const index, DataType const & nTop) {
	if (index > count()) return true;

	unsigned const nSize(size() + sizeInc);
	DataType * nElements(count() == size() ? new DataType[nSize]
					       : fElements);

	::memmove(nElements + index + 1, fElements + index,
		  (fCount - index) * sizeof(DataType));

	if (nElements != fElements) {
	    ::memcpy(nElements, fElements, index * sizeof(DataType));

	    delete[] fElements;
	    fElements = nElements;
	    fSize = nSize;
	}

	fCount++;
	fElements[index] = nTop;

	return false;
    }

    void remove(unsigned const index) {
	if (--fCount == 0) {
	    delete[] fElements;
	    fElements = NULL;
	    fSize = 0;
	} else
	    ::memmove(fElements + index,
		      fElements + index + 1,
		      (fCount - index) * sizeof(DataType));
    }

    DataType const * find(DataType const & pattern) const {
	if (fElements)
	    for (DataType const * cptr(fElements);
	         cptr < fElements + fCount; ++cptr)
		if (*cptr == pattern) return cptr;

	return NULL;
    }

    unsigned const index(DataType const * ptr) const {
	return (ptr >= fElements &&
	        ptr < fElements + count() ? ptr - fElements: npos);
    }

    unsigned const index(DataType const & pattern) const {
	return index(find(pattern));
    }

    DataType const & operator[](unsigned const index) const { 
    	return element(index);
    }

    static unsigned const sizeInc = 10;
    static unsigned const npos((unsigned) -1);

private:
    unsigned fSize, fCount;
    DataType * fElements;
};

/*******************************************************************************
 * A stack emulated by a dynamic array
 ******************************************************************************/

template <class DataType, DataType const Null = 0>
class YStack:
public YArray<DataType, Null> {
public:
    DataType const & top() const { return element(count() - 1); }
    virtual void push(DataType const & nTop) { insert(count(), nTop); }
    void pop() { remove(count() - 1); }

    DataType const & operator*() const { return top(); }
    void operator+=(DataType const & value) { push(value); }
    void operator--() { pop(); }
};

/*******************************************************************************
 * A set emulated by a stack
 ******************************************************************************/

template <class DataType, DataType Null = 0>
class YStackSet:
public YStack<DataType, Null> {
public:
    virtual void push(DataType const & nTop) {
	DataType const * pElem(find(nTop));

	if(pElem) remove(index(pElem));

	YStack<DataType, Null>::push(nTop);
    }
};

#endif

/*
 *  IceWM - Simple smart pointer
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/08/02: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 *  2017/05/23 BG: redesign.
 */

#ifndef __YPOINTER_H
#define __YPOINTER_H

/*******************************************************************************
 * Smart pointers behave like pointers but delete the object they own when 
 * being destructed. Three flavors of smart pointers are distinguished:
 *
 *   1. YSmartPtr destructor uses delete to deallocate the pointer data.
 *   2. ASmartPtr destructor uses delete[] to deallocate the pointer data.
 *   3. FSmartPtr destructor uses free to deallocate the pointer data.
 *
 * In addition CSmartPtr is an alias for smart new[] character strings.
 *
 * Please beware that currently copy constructors and copy assignment
 * are not available. This keeps the semantics clean and simple.
 * Open for discussion.
 ******************************************************************************/

#ifndef EXIT_SUCCESS
#include <stdlib.h>
#endif

template <class DataType>
class YSmartPtr {
public:
    YSmartPtr(DataType *data = 0):
        fData(data) {}

    virtual ~YSmartPtr() {
        unref();
    }

    void assign(DataType *other) {
        if (fData != other) {
            unref();
            fData = other;
        }
    }

    void assign(YSmartPtr &other) {
        if (this != &other) {
            unref();
            fData = other.fData;
            other.fData = 0;
        }
    }

    DataType *release() {
        DataType *res = fData;
        fData = 0;
        return res;
    }

    DataType *data() const { return fData; }

    void operator=(DataType *other) { assign(other); }
    void operator=(YSmartPtr& other) { assign(other); }
    operator DataType *() const { return data(); }
    DataType& operator*() const { return *data(); }
    DataType *operator->() const { return data(); }

protected:
    virtual void unref() {
        if (fData) {
            delete fData;
            fData = 0;
        }
    }

    void reset() {
        fData = 0;
    }

private:
    DataType *fData;

    YSmartPtr(const YSmartPtr &other);    // no copies!
    void operator=(const YSmartPtr &other);  // no copies!
};

template <class DataType>
class ASmartPtr : public YSmartPtr<DataType> {
public:
    typedef YSmartPtr<DataType> super;

    ASmartPtr(DataType *d = 0) : super(d) {}

    virtual ~ASmartPtr() {
        unref();
    }

    void operator=(ASmartPtr& other) { super::assign(other); }

protected:
    virtual void unref() {
        if (super::data()) {
            delete[] super::data();
            super::reset();
        }
    }
};

class CSmartPtr : public ASmartPtr<char> {
public:
    typedef ASmartPtr<char> super;

    CSmartPtr(char *d = 0) : super(d) {}

    void operator=(CSmartPtr& other) { super::assign(other); }
};

template <class DataType>
class FSmartPtr : public YSmartPtr<DataType> {
public:
    typedef YSmartPtr<DataType> super;

    FSmartPtr(DataType *d = 0) : super(d) {}

    virtual ~FSmartPtr() {
        unref();
    }

    void operator=(FSmartPtr& other) { super::assign(other); }

protected:
    virtual void unref() {
        if (super::data()) {
            free(super::data());
            super::reset();
        }
    }
};

#endif

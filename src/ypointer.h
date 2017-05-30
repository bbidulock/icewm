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

/**************************************************************************
 * Smart pointers behave like pointers but delete the object they own when 
 * destructed. The following flavors of smart pointers are distinguished:
 *
 *   1. OSmartPtr for `new` objects, deallocated by delete.
 *   2. ASmartPtr for `new[]` arrays, deallocated by delete[].
 *   3. FSmartPtr for malloc data, which is deallocated with free.
 *
 * In addition CSmartPtr is an alias for smart new[] character strings.
 *
 * Copy operations are allowed. Ownership of the memory is controlled by an
 * extra parameter 'fOwn'. Only a smart pointer which owns will deallocate.
 *
 * The implementations are all based on a template YSmartPtr which takes
 * as parameters the primitive type of the underlying allocated data and
 * a type which provides the appropriate free/delete/delete[] operator.
 *************************************************************************/

#include <stdlib.h>

// internal common base
template <class DataType, class Disposer>
class YSmartPtr {
private:
    DataType *fData;
    bool      fOwn;

    void unref() {
        if (fOwn) {
            fOwn = false;
            Disposer::dispose(fData);
        }
        fData = 0;
    }

public:
    explicit YSmartPtr(DataType *data = 0, bool own = true)
        : fData(data), fOwn(own) {}
    YSmartPtr(const YSmartPtr& other)
        : fData(other.fData), fOwn(false) {}

    ~YSmartPtr() { unref(); }

    void data(DataType *other, bool own = true) {
        if (fData != other) {
            unref();
            fData = other;
        }
        fOwn = own;
    }

    void take(YSmartPtr& other, bool own = true) {
        if (this != &other) {
            if (fData != other.fData) {
                unref();
                fData = other.fData;
            }
            if (other.fOwn && own) {
                other.fOwn = false;
                fOwn = true;
            }
        }
    }

    void copy(const YSmartPtr& other) {
        if (this != &other) {
            if (fData != other.fData) {
                unref();
                fData = other.fData;
            }
            fOwn = false;
        }
    }

    void operator=(const YSmartPtr& other) { copy(other); }

    DataType *release() {
        DataType *res = fData;
        fData = 0;
        fOwn = false;
        return res;
    }

    DataType *data() const { return fData; }
    bool owner() const { return fOwn; }

    bool operator==(const YSmartPtr& p) const { return fData == p.fData; }
    bool operator!=(const YSmartPtr& p) const { return fData != p.fData; }
    bool operator==(void *p) const { return fData == p; }
    bool operator!=(void *p) const { return fData != p; }

    operator DataType *() const { return data(); }
    DataType& operator*() const { return *data(); }
    DataType *operator->() const { return data(); }
};

// for new objects
template <class DataType>
class OSmartPtr : public YSmartPtr<DataType, OSmartPtr<DataType> > {
public:
    typedef YSmartPtr<DataType, OSmartPtr<DataType> > super;

    explicit OSmartPtr(DataType *dat = 0, bool own = true)
        : super(dat, own) {}
    OSmartPtr(const OSmartPtr& other)
        : super(other) {}

    void operator=(const OSmartPtr& other) { super::copy(other); }

    static inline void dispose(DataType *ptr) { delete ptr; }
};

// for new[] arrays
template <class DataType>
class ASmartPtr : public YSmartPtr<DataType, ASmartPtr<DataType> > {
public:
    typedef YSmartPtr<DataType, ASmartPtr<DataType> > super;

    explicit ASmartPtr(DataType *dat = 0, bool own = true)
        : super(dat, own) {}
    ASmartPtr(const ASmartPtr& other)
        : super(other) {}

    void operator=(const ASmartPtr& other) { super::copy(other); }

    DataType& operator[](int index) const { return super::data()[index]; }

    static inline void dispose(DataType *ptr) { delete[] ptr; }
};

// for new[] character strings
class CSmartPtr : public ASmartPtr<char> {
public:
    typedef ASmartPtr<char> super;

    explicit CSmartPtr(char *dat = 0, bool own = true)
        : super(dat, own) {}
    CSmartPtr(const CSmartPtr& other)
        : super(other) {}

    void operator=(const CSmartPtr& other) { super::copy(other); }
};

// for malloc data
template <class DataType>
class FSmartPtr : public YSmartPtr<DataType, FSmartPtr<DataType> > {
public:
    typedef YSmartPtr<DataType, FSmartPtr<DataType> > super;

    explicit FSmartPtr(DataType *dat = 0, bool own = true)
        : super(dat, own) {}
    FSmartPtr(const FSmartPtr& other) : super(other) {}

    void operator=(const FSmartPtr& other) { super::copy(other); }

    static inline void dispose(DataType *ptr) { free(ptr); }
};

#endif

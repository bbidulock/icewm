/*
 *  IceWM - Simple smart pointer
 *  Copyright (C) 2002, 2017, 2019 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/08/02: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 *  2017/06/10: Bert Gijsbers - complete redesign (no virtual, no refcount).
 *  2019/12/30: Bert Gijsbers - rewrite to use derived class in super class.
 */

#ifndef YPOINTER_H
#define YPOINTER_H

/**************************************************************************
 * Smart pointers behave like pointers but delete the object they own when
 * they go out of scope. The following smart pointers are distinguished:
 *
 *   0. ysmart is a common super class.
 *   1. osmart for `new` objects, deallocated by delete.
 *   2. asmart for `new[]` arrays, deallocated by delete[].
 *   3. csmart for `new[]` character strings, deallocated by delete[].
 *   5. fsmart for `malloc` data, deallocated by free.
 *   6. xsmart for `Xmalloc` data, deallocated by XFree.
 *
 *************************************************************************/

#if !defined(RAND_MAX) || !defined(EXIT_SUCCESS)
#include <stdlib.h>     // require free()
#endif

// common smart base type
template<class DataType, class Derived>
class ysmart {
    DataType* fData;
    ysmart(const ysmart<DataType, Derived>&);
public:
    explicit ysmart(DataType* data = nullptr) : fData(data) { }

    ~ysmart() { unref(); }

    void unref() {
        if (fData) {
            Derived::dispose(fData);
            fData = nullptr;
        }
    }

    DataType* release() {
        DataType* p = fData;
        fData = nullptr;
        return p;
    }

    DataType* data() const { return fData; }

    void operator=(DataType* data) {
        unref();
        fData = data;
    }

    operator DataType *() const { return fData; }
    DataType& operator*() const { return *fData; }
    DataType* operator->() const { return fData; }

protected:
    void operator=(const ysmart<DataType, Derived>&);
    DataType** operator&() { return &fData; }
};

// For pointers to objects which were allocated with 'new'.
template <class DataType>
class osmart : public ysmart<DataType, osmart<DataType> > {
    typedef ysmart<DataType, osmart<DataType> > super;
    osmart(const osmart<DataType>&);
public:
    static inline void dispose(DataType* p) { delete p; }

    explicit osmart(DataType* data = nullptr) : super(data) { }

    using super::operator=;
};

// For arrays which were allocated with 'new[]'.
template <class DataType>
class asmart : public ysmart<DataType, asmart<DataType> > {
protected:
    typedef ysmart<DataType, asmart<DataType> > super;
    asmart(const asmart<DataType>&);
public:
    static inline void dispose(DataType* p) { delete[] p; }

    explicit asmart(DataType* data = nullptr) : super(data) { }

    asmart<DataType>& operator=(DataType* data) {
        super::operator=(data);
        return *this;
    }

    using super::operator&;
};

// for new[] character strings
typedef asmart<char> csmart;

// for malloc data
template <class DataType>
class fsmart : public ysmart<DataType, fsmart<DataType> > {
    typedef ysmart<DataType, fsmart<DataType> > super;
    fsmart(const fsmart<DataType>&);
public:
    static inline void dispose(DataType* p) { ::free(p); }

    explicit fsmart(DataType* data = nullptr) : super(data) { }

    using super::operator=;
};

extern "C" {
    extern int XFree(void*);
}

// for XFree-able data
template <class DataType>
class xsmart : public ysmart<DataType, xsmart<DataType> > {
    typedef ysmart<DataType, xsmart<DataType> > super;
    xsmart(const xsmart<DataType>&);
public:
    static inline void dispose(DataType* p) { ::XFree(p); }

    explicit xsmart(DataType* data = nullptr) : super(data) { }

    using super::operator=;

    using super::operator&;

    template <typename T>
    T* convert() const {
        return reinterpret_cast<T *>(super::data());
    }
    template <typename T>
    T& extract() const {
        return *convert<T>();
    }
};

#endif

// vim: set sw=4 ts=4 et:

/*
 *  IceWM - Simple smart pointer
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/08/02: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 *  2017/06/10: Bert Gijsbers - complete redesign (no virtual, no refcount).
 */

#ifndef __YPOINTER_H
#define __YPOINTER_H

/**************************************************************************
 * Smart pointers behave like pointers but delete the object they own when
 * they go out of scope. The following smart pointers are distinguished:
 *
 *   1. osmart for `new` objects, deallocated by delete.
 *   2. asmart for `new[]` arrays, deallocated by delete[].
 *   3. fsmart for `malloc` data, deallocated by free.
 *   3. xsmart for `Xmalloc` data, deallocated by XFree.
 *
 * 2b. csmart is an alias for asmart for new[] character strings.
 *
 * For all these types ysmart is a common super class.
 * All types can be assigned to ysmart.
 *************************************************************************/

#if !defined(RAND_MAX) || !defined(EXIT_SUCCESS)
#include <stdlib.h>     // require free()
#endif

// common smart base type
template <class DataType>
class ysmart {
public:
    typedef void (*dispose_t)(DataType *);
    static inline void ydispose(DataType *) { }

private:
    DataType *fData;
    dispose_t fDisp;

    void unref() {
        fDisp(fData);
        fData = 0;
        fDisp = ydispose;
    }

public:
    explicit ysmart(DataType *data = 0, dispose_t disp = ydispose)
        : fData(data), fDisp(disp) {}
    ysmart(const ysmart& copy)
        : fData(copy.fData), fDisp(ydispose) {}

    ~ysmart() { unref(); }

    void data(DataType *some, dispose_t disp = ydispose) {
        if (fData != some) {
            unref();
            fData = some;
        }
        fDisp = disp;
    }

    void take(ysmart& some) {
        if (this != &some) {
            if (fData != some.fData) {
                unref();
                fData = some.fData;
                fDisp = some.fDisp;
                some.fDisp = ydispose;
            }
            else if (some.fDisp != ydispose) {
                fDisp = some.fDisp;
                some.fDisp = ydispose;
            }
        }
    }

    void copy(const ysmart& some) {
        if (fData != some.fData) {
            unref();
            fData = some.fData;
            fDisp = ydispose;
        }
    }

    void operator=(const ysmart& some) { copy(some); }

    DataType *release() {
        fDisp = ydispose;
        return fData;
    }

    DataType *data() const { return fData; }

    bool operator==(const ysmart& p) const { return fData == p.fData; }
    bool operator!=(const ysmart& p) const { return fData != p.fData; }
    bool operator==(void *p) const { return fData == p; }
    bool operator!=(void *p) const { return fData != p; }
    bool operator==(const class null_ref&) const { return fData == 0; }
    bool operator!=(const class null_ref&) const { return fData != 0; }

    operator DataType *() const { return fData; }
    DataType& operator*() const { return *fData; }
    DataType *operator->() const { return fData; }

protected:
    DataType** address() { return &fData; }
};

// For pointers to objects which were allocated with 'new'.
template <class DataType>
class osmart : public ysmart<DataType> {
public:
    typedef ysmart<DataType> super;
    typedef typename super::dispose_t dispose_t;
    static inline void odispose(DataType *p) { delete p; }

    explicit osmart(DataType *data = 0, dispose_t disp = odispose)
        : super(data, disp) {}
    osmart(const osmart& copy)
        : super(copy) {}

    void data(DataType *some, dispose_t disp = odispose) {
        super::data(some, disp);
    }

    void operator=(const osmart& some) { super::copy(some); }
    void operator=(DataType *some) { super::data(some, odispose); }
};

// For arrays which were allocated with 'new[]'.
template <class DataType>
class asmart : public ysmart<DataType> {
public:
    typedef ysmart<DataType> super;
    typedef typename super::dispose_t dispose_t;
    static inline void adispose(DataType *p) { delete[] p; }

    explicit asmart(DataType *data = 0, dispose_t disp = adispose)
        : super(data, disp) {}
    asmart(const asmart& copy)
        : super(copy) {}

    void operator=(const asmart& some) { super::copy(some); }
    void operator=(DataType *some) { super::data(some, adispose); }

    void data(DataType *some, dispose_t disp = adispose) {
        super::data(some, disp);
    }
};

// for new[] character strings
class csmart : public asmart<char> {
public:
    typedef asmart<char> super;
    typedef super::dispose_t dispose_t;

    explicit csmart(char *data = 0, dispose_t disp = super::adispose)
        : super(data, disp) {}
    csmart(const csmart& copy)
        : super(copy) {}

    csmart& operator=(const csmart& some) { super::copy(some); return *this; }
    csmart& operator=(char *some) { super::data(some, adispose); return *this; }

    char** operator&() { return super::address(); }
};

// for malloc data
template <class DataType>
class fsmart : public ysmart<DataType> {
public:
    typedef ysmart<DataType> super;
    typedef typename super::dispose_t dispose_t;
    static inline void fdispose(DataType *p) { ::free(p); }

    explicit fsmart(DataType *data = 0, dispose_t disp = fdispose)
        : super(data, disp) {}
    fsmart(const fsmart& copy) : super(copy) {}

    void data(DataType *some, dispose_t disp = fdispose) {
        super::data(some, disp);
    }

    void operator=(const fsmart& some) { super::copy(some); }
    void operator=(DataType *some) { super::data(some, fdispose); }

    DataType** operator&() { return super::address(); }
};

extern "C" {
    extern int XFree(void*);
}

// for XFree-able data
template <class DataType>
class xsmart : public ysmart<DataType> {
public:
    typedef ysmart<DataType> super;
    typedef typename super::dispose_t dispose_t;
    static inline void xdispose(DataType *p) { if (p) ::XFree(p); }

    explicit xsmart(DataType *data = 0, dispose_t disp = xdispose)
        : super(data, disp) {}
    xsmart(const xsmart& copy) : super(copy) {}

    void data(DataType *some, dispose_t disp = xdispose) {
        super::data(some, disp);
    }

    void operator=(const xsmart& some) { super::copy(some); }
    void operator=(DataType *some) { super::data(some, xdispose); }

    DataType** operator&() { return super::address(); }

    template <typename T> T* convert() const { return (T *)super::data(); }
    template <typename T> T& extract() const { return *convert<T>(); }
};

#endif

// vim: set sw=4 ts=4 et:

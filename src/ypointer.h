/*
 *  IceWM - Simple smart pointer
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/08/02: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */

#ifndef __YPOINTER_H
#define __YPOINTER_H

/*******************************************************************************
 * Smart pointers behave like pointers but delete the object they own when 
 * being destructed.
 ******************************************************************************/

template <class DataType>
class YSmartPtr {
public:
    YSmartPtr(DataType *data = 0): 
    fOwner(true), fData(data) {}

#if 0
    YSmartPtr(YSmartPtr &other):
    fOwner(other.isOwner()), fData(other.release()) {}
#endif

    virtual ~YSmartPtr() {
        unref();
    }

    void assign(DataType *other) {
        if (data() != other) {
            unref();
            fOwner = true;
            fData = other;
        }
    }

#if 0
    void assign(YSmartPtr &other) {
        if (data() != other.data()) {
            unref();
            fOwner = other.isOwner();
            fData = other.release();
        }
    }
#endif

    bool equals(const YSmartPtr &other) const { 
        return data() == other.data();
    }

    DataType *release() {
        fOwner = false;
        return data();
    }

    DataType *data() const { return fData; }
    bool isOwner() const { return fOwner; }

    void operator=(DataType *other) { assign(other); }
    operator DataType *() const { return data(); }
    DataType *operator*() const { return data(); }
    DataType *operator->() const { return data(); }
    bool operator==(const YSmartPtr &other) const { return equals(other); }

private:
    YSmartPtr(const DataType *data) {}
    YSmartPtr(const YSmartPtr &other) {}

    void unref() {
        if (isOwner()) {
            delete fData;
            fData = 0;
        }
    }

    bool fOwner;
    DataType *fData;

    void operator=(YSmartPtr &other); // { assign(other); }
};

#endif

#ifndef __REF_H
#define __REF_H

class refcounted {
public:
    int __refcount;
//    friend class ref;

public:
    refcounted(): __refcount(0) {};
    virtual ~refcounted() {}

    void __destroy();
};

class null_ref;

template<class T> class ref {
private:
    T *ptr;
public:
#define null (*(class null_ref *)0)

    void __ref() {
        ptr->__refcount++;
    }
    void __unref() {
        if (--ptr->__refcount == 0) {
            ptr->__destroy();
        }
    }

    ref(): ptr(0) {}
    ref(class null_ref &): ptr(0) {}
    explicit ref(T *r) : ptr(r) { if (ptr) __ref(); }
    ref(const ref<T> &p): ptr(p.ptr) { if (ptr) __ref(); }
    ~ref() { if (ptr) { __unref(); ptr = 0; } }

    const T& operator*() const { return *ptr; }
    T& operator*() { return *ptr; }
    const T* operator->() const { return ptr; }
    T *operator->() { return ptr; }

    ref<T>& operator=(const ref<T>& rv) {
        if (this != &rv) {
            if (ptr) __unref();
            ptr = rv.ptr;
            if (ptr) __ref();
        }
        return *this;
    }
    ref<T>& init(T *rv) {
        if (ptr) __unref();
        ptr = rv;
        if (ptr) __ref();
        return *this;
    }
    bool operator==(const ref<T> &r) const { return ptr == r.ptr; }
    bool operator!=(const ref<T> &r) const { return ptr != r.ptr; }
    bool operator==(const class null_ref &) const { return ptr == 0; }
    bool operator!=(const class null_ref &) const { return ptr != 0; }

    ref<T>& operator=(const class null_ref &) {
        if (ptr)
            __unref();
        ptr = 0;
        return *this;
    }
};

#endif

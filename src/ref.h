#ifndef REF_H
#define REF_H

class refcounted {
public:
    int __refcount;

protected:
    virtual ~refcounted() {}
public:
    refcounted(): __refcount(0) {}

    virtual void __destroy();
};

extern class null_ref& null;

template<class T>
class ref {
private:
    T *ptr;
    ref(int);   // avoid NULL and 0; require null.
public:
    void __ref() {
        ptr->__refcount++;
    }
    void __unref() {
        if (--ptr->__refcount == 0) {
            ptr->__destroy();
        }
    }

    ref(): ptr(nullptr) {}
    ref(null_ref &): ptr(nullptr) {}
    explicit ref(T *r) : ptr(r) { if (ptr) __ref(); }
    ref(const ref<T> &p): ptr(p.ptr) { if (ptr) __ref(); }
    template<class T2>
    ref(const ref<T2> &p): ptr(static_cast<T *>(p._ptr())) { if (ptr) __ref(); }
    ~ref() { if (ptr) { __unref(); ptr = nullptr; } }

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
    template<class T2>
    ref<T>& operator=(const ref<T2>& rv) {
        if (this->ptr != rv._ptr()) {
            if (ptr) __unref();
            ptr = static_cast<T *>(rv._ptr());
            if (ptr) __ref();
        }
        return *this;
    }
    ref<T>& init(T *rv) {
        if (this->ptr != rv) {
            if (ptr) __unref();
            ptr = rv;
            if (ptr) __ref();
        }
        return *this;
    }
    bool operator==(const ref<T> &r) const { return ptr == r.ptr; }
    bool operator!=(const ref<T> &r) const { return ptr != r.ptr; }
    bool operator==(null_ref &) const { return ptr == nullptr; }
    bool operator!=(null_ref &) const { return ptr != nullptr; }

    void operator=(null_ref &) {
        if (ptr) {
            __unref();
            ptr = nullptr;
        }
    }
    T *_ptr() const { return ptr; }
};

template<class T>
class lazy {
public:
    lazy() : ptr(nullptr) {}
    explicit lazy(T* p) : ptr(p) {}

    operator T*() { return ptr ? ptr : ptr = new T; }
    operator bool() { return ptr != nullptr; }
    operator bool() const { return ptr != nullptr; }
    T* operator->() { return operator T*(); }
    T& operator*() { return *operator T*(); }
    T** operator&() { return &ptr; }
    T* _ptr() const { return ptr; }
    bool operator==(const T* q) const { return q == ptr; }
    bool operator!=(const T* q) const { return q != ptr; }

    void operator=(null_ref &) { if (ptr) { delete ptr; ptr = nullptr; } }
    ~lazy() { *this = null; }

private:
    T* ptr;

    // undefined
    lazy(const lazy<T>&);
    void operator=(const lazy<T>&);
    operator int();
    operator void*();
};

template<class T>
class lazily : public lazy<T> {
public:
    operator bool() { return true; }
    void operator=(null_ref&) { lazy<T>::operator=(null); }
};

template<class T>
bool operator==(T* q, lazy<T>& p) { return q == p._ptr(); }
template<class T>
bool operator!=(T* q, lazy<T>& p) { return q != p._ptr(); }

#endif

// vim: set sw=4 ts=4 et:

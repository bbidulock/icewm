#ifndef __YACTION_H
#define __YACTION_H

class YAction {
public:
    YAction() : id(next()) {}
    explicit YAction(int id) : id(id) {}
    YAction(const YAction& copy) : id(copy.id) {}
    void operator=(const YAction& copy) { id = copy.id; }

    static int next() {
        static int gen(1001);
        return gen += 2;
    }

    bool operator==(const YAction& rhs) const {
        return id == rhs.id;
    }
    bool operator!=(const YAction& rhs) const {
        return id != rhs.id;
    }

private:
    int id;

    operator bool() const; // no bool
    void operator&() const; // no address taking
    bool operator==(int) const; // no integer
    bool operator!=(int) const; // no integer
};

class YActionListener {
public:
    virtual void actionPerformed(YAction action, unsigned int modifiers) = 0;
protected:
    virtual ~YActionListener() {}
};

#endif

// vim: set sw=4 ts=4 et:

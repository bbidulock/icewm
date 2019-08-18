#ifndef WORKSPACES_H
#define WORKSPACES_H

enum {
    AllWorkspaces = -1,
    WorkspaceInvalid = -1,
    OldMaxWorkspaces = 20,
    NewMaxWorkspaces = 1000,
};

class Workspace {
    char* str;

public:
    const YAction active;
    const YAction moveto;
    class YFrameWindow* focused;

    Workspace(const char* name) :
        str(newstr(name)),
        focused(nullptr)
    { }
    ~Workspace() { delete[] str; }

    char*& operator *() { return str; }
    operator const char*() const { return str; }
};

extern class Workspaces {
    YObjectArray<Workspace> list;
    YStringArray spares;

public:
    int count() const {
        return list.getCount();
    }

    bool add(const char* name) {
        int size = count();
        if (name && size < NewMaxWorkspaces) {
            Workspace* work = new Workspace(name);
            if (work) {
                list.append(work);
            }
        }
        return 1 + size == count();
    }

    bool drop() {
        int last = count() - 1;
        if (0 <= last) {
            spare(last, *list[last]);
            list.shrink(last);
        }
        return last == count();
    }

    Workspace& operator[](int index) const {
        return *list[index];
    }

    void reset() {
        list.clear();
        spares.clear();
    };

    void spare(int index, const char* name) {
        if (inrange(index, 0, NewMaxWorkspaces - 1)) {
            if (index >= spares.getCount())
                spares.extend(1 + index);
            spares.replace(index, name);
        }
    }

    const char* spare(int index) {
        return inrange(index, 0, spares.getCount() - 1)
             ? spares[index] : nullptr;
    }

    Workspaces& operator+(const char* name) {
        add(name);
        return *this;
    }
} workspaces;

extern struct WorkspacesCount {
    operator int() const { return workspaces.count(); }
    int operator ()() const { return workspaces.count(); }
} workspaceCount;

extern struct WorkspacesNames {
    const char* operator[](int index) const { return workspaces[index]; }
    const char* operator()(int index) const { return workspaces[index]; }
} workspaceNames;

extern struct WorkspacesActive {
    YAction operator[](int index) const { return workspaces[index].active; }
} workspaceActionActivate;

extern struct WorkspacesMoveTo {
    YAction operator[](int index) const { return workspaces[index].moveto; }
} workspaceActionMoveTo;

#endif

// vim: set sw=4 ts=4 et:

#ifndef __YLIST_H
#define __YLIST_H

template <class Node>
class YListNode {
public:
    YListNode() : nextNode(nullptr), prevNode(nullptr) { }
    ~YListNode() { PRECONDITION(zero()); }

    bool zero() const { return !nextNode && !prevNode; }
    void set(Node* n, Node* p) { nextNode = n; prevNode = p; }

    Node* nodeNext() const { return nextNode; }
    Node* nodePrev() const { return prevNode; }

private:
    Node* nextNode;
    Node* prevNode;

    YListNode(const YListNode&);
    void operator=(const YListNode&);

    template <class T> friend class YList;
};

template <class Node>
class YListIter;

template <class Node>
class YList {
public:
    YList() : head(nullptr), tail(nullptr), size(0) { }
    ~YList() { PRECONDITION(!head && !tail && !size); }

    Node* front() const { return head; }
    Node* back() const { return tail; }
    int   count() const { return size; }
    operator bool() const { return 0 < count(); }
    YListIter<Node> iterator();
    YListIter<Node> reverseIterator();

    void prepend(Node* node) {
        PRECONDITION(node->zero());
        node->set(head, nullptr);
        if (head) {
            PRECONDITION(tail && size);
            head->prevNode = node;
            head = node;
        }
        else {
            PRECONDITION(!tail && !size);
            head = tail = node;
        }
        ++size;
    }

    void append(Node* node) {
        PRECONDITION(node->zero());
        node->set(nullptr, tail);
        if (tail) {
            PRECONDITION(head && size);
            tail->nextNode = node;
            tail = node;
        }
        else {
            PRECONDITION(!head && !size);
            tail = head = node;
        }
        ++size;
    }

    void insertAfter(Node* node, Node* after) {
        PRECONDITION(head && tail && size);
        PRECONDITION(node->zero() && after && node != after);
        node->set(after->nextNode, after);
        if (tail == after) {
            tail = node;
        }
        else {
            after->nextNode->prevNode = node;
        }
        after->nextNode = node;
        ++size;
    }

    void insertBefore(Node* node, Node* before) {
        PRECONDITION(head && tail && size);
        PRECONDITION(node->zero() && before && node != before);
        node->set(before, before->prevNode);
        if (head == before) {
            head = node;
        }
        else {
            before->prevNode->nextNode = node;
        }
        before->prevNode = node;
        ++size;
    }

    Node* remove(Node* node) {
        PRECONDITION(head && tail && size);
        if (head == node) {
            head = static_cast<Node *>(head->nextNode);
            if (head) head->prevNode = nullptr;
        } else {
            node->prevNode->nextNode = node->nextNode;
        }
        if (tail == node) {
            tail = static_cast<Node *>(tail->prevNode);
            if (tail) tail->nextNode = nullptr;
        } else {
            node->nextNode->prevNode = node->prevNode;
        }
        node->set(nullptr, nullptr);
        --size;
        return node;
    }

private:
    Node* head;
    Node* tail;
    int   size;

    YList(const YList&);
    void operator=(const YList&);
    operator int() const;
    operator void*() const;
};

template <class Node>
class YListIter {
public:
    YListIter(Node* list, bool reverse = false)
        : list(list), node(nullptr), reverse(reverse)
    { }
    operator bool() const { return node; }
    operator Node*() const { return node; }
    Node* operator*() const { return node; }
    Node* operator->() const { return node; }
    bool operator++() {
        node = list;
        if (list) {
            list = reverse ? list->nodePrev() : list->nodeNext();
        }
        return node;
    }

private:
    Node* list;
    Node* node;
    bool reverse;

    operator int() const;
    operator void*() const;
};

template <class Node>
inline YListIter<Node> YList<Node>::iterator() {
    return YListIter<Node>(front());
}

template <class Node>
inline YListIter<Node> YList<Node>::reverseIterator() {
    return YListIter<Node>(back(), true);
}

class YFrameWindow;

class YFrameNode : public YListNode<YFrameNode> {
public:
    virtual ~YFrameNode() { }
    virtual YFrameWindow* frame() = 0;
    YFrameWindow* nextFrame() const {
        return nodeNext() ? nodeNext()->frame() : nullptr;
    }
    YFrameWindow* prevFrame() const {
        return nodePrev() ? nodePrev()->frame() : nullptr;
    }
};

class YLayeredNode : public YFrameNode {
public:
    YFrameWindow* next() const { return nextFrame(); }
    YFrameWindow* prev() const { return prevFrame(); }
};

class YFocusedNode : public YFrameNode {
};

class YCreatedNode : public YFrameNode {
};

class YFrameIter;

template <class Node>
class YFrameList : public YList<Node> {
    typedef YList<Node> List;

public:
    YFrameIter iterator();
    YFrameIter reverseIterator();

    YFrameWindow* front() const {
        return List::front() ? List::front()->frame() : nullptr;
    }
    YFrameWindow* back() const {
        return List::back() ? List::back()->frame() : nullptr;
    }
};

class YLayeredList : public YFrameList<YLayeredNode> { };

class YFocusedList : public YFrameList<YFocusedNode> { };

class YCreatedList : public YFrameList<YCreatedNode> { };

class YFrameIter : public YListIter<YFrameNode> {
    typedef YListIter<YFrameNode> ListIter;

public:
    YFrameIter(YFrameNode* list, bool reverse = false)
        : YListIter(list, reverse) {
    }
    operator YFrameWindow*() const {
        return ListIter::operator YFrameNode*()->frame();
    }
    YFrameWindow* operator->() const {
        return ListIter::operator->()->frame();
    }
};

template <class Node>
inline YFrameIter YFrameList<Node>::iterator() {
    return YFrameIter(List::front());
}

template <class Node>
inline YFrameIter YFrameList<Node>::reverseIterator() {
    return YFrameIter(List::back(), true);
}

class YWindow;

class YWindowNode : public YListNode<YWindowNode> {
public:
    virtual ~YWindowNode() { }
    virtual YWindow* window() = 0;
    YWindow* nextWindow() const {
        return nodeNext() ? nodeNext()->window() : nullptr;
    }
    YWindow* prevWindow() const {
        return nodePrev() ? nodePrev()->window() : nullptr;
    }
};

class YWindowList : public YList<YWindowNode> {
    typedef YList<YWindowNode> List;

public:
    YWindow* firstWindow() const {
        return List::front() ? List::front()->window() : nullptr;
    }
    YWindow* lastWindow() const {
        return List::back() ? List::back()->window() : nullptr;
    }
};

#endif

#ifndef __YLIST_H
#define __YLIST_H

template <class Node>
class YListNode {
public:
    YListNode() : nextNode(0), prevNode(0) { }
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
class YList {
public:
    YList() : head(0), tail(0), size(0) { }
    ~YList() { PRECONDITION(!head && !tail && !size); }

    Node* front() const { return head; }
    Node* back() const { return tail; }
    int   count() const { return size; }
    operator bool() const { return 0 < count(); }

    void prepend(Node* node) {
        PRECONDITION(node->zero());
        node->set(head, 0);
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
        node->set(0, tail);
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
            head = (Node *) head->nextNode;
            if (head) head->prevNode = 0;
        } else {
            node->prevNode->nextNode = node->nextNode;
        }
        if (tail == node) {
            tail = (Node *) tail->prevNode;
            if (tail) tail->nextNode = 0;
        } else {
            node->nextNode->prevNode = node->prevNode;
        }
        node->set(0, 0);
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

class YFrameWindow;

class YFrameNode : public YListNode<YFrameNode> {
public:
    virtual ~YFrameNode() { }
    virtual YFrameWindow* frame() = 0;

    YFrameWindow* nextFrame() {
        return nodeNext() ? nodeNext()->frame() : 0;
    }

    YFrameWindow* prevFrame() {
        return nodePrev() ? nodePrev()->frame() : 0;
    }
};

class YLayeredNode : public YFrameNode {
public:
    YFrameWindow* next() { return nextFrame(); }
    YFrameWindow* prev() { return prevFrame(); }
};

class YFocusedNode : public YFrameNode {
};

class YCreatedNode : public YFrameNode {
};

class YFrameIter;

template <class Node>
class YFrameList : public YList<Node> {
    typedef YList<Node> List;
    friend class YFrameIter;

public:
    YFrameIter iterator();
    YFrameIter reverseIterator();

    YFrameWindow* front() const {
        return List::front() ? List::front()->frame() : 0;
    }
    YFrameWindow* back() const {
        return List::back() ? List::back()->frame() : 0;
    }

    YFrameWindow* popHead() {
        return List::front() ? remove(List::front())->frame() : 0;
    }
    YFrameWindow* popTail() {
        return List::back() ? remove(List::back())->frame() : 0;
    }
};

class YLayeredList : public YFrameList<YLayeredNode> { };

class YFocusedList : public YFrameList<YFocusedNode> { };

class YCreatedList : public YFrameList<YCreatedNode> { };

class YFrameIter {
public:
    YFrameIter(YFrameNode* list, bool reverse = false)
        : list(list), node(0), reverse(reverse) {
    }
    operator bool() const { return node; }
    operator YFrameWindow*() const { return node->frame(); }
    bool operator++() {
        node = list;
        if (list) {
            list = (reverse ? list->nodePrev() : list->nodeNext());
        }
        return *this;
    }
    YFrameWindow* operator->() const { return node->frame(); }

private:
    YFrameNode* list;
    YFrameNode* node;
    bool reverse;

    operator int() const;
    operator void*() const;
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
        return nodeNext() ? nodeNext()->window() : 0;
    }
    YWindow* prevWindow() const {
        return nodePrev() ? nodePrev()->window() : 0;
    }
};

class YWindowList : public YList<YWindowNode> {
    typedef YList<YWindowNode> List;

public:
    YWindow* firstWindow() const {
        return List::front() ? List::front()->window() : 0;
    }
    YWindow* lastWindow() const {
        return List::back() ? List::back()->window() : 0;
    }
};

#endif

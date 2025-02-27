/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2011, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2011, Benjamin Poulain <ikipou@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef WTF_ListHashSet_h
#define WTF_ListHashSet_h

#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WTF {

// ListHashSet: Just like HashSet, this class provides a Set
// interface - a collection of unique objects with O(1) insertion,
// removal and test for containership. However, it also has an
// order - iterating it will always give back values in the order
// in which they are added.

// Unlike iteration of most WTF Hash data structures, iteration is
// guaranteed safe against mutation of the ListHashSet, except for
// removal of the item currently pointed to by a given iterator.

template<typename Value, size_t inlineCapacity, typename HashFunctions> class ListHashSet;

template<typename Value, size_t inlineCapacity, typename HashFunctions>
void deleteAllValues(const ListHashSet<Value, inlineCapacity, HashFunctions>&);

template<typename ValueArg, size_t inlineCapacity, typename HashArg> class ListHashSetIterator;
template<typename ValueArg, size_t inlineCapacity, typename HashArg> class ListHashSetConstIterator;

template<typename ValueArg, size_t inlineCapacity> struct ListHashSetNode;
template<typename ValueArg, size_t inlineCapacity> struct ListHashSetNodeAllocator;

template<typename HashArg> struct ListHashSetNodeHashFunctions;
template<typename HashArg> struct ListHashSetTranslator;

template<typename ValueArg, size_t inlineCapacity = 256, typename HashArg = typename DefaultHash<ValueArg>::Hash> class ListHashSet {
    WTF_MAKE_FAST_ALLOCATED;
private:
    typedef ListHashSetNode<ValueArg, inlineCapacity> Node;
    typedef ListHashSetNodeAllocator<ValueArg, inlineCapacity> NodeAllocator;

    typedef HashTraits<Node*> NodeTraits;
    typedef ListHashSetNodeHashFunctions<HashArg> NodeHash;
    typedef ListHashSetTranslator<HashArg> BaseTranslator;

    typedef HashArg HashFunctions;

public:
    typedef ValueArg ValueType;

    typedef ListHashSetIterator<ValueType, inlineCapacity, HashArg> iterator;
    typedef ListHashSetConstIterator<ValueType, inlineCapacity, HashArg> const_iterator;
    friend class ListHashSetConstIterator<ValueType, inlineCapacity, HashArg>;

    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef HashTableAddResult<iterator> AddResult;

    ListHashSet();
    ListHashSet(const ListHashSet&);
    ListHashSet& operator=(const ListHashSet&);
    ~ListHashSet();

    void swap(ListHashSet&);

    int size() const;
    int capacity() const;
    bool isEmpty() const;

    iterator begin() { return makeIterator(m_head); }
    iterator end() { return makeIterator(nullptr); }
    const_iterator begin() const { return makeConstIterator(m_head); }
    const_iterator end() const { return makeConstIterator(nullptr); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    ValueType& first();
    const ValueType& first() const;
    void removeFirst();
    ValueType takeFirst();

    ValueType& last();
    const ValueType& last() const;
    void removeLast();
    ValueType takeLast();

    iterator find(const ValueType&);
    const_iterator find(const ValueType&) const;
    bool contains(const ValueType&) const;

    // An alternate version of find() that finds the object by hashing and comparing
    // with some other type, to avoid the cost of type conversion.
    // The HashTranslator interface is defined in HashSet.
    // FIXME: We should reverse the order of the template arguments so that callers
    // can just pass the translator let the compiler deduce T.
    template<typename T, typename HashTranslator> iterator find(const T&);
    template<typename T, typename HashTranslator> const_iterator find(const T&) const;
    template<typename T, typename HashTranslator> bool contains(const T&) const;

    // The return value of add is a pair of an iterator to the new value's location, 
    // and a bool that is true if an new entry was added.
    AddResult add(const ValueType&);
    AddResult add(ValueType&&);

    // Add the value to the end of the collection. If the value was already in
    // the list, it is moved to the end.
    AddResult appendOrMoveToLast(const ValueType&);
    AddResult appendOrMoveToLast(ValueType&&);

    // Add the value to the beginning of the collection. If the value was already in
    // the list, it is moved to the beginning.
    AddResult prependOrMoveToFirst(const ValueType&);
    AddResult prependOrMoveToFirst(ValueType&&);

    AddResult insertBefore(const ValueType& beforeValue, const ValueType& newValue);
    AddResult insertBefore(const ValueType& beforeValue, ValueType&& newValue);
    AddResult insertBefore(iterator, const ValueType&);
    AddResult insertBefore(iterator, ValueType&&);

    bool remove(const ValueType&);
    bool remove(iterator);
    void clear();

private:
    void unlink(Node*);
    void unlinkAndDelete(Node*);
    void appendNode(Node*);
    void prependNode(Node*);
    void insertNodeBefore(Node* beforeNode, Node* newNode);
    void deleteAllNodes();
    
    iterator makeIterator(Node*);
    const_iterator makeConstIterator(Node*) const;

    friend void deleteAllValues<>(const ListHashSet&);

    HashTable<Node*, Node*, IdentityExtractor, NodeHash, NodeTraits, NodeTraits> m_impl;
    Node* m_head;
    Node* m_tail;
    std::unique_ptr<NodeAllocator> m_allocator;
};

template<typename ValueArg, size_t inlineCapacity> struct ListHashSetNodeAllocator {
    typedef ListHashSetNode<ValueArg, inlineCapacity> Node;
    typedef ListHashSetNodeAllocator<ValueArg, inlineCapacity> NodeAllocator;

    ListHashSetNodeAllocator() 
        : m_freeList(pool())
        , m_isDoneWithInitialFreeList(false)
    { 
        memset(m_pool.pool, 0, sizeof(m_pool.pool));
    }

    Node* allocate()
    { 
        Node* result = m_freeList;

        if (!result)
            return static_cast<Node*>(fastMalloc(sizeof(Node)));

        ASSERT(!result->m_isAllocated);

        Node* next = result->m_next;
        ASSERT(!next || !next->m_isAllocated);
        if (!next && !m_isDoneWithInitialFreeList) {
            next = result + 1;
            if (next == pastPool()) {
                m_isDoneWithInitialFreeList = true;
                next = 0;
            } else {
                ASSERT(inPool(next));
                ASSERT(!next->m_isAllocated);
            }
        }
        m_freeList = next;

        return result;
    }

    void deallocate(Node* node) 
    {
        if (inPool(node)) {
#ifndef NDEBUG
            node->m_isAllocated = false;
#endif
            node->m_next = m_freeList;
            m_freeList = node;
            return;
        }

        fastFree(node);
    }

private:
    Node* pool() { return reinterpret_cast_ptr<Node*>(m_pool.pool); }
    Node* pastPool() { return pool() + m_poolSize; }
    bool inPool(Node* node)
    {
        return node >= pool() && node < pastPool();
    }

    Node* m_freeList;
    bool m_isDoneWithInitialFreeList;
    static const size_t m_poolSize = inlineCapacity;
    union {
        char pool[sizeof(Node) * m_poolSize];
        double forAlignment;
    } m_pool;
};

template<typename ValueArg, size_t inlineCapacity> struct ListHashSetNode {
    typedef ListHashSetNodeAllocator<ValueArg, inlineCapacity> NodeAllocator;

    template<typename T>
    ListHashSetNode(T&& value)
        : m_value(std::forward<T>(value))
        , m_prev(0)
        , m_next(0)
#ifndef NDEBUG
        , m_isAllocated(true)
#endif
    {
    }

    void* operator new(size_t, NodeAllocator* allocator)
    {
        return allocator->allocate();
    }
    void destroy(NodeAllocator* allocator)
    {
        this->~ListHashSetNode();
        allocator->deallocate(this);
    }

    ValueArg m_value;
    ListHashSetNode* m_prev;
    ListHashSetNode* m_next;

#ifndef NDEBUG
    bool m_isAllocated;
#endif
};

template<typename HashArg> struct ListHashSetNodeHashFunctions {
    template<typename T> static unsigned hash(const T& key) { return HashArg::hash(key->m_value); }
    template<typename T> static bool equal(const T& a, const T& b) { return HashArg::equal(a->m_value, b->m_value); }
    static const bool safeToCompareToEmptyOrDeleted = false;
};

template<typename ValueArg, size_t inlineCapacity, typename HashArg> class ListHashSetIterator {
private:
    typedef ListHashSet<ValueArg, inlineCapacity, HashArg> ListHashSetType;
    typedef ListHashSetIterator<ValueArg, inlineCapacity, HashArg> iterator;
    typedef ListHashSetConstIterator<ValueArg, inlineCapacity, HashArg> const_iterator;
    typedef ListHashSetNode<ValueArg, inlineCapacity> Node;
    typedef ValueArg ValueType;

    friend class ListHashSet<ValueArg, inlineCapacity, HashArg>;

    ListHashSetIterator(const ListHashSetType* set, Node* position) : m_iterator(set, position) { }

public:
    typedef ptrdiff_t difference_type;
    typedef ValueType value_type;
    typedef ValueType* pointer;
    typedef ValueType& reference;
    typedef std::bidirectional_iterator_tag iterator_category;

    ListHashSetIterator() { }

    // default copy, assignment and destructor are OK

    ValueType* get() const { return const_cast<ValueType*>(m_iterator.get()); }
    ValueType& operator*() const { return *get(); }
    ValueType* operator->() const { return get(); }

    iterator& operator++() { ++m_iterator; return *this; }

    // postfix ++ intentionally omitted

    iterator& operator--() { --m_iterator; return *this; }

    // postfix -- intentionally omitted

    // Comparison.
    bool operator==(const iterator& other) const { return m_iterator == other.m_iterator; }
    bool operator!=(const iterator& other) const { return m_iterator != other.m_iterator; }

    operator const_iterator() const { return m_iterator; }

private:
    Node* node() { return m_iterator.node(); }

    const_iterator m_iterator;
};

template<typename ValueArg, size_t inlineCapacity, typename HashArg> class ListHashSetConstIterator {
private:
    typedef ListHashSet<ValueArg, inlineCapacity, HashArg> ListHashSetType;
    typedef ListHashSetIterator<ValueArg, inlineCapacity, HashArg> iterator;
    typedef ListHashSetConstIterator<ValueArg, inlineCapacity, HashArg> const_iterator;
    typedef ListHashSetNode<ValueArg, inlineCapacity> Node;
    typedef ValueArg ValueType;

    friend class ListHashSet<ValueArg, inlineCapacity, HashArg>;
    friend class ListHashSetIterator<ValueArg, inlineCapacity, HashArg>;

    ListHashSetConstIterator(const ListHashSetType* set, Node* position)
        : m_set(set)
        , m_position(position)
    {
    }

public:
    typedef ptrdiff_t difference_type;
    typedef const ValueType value_type;
    typedef const ValueType* pointer;
    typedef const ValueType& reference;
    typedef std::bidirectional_iterator_tag iterator_category;

    ListHashSetConstIterator()
    {
    }

    const ValueType* get() const
    {
        return &m_position->m_value;
    }

    const ValueType& operator*() const { return *get(); }
    const ValueType* operator->() const { return get(); }

    const_iterator& operator++()
    {
        ASSERT(m_position != 0);
        m_position = m_position->m_next;
        return *this;
    }

    // postfix ++ intentionally omitted

    const_iterator& operator--()
    {
        ASSERT(m_position != m_set->m_head);
        if (!m_position)
            m_position = m_set->m_tail;
        else
            m_position = m_position->m_prev;
        return *this;
    }

    // postfix -- intentionally omitted

    // Comparison.
    bool operator==(const const_iterator& other) const
    {
        return m_position == other.m_position;
    }
    bool operator!=(const const_iterator& other) const
    {
        return m_position != other.m_position;
    }

private:
    Node* node() { return m_position; }

    const ListHashSetType* m_set;
    Node* m_position;
};

template<typename HashFunctions>
struct ListHashSetTranslator {
    template<typename T> static unsigned hash(const T& key) { return HashFunctions::hash(key); }
    template<typename T, typename U> static bool equal(const T& a, const U& b) { return HashFunctions::equal(a->m_value, b); }
    template<typename T, typename U, typename V> static void translate(T*& location, U&& key, const V& allocator)
    {
        location = new (allocator) T(std::forward<U>(key));
    }
};

template<typename T, size_t inlineCapacity, typename U>
inline ListHashSet<T, inlineCapacity, U>::ListHashSet()
    : m_head(0)
    , m_tail(0)
    , m_allocator(std::make_unique<NodeAllocator>())
{
}

template<typename T, size_t inlineCapacity, typename U>
inline ListHashSet<T, inlineCapacity, U>::ListHashSet(const ListHashSet& other)
    : m_head(0)
    , m_tail(0)
    , m_allocator(std::make_unique<NodeAllocator>())
{
    for (auto it = other.begin(), end = other.end(); it != end; ++it)
        add(*it);
}

template<typename T, size_t inlineCapacity, typename U>
inline ListHashSet<T, inlineCapacity, U>& ListHashSet<T, inlineCapacity, U>::operator=(const ListHashSet& other)
{
    ListHashSet tmp(other);
    swap(tmp);
    return *this;
}

template<typename T, size_t inlineCapacity, typename U>
inline void ListHashSet<T, inlineCapacity, U>::swap(ListHashSet& other)
{
    m_impl.swap(other.m_impl);
    std::swap(m_head, other.m_head);
    std::swap(m_tail, other.m_tail);
    m_allocator.swap(other.m_allocator);
}

template<typename T, size_t inlineCapacity, typename U>
inline ListHashSet<T, inlineCapacity, U>::~ListHashSet()
{
    deleteAllNodes();
}

template<typename T, size_t inlineCapacity, typename U>
inline int ListHashSet<T, inlineCapacity, U>::size() const
{
    return m_impl.size(); 
}

template<typename T, size_t inlineCapacity, typename U>
inline int ListHashSet<T, inlineCapacity, U>::capacity() const
{
    return m_impl.capacity(); 
}

template<typename T, size_t inlineCapacity, typename U>
inline bool ListHashSet<T, inlineCapacity, U>::isEmpty() const
{
    return m_impl.isEmpty(); 
}

template<typename T, size_t inlineCapacity, typename U>
inline T& ListHashSet<T, inlineCapacity, U>::first()
{
    ASSERT(!isEmpty());
    return m_head->m_value;
}

template<typename T, size_t inlineCapacity, typename U>
inline void ListHashSet<T, inlineCapacity, U>::removeFirst()
{
    takeFirst();
}

template<typename T, size_t inlineCapacity, typename U>
inline T ListHashSet<T, inlineCapacity, U>::takeFirst()
{
    ASSERT(!isEmpty());
    auto it = m_impl.find(m_head);

    T result = std::move((*it)->m_value);
    m_impl.remove(it);
    unlinkAndDelete(m_head);

    return result;
}

template<typename T, size_t inlineCapacity, typename U>
inline const T& ListHashSet<T, inlineCapacity, U>::first() const
{
    ASSERT(!isEmpty());
    return m_head->m_value;
}

template<typename T, size_t inlineCapacity, typename U>
inline T& ListHashSet<T, inlineCapacity, U>::last()
{
    ASSERT(!isEmpty());
    return m_tail->m_value;
}

template<typename T, size_t inlineCapacity, typename U>
inline const T& ListHashSet<T, inlineCapacity, U>::last() const
{
    ASSERT(!isEmpty());
    return m_tail->m_value;
}

template<typename T, size_t inlineCapacity, typename U>
inline void ListHashSet<T, inlineCapacity, U>::removeLast()
{
    takeLast();
}

template<typename T, size_t inlineCapacity, typename U>
inline T ListHashSet<T, inlineCapacity, U>::takeLast()
{
    ASSERT(!isEmpty());
    auto it = m_impl.find(m_tail);

    T result = std::move((*it)->m_value);
    m_impl.remove(it);
    unlinkAndDelete(m_tail);

    return result;
}

template<typename T, size_t inlineCapacity, typename U>
inline auto ListHashSet<T, inlineCapacity, U>::find(const ValueType& value) -> iterator
{
    auto it = m_impl.template find<BaseTranslator>(value);
    if (it == m_impl.end())
        return end();
    return makeIterator(*it); 
}

template<typename T, size_t inlineCapacity, typename U>
inline auto ListHashSet<T, inlineCapacity, U>::find(const ValueType& value) const -> const_iterator
{
    auto it = m_impl.template find<BaseTranslator>(value);
    if (it == m_impl.end())
        return end();
    return makeConstIterator(*it);
}

template<typename Translator>
struct ListHashSetTranslatorAdapter {
    template<typename T> static unsigned hash(const T& key) { return Translator::hash(key); }
    template<typename T, typename U> static bool equal(const T& a, const U& b) { return Translator::equal(a->m_value, b); }
};

template<typename ValueType, size_t inlineCapacity, typename U>
template<typename T, typename HashTranslator>
inline auto ListHashSet<ValueType, inlineCapacity, U>::find(const T& value) -> iterator
{
    auto it = m_impl.template find<ListHashSetTranslatorAdapter<HashTranslator>>(value);
    if (it == m_impl.end())
        return end();
    return makeIterator(*it);
}

template<typename ValueType, size_t inlineCapacity, typename U>
template<typename T, typename HashTranslator>
inline auto ListHashSet<ValueType, inlineCapacity, U>::find(const T& value) const -> const_iterator
{
    auto it = m_impl.template find<ListHashSetTranslatorAdapter<HashTranslator>>(value);
    if (it == m_impl.end())
        return end();
    return makeConstIterator(*it);
}

template<typename ValueType, size_t inlineCapacity, typename U>
template<typename T, typename HashTranslator>
inline bool ListHashSet<ValueType, inlineCapacity, U>::contains(const T& value) const
{
    return m_impl.template contains<ListHashSetTranslatorAdapter<HashTranslator> >(value);
}

template<typename T, size_t inlineCapacity, typename U>
inline bool ListHashSet<T, inlineCapacity, U>::contains(const ValueType& value) const
{
    return m_impl.template contains<BaseTranslator>(value);
}

template<typename T, size_t inlineCapacity, typename U>
auto ListHashSet<T, inlineCapacity, U>::add(const ValueType& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator>(value, m_allocator.get());
    if (result.isNewEntry)
        appendNode(*result.iterator);
    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, size_t inlineCapacity, typename U>
auto ListHashSet<T, inlineCapacity, U>::add(ValueType&& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator>(std::move(value), m_allocator.get());
    if (result.isNewEntry)
        appendNode(*result.iterator);
    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, size_t inlineCapacity, typename U>
auto ListHashSet<T, inlineCapacity, U>::appendOrMoveToLast(const ValueType& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator>(value, m_allocator.get());
    Node* node = *result.iterator;
    if (!result.isNewEntry)
        unlink(node);
    appendNode(node);

    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, size_t inlineCapacity, typename U>
auto ListHashSet<T, inlineCapacity, U>::appendOrMoveToLast(ValueType&& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator>(std::move(value), m_allocator.get());
    Node* node = *result.iterator;
    if (!result.isNewEntry)
        unlink(node);
    appendNode(node);

    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, size_t inlineCapacity, typename U>
auto ListHashSet<T, inlineCapacity, U>::prependOrMoveToFirst(const ValueType& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator>(value, m_allocator.get());
    Node* node = *result.iterator;
    if (!result.isNewEntry)
        unlink(node);
    prependNode(node);

    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, size_t inlineCapacity, typename U>
auto ListHashSet<T, inlineCapacity, U>::prependOrMoveToFirst(ValueType&& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator>(std::move(value), m_allocator.get());
    Node* node = *result.iterator;
    if (!result.isNewEntry)
        unlink(node);
    prependNode(node);

    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, size_t inlineCapacity, typename U>
auto ListHashSet<T, inlineCapacity, U>::insertBefore(const ValueType& beforeValue, const ValueType& newValue) -> AddResult
{
    return insertBefore(find(beforeValue), newValue);
}

template<typename T, size_t inlineCapacity, typename U>
auto ListHashSet<T, inlineCapacity, U>::insertBefore(const ValueType& beforeValue, ValueType&& newValue) -> AddResult
{
    return insertBefore(find(beforeValue), std::move(newValue));
}

template<typename T, size_t inlineCapacity, typename U>
auto ListHashSet<T, inlineCapacity, U>::insertBefore(iterator it, const ValueType& newValue) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator>(newValue, m_allocator.get());
    if (result.isNewEntry)
        insertNodeBefore(it.node(), *result.iterator);
    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, size_t inlineCapacity, typename U>
auto ListHashSet<T, inlineCapacity, U>::insertBefore(iterator it, ValueType&& newValue) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator>(std::move(newValue), m_allocator.get());
    if (result.isNewEntry)
        insertNodeBefore(it.node(), *result.iterator);
    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, size_t inlineCapacity, typename U>
inline bool ListHashSet<T, inlineCapacity, U>::remove(iterator it)
{
    if (it == end())
        return false;
    m_impl.remove(it.node());
    unlinkAndDelete(it.node());
    return true;
}

template<typename T, size_t inlineCapacity, typename U>
inline bool ListHashSet<T, inlineCapacity, U>::remove(const ValueType& value)
{
    return remove(find(value));
}

template<typename T, size_t inlineCapacity, typename U>
inline void ListHashSet<T, inlineCapacity, U>::clear()
{
    deleteAllNodes();
    m_impl.clear(); 
    m_head = 0;
    m_tail = 0;
}

template<typename T, size_t inlineCapacity, typename U>
void ListHashSet<T, inlineCapacity, U>::unlink(Node* node)
{
    if (!node->m_prev) {
        ASSERT(node == m_head);
        m_head = node->m_next;
    } else {
        ASSERT(node != m_head);
        node->m_prev->m_next = node->m_next;
    }

    if (!node->m_next) {
        ASSERT(node == m_tail);
        m_tail = node->m_prev;
    } else {
        ASSERT(node != m_tail);
        node->m_next->m_prev = node->m_prev;
    }
}

template<typename T, size_t inlineCapacity, typename U>
void ListHashSet<T, inlineCapacity, U>::unlinkAndDelete(Node* node)
{
    unlink(node);
    node->destroy(m_allocator.get());
}

template<typename T, size_t inlineCapacity, typename U>
void ListHashSet<T, inlineCapacity, U>::appendNode(Node* node)
{
    node->m_prev = m_tail;
    node->m_next = 0;

    if (m_tail) {
        ASSERT(m_head);
        m_tail->m_next = node;
    } else {
        ASSERT(!m_head);
        m_head = node;
    }

    m_tail = node;
}

template<typename T, size_t inlineCapacity, typename U>
void ListHashSet<T, inlineCapacity, U>::prependNode(Node* node)
{
    node->m_prev = 0;
    node->m_next = m_head;

    if (m_head)
        m_head->m_prev = node;
    else
        m_tail = node;

    m_head = node;
}

template<typename T, size_t inlineCapacity, typename U>
void ListHashSet<T, inlineCapacity, U>::insertNodeBefore(Node* beforeNode, Node* newNode)
{
    if (!beforeNode)
        return appendNode(newNode);
    
    newNode->m_next = beforeNode;
    newNode->m_prev = beforeNode->m_prev;
    if (beforeNode->m_prev)
        beforeNode->m_prev->m_next = newNode;
    beforeNode->m_prev = newNode;

    if (!newNode->m_prev)
        m_head = newNode;
}

template<typename T, size_t inlineCapacity, typename U>
void ListHashSet<T, inlineCapacity, U>::deleteAllNodes()
{
    if (!m_head)
        return;

    for (Node* node = m_head, *next = m_head->m_next; node; node = next, next = node ? node->m_next : 0)
        node->destroy(m_allocator.get());
}

template<typename T, size_t inlineCapacity, typename U>
inline auto ListHashSet<T, inlineCapacity, U>::makeIterator(Node* position) -> iterator
{
    return iterator(this, position);
}

template<typename T, size_t inlineCapacity, typename U>
inline auto ListHashSet<T, inlineCapacity, U>::makeConstIterator(Node* position) const -> const_iterator
{ 
    return const_iterator(this, position);
}

template<bool, typename ValueType, typename HashTableType>
void deleteAllValues(HashTableType& collection)
{
    for (auto it = collection.begin(), end = collection.end(); it != end; ++it)
        delete (*it)->m_value;
}

template<typename T, size_t inlineCapacity, typename U>
inline void deleteAllValues(const ListHashSet<T, inlineCapacity, U>& collection)
{
    deleteAllValues<true, typename ListHashSet<T, inlineCapacity, U>::ValueType>(collection.m_impl);
}

} // namespace WTF

using WTF::ListHashSet;

#endif /* WTF_ListHashSet_h */

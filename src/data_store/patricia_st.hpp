/*
 * patricia_st.hpp
 *
 * C++ adaptation of the Patricia Trie (PatriciaST) implementation
 * from Robert Sedgewick and Kevin Wayne's Algorithms, 4th Edition library.
 * Original Java source:
 *   https://algs4.cs.princeton.edu/code/edu/princeton/cs/algs4/PatriciaST.java.html
 *
 * Copyright (c) 2002-2025 Robert Sedgewick and Kevin Wayne.
 * All rights reserved.
 *
 * This C++ version was adapted for educational and practical use.
 * Original Java license: https://algs4.cs.princeton.edu/license.txt
 */

#pragma once

#include <string>
#include <vector>
#include <queue>
#include <stdexcept>
#include <optional>

template <typename Value>
class PatriciaST
{
private:
  struct Node
  {
    Node *left;
    Node *right;
    std::string key;
    std::optional<Value> val;
    int b;

    Node(const std::string &key, const std::optional<Value> &val, int b)
        : left(nullptr), right(nullptr), key(key), val(val), b(b) {}
  };

  Node *head;
  int count;

public:
  PatriciaST();
  ~PatriciaST();

  // Disable copy
  PatriciaST(const PatriciaST &) = delete;
  PatriciaST &operator=(const PatriciaST &) = delete;

  // Enable move
  PatriciaST(PatriciaST &&other) noexcept;
  PatriciaST &operator=(PatriciaST &&other) noexcept;

  void insert(const std::string &key, const Value &val);
  std::optional<Value> find(const std::string &key) const;
  void erase(const std::string &key);
  bool contains(const std::string &key) const;
  bool empty() const;
  int size() const;
  std::vector<std::string> keys() const;

private:
  static bool safeBitTest(const std::string &key, int b);
  static int bitTest(const std::string &key, int b);
  static int safeCharAt(const std::string &key, int i);
  static int firstDifferingBit(const std::string &k1, const std::string &k2);

  void keys(Node *node, int b, std::vector<std::string> &result) const;
  void collectNodes(Node *node, std::vector<Node *> &nodes);

  void clear();
};

// ==================== Implementation ====================

template <typename Value>
PatriciaST<Value>::PatriciaST(PatriciaST &&other) noexcept
    : head(other.head), count(other.count)
{
  other.head = nullptr;
  other.count = 0;
}

template <typename Value>
PatriciaST<Value> &PatriciaST<Value>::operator=(PatriciaST &&other) noexcept
{
  if (this != &other)
  {
    clear(); // Free current resources
    head = other.head;
    count = other.count;
    other.head = nullptr;
    other.count = 0;
  }
  return *this;
}

template <typename Value>
PatriciaST<Value>::PatriciaST()
{
  head = new Node("", std::nullopt, 0);
  head->left = head;
  head->right = head;
  count = 0;
}

template <typename Value>
PatriciaST<Value>::~PatriciaST()
{
  clear();
}

template <typename Value>
void PatriciaST<Value>::insert(const std::string &key, const Value &val)
{
  if (key.empty())
    throw std::invalid_argument("invalid key");

  Node *parent;
  Node *current = head;

  // 1. Traverse to find the closest node to 'key'
  do
  {
    parent = current;
    if (safeBitTest(key, current->b))
      current = current->right;
    else
      current = current->left;
  } while (parent->b < current->b);

  // 2. If the key already exists, update its value
  if (current->key == key)
  {
    current->val = val;
    return;
  }

  // 3. Key does not exist: find the first differing bit
  int b = firstDifferingBit(current->key, key);

  // 4. Traverse again to find the parent node where new node should be inserted
  current = head;
  do
  {
    parent = current;
    if (safeBitTest(key, current->b))
      current = current->right;
    else
      current = current->left;
  } while (parent->b < current->b && current->b < b);

  // 5. Create the new node at bit position 'b'
  Node *node = new Node(key, val, b);

  // 6. Set the new node's children
  if (safeBitTest(key, b))
  {
    node->left = current; // Existing subtree becomes left child
    node->right = node;   // New node points to itself as right child (leaf)
  }
  else
  {
    node->left = node;     // New node points to itself as left child (leaf)
    node->right = current; // Existing subtree becomes right child
  }

  // 7. Attach the new node to the parent
  if (safeBitTest(key, parent->b))
    parent->right = node;
  else
    parent->left = node;

  count++;
}

template <typename Value>
std::optional<Value> PatriciaST<Value>::find(const std::string &key) const
{
  if (key.empty())
    throw std::invalid_argument("invalid key");

  Node *parent;
  Node *current = head;

  // Traverse to find the closest node to 'key'
  do
  {
    parent = current;
    if (safeBitTest(key, current->b))
      current = current->right;
    else
      current = current->left;
  } while (parent->b < current->b);

  if (current->key == key)
    return current->val;
  else
    return std::nullopt;
}

template <typename Value>
void PatriciaST<Value>::erase(const std::string &key)
{
  if (key.empty())
    throw std::invalid_argument("invalid key");

  Node *grandparent = nullptr; // Node before parent in traversal
  Node *parent = head;         // Previous node in traversal
  Node *current = head;        // Node being examined

  // Traverse to find the node with the key (current), its parent, and grandparent
  do
  {
    grandparent = parent;
    parent = current;
    if (safeBitTest(key, current->b))
      current = current->right;
    else
      current = current->left;
  } while (parent->b < current->b);

  // If the key is found in the trie
  if (current->key == key)
  {
    Node *trueParent = nullptr; // The actual parent of current
    Node *search = head;

    // Find the true parent of 'current'
    do
    {
      trueParent = search;
      if (safeBitTest(key, search->b))
        search = search->right;
      else
        search = search->left;
    } while (search != current);

    if (current == parent) // Case 1: current is a leaf node
    {
      Node *child;
      if (safeBitTest(key, current->b))
        child = current->left;
      else
        child = current->right;
      if (safeBitTest(key, trueParent->b))
        trueParent->right = child;
      else
        trueParent->left = child;
      delete current;
    }
    else // Case 2: parent replaces current (current is an internal node)
    {
      Node *child;
      if (safeBitTest(key, parent->b))
        child = parent->left;
      else
        child = parent->right;
      if (safeBitTest(key, grandparent->b))
        grandparent->right = child;
      else
        grandparent->left = child;
      if (safeBitTest(key, trueParent->b))
        trueParent->right = parent;
      else
        trueParent->left = parent;

      Node *toDelete = current;
      parent->left = current->left;
      parent->right = current->right;
      parent->b = current->b;
      delete toDelete;
    }

    count--;
  }
}

template <typename Value>
bool PatriciaST<Value>::contains(const std::string &key) const
{
  return find(key).has_value();
}

template <typename Value>
bool PatriciaST<Value>::empty() const
{
  return count == 0;
}

template <typename Value>
int PatriciaST<Value>::size() const
{
  return count;
}

template <typename Value>
std::vector<std::string> PatriciaST<Value>::keys() const
{
  std::vector<std::string> result;
  if (head->left != head)
    keys(head->left, 0, result);
  if (head->right != head)
    keys(head->right, 0, result);
  return result;
}

/* The safeBitTest function logically appends a terminating sequence (when
 * required) to extend (logically) the string beyond its length.
 *
 * The inner loops of the get and insert methods flow much better when they
 * are not concerned with the lengths of strings, so a trick is employed to
 * allow the get and insert methods to view every string as an "infinite"
 * sequence of bits. Logically, every string gets a '\uff' character,
 * followed by an "infinite" sequence of '\u00' characters, appended to
 * the end.
 *
 * Note that the '\uff' character serves to mark the end of the string,
 * and it is necessary. Simply padding with '\u00' is insufficient to
 * make all unique Unicode strings "look" unique to the get and insert methods
 * (because these methods do not regard string lengths).
 */
template <typename Value>
bool PatriciaST<Value>::safeBitTest(const std::string &key, int b)
{
  if (b < key.length() * 8)
    return bitTest(key, b) != 0;
  if (b > key.length() * 8 + 7) // padding
    return false;
  return true; // end marker
}

template <typename Value>
int PatriciaST<Value>::bitTest(const std::string &key, int b)
{
  size_t idx = b >> 3; // b / 8 (which character)
  if (idx >= key.length())
    return 0;
  return (key[idx] >> (b & 0x7)) & 1; // b & 16 (which bit)
}

/* Like the safeBitTest function, the safeCharAt function makes every
 * string look like an "infinite" sequence of characters. Logically, every
 * string gets a '\uff' character, followed by an "infinite" sequence of
 * '\u00' characters, appended to the end.
 */
template <typename Value>
int PatriciaST<Value>::safeCharAt(const std::string &key, int i)
{
  if (i < static_cast<int>(key.length()))
    return static_cast<unsigned char>(key[i]);
  if (i > static_cast<int>(key.length()))
    return 0x00;
  else
    return 0xff;
}

/*
 * For efficiency's sake, the firstDifferingBit function compares entire
 * characters first, and then considers the individual bits (once it finds
 * two characters that do not match).
 *
 * Notice that the very first character comparison excludes the
 * least-significant bit. The firstDifferingBit function must never return
 * zero; otherwise, a node would become created as a child to the head
 * (sentinel) node that matches the bit-index value (zero) stored in the
 * head node. This would violate the invariant that bit-index values
 * increase as you descend into the trie.
 */
template <typename Value>
int PatriciaST<Value>::firstDifferingBit(const std::string &k1, const std::string &k2)
{
  int i = 0;
  int c1 = safeCharAt(k1, 0) & ~1;
  int c2 = safeCharAt(k2, 0) & ~1;
  if (c1 == c2)
  {
    i = 1;
    while (safeCharAt(k1, i) == safeCharAt(k2, i))
      i++;
    c1 = safeCharAt(k1, i);
    c2 = safeCharAt(k2, i);
  }
  int b = 0;
  while (((c1 >> b) & 1) == ((c2 >> b) & 1))
    b++;
  return i * 8 + b;
}

template <typename Value>
void PatriciaST<Value>::keys(Node *node, int b, std::vector<std::string> &result) const
{
  if (node->b > b)
  {
    keys(node->left, node->b, result);
    result.push_back(node->key);
    keys(node->right, node->b, result);
  }
}

template <typename Value>
void PatriciaST<Value>::collectNodes(Node *node, std::vector<Node *> &nodes)
{
  if (node == nullptr || node == head)
    return;
  nodes.push_back(node);
  if (node->left != node && node->left != head)
    collectNodes(node->left, nodes);
  if (node->right != node && node->right != head)
    collectNodes(node->right, nodes);
}

template <typename Value>
void PatriciaST<Value>::clear()
{
  if (head)
  {
    std::vector<Node *> nodes;
    collectNodes(head, nodes);
    for (Node *n : nodes)
      if (n != nullptr)
        delete n;
    head = nullptr;
    count = 0;
  }
}

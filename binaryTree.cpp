/*
    Dynamic Binary Search Tree (BST) Library
    -----------------------------------------
    A templated, pointer-based BST implementation supporting:
      - Dynamic node allocation (new/delete, RAII cleanup)
      - Insertion (iterative, with parent-pointer wiring)
      - Lookup / search
      - Deletion (all 3 cases, using parent-child transplant)
      - Min / Max
      - In-order successor / predecessor
      - Traversals: in-order, pre-order, post-order (iterators via callback)
      - Height, size, and tree validity check
      - Explicit parent-child pointer manipulation helpers (transplant,
        attachLeft, attachRight) exposed for direct low-level use

    Compile the demo with:
        g++ -std=c++17 bst.cpp -o bst
        ./bst
*/

#include <iostream>
#include <functional>
#include <stdexcept>
#include <vector>

// ---------------------------------------------------------------------
// Node
// ---------------------------------------------------------------------
template <typename T>
struct BSTNode {
    T data;
    BSTNode* left;
    BSTNode* right;
    BSTNode* parent;

    explicit BSTNode(const T& value, BSTNode* p = nullptr)
        : data(value), left(nullptr), right(nullptr), parent(p) {}
};

// ---------------------------------------------------------------------
// BST
// ---------------------------------------------------------------------
template <typename T>
class BST {
public:
    BST() : root(nullptr), nodeCount(0) {}

    // Deep-copy is disabled by default to keep pointer ownership simple.
    BST(const BST&) = delete;
    BST& operator=(const BST&) = delete;

    ~BST() { clear(); }

    // ---------------- Node allocation ----------------
    // Allocates a new node on the heap. Exposed so callers can perform
    // manual pointer manipulation if needed.
    BSTNode<T>* allocateNode(const T& value, BSTNode<T>* parent = nullptr) {
        return new BSTNode<T>(value, parent);
    }

    void deallocateNode(BSTNode<T>* node) {
        delete node;
    }

    // ---------------- Insertion ----------------
    // Iterative insertion; duplicates are ignored.
    BSTNode<T>* insert(const T& value) {
        if (root == nullptr) {
            root = allocateNode(value);
            nodeCount++;
            return root;
        }

        BSTNode<T>* current = root;
        BSTNode<T>* parent = nullptr;

        while (current != nullptr) {
            parent = current;
            if (value < current->data) {
                current = current->left;
            } else if (value > current->data) {
                current = current->right;
            } else {
                return current; // duplicate: no insertion
            }
        }

        BSTNode<T>* newNode = allocateNode(value, parent);
        if (value < parent->data) {
            attachLeft(parent, newNode);
        } else {
            attachRight(parent, newNode);
        }
        nodeCount++;
        return newNode;
    }

    // ---------------- Lookup ----------------
    BSTNode<T>* search(const T& value) const {
        BSTNode<T>* current = root;
        while (current != nullptr) {
            if (value == current->data) return current;
            current = (value < current->data) ? current->left : current->right;
        }
        return nullptr;
    }

    bool contains(const T& value) const {
        return search(value) != nullptr;
    }

    // ---------------- Min / Max ----------------
    BSTNode<T>* minNode(BSTNode<T>* node) const {
        if (node == nullptr) return nullptr;
        while (node->left != nullptr) node = node->left;
        return node;
    }

    BSTNode<T>* maxNode(BSTNode<T>* node) const {
        if (node == nullptr) return nullptr;
        while (node->right != nullptr) node = node->right;
        return node;
    }

    T minimum() const {
        BSTNode<T>* m = minNode(root);
        if (!m) throw std::runtime_error("Tree is empty");
        return m->data;
    }

    T maximum() const {
        BSTNode<T>* m = maxNode(root);
        if (!m) throw std::runtime_error("Tree is empty");
        return m->data;
    }

    // ---------------- Successor / Predecessor ----------------
    BSTNode<T>* successor(BSTNode<T>* node) const {
        if (node == nullptr) return nullptr;
        if (node->right != nullptr) return minNode(node->right);

        BSTNode<T>* parent = node->parent;
        while (parent != nullptr && node == parent->right) {
            node = parent;
            parent = parent->parent;
        }
        return parent;
    }

    BSTNode<T>* predecessor(BSTNode<T>* node) const {
        if (node == nullptr) return nullptr;
        if (node->left != nullptr) return maxNode(node->left);

        BSTNode<T>* parent = node->parent;
        while (parent != nullptr && node == parent->left) {
            node = parent;
            parent = parent->parent;
        }
        return parent;
    }

    // ---------------- Parent-child pointer manipulation ----------------
    // Attaches `child` as the left child of `parent`, wiring both directions.
    void attachLeft(BSTNode<T>* parent, BSTNode<T>* child) {
        if (parent) parent->left = child;
        if (child) child->parent = parent;
    }

    // Attaches `child` as the right child of `parent`, wiring both directions.
    void attachRight(BSTNode<T>* parent, BSTNode<T>* child) {
        if (parent) parent->right = child;
        if (child) child->parent = parent;
    }

    // Replaces the subtree rooted at `u` with the subtree rooted at `v`,
    // rewiring u's parent to point at v (classic CLRS "transplant").
    void transplant(BSTNode<T>* u, BSTNode<T>* v) {
        if (u->parent == nullptr) {
            root = v;
        } else if (u == u->parent->left) {
            u->parent->left = v;
        } else {
            u->parent->right = v;
        }
        if (v != nullptr) {
            v->parent = u->parent;
        }
    }

    // ---------------- Deletion ----------------
    bool remove(const T& value) {
        BSTNode<T>* node = search(value);
        if (node == nullptr) return false;
        removeNode(node);
        nodeCount--;
        return true;
    }

    void removeNode(BSTNode<T>* node) {
        if (node == nullptr) return;

        if (node->left == nullptr) {
            // Case 1: no left child (includes leaf nodes)
            transplant(node, node->right);
        } else if (node->right == nullptr) {
            // Case 2: no right child
            transplant(node, node->left);
        } else {
            // Case 3: two children -> use in-order successor
            BSTNode<T>* succ = minNode(node->right);
            if (succ->parent != node) {
                transplant(succ, succ->right);
                succ->right = node->right;
                succ->right->parent = succ;
            }
            transplant(node, succ);
            succ->left = node->left;
            succ->left->parent = succ;
        }
        deallocateNode(node);
    }

    // ---------------- Traversals ----------------
    void inorder(const std::function<void(const T&)>& visit) const {
        inorderHelper(root, visit);
    }

    void preorder(const std::function<void(const T&)>& visit) const {
        preorderHelper(root, visit);
    }

    void postorder(const std::function<void(const T&)>& visit) const {
        postorderHelper(root, visit);
    }

    std::vector<T> toSortedVector() const {
        std::vector<T> result;
        inorder([&](const T& v) { result.push_back(v); });
        return result;
    }

    // ---------------- Utility ----------------
    int height() const { return heightHelper(root); }

    size_t size() const { return nodeCount; }

    bool empty() const { return root == nullptr; }

    BSTNode<T>* getRoot() const { return root; }

    // Validates the BST property holds for the entire tree.
    bool isValid() const { return isValidHelper(root, nullptr, nullptr); }

    void clear() {
        clearHelper(root);
        root = nullptr;
        nodeCount = 0;
    }

private:
    BSTNode<T>* root;
    size_t nodeCount;

    void inorderHelper(BSTNode<T>* node, const std::function<void(const T&)>& visit) const {
        if (!node) return;
        inorderHelper(node->left, visit);
        visit(node->data);
        inorderHelper(node->right, visit);
    }

    void preorderHelper(BSTNode<T>* node, const std::function<void(const T&)>& visit) const {
        if (!node) return;
        visit(node->data);
        preorderHelper(node->left, visit);
        preorderHelper(node->right, visit);
    }

    void postorderHelper(BSTNode<T>* node, const std::function<void(const T&)>& visit) const {
        if (!node) return;
        postorderHelper(node->left, visit);
        postorderHelper(node->right, visit);
        visit(node->data);
    }

    int heightHelper(BSTNode<T>* node) const {
        if (!node) return -1; // empty subtree has height -1; single node has height 0
        int leftH = heightHelper(node->left);
        int rightH = heightHelper(node->right);
        return 1 + std::max(leftH, rightH);
    }

    bool isValidHelper(BSTNode<T>* node, const T* lowBound, const T* highBound) const {
        if (!node) return true;
        if (lowBound && !(*lowBound < node->data)) return false;
        if (highBound && !(node->data < *highBound)) return false;
        return isValidHelper(node->left, lowBound, &node->data) &&
               isValidHelper(node->right, &node->data, highBound);
    }

    void clearHelper(BSTNode<T>* node) {
        if (!node) return;
        clearHelper(node->left);
        clearHelper(node->right);
        delete node;
    }
};

// ---------------------------------------------------------------------
// Demo / manual test driver
// ---------------------------------------------------------------------
int main() {
    BST<int> tree;

    std::vector<int> values = {50, 30, 70, 20, 40, 60, 80, 10, 25, 65};
    std::cout << "Inserting: ";
    for (int v : values) {
        std::cout << v << " ";
        tree.insert(v);
    }
    std::cout << "\n\n";

    std::cout << "In-order traversal (sorted): ";
    tree.inorder([](const int& v) { std::cout << v << " "; });
    std::cout << "\n";

    std::cout << "Pre-order traversal: ";
    tree.preorder([](const int& v) { std::cout << v << " "; });
    std::cout << "\n";

    std::cout << "Post-order traversal: ";
    tree.postorder([](const int& v) { std::cout << v << " "; });
    std::cout << "\n\n";

    std::cout << "Tree size: " << tree.size() << "\n";
    std::cout << "Tree height: " << tree.height() << "\n";
    std::cout << "Is valid BST? " << (tree.isValid() ? "Yes" : "No") << "\n\n";

    std::cout << "Minimum: " << tree.minimum() << "\n";
    std::cout << "Maximum: " << tree.maximum() << "\n\n";

    int lookupValue = 40;
    BSTNode<int>* found = tree.search(lookupValue);
    std::cout << "Search " << lookupValue << ": "
              << (found ? "Found" : "Not found") << "\n";
    if (found) {
        BSTNode<int>* succ = tree.successor(found);
        BSTNode<int>* pred = tree.predecessor(found);
        std::cout << "  Successor of " << lookupValue << ": "
                  << (succ ? std::to_string(succ->data) : "None") << "\n";
        std::cout << "  Predecessor of " << lookupValue << ": "
                  << (pred ? std::to_string(pred->data) : "None") << "\n";
        std::cout << "  Parent: "
                  << (found->parent ? std::to_string(found->parent->data) : "None (root)") << "\n";
    }
    std::cout << "\n";

    std::cout << "Deleting 30 (node with two children)...\n";
    tree.remove(30);
    std::cout << "In-order after deletion: ";
    tree.inorder([](const int& v) { std::cout << v << " "; });
    std::cout << "\n";
    std::cout << "Is valid BST after deletion? " << (tree.isValid() ? "Yes" : "No") << "\n\n";

    std::cout << "Deleting 70 (node with two children, root of subtree)...\n";
    tree.remove(70);
    std::cout << "In-order after deletion: ";
    tree.inorder([](const int& v) { std::cout << v << " "; });
    std::cout << "\n";
    std::cout << "Tree size after deletions: " << tree.size() << "\n\n";

    std::cout << "Manual pointer manipulation demo:\n";
    BSTNode<int>* manualNode = tree.allocateNode(999);
    BSTNode<int>* root = tree.getRoot();
    tree.attachRight(root, nullptr); // detach existing right child link only as illustration
    std::cout << "  Allocated standalone node with value: " << manualNode->data << "\n";
    tree.deallocateNode(manualNode); // clean up manually allocated node
    std::cout << "  Deallocated the standalone node.\n";

    tree.clear();
    std::cout << "\nTree cleared. Empty? " << (tree.empty() ? "Yes" : "No") << "\n";

    return 0;
}

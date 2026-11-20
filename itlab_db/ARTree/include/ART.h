#pragma once
#include <iostream>
#include <string>
#include <utility>
#include <memory>
#include <cstdint>
#include <new>
#include <stdexcept>


struct PoolState {
    size_t size;
    char* start;
    char* current;
    void* free_head;

    PoolState(size_t bytes = 1024 * 1024):
        start(reinterpret_cast<char*>(::operator new(bytes))), size(bytes), current(start), free_head(nullptr){
    }
    ~PoolState() {
        ::operator delete (start);
    }
};

template <class T>
class FastAllocator {
public:
    std::shared_ptr<PoolState> pointer;
    using value_type = T;

    FastAllocator(size_t bytes = 1024 * 1024){
        pointer = std::make_shared<PoolState>(bytes);
    }

    template <class U>
    FastAllocator(FastAllocator<U>& other){  
        (*this).pointer = other.pointer;
    }

    template <typename... Args>
    void construct(T* ptr,Args&&... args) {
        new(ptr) T(std::forward<Args>(args)...);
    }

    template <class U>
    void destroy(U* pointer) {
        pointer->~U();
    }
    T* allocate(size_t cnt) {
        if (cnt == 1 && pointer->free_head) { 
            T* res = reinterpret_cast<T*>(pointer->free_head);
            pointer->free_head = *reinterpret_cast<void**>(pointer->free_head);
            return res; 
        }
        if (pointer->current + sizeof(T) * cnt > pointer->start + pointer->size) { throw std::bad_alloc(); }
        T* res = reinterpret_cast<T*>(pointer->current);
        pointer->current += sizeof(T)*cnt;
        return res;
    }

    void deallocate(T* ptr,size_t) {
        *reinterpret_cast<void**>(ptr) = pointer->free_head; // �������� � ������ 8 ���� ��������� �� ������
        pointer->free_head = ptr;
    }
    bool operator==(const FastAllocator& other)const {
        return (this->pointer->start == other.pointer->start);
    }
    bool operator!=(const FastAllocator& other) {
        return !((*this) == other);
    }

};



enum class NodeType : uint8_t { Node4, Node16, Node48, Node256, Leaf };

template <std::size_t size>
struct Node {
  NodeType type;
  char prefix[16];
  void* childrens[size];
  char keys[size];
  uint16_t prefix_len;
  uint16_t count;
  Node() : prefix_len(0), prefix{}, count(0), keys{},type(NodeType::Node4) {
    for (int i = 0; i < size; ++i) {
      childrens[i] = nullptr;
    }
  }
  void set_prefix(std::string prefix_) {
      prefix_len = (prefix_.length() > 16) ? 16 : prefix_.length();
      for (int i = 0;i < prefix_len;++i) {
          prefix[i] = prefix_[i];
      }

  }
  template <std::size_t other_size>
  Node(const Node<other_size>& other,NodeType type) {
      this->type = type;
      this->count = other.count;
      this->prefix_len = other.prefix_len;
      for (int i = 0;i < prefix_len;++i) {
          this->prefix[i] = other.prefix[i];
      }
      for (int i = 0;i < other.count;++i) {
          this->childrens[i] = other.childrens[i];
          this->keys[i] = other.keys[i];
      }
      for (int i = count;i < size;++i) {
          this->childrens[i] = nullptr;
          this->keys[i] = '\0';
      }
  }
};
template <typename T>
struct Leaf {
  NodeType type;
  uint16_t sufix_len;
  char sufix[16];
  T value;

  Leaf(std::string sufix_, T value_) {
      type = NodeType::Leaf;
      sufix_len = (sufix_.length() > 16) ? 16 : sufix_.length();
      for (int i = 0;i < sufix_len;++i) {
          sufix[i] = sufix_[i];
      }
      for (int i = sufix_len;i < 16;++i) {
          sufix[i] = '\0';
      }
      if (sufix_.length() == 0) { sufix[0] = ' '; }
      value = value_;
  }
};


NodeType getType(void* node) {
    NodeType* type = reinterpret_cast<NodeType*>(node);
    return (*type);
}

size_t getSize(void* node) {
    NodeType type = getType(node);
    switch (type) {
    case NodeType::Node4:return 4;
    case NodeType::Node16:return 16;
    case NodeType::Node48: return 48;
    case NodeType::Node256:return 256;
    case NodeType::Leaf:return 0;
    default:throw std::logic_error("invalid node");
    }
}

std::string find_prefix(std::string key1, std::string key2) {
    std::string res = "";
    int32_t len = (key1.length() < key2.length()) ? key1.length() : key2.length();
    for (int32_t i = 0;i < len;++i) {
        if (key1[i] == key2[i]) { res += key1[i]; }
        else { break; }
    }
    return res;
}

template <typename T>
class ARTree {
private:
    void* root;

    FastAllocator<Node<4>> node4_alloc{ 1024 * 1024 };
    FastAllocator<Node<16>> node16_alloc{ 1024 * 1024 };
    FastAllocator<Node<48>> node48_alloc{ 1024 * 1024 };
    FastAllocator<Node<256>> node256_alloc{ 1024 * 1024 };
    FastAllocator<Leaf<T>> leaf_alloc{ 1024 * 1024 };

    template <std::size_t size, typename... Args>
    Node<size>* create_node(Args&&... args) {
        if constexpr (size == 4) {
            Node<4>* res = node4_alloc.allocate(1);
            node4_alloc.construct(res, std::forward<Args>(args)...);
            return res;
        }
        else if constexpr (size == 16) {
            Node<16>* res = node16_alloc.allocate(1);
            node16_alloc.construct(res, std::forward<Args>(args)...);
            return res;
        }
        else if constexpr (size == 48) {
            Node<48>* res = node48_alloc.allocate(1);
            node48_alloc.construct(res, std::forward<Args>(args)...);
            return res;
        }
        else if constexpr (size == 256) {
            Node<256>* res = node256_alloc.allocate(1);
            node256_alloc.construct(res, std::forward<Args>(args)...);
            return res;
        }
        else {
            static_assert(size == 4 || size == 16 || size == 48 || size == 256, "unknown size");
        }
    }
    
    template <std::size_t size>
    bool isCheckRootAndKey(std::string key, void* cur_root) {
        NodeType type = getType(cur_root);
        Node<size>* root = reinterpret_cast<Node<size>*>(cur_root);
        if (key.length() < root->prefix_len) { return false; }
        for (int32_t i = 0;i < root->prefix_len;++i) {
            if (key[i] != root->prefix[i]) { return false; }
        }
        return true;
    }

    template <std::size_t size>
    void* pereform_root(void* old_root) {
        NodeType type = getType(old_root);
        Node<size>* root = reinterpret_cast<Node<size>*>(old_root);
        Node<4>* new_root = create_node<4>();
        node_add_child<4>(new_root, reinterpret_cast<void*>(root), root->prefix[0]);
        return reinterpret_cast<void*>(new_root);
    }

    template <std::size_t size>
    int32_t node_find_child(const Node<size>* node, char letter) {
        for (int32_t i = 0;i < node->count;++i) {
            if (node->keys[i] == letter) { return i; }
        }
        return -1;
    }

    template <std::size_t size>
    bool node_isFull(Node<size>* node) {
        return (node->count == size);
    }

    template <std::size_t size>
    void node_add_child(Node<size>* node, void* child, char letter) {
        if (node_isFull<size>(node)) { throw std::logic_error("node full"); }
        if (letter == '\0') { letter = ' '; }
        node->childrens[node->count] = child;
        node->keys[node->count++] = letter;
    }
    
    Node<16>* node4_grow_to_node16(Node<4>* node) {
        Node<16>* new_node = create_node<16>(*node, NodeType::Node16);
        node4_alloc.deallocate(node,1);
        return new_node;
    }

    Node<48>* node16_grow_to_node48(Node<16>* node) {
        Node<48>* new_node = create_node<48>(*node, NodeType::Node48);
        node16_alloc.deallocate(node,1);
        return new_node;
    }

    Node<256>* node48_grow_to_node256(Node<48>* node) {
        Node<256>* new_node = create_node<256>(*node, NodeType::Node256);
        node48_alloc.deallocate(node,1);
        return new_node;
    }

    template <std::size_t size>
    void shiftPrefixLeft(Node<size>* node, size_t count) {
        while (count > 0) {
            for (int32_t i = 0;i < node->prefix_len-1;++i) {
                node->prefix[i] = node->prefix[i + 1];
            }
            if (node->prefix_len > 0) {
                node->prefix[node->prefix_len - 1] = '\0';
                node->prefix_len--;
            }
            count--;
        }
    }

    bool leaf_compare(Leaf<T>* leaf, std::string key) {
        if (key.length() != leaf->sufix_len) { return false; }
        for (int32_t i = 0;i < leaf->sufix_len;++i) {
            if (leaf->sufix[i] != key[i]) { return false; }
        }
        return true;
    }

    void leaf_update(Leaf<T>* leaf, T new_value) {
        leaf->value = new_value;
    }

    Leaf<T>* create_leaf(std::string key,T value) {
        Leaf<T>* new_leaf = leaf_alloc.allocate(1);
        leaf_alloc.construct(new_leaf, key, value);
        return new_leaf;
    }

    
    Node<4>* leaf_split(Leaf<T>* old_leaf, std::string new_key, T new_value) {
        Node<4>* new_node = create_node<4>();
        std::string pref = find_prefix(new_key, std::string(old_leaf->sufix));
        new_node->set_prefix(pref);
        new_node->keys[0] = old_leaf->sufix[pref.length()];new_node->keys[1] = new_key[pref.length()]; 
        Leaf<T>* leaf1 = create_leaf(std::string(old_leaf->sufix).substr(new_node->prefix_len),old_leaf->value);
        Leaf<T>* leaf2 = create_leaf(new_key.substr(new_node->prefix_len), new_value); 
        new_node->childrens[0] = leaf1;new_node->childrens[1] = leaf2;
        new_node->count = 2;
        leaf_alloc.deallocate(old_leaf,1);
        return new_node;
    }
    
    template <std::size_t size>
    void* node_split(void* split, std::string key, T value) {
        Node<size>* node = reinterpret_cast<Node<size>*>(split);
        std::string pref = find_prefix(key, std::string(node->prefix));
        shiftPrefixLeft(node, pref.length());
        Leaf<T>* leaf = create_leaf(key.substr(pref.length()), value);
        Node<4>* new_node = create_node<4>();
        new_node->set_prefix(pref);
        node_add_child<4>(new_node, reinterpret_cast<void*>(leaf), leaf->sufix[0]);
        node_add_child<4>(new_node, split, node->prefix[0]);
        return reinterpret_cast<void*>(new_node);
    }

    
    void* insert_recursive(void* current_node, std::string key, T value) {
        NodeType type = getType(current_node);
        switch (type) {
        case NodeType::Leaf: {
            Leaf<T>* leaf_node = reinterpret_cast<Leaf<T>*>(current_node);
            if (leaf_compare(leaf_node, key)) { leaf_update(leaf_node, value);return current_node; }
            else {
                Node<4>* new_node = leaf_split(leaf_node, key, value);
                return reinterpret_cast<void*>(new_node);
            }
        }

        case NodeType::Node4: {
            Node<4>* node4 = reinterpret_cast<Node<4>*>(current_node);
            int32_t index_child;
            if (!isCheckRootAndKey<4>(key, current_node)) {
                 return node_split<4>(current_node, key, value);
            }
            if (key.length() == node4->prefix_len) {
                index_child = -1;
            }
            else {
                index_child = node_find_child<4>(node4, key.substr(node4->prefix_len)[0]);
            }
            if (index_child != -1) {
                void* new_child = insert_recursive(reinterpret_cast<void*>(node4->childrens[index_child]), key.substr(node4->prefix_len), value);
                node4->childrens[index_child] = new_child;
                return current_node;
            }
            else {
                std::string true_key = key.substr(node4->prefix_len);
                if (true_key.length() == 0) { key = ""; }
                Leaf<T>* new_leaf = create_leaf(true_key, value);
                if (node_isFull(node4)) {
                    Node<16>* new_node16 = node4_grow_to_node16(node4);
                    node_add_child<16>(new_node16, reinterpret_cast<void*>(new_leaf), true_key[0]);
                    return reinterpret_cast<void*>(new_node16);
                }
                else {
                    node_add_child<4>(node4, reinterpret_cast<void*>(new_leaf), true_key[0]);
                    return current_node;
                }
            }
        }

        case NodeType::Node16: {
            Node<16>* node16 = reinterpret_cast<Node<16>*>(current_node);
            if (!isCheckRootAndKey<16>(key, current_node)) {
                return node_split<16>(current_node, key, value);
            }
            int32_t index_child = node_find_child<16>(node16, key.substr(node16->prefix_len)[0]);
            if (index_child != -1) {
                void* new_child = insert_recursive(reinterpret_cast<void*>(node16->childrens[index_child]), key.substr(node16->prefix_len), value);
                node16->childrens[index_child] = new_child;
                return current_node;
            }
            else {
                std::string true_key = key.substr(node16->prefix_len);
                if (true_key.length() == 0) { key = ""; }
                Leaf<T>* new_leaf = create_leaf(true_key, value);
                if (node_isFull(node16)) {
                    Node<48>* new_node48 = node16_grow_to_node48(node16);
                    node_add_child<48>(new_node48, reinterpret_cast<void*>(new_leaf), true_key[0]);
                    return reinterpret_cast<void*>(new_node48);
                }
                else {
                    node_add_child<16>(node16, reinterpret_cast<void*>(new_leaf), true_key[0]);
                    return current_node;
                }
            }
        }

        case NodeType::Node48: {
            Node<48>* node48 = reinterpret_cast<Node<48>*>(current_node);
            if (!isCheckRootAndKey<48>(key, current_node)) {
                return node_split<48>(current_node, key, value);
            }
            int32_t index_child = node_find_child<48>(node48, key.substr(node48->prefix_len)[0]);
            if (index_child != -1) {
                void* new_child = insert_recursive(reinterpret_cast<void*>(node48->childrens[index_child]), key.substr(node48->prefix_len), value);
                node48->childrens[index_child] = new_child;
                return current_node;
            }
            else {
                std::string true_key = key.substr(node48->prefix_len);
                if (true_key.length() == 0) { key = ""; }
                Leaf<T>* new_leaf = create_leaf(true_key, value);
                if (node_isFull(node48)) {
                    Node<256>* new_node256 = node48_grow_to_node256(node48);
                    node_add_child<256>(new_node256, reinterpret_cast<void*>(new_leaf), true_key[0]);
                    return reinterpret_cast<void*>(new_node256);
                }
                else {
                    node_add_child<48>(node48, reinterpret_cast<void*>(new_leaf), true_key[0]);
                    return current_node;
                }
            }
        }

        case NodeType::Node256: {
            Node<256>* node256 = reinterpret_cast<Node<256>*>(current_node);
            if (!isCheckRootAndKey<256>(key, current_node)) {
                return node_split<256>(current_node, key, value);
            }
            int32_t index_child = node_find_child<256>(node256, key.substr(node256->prefix_len)[0]);
            if (index_child != -1) {
                void* new_child = insert_recursive(reinterpret_cast<void*>(node256->childrens[index_child]), key.substr(node256->prefix_len), value);
                node256->childrens[index_child] = new_child;
                return current_node;
            }
            else {
                std::string true_key = key.substr(node256->prefix_len);
                if (true_key.length() == 0) { key = ""; }
                Leaf<T>* new_leaf = create_leaf(true_key, value);
                if (node_isFull(node256)) { throw std::out_of_range("max node 256"); }
                else {
                    node_add_child<256>(node256, reinterpret_cast<void*>(new_leaf), true_key[0]);
                    return current_node;
                }
            }
        }

        }
    }

    T search(void* start_node,std::string key) {
        NodeType type = getType(start_node);
        bool isFind = false; 
        switch (type) {
        case NodeType::Leaf: {
            Leaf<T>* leaf = reinterpret_cast<Leaf<T>*>(start_node);
            if ((std::string(leaf->sufix) == key) || (std::string(leaf->sufix)[0] == ' ' && key.length() == 0)) { return leaf->value; }
            else { return T(); }
        }

        case NodeType::Node4: {
            Node<4>* node = reinterpret_cast<Node<4>*>(start_node);
            for (int32_t i = 0;i < node->count;++i) {
                if ((node->keys[i] == key.substr(node->prefix_len)[0]) || (key.substr(node->prefix_len).length() == 0 && node->keys[i] == ' ')) {
                    return search(node->childrens[i], key.substr(node->prefix_len));
                    isFind = true;
                    break;
                }
            }
            if (!isFind) { return T(); }
        }

        case NodeType::Node16: {
            Node<16>* node = reinterpret_cast<Node<16>*>(start_node);
            for (int32_t i = 0;i < node->count;++i) {
                if ((node->keys[i] == key.substr(node->prefix_len)[0]) || (key.substr(node->prefix_len).length() == 0 && node->keys[i] == ' ')) {
                    return search(node->childrens[i], key.substr(node->prefix_len));
                    isFind = true;
                    break;
                }
            }
            if (!isFind) { return T(); }
        }

        case NodeType::Node48: {
            Node<48>* node = reinterpret_cast<Node<48>*>(start_node);
            for (int32_t i = 0;i < node->count;++i) {
                if ((node->keys[i] == key.substr(node->prefix_len)[0]) || (key.substr(node->prefix_len).length() == 0 && node->keys[i] == ' ')) {
                    return search(node->childrens[i], key.substr(node->prefix_len));
                    isFind = true;
                    break;
                }
            }
            if (!isFind) { return T(); }
        }

        case NodeType::Node256: {
            Node<256>* node = reinterpret_cast<Node<256>*>(start_node);
            for (int32_t i = 0;i < node->count;++i) {
                if ((node->keys[i] == key.substr(node->prefix_len)[0]) || (key.substr(node->prefix_len).length() == 0 && node->keys[i] == ' ')) {
                    return search(node->childrens[i], key.substr(node->prefix_len));
                    isFind = true;
                    break;
                }
            }
            if (!isFind) { return T(); }
        }

        }
    }
public:
    void insert(std::string key, T value) {
        if (root == nullptr) {
            Leaf<T>* leaf = create_leaf(key, value);
            root = reinterpret_cast<void*>(leaf);
        }
        else {
            if (getType(root) != NodeType::Leaf) {
                std::size_t size = getSize(root);
                switch (size) {
                case 0: break;
                case 4: if (!isCheckRootAndKey<4>(key, root)) { root = pereform_root<4>(root); }break;
                case 16: if (!isCheckRootAndKey<16>(key, root)) { root = pereform_root<16>(root); }break;
                case 48: if (!isCheckRootAndKey<48>(key, root)) { root = pereform_root<48>(root); }break;
                case 256: if (!isCheckRootAndKey<256>(key, root)) { root = pereform_root<256>(root); }break;
                }
            }
            root = insert_recursive(root, key, value);
        }
    }
    void* getRoot() {
        return root;
    }
    ARTree(void* node = nullptr) :root(node) {}

    template<std::size_t size>
    ARTree(Node<size>* node) {
        Node<size>* new_root = create_node<size>();
        root = reinterpret_cast<void*>(new_root);
    }
    T search(std::string key) {
        return search(root, key);
    }
};
   
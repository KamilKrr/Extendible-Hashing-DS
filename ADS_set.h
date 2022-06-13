#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

template<typename Key, size_t N = 11>
class ADS_set {
public:
    class Iterator;
    using value_type = Key;
    using key_type = Key;
    using reference = value_type &;
    using const_reference = const value_type &;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using const_iterator = Iterator;
    using iterator = const_iterator;
    using key_equal = std::equal_to<key_type>;                       // Hashing
    using hasher = std::hash<key_type>;                              // Hashing

private:
    //TODO
    //if bucket full split instant in add
    //if bucket empty skip bucket in iterator loop
    //add new bucket_count variable
    //clear and others: manually reset DS (use delete[] and new[]) instead of swapping with empty set...
    //remove search for duplicate elements, when copying!
    //dont return pair from add! its slow
    //range insert pre split if possible at all?
    struct Bucket {
        key_type elements[N];
        bool occupied[N] = {false};
        size_type local_depth{0};
        Bucket *next{nullptr};
    };
    Bucket **index;
    size_type global_depth{1};
    size_type current_size{0};
    Bucket *entry{nullptr};
    void split(size_type idx) {
        Bucket *o = index[idx];
        if (o->local_depth >= global_depth) expand();
        Bucket *b = new Bucket;
        b->next = o->next;
        o->next = b;
        int new_local_depth = ++o->local_depth;
        b->local_depth = new_local_depth;

        for (size_type i{1}; i < static_cast<size_type>(1 << (global_depth-(new_local_depth-1))); i+=2) {
            index[((i << (new_local_depth-1)) | (idx & ((1 << (new_local_depth-1))-1)))] = b;
        }

        for (size_type i{0}; i < N; ++i) {
            if (index[h(o->elements[i]) % (1 << new_local_depth)] != o) {
                b->elements[i] = o->elements[i];
                std::swap(b->occupied[i], o->occupied[i]);
            }
        }
    }
    void expand() {
        global_depth++;
        Bucket **new_index = new Bucket *[1 << global_depth];
        size_type half = 1 << (global_depth - 1);

        for (size_type i{0}; i < half; ++i) {
            new_index[i] = new_index[i+half] = index[i];
        }

        delete[] index;
        index = new_index;
    }
    std::pair<Bucket*, size_type> locate(const key_type &key) const;

    size_type h(const key_type &key) const { return hasher{}(key) & ((1 << global_depth) - 1); };
    std::pair<Bucket*, size_type> add(const key_type &key);

public:
    ADS_set() : global_depth{1}, current_size{0} {
        Bucket *b = new Bucket;
        index = new Bucket *[(1 << global_depth)];
        index[0] = b;
        index[1] = b;
        entry = b;
    };

    ADS_set(std::initializer_list<key_type> ilist) : ADS_set{} { insert(ilist); }

    template<typename InputIt>
    ADS_set(InputIt first, InputIt last): ADS_set{} { insert(first, last); }
    ADS_set(const ADS_set &other);

    ~ADS_set();

    ADS_set &operator=(const ADS_set &other);
    ADS_set &operator=(std::initializer_list<key_type> ilist);

    size_type size() const { return current_size; }

    bool empty() const { return current_size == 0; }

    void insert(std::initializer_list<key_type> ilist) { insert(ilist.begin(), ilist.end()); }

    std::pair<iterator,bool> insert(const key_type &key);
    template<typename InputIt>
    void insert(InputIt first, InputIt last);

    void clear();
    size_type erase(const key_type &key);

    size_type count(const key_type &key) const;
    iterator find(const key_type &key) const;

    void swap(ADS_set &other);


    const_iterator begin() const { return const_iterator{entry, 0}; }

    const_iterator end() const { return const_iterator{nullptr, 0}; }

    void dump(std::ostream &o = std::cerr) const;

    friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {
        if (lhs.current_size != rhs.current_size) return false;
        for (const auto &k: lhs) if (!rhs.count(k)) return false;
        return true;
    }
    friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) { return !(lhs == rhs); }
};


template <typename Key, size_t N>
ADS_set<Key,N>::ADS_set(const ADS_set &other): ADS_set{} {
    //TODO: no checking for duplicates neccessary!
    for (const auto &k: other) add(k);
}

template<typename Key, size_t N>
ADS_set<Key, N>::~ADS_set() {
    while (entry != nullptr) {
        Bucket *help = entry->next;
        delete entry;
        entry = help;
    }
    delete[] index;
}

template <typename Key, size_t N>
ADS_set<Key,N> &ADS_set<Key,N>::operator=(const ADS_set &other) {
    ADS_set tmp{other};
    swap(tmp);
    return *this;
}

template <typename Key, size_t N>
ADS_set<Key,N> &ADS_set<Key,N>::operator=(std::initializer_list<key_type> ilist) {
    ADS_set tmp{ilist};
    swap(tmp);
    return *this;
}

template<typename Key, size_t N>
void ADS_set<Key, N>::clear() {
    ADS_set tmp;
    swap(tmp);
}

template<typename Key, size_t N>
void ADS_set<Key, N>::swap(ADS_set &other) {
    using std::swap;
    swap(index, other.index);
    swap(global_depth, other.global_depth);
    swap(current_size, other.current_size);
    swap(entry, other.entry);
}

template<typename Key, size_t N>
std::pair<typename ADS_set<Key,N>::Bucket*, size_t> ADS_set<Key,N>::add(const key_type &key) {
    size_type idx{h(key)};

    bool found{false};
    size_type empty{0};

    key_type *elements{index[idx]->elements};
    bool *occupied{index[idx]->occupied};

    for (size_type i{0}; i < N; ++i) {
        if(occupied[i] && key_equal{}(elements[i], key)){
            return {index[idx], i};
        }else if(!found && !occupied[i]){
            elements[i] = key;
            empty = i;
            found = true;
        }
    }
    if(found){
        occupied[empty] = true;
        current_size++;
        return {index[idx], empty};
    }
    split(idx);
    return add(key);
}

template<typename Key, size_t N>
typename ADS_set<Key, N>::size_type ADS_set<Key, N>::count(const key_type &key) const {
    auto e {locate(key)};
    return e.first != nullptr;
}

template <typename Key, size_t N>
std::pair<typename ADS_set<Key,N>::Bucket*, size_t> ADS_set<Key,N>::locate(const key_type &key) const {
    size_type idx{h(key)};

    for (size_type i{0}; i < N; ++i) {
        if (index[idx]->occupied[i] && key_equal{}(index[idx]->elements[i], key)) return {index[idx], i};
    }
    return {nullptr, 0};
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::iterator ADS_set<Key,N>::find(const key_type &key) const {
    auto e {locate(key)};
    return iterator{e.first, e.second};
}

template<typename Key, size_t N>
void ADS_set<Key, N>::dump(std::ostream &o) const {
    o << "global_depth = " << global_depth << "\n";
    for (size_type i{0}; i < static_cast<size_type>(1 << global_depth); ++i) {
        o << " - " << i << ": [";
        bool first{true};
        for (size_type j{0}; j < N; ++j) {
            if (first) first = false;
            else o << ", ";

            if (index[i]->occupied[j]) {
                o << index[i]->elements[j];
            } else {
                o << "?";
            }
        }
        o << "] lt = " << index[i]->local_depth << "\n";
    }
}

template<typename Key, size_t N>
template<typename InputIt>
void ADS_set<Key, N>::insert(InputIt first, InputIt last) {
    for (auto it{first}; it != last; ++it) {
        add(*it);
    }
}

template <typename Key, size_t N>
std::pair<typename ADS_set<Key,N>::iterator,bool> ADS_set<Key,N>::insert(const key_type &key) {
    size_type sz_before {current_size};
    auto it{add(key)};
    return std::make_pair(iterator{it.first, it.second}, current_size != sz_before);
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::size_type ADS_set<Key,N>::erase(const key_type &key) {
    auto e {locate(key)};
    if (e.first != nullptr && e.first->occupied[e.second]) {
        e.first->occupied[e.second] = false;
        --current_size;
        return 1;
    }
    return 0;
}

template<typename Key, size_t N>
class ADS_set<Key, N>::Iterator {
    Bucket *b;
    size_type idx;

    void skip() {
        while (b != nullptr && (idx >= N || !b->occupied[idx])) {
            idx++;
            if (idx >= N) {
                idx = 0;
                b = b->next;
            }
        }
    }

public:
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type &;
    using pointer = const value_type *;
    using iterator_category = std::forward_iterator_tag;

    explicit Iterator(Bucket *b = nullptr, size_type idx = 0) : b{b}, idx{idx} { if(b) skip(); };

    reference operator*() const { return b->elements[idx]; }

    pointer operator->() const { return &b->elements[idx]; }

    Iterator &operator++() {
        ++idx;
        skip();
        return *this;
    }

    Iterator operator++(int) {
        auto rc{*this};
        ++*this;
        return rc;
    }

    friend bool operator==(const Iterator &lhs, const Iterator &rhs) { return (lhs.b == rhs.b && lhs.idx == rhs.idx); }

    friend bool operator!=(const Iterator &lhs, const Iterator &rhs) { return !(lhs == rhs); }
};

template<typename Key, size_t N>
void swap(ADS_set<Key, N> &lhs, ADS_set<Key, N> &rhs) { lhs.swap(rhs); }

#endif // ADS_SET_H
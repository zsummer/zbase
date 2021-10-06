
/*
* zlist_ext License
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/



#ifndef  ZLIST_EXT_H
#define ZLIST_EXT_H


namespace zsummer
{
    using s8 = char;
    using u8 = unsigned char;
    using s16 = short int;
    using u16 = unsigned short int;
    using s32 = int;
    using u32 = unsigned int;
    using s64 = long long;
    using u64 = unsigned long long;
    using f32 = float;
    using f64 = double;

#if __GNUG__ && __GNUC__ < 5
#define IS_TRIVIALLY_COPYABLE(T) __has_trivial_copy(T)
#else
#define IS_TRIVIALLY_COPYABLE(T) std::is_trivially_copyable<T>::value
#endif

#if __GNUG__
#define MAY_ALIAS __attribute__((__may_alias__))
#else
#define MAY_ALIAS
#endif

    template<class _List>
    struct ConstListExtIterator;

    template<class _List>
    struct ListExtIterator
    {
        using _Node = typename _List::Node;
        using _Ty = typename _List::value_type;
        using difference_type = typename _List::difference_type;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename _List::value_type;
        using pointer = typename _List::pointer;
        using reference = typename _List::reference;

        ListExtIterator(const _Node* const head, u32 id) { head_ = const_cast<_Node*>(head); id_ = id; }
        ListExtIterator(const ListExtIterator<_List>& other) { head_ = const_cast<_Node*>(other.head_); id_ = other.id_; }
        ListExtIterator(const ConstListExtIterator<_List>& other) { head_ = const_cast<_Node*>(other.head_); id_ = other.id_; }
        ListExtIterator() :ListExtIterator(NULL, 0) {}
        ListExtIterator& operator ++()
        {
            id_ = (head_ + id_)->next;
            return *this;
        }
        ListExtIterator& operator --()
        {
            id_ = (head_ + id_)->front;
            return *this;
        }
        ListExtIterator operator ++(int)
        {
            ListExtIterator old = *this;
            id_ = (head_ + id_)->next;
            return old;
        }
        ListExtIterator operator --(int)
        {
            ListExtIterator old = *this;
            id_ = (head_ + id_)->front;
            return old;
        }

        pointer operator ->() const
        {
            return _List::node_cast(head_ + id_);
        }
        reference operator *() const
        {
            return *_List::node_cast(head_ + id_);
        }

    public:
        _Node* head_;
        u32 id_;
    };
    template<class _List>
    bool operator == (const ListExtIterator<_List>& n1, const ListExtIterator<_List>& n2)
    {
        return n1.head_ == n2.head_ && n1.id_ == n2.id_;
    }
    template<class _List>
    bool operator != (const ListExtIterator<_List>& n1, const ListExtIterator<_List>& n2)
    {
        return !(n1 == n2);
    }

    template<class _List>
    struct ConstListExtIterator
    {
        using _Node = typename _List::Node;
        using _Ty = typename _List::value_type;
        using difference_type = typename _List::difference_type;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename _List::value_type;
        using const_pointer = typename _List::const_pointer;
        using const_reference = typename _List::const_reference;

        ConstListExtIterator(const _Node* const head, u32 id) { head_ = const_cast<_Node*>(head); id_ = id; }
        ConstListExtIterator(const ConstListExtIterator<_List>& other) { head_ = const_cast<_Node*>(other.head_); id_ = other.id_; }
        ConstListExtIterator(const ListExtIterator<_List>& other) { head_ = const_cast<_Node*>(other.head_); id_ = other.id_; }
        ConstListExtIterator() :ConstListExtIterator(NULL, 0) {}
        ConstListExtIterator& operator ++()
        {
            id_ = (head_ + id_)->next;
            return *this;
        }
        ConstListExtIterator& operator --()
        {
            id_ = (head_ + id_)->front;
            return *this;
        }
        ConstListExtIterator operator ++(int)
        {
            ConstListExtIterator old = *this;
            id_ = (head_ + id_)->next;
            return old;
        }
        ConstListExtIterator operator --(int)
        {
            ConstListExtIterator old = *this;
            id_ = (head_ + id_)->front;
            return old;
        }

        const_pointer operator ->() const
        {
            return _List::node_cast(head_ + id_);
        }
        const_reference operator *() const
        {
            return *_List::node_cast(head_ + id_);
        }
    public:
        _Node* head_;
        u32 id_;
    };
    template<class _List>
    bool operator == (const ConstListExtIterator<_List>& n1, const ConstListExtIterator<_List>& n2)
    {
        return n1.head_ == n2.head_ && n1.id_ == n2.id_;
    }
    template<class _List>
    bool operator != (const ConstListExtIterator<_List>& n1, const ConstListExtIterator<_List>& n2)
    {
        return !(n1 == n2);
    }

    //list need init all nodes.
    template<class _Ty, size_t _Size, size_t _FixedSize>
    class zlist_ext
    {
    public:
        struct Node;
        using value_type = _Ty;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using pointer = _Ty*;
        using const_pointer = const _Ty*;
        using reference = _Ty&;
        using const_reference = const _Ty&;

        static const u64 FENCE_VAL = 0xdeadbeafdeadbeafULL;
        static const u64 MAX_SIZE = _Size;
        static_assert(_Size > 0, "");
        static_assert(_FixedSize > 0, "");
        static_assert(_FixedSize <= _Size, "");

        using iterator = ListExtIterator<zlist_ext<_Ty, _Size, _FixedSize>>;
        using const_iterator = ConstListExtIterator<zlist_ext<_Ty, _Size, _FixedSize>>;

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using space_type = typename std::aligned_storage<sizeof(_Ty), alignof(_Ty)>::type;

    public:
        struct Node
        {
            u32 front;
            u32 next;
            space_type *space;
        };
        static _Ty* MAY_ALIAS node_cast(Node* node) { return reinterpret_cast<_Ty*>(node->space); }
    public:

        zlist_ext()
        {
            init();
        }
        ~zlist_ext()
        {
            if (std::is_object<_Ty>::value)
            {
                clear();
            }
            if (dync_space_ != NULL)
            {
                delete[] dync_space_;
            }
        }

        zlist_ext(std::initializer_list<_Ty> init_list)
        {
            init();
            assign(init_list.begin(), init_list.end());
        }
        zlist_ext<_Ty, _Size, _FixedSize>& operator = (const zlist_ext<_Ty, _Size, _FixedSize>& other)
        {
            if (data() == other.data())
            {
                return *this;
            }
            assign(other.begin(), other.end());
            return *this;
        }



        //std::array api
        iterator begin() noexcept { return iterator(&data_[0], used_id_); }
        const_iterator begin() const noexcept { return const_iterator(&data_[0], used_id_); }
        const_iterator cbegin() const noexcept { return const_iterator(&data_[0], used_id_); }

        iterator end() noexcept { return iterator(&data_[0], end_id_); }
        const_iterator end() const noexcept { return const_iterator(&data_[0], end_id_); }
        const_iterator cend() const noexcept { return const_iterator(&data_[0], end_id_); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }


        reference front() { return *node_cast(&data_[used_id_]); }
        const_reference front() const { return *node_cast(&data_[used_id_]); }
        reference back() { return *node_cast(&data_[data_[end_id_].front]); }
        const_reference back() const { *node_cast(&data_[data_[end_id_].front]); }

 //       static constexpr u32 static_buf_size(u32 obj_count) { return sizeof(zlist_ext<_Ty, 1>) + sizeof(Node) * (obj_count - 1); }


        const size_type size() const noexcept { return used_count_; }
        const size_type max_size()  const noexcept { return end_id_; }
        const bool empty() const noexcept { return !used_count_; }
        const bool full() const noexcept { return size() == max_size(); }
        size_type capacity() const { return max_size(); }
        const Node* data() const noexcept { return data_; }
        const bool is_valid_node(void* addr) const noexcept
        {
            u64 uaddr = (u64)addr;
            u64 udata = (u64)data_;
            if (uaddr < udata || uaddr >= udata + sizeof(Node) * max_size())
            {
                return false;
            }
            return true;
        }

        void clear()
        {
            erase(begin(), end());
        }

        void fill(const _Ty& value)
        {
            clear();
            insert(end(), max_size(), value);
        }

        void init()
        {
            end_id_ = _Size;
            used_count_ = 0;
            free_id_ = 0;
            for (u32 i = 0; i < _Size + 1; i++)
            {
                data_[i].next = (u32)i + 1;
                data_[i].front = end_id_;
                data_[i].space = NULL;
            }
            data_[end_id_].next = end_id_;
            used_id_ = end_id_;
            dync_space_ = 0;
        }
    private:
        bool push_free_node(u32 id)
        {
            Node* node = &data_[id];
            node->next = free_id_;
            node->front = end_id_;
            free_id_ = id;
            return true;
        }


        bool pick_used_node(u32 id)
        {
            if (id >= end_id_)
            {
                return false;
            }
            Node* node = &data_[id];
            if (used_id_ >= end_id_)
            {
                return false; //empty
            }
            if (std::is_object<_Ty>::value)
            {
                node_cast(node)->~_Ty();
            }

            if (used_id_ == id)
            {
                used_id_ = node->next;
                data_[used_id_].front = end_id_;
            }
            else
            {
                data_[node->front].next = node->next;
                data_[node->next].front = node->front;
            }
            used_count_--;
            return true;
        }
        bool release_used_node(u32 id)
        {
            if (pick_used_node(id))
            {
                return push_free_node(id);
            }
            //LogError() << "release used node error";
            return false;
        }
        void inject(u32 pos_id, u32 new_id)
        {
            data_[new_id].next = pos_id;
            data_[new_id].front = data_[pos_id].front;
            if (pos_id == used_id_)
            {
                used_id_ = new_id;
            }
            else
            {
                data_[data_[pos_id].front].next = new_id;
            }

            data_[pos_id].front = new_id;
            used_count_++;
        }
        u32 inject(u32 id, const _Ty& value)
        {
            if (free_id_ >= end_id_ || free_id_ >= _Size)
            {
                return end_id_;
            }
            u32 new_id = free_id_;
            free_id_ = data_[new_id].next;

            if (new_id >= _FixedSize)
            {
                if (dync_space_ == NULL)
                {
                    dync_space_ = new space_type[_Size - _FixedSize];
                }
                data_[new_id].space = &dync_space_[new_id - _FixedSize];
            }
            else
            {
                data_[new_id].space = &fixed_space_[new_id];
            }
            inject(id, new_id);
            new (data_[new_id].space) _Ty(value);
            return new_id;
        }
        template< class... Args >
        u32 inject_emplace(u32 id, Args&&... args)
        {
            if (free_id_ >= end_id_ || free_id_ >= _Size)
            {
                return end_id_;
            }
            u32 new_id = free_id_;
            free_id_ = data_[new_id].next;
            if (new_id >= _FixedSize)
            {
                if (dync_space_ == NULL)
                {
                    dync_space_ = new space_type[_Size - _FixedSize ];
                }
                data_[new_id].space = &dync_space_[new_id - _FixedSize];
            }
            else
            {
                data_[new_id].space = &fixed_space_[new_id];
            }

            inject(id, new_id);
            new (data_[new_id].space) _Ty(args ...);
            return new_id;
        }
    public:
        iterator insert(iterator pos, const _Ty& value)
        {
            return iterator(&data_[0], inject(pos.id_, value));
        }

        template< class... Args >
        iterator emplace(iterator pos, Args&&... args)
        {
            return iterator(&data_[0], inject_emplace(pos.id_, args...));
        }

        iterator insert(iterator pos, size_type count, const _Ty& value)
        {
            if (used_count_ + count > max_size())
            {
                return end();
            }
            if (count == 0)
            {
                return end();
            }
            for (size_t i = 0; i < count; i++)
            {
                pos.id_ = inject(pos.id_, value);
            }
            return pos;
        }

        template<class other_iterator>
        iterator insert(iterator pos, other_iterator first, other_iterator last)
        {
            if (first == last)
            {
                return end();
            }
            --last;
            while (first != last)
            {
                pos = insert(pos, *(last--));
            }
            pos = insert(pos, *last);
            return pos;
        }

        template<class other_iterator>
        iterator assign(other_iterator first, other_iterator last)
        {
            clear();
            return insert(end(), first, last);
        }


        void push_back(const _Ty& value) { inject(end_id_, value); }
        bool pop_back() { return release_used_node(data_[end_id_].front); }
        void push_front(const _Ty& value) { inject(used_id_, value); }
        bool pop_front() { return release_used_node(used_id_); }

        iterator erase(iterator pos)
        {
            u32 pos_id = pos.id_;
            pos++;
            if (!release_used_node(pos_id))
            {
                pos = end();
            }
            return pos;
        }

        //[first,last)
        iterator erase(iterator first, iterator last)
        {
            while (first != last)
            {
                erase(first++);
            }
            return last;
        }


        template< class... Args >
        iterator emplace_back(Args&&... args)
        {
            return emplace(end(), args...);
        }
        template< class... Args >
        iterator emplace_front(Args&&... args)
        {
            return emplace(begin(), args...);
        }

    private:
        template<class Comp>
        iterator comp_bound(iterator first, iterator last, const _Ty& value, Comp comp)
        {
            while (first != last)
            {
                if (comp(*first, value))
                {
                    first++;
                    continue;
                }
                break;
            }
            return first;
        }
    public:
        template<class Less = std::less<_Ty>>
        iterator lower_bound(iterator first, iterator last, const _Ty& value, Less less = Less())
        {
            return comp_bound(first, last, value, less);
        }
        template<class Greater = std::greater<_Ty>>
        iterator upper_bound(iterator first, iterator last, const _Ty& value, Greater greater = Greater())
        {
            return comp_bound(first, last, value, greater);
        }
    private:
        u32 used_count_;
        u32 free_id_;
        u32 used_id_;
        u32 end_id_;
        Node data_[_Size + 1];
        space_type fixed_space_[_FixedSize];
        space_type* dync_space_;// space_type dync_space_[Size - _FixedSize];_
    };

    template<class _Ty, size_t _Size, size_t _FixedSize>
    bool operator==(const zlist_ext<_Ty, _Size, _FixedSize>& a1, const zlist_ext<_Ty, _Size, _FixedSize>& a2)
    {
        return a1.data() == a2.data();
    }




}


#endif
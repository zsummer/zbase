
/*
* zhash_map License
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



#ifndef  ZHASH_MAP_H
#define ZHASH_MAP_H


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




    template<class node_type, class Key, class _Ty, u32 INVALID_NODE_ID, u32 HASH_COUNT>
    struct zhash_map_iterator
    {
        using value_type = std::pair<const Key, _Ty>;
        node_type* node_pool_;
        u32 cur_node_id_;
        u32 max_node_id_; //迭代器使用过程中不更新 即迭代器一旦创建 不保证能迭代到新增元素;   

        operator node_type* () const { return node_pool_[cur_node_id_]; }
        operator const node_type* ()const { return node_pool_[cur_node_id_]; }
        zhash_map_iterator()
        {
            node_pool_ = NULL;
            cur_node_id_ = INVALID_NODE_ID;
            max_node_id_ = 0;
        }
        zhash_map_iterator(node_type* pool,  u32 node_id, u32 max_node_id)
        {
            node_pool_ = pool;
            cur_node_id_ = node_id;
            max_node_id_ = max_node_id;
        }
        zhash_map_iterator(const zhash_map_iterator& other)
        {
            node_pool_ = other.node_pool_;
            cur_node_id_ = other.cur_node_id_;
            max_node_id_ = other.max_node_id_;
        }

        void next()
        {
            if (node_pool_ == NULL)
            {
                return;
            }

            for (u32 i = cur_node_id_ + 1; i < max_node_id_; i++)
            {
                if (node_pool_[i].hash_id < HASH_COUNT)
                {
                    cur_node_id_ = i;
                    return;
                }
            }
            cur_node_id_ = INVALID_NODE_ID;
            return;
        }

        zhash_map_iterator& operator ++()
        {
            next();
            return *this;
        }

        zhash_map_iterator operator ++(int)
        {
            zhash_map_iterator result(*this);
            next();
            return result;
        }

        value_type* operator ->()
        {
            return (value_type*)&node_pool_[cur_node_id_].val_space;
        }
        value_type& operator *()
        {
            return *((value_type*)&node_pool_[cur_node_id_].val_space);
        }
    };

    template<class node_type, class Key, class _Ty, u32 INVALID_NODE_ID, u32 HASH_COUNT>
    bool operator == (const zhash_map_iterator<node_type, Key, _Ty, INVALID_NODE_ID, HASH_COUNT>& n1, const zhash_map_iterator<node_type, Key, _Ty, INVALID_NODE_ID, HASH_COUNT>& n2)
    {
        return n1.node_pool_ == n2.node_pool_ && n1.cur_node_id_ == n2.cur_node_id_;
    }
    template<class node_type, class Key, class _Ty, u32 INVALID_NODE_ID, u32 HASH_COUNT>
    bool operator != (const zhash_map_iterator<node_type, Key, _Ty, INVALID_NODE_ID, HASH_COUNT>& n1, const zhash_map_iterator<node_type, Key, _Ty, INVALID_NODE_ID, HASH_COUNT>& n2)
    {
        return !(n1 == n2);
    }

    template<class Key,
        class _Ty,
        u32 _Size,
        class Hash = std::hash<Key>,
        class KeyEqual = std::equal_to<Key>>
        class zhash_map
    {
    public:

        using size_type = u32;
        const static size_type FREE_POOL_SIZE = 0;
        const static size_type NODE_COUNT = _Size;
        const static size_type INVALID_NODE_ID = NODE_COUNT + 1;
        const static size_type HASH_COUNT = NODE_COUNT * 2;
        constexpr size_type max_size() const { return NODE_COUNT; }
        //constexpr size_type max_bucket_count() const { return HASH_COUNT; }
        using key_type = Key;
        using mapped_type = _Ty;
        using value_type = std::pair<const key_type, mapped_type>;
        using reference = value_type&;
        using const_reference = const value_type&;
        using space_type = typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type;
        struct node_type
        {
            u32 next;
            u32 hash_id;
            space_type val_space;
        };
        using iterator = zhash_map_iterator<node_type, key_type, mapped_type, INVALID_NODE_ID, HASH_COUNT>;
        using const_iterator = const iterator;
        Hash hasher;
        KeyEqual key_equal;
    private:
        u32 buckets_[HASH_COUNT];
        node_type node_pool_[INVALID_NODE_ID];
        u32 first_valid_node_id_;
        u32 exploit_offset_;
        u32 count_;
        iterator mi(u32 node_id) { return iterator(node_pool_, node_id, exploit_offset_ + 1); }
        static reference rf(node_type& b) { return *reinterpret_cast<value_type*>(&b.val_space); }

        void reset()
        {
            exploit_offset_ = 0;
            count_ = 0;
            first_valid_node_id_ = 0;
            node_pool_[FREE_POOL_SIZE].next = 0;
            node_pool_[FREE_POOL_SIZE].hash_id = HASH_COUNT;
            memset(buckets_, 0, sizeof(u32) * HASH_COUNT);
        }

        u32 pop_free()
        {
            if (node_pool_[FREE_POOL_SIZE].next != 0)
            {
                u32 ret = node_pool_[FREE_POOL_SIZE].next;
                node_pool_[ret].hash_id = HASH_COUNT;
                node_pool_[FREE_POOL_SIZE].next = node_pool_[ret].next;
                count_++;
                if (first_valid_node_id_ == 0 || ret < first_valid_node_id_)
                {
                    first_valid_node_id_ = ret;
                }
                return ret;
            }
            if (exploit_offset_ < NODE_COUNT)
            {
                u32 ret = ++exploit_offset_;
                node_pool_[ret].hash_id = HASH_COUNT;
                count_++;
                if (first_valid_node_id_ == 0 || ret < first_valid_node_id_)
                {
                    first_valid_node_id_ = ret;
                }
                return ret;
            }
            return 0;
        }

        void push_free(u32 node_id)
        {
            node_pool_[node_id].hash_id = HASH_COUNT;
            node_pool_[node_id].next = node_pool_[FREE_POOL_SIZE].next;
            node_pool_[FREE_POOL_SIZE].next = node_id;
            count_--;
            if (node_id == first_valid_node_id_)
            {
                first_valid_node_id_ = next_b(first_valid_node_id_+1).cur_node_id_;
            }
        }

        iterator next_b(u32 node_id)
        {
            for (u32 i = node_id; i <= exploit_offset_; i++)
            {
                if (node_pool_[i].hash_id != HASH_COUNT)
                {
                    return mi(i);
                }
            }
            return end();
        }



        std::pair<iterator, bool> insert_v(const value_type& val, bool assign)
        {
            iterator finder = find(val.first);
            if (finder != end())
            {
                if (assign)
                {
                    finder->second = val.second;
                }
                return { finder, false };
            }

            u32 ukey = (u32)hasher(val.first);
            u32 hash_id = ukey % HASH_COUNT;

            u32 new_node_id = pop_free();
            if (new_node_id == 0)
            {
                return { end(), false };
            }
            node_type& node = node_pool_[new_node_id];
            node.next = 0;
            node.hash_id = (u32)hash_id;
            if (!std::is_trivial<_Ty>::value)
            {
                new (&node.val_space) value_type(val);
            }
            else
            {
                memcpy(&node.val_space, &val, sizeof(val));
                //rf(node) = val;
            }

            if (buckets_[hash_id] != 0)
            {
                node.next = buckets_[hash_id];
            }
            buckets_[hash_id] = new_node_id;
            return { mi(new_node_id), true };
        }

    public:
        iterator begin() noexcept { return next_b(first_valid_node_id_); }
        const_iterator begin() const noexcept { return next_b(first_valid_node_id_); }
        const_iterator cbegin() const noexcept { return next_b(first_valid_node_id_); }

        iterator end() noexcept { return mi(INVALID_NODE_ID); }
        const_iterator end() const noexcept { return mi(INVALID_NODE_ID); }
        const_iterator cend() const noexcept { return mi(INVALID_NODE_ID); }

    public:
        zhash_map()
        {
            reset();
        }
        zhash_map(std::initializer_list<value_type> init)
        {
            reset();
            for (const auto& v : init)
            {
                insert(v);
            }
        }
        ~zhash_map()
        {
            if (!std::is_trivial<_Ty>::value)
            {
                for (reference kv : *this)
                {
                    kv.second.~_Ty();
                }
            }
        }
        void clear()
        {
            if (!std::is_trivial<_Ty>::value)
            {
                for (reference kv : *this)
                {
                    kv.second.~_Ty();
                }
            }
            reset();
        }
        const size_type size() const noexcept { return count_; }
        const bool empty() const noexcept { return !size(); }
        const bool full() const noexcept { return size() == NODE_COUNT; }
        size_type bucket_size(size_type bid)
        {
            return count_;
        }
        float load_factor() const
        {
            return size() / 1.0f / HASH_COUNT;
        }
        std::pair<iterator, bool> insert(const value_type& val)
        {
            return insert_v(val, false);
        }
        mapped_type& operator[](const key_type& key)
        {
            std::pair<iterator, bool> ret = insert_v(std::make_pair(key, mapped_type()), true);
            if (ret.first != end())
            {
                return ret.first->second;
            }
            throw std::overflow_error("mapped_type& operator[](const key_type& key)");
        }

        iterator find(const key_type& key)
        {
            u32 ukey = (u32)hasher(key);
            u32 hash_id = ukey % HASH_COUNT;
            u32 node_id = buckets_[hash_id];
            while (node_id != 0 && rf(node_pool_ [node_id]).first != key)
            {
                node_id = node_pool_[node_id].next;
            }
            if (node_id != 0)
            {
                return mi(node_id);
            }
            return end();
        }

        iterator erase(iterator iter)
        {
            u32 node_id = iter.cur_node_id_;
            if (node_id == 0 || node_id > exploit_offset_)
            {
                return end();
            }
            node_type& node = node_pool_[node_id];

            //return erase(rf(node).first);
            u32 hash_id = node.hash_id;
            
            if (hash_id >= HASH_COUNT)
            {
                return end();
            }
            if (buckets_[hash_id] == 0)
            {
                return end();
            }
            
            u32 pre_node_id = buckets_[hash_id];
            if (pre_node_id == node_id)
            {
                buckets_[hash_id] = node_pool_[node_id].next;
            }
            else
            {
                u32 cur_node_id = pre_node_id;
                while (node_pool_[cur_node_id].next != 0 && node_pool_[cur_node_id].next != node_id)
                {
                    cur_node_id = node_pool_[cur_node_id].next;
                }
                if (node_pool_[cur_node_id].next != node_id)
                {
                    return end();
                }
                node_pool_[cur_node_id].next = node_pool_[node_id].next;
            }

            if (!std::is_trivial<_Ty>::value)
            {
                rf(node).second.~_Ty();
            }
            push_free(node_id);
            return begin();
        }

        iterator erase(const key_type& key)
        {
            u32 ukey = (u32)hasher(key);
            u32 hash_id = ukey % HASH_COUNT;
            u32 pre_node_id = buckets_[hash_id];
            if (pre_node_id == 0)
            {
                return end();
            }

            u32 node_id = 0;
            if (rf(node_pool_[pre_node_id]).first == key)
            {
                node_id = pre_node_id;
                buckets_[hash_id] = node_pool_[pre_node_id].next;
            }
            else
            {
                u32 cur_node_id = pre_node_id;
                while (node_pool_[cur_node_id].next != 0 && rf(node_pool_[node_pool_[cur_node_id].next]).first != key)
                {
                    cur_node_id = node_pool_[cur_node_id].next;
                }
                if (rf(node_pool_[node_pool_[cur_node_id].next]).first != key)
                {
                    return end();
                }
                node_id = node_pool_[cur_node_id].next;
                node_pool_[cur_node_id].next = node_pool_[node_pool_[cur_node_id].next].next;
            }

            if (!std::is_trivial<_Ty>::value)
            {
                rf(node_pool_[node_id]).second.~_Ty();
            }
            push_free(node_id);
            return begin();
        }


    };

}


#endif
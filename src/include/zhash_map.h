
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

#if __GNUG__ && __GNUC__ < 5
#define IS_TRIVIALLY_COPYABLE(T) __has_trivial_copy(T)
#else
#define IS_TRIVIALLY_COPYABLE(T) std::is_trivially_copyable<T>::value
#endif


    template<class Bucket, class Key, class _Ty, u32 INVALID_POOL_SIZE, u32 HASH_COUNT>
    struct HashMapIterator
    {
        using value_type = std::pair<const Key, _Ty>;
        Bucket* pool_;
        u32 cur_obj_id_;
        u32 max_obj_id_; //迭代器使用过程中不更新 即迭代器一旦创建 不保证能迭代到新增元素;   

        operator Bucket* () const { return pool_[cur_obj_id_]; }
        operator const Bucket* ()const { return pool_[cur_obj_id_]; }
        HashMapIterator()
        {
            pool_ = NULL;
            cur_obj_id_ = INVALID_POOL_SIZE;
            max_obj_id_ = 0;
        }
        HashMapIterator(Bucket* pool,  u32 obj_id, u32 max_obj_id)
        {
            pool_ = pool;
            cur_obj_id_ = obj_id;
            max_obj_id_ = max_obj_id;
        }
        HashMapIterator(const HashMapIterator& other)
        {
            pool_ = other.pool_;
            cur_obj_id_ = other.cur_obj_id_;
            max_obj_id_ = other.max_obj_id_;
        }

        void next()
        {
            if (pool_ == NULL)
            {
                return;
            }

            for (u32 i = cur_obj_id_ + 1; i < max_obj_id_; i++)
            {
                if (pool_[i].hash_id < HASH_COUNT)
                {
                    cur_obj_id_ = i;
                    return;
                }
            }
            cur_obj_id_ = INVALID_POOL_SIZE;
            return;
        }

        HashMapIterator& operator ++()
        {
            next();
            return *this;
        }

        HashMapIterator operator ++(int)
        {
            HashMapIterator result(*this);
            next();
            return result;
        }

        value_type* operator ->()
        {
            return (value_type*)&pool_[cur_obj_id_].val_space;
        }
        value_type& operator *()
        {
            return *((value_type*)&pool_[cur_obj_id_].val_space);
        }
    };

    template<class Bucket, class Key, class _Ty, u32 INVALID_POOL_SIZE, u32 HASH_COUNT>
    bool operator == (const HashMapIterator<Bucket, Key, _Ty, INVALID_POOL_SIZE, HASH_COUNT>& n1, const HashMapIterator<Bucket, Key, _Ty, INVALID_POOL_SIZE, HASH_COUNT>& n2)
    {
        return n1.pool_ == n2.pool_ && n1.cur_obj_id_ == n2.cur_obj_id_;
    }
    template<class Bucket, class Key, class _Ty, u32 INVALID_POOL_SIZE, u32 HASH_COUNT>
    bool operator != (const HashMapIterator<Bucket, Key, _Ty, INVALID_POOL_SIZE, HASH_COUNT>& n1, const HashMapIterator<Bucket, Key, _Ty, INVALID_POOL_SIZE, HASH_COUNT>& n2)
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
        const static size_type POOL_SIZE = _Size;
        const static size_type INVALID_POOL_SIZE = POOL_SIZE + 1;
        const static size_type HASH_COUNT = POOL_SIZE * 2;
        constexpr size_type max_size() const { return POOL_SIZE; }
        //constexpr size_type max_bucket_count() const { return HASH_COUNT; }
        using key_type = Key;
        using mapped_type = _Ty;
        using value_type = std::pair<const key_type, mapped_type>;
        using reference = value_type&;
        using const_reference = const value_type&;
        using space_type = typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type;
        struct bucket
        {
            u32 next;
            u32 hash_id;
            space_type val_space;
        };
        using iterator = HashMapIterator<bucket, key_type, mapped_type, INVALID_POOL_SIZE, HASH_COUNT>;
        using const_iterator = const iterator;
        Hash hasher;
        KeyEqual key_equal;
    private:
        u32 buckets_[HASH_COUNT];
        bucket obj_pool_[INVALID_POOL_SIZE];
        u32 first_valid_;
        u32 exploit_offset_;
        u32 count_;
        iterator mi(u32 obj_id) { return iterator(obj_pool_, obj_id, exploit_offset_ + 1); }
        static reference rf(bucket& b) { return *reinterpret_cast<value_type*>(&b.val_space); }

        void reset()
        {
            exploit_offset_ = 0;
            count_ = 0;
            first_valid_ = 0;
            obj_pool_[FREE_POOL_SIZE].next = 0;
            obj_pool_[FREE_POOL_SIZE].hash_id = HASH_COUNT;
            memset(buckets_, 0, sizeof(u32) * HASH_COUNT);
        }

        u32 pop_free()
        {
            if (obj_pool_[FREE_POOL_SIZE].next != 0)
            {
                u32 ret = obj_pool_[FREE_POOL_SIZE].next;
                obj_pool_[ret].hash_id = HASH_COUNT;
                obj_pool_[FREE_POOL_SIZE].next = obj_pool_[ret].next;
                count_++;
                if (first_valid_ == 0 || ret < first_valid_)
                {
                    first_valid_ = ret;
                }
                return ret;
            }
            if (exploit_offset_ < POOL_SIZE)
            {
                u32 ret = ++exploit_offset_;
                obj_pool_[ret].hash_id = HASH_COUNT;
                count_++;
                if (first_valid_ == 0 || ret < first_valid_)
                {
                    first_valid_ = ret;
                }
                return ret;
            }
            return 0;
        }

        void push_free(u32 obj_id)
        {
            obj_pool_[obj_id].hash_id = HASH_COUNT;
            obj_pool_[obj_id].next = obj_pool_[FREE_POOL_SIZE].next;
            obj_pool_[FREE_POOL_SIZE].next = obj_id;
            count_--;
            if (obj_id == first_valid_)
            {
                first_valid_ = next_b(first_valid_+1).cur_obj_id_;
            }
        }

        iterator next_b(u32 obj_id)
        {
            for (u32 i = obj_id; i <= exploit_offset_; i++)
            {
                if (obj_pool_[i].hash_id != HASH_COUNT)
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

            u32 new_obj_id = pop_free();
            if (new_obj_id == 0)
            {
                return { end(), false };
            }
            bucket& obj = obj_pool_[new_obj_id];
            obj.next = 0;
            obj.hash_id = (u32)hash_id;
            if (!std::is_trivial<_Ty>::value)
            {
                new (&obj.val_space) value_type(val);
            }
            else
            {
                memcpy(&obj.val_space, &val, sizeof(val));
                //rf(obj) = val;
            }

            if (buckets_[hash_id] != 0)
            {
                obj.next = buckets_[hash_id];
            }
            buckets_[hash_id] = new_obj_id;
            return { mi(new_obj_id), true };
        }

    public:
        iterator begin() noexcept { return next_b(first_valid_); }
        const_iterator begin() const noexcept { return next_b(first_valid_); }
        const_iterator cbegin() const noexcept { return next_b(first_valid_); }

        iterator end() noexcept { return mi(INVALID_POOL_SIZE); }
        const_iterator end() const noexcept { return mi(INVALID_POOL_SIZE); }
        const_iterator cend() const noexcept { return mi(INVALID_POOL_SIZE); }

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
        const bool full() const noexcept { return size() == POOL_SIZE; }
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
            u32 obj_id = buckets_[hash_id];
            while (obj_id != 0 && rf(obj_pool_ [obj_id]).first != key)
            {
                obj_id = obj_pool_[obj_id].next;
            }
            if (obj_id != 0)
            {
                return mi(obj_id);
            }
            return end();
        }

        iterator erase(iterator iter)
        {
            u32 obj_id = iter.cur_obj_id_;
            if (obj_id == 0 || obj_id > exploit_offset_)
            {
                return end();
            }
            bucket& obj = obj_pool_[obj_id];

            //return erase(rf(obj).first);
            u32 hash_id = obj.hash_id;
            
            if (hash_id >= HASH_COUNT)
            {
                return end();
            }
            if (buckets_[hash_id] == 0)
            {
                return end();
            }
            
            u32 pre_obj_id = buckets_[hash_id];
            if (pre_obj_id == obj_id)
            {
                buckets_[hash_id] = obj_pool_[obj_id].next;
            }
            else
            {
                u32 cur_obj_id = pre_obj_id;
                while (obj_pool_[cur_obj_id].next != 0 && obj_pool_[cur_obj_id].next != obj_id)
                {
                    cur_obj_id = obj_pool_[cur_obj_id].next;
                }
                if (obj_pool_[cur_obj_id].next != obj_id)
                {
                    return end();
                }
                obj_pool_[cur_obj_id].next = obj_pool_[obj_id].next;
            }

            if (!std::is_trivial<_Ty>::value)
            {
                rf(obj).second.~_Ty();
            }
            push_free(obj_id);
            return begin();
        }

        iterator erase(const key_type& key)
        {
            u32 ukey = (u32)hasher(key);
            u32 hash_id = ukey % HASH_COUNT;
            u32 pre_obj_id = buckets_[hash_id];
            if (pre_obj_id == 0)
            {
                return end();
            }

            u32 obj_id = 0;
            if (rf(obj_pool_[pre_obj_id]).first == key)
            {
                obj_id = pre_obj_id;
                buckets_[hash_id] = obj_pool_[pre_obj_id].next;
            }
            else
            {
                u32 cur_obj_id = pre_obj_id;
                while (obj_pool_[cur_obj_id].next != 0 && rf(obj_pool_[obj_pool_[cur_obj_id].next]).first != key)
                {
                    cur_obj_id = obj_pool_[cur_obj_id].next;
                }
                if (rf(obj_pool_[obj_pool_[cur_obj_id].next]).first != key)
                {
                    return end();
                }
                obj_id = obj_pool_[cur_obj_id].next;
                obj_pool_[cur_obj_id].next = obj_pool_[obj_pool_[cur_obj_id].next].next;
            }

            if (!std::is_trivial<_Ty>::value)
            {
                rf(obj_pool_[obj_id]).second.~_Ty();
            }
            push_free(obj_id);
            return begin();
        }


    };

}


#endif
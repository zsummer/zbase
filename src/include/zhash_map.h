
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


    template<class Bucket, class Key, class _Ty, size_t _Buckets, size_t _Size>
    struct HashMapIterator
    {
        using value_type = std::pair<const Key, _Ty>;
        u32* buckets_;
        Bucket* pool_;
        u32 cur_hash_id_;
        u32 cur_obj_id_;

        operator Bucket* () const { return pool_[cur_obj_id_]; }
        operator const Bucket* ()const { return pool_[cur_obj_id_]; }
        HashMapIterator()
        {
            buckets_ = NULL;
            pool_ = NULL;
            cur_hash_id_ = 0;
            cur_obj_id_ = 0;
        }
        HashMapIterator(u32* buckets, Bucket* pool,  u32 hash_id, u32 obj_id)
        {
            buckets_ = buckets;
            pool_ = pool;
            cur_hash_id_ = hash_id;
            cur_obj_id_ = obj_id;
        }
        HashMapIterator(const HashMapIterator& other)
        {
            buckets_ = other.buckets_;
            pool_ = other.pool_;
            cur_hash_id_ = other.cur_hash_id_;
            cur_obj_id_ = other.cur_obj_id_;
        }

        void next()
        {
            if (buckets_ == NULL)
            {
                return;
            }
            if (cur_obj_id_ != 0 && pool_[cur_obj_id_].next != 0)
            {
                cur_obj_id_ = pool_[cur_obj_id_].next;
                return;
            }
            cur_obj_id_ = 0;
            for (u32 hash_id = cur_hash_id_ + 1; hash_id < _Buckets; hash_id++)
            {
                if (buckets_[hash_id] == 0)
                {
                    continue;
                }
                cur_hash_id_ = hash_id;
                cur_obj_id_ = buckets_[hash_id];
                return;
            }
            cur_hash_id_ = _Buckets;
            cur_obj_id_ = 0;
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

    template<class Bucket, class Key, class _Ty, size_t _Buckets, size_t _Size>
    bool operator == (const HashMapIterator<Bucket, Key, _Ty, _Buckets, _Size>& n1, const HashMapIterator<Bucket, Key, _Ty, _Buckets, _Size>& n2)
    {
        return n1.pool_ == n2.pool_ && n1.cur_obj_id_ == n2.cur_obj_id_;
    }
    template<class Bucket, class Key, class _Ty, size_t _Buckets, size_t _Size>
    bool operator != (const HashMapIterator<Bucket, Key, _Ty, _Buckets, _Size>& n1, const HashMapIterator<Bucket, Key, _Ty, _Buckets, _Size>& n2)
    {
        return !(n1 == n2);
    }

    template<class Key,
        class _Ty,
        size_t _Size,
        class Hash = std::hash<Key>,
        class KeyEqual = std::equal_to<Key>>
        class zhash_map
    {
    public:

        using size_type = size_t;
        const static size_type FREE_INDEX = 0;
        const static size_type BUCKET_COUNT = _Size;
        const static size_type HASH_COUNT = _Size * 2;
        constexpr size_type max_size() const { return _Size; }
        constexpr size_type max_bucket_count() const { return HASH_COUNT; }
        using key_type = Key;
        using mapped_type = _Ty;
        using value_type = std::pair<const key_type, mapped_type>;
        using reference = value_type&;
        using const_reference = const value_type&;
        using space_type = typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type;
        struct bucket
        {
            space_type val_space;
            u32 next;
        };
        using iterator = HashMapIterator<bucket, key_type, mapped_type, HASH_COUNT,  _Size + 1>;
        using const_iterator = const iterator;
        Hash hasher;
        KeyEqual key_equal;
    private:
        u32 buckets_[HASH_COUNT];
        bucket obj_pool_[BUCKET_COUNT+1];
        u32 exploit_offset_;
        size_t count_;
        iterator mi(u32 hash_id, u32 obj_id) { return iterator(buckets_, obj_pool_, hash_id, obj_id); }
        static reference rf(bucket& b) { return *reinterpret_cast<value_type*>(&b.val_space); }

        void reset()
        {
            exploit_offset_ = 0;
            count_ = 0;
            obj_pool_[FREE_INDEX].next = 0;
            memset(buckets_, 0, sizeof(u32) * HASH_COUNT);
        }

        u32 pop_free()
        {
            if (obj_pool_[FREE_INDEX].next != 0)
            {
                u32 ret = obj_pool_[FREE_INDEX].next;
                obj_pool_[FREE_INDEX].next = obj_pool_[ret].next;
                count_++;
                return ret;
            }
            if (exploit_offset_ < BUCKET_COUNT)
            {
                u32 ret = ++exploit_offset_;
                count_++;
                return ret;
            }
            return 0;
        }

        void push_free(u32 obj_id)
        {
            obj_pool_[obj_id].next = obj_pool_[FREE_INDEX].next;
            obj_pool_[FREE_INDEX].next = obj_id;
            count_--;
        }

        iterator next_b(u32 hash_id)
        {
            while (hash_id < HASH_COUNT && buckets_[hash_id] == 0)
            {
                hash_id++;
            }
            if (hash_id >= HASH_COUNT)
            {
                return mi(HASH_COUNT, 0);
            }
            return mi(hash_id, buckets_[hash_id]);
        }

        iterator erase_bucket(u32 hash_id, u32 obj_id)
        {
            if (obj_id == 0)
            {
                return end();
            }
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
                while (obj_pool_[cur_obj_id].next != obj_id && obj_pool_[cur_obj_id].next != 0)
                {
                    cur_obj_id = obj_pool_[cur_obj_id].next;
                }
                if (obj_pool_[cur_obj_id].next != obj_id)
                {
                    return end();
                }
                obj_pool_[cur_obj_id].next = obj_pool_[obj_id].next;
            }
            bucket& obj = obj_pool_[obj_id];
            if (std::is_object<_Ty>::value)
            {
                rf(obj).second.~_Ty();
            }

            u32 next_obj_id = obj.next;
            push_free(obj_id);
            if (next_obj_id != 0)
            {
                return mi(hash_id, next_obj_id);
            }
            return next_b(hash_id + 1);
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

            size_t ukey = hasher(val.first);
            size_t hash_id = ukey % HASH_COUNT;

            u32 new_obj_id = pop_free();
            if (new_obj_id == 0)
            {
                return { end(), false };
            }
            bucket& obj = obj_pool_[new_obj_id];
            obj.next = 0;
            if (std::is_object<_Ty>::value)
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
            return { mi((u32)hash_id, new_obj_id), true };
        }

    public:
        iterator begin() noexcept { return next_b(0); }
        const_iterator begin() const noexcept { return next_b(0); }
        const_iterator cbegin() const noexcept { return next_b(0); }

        iterator end() noexcept { return mi(HASH_COUNT, 0); }
        const_iterator end() const noexcept { return mi(HASH_COUNT, 0); }
        const_iterator cend() const noexcept { return mi(HASH_COUNT, 0); }

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
            clear();
        }
        void clear()
        {
            if (std::is_object<_Ty>::value)
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
        const bool full() const noexcept { return size() == BUCKET_COUNT; }
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
            size_t ukey = hasher(key);
            size_t hash_id = ukey % HASH_COUNT;
            u32 obj_id = buckets_[hash_id];
            while (obj_id != 0 && rf(obj_pool_ [obj_id]).first != key)
            {
                obj_id = obj_pool_[obj_id].next;
            }
            if (obj_id != 0)
            {
                return mi((u32)hash_id, obj_id);
            }
            return end();
        }

        iterator erase(iterator iter)
        {
            return erase_bucket(iter.cur_hash_id_, iter.cur_obj_id_);
        }

        iterator erase(const key_type& key)
        {
            return erase(find(key));
        }


    };

}


#endif
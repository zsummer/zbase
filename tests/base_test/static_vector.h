#pragma once

#include <assert.h>
#include <algorithm>
#include <memory>
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

// A vector like container but the storage(capacity) is fixed.
// This is the general part which has nothing to do with the capacity.
// It's for template reduction but add an extra capacity_ number.
template <typename T>
class StaticVectorTemplateCommon
{
public:
    using size_type = s32;
    using difference_type = ptrdiff_t;
    using value_type = T;
    using iterator = T *;
    using const_iterator = const T *;

    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using reference = T &;
    using const_reference = const T &;
    using pointer = T *;
    using const_pointer = const T *;

protected:
    StaticVectorTemplateCommon(T *data, s32 capacity)
        : data_ { data },
          capacity_ { capacity },
          len_ { 0 }
    {
    }

    StaticVectorTemplateCommon(const StaticVectorTemplateCommon&) = default;
    ~StaticVectorTemplateCommon() = default;

public:
    size_type size() const
    {
        return len_;
    }

    size_type capacity() const
    {
        return capacity_;
    }

    bool empty() const
    {
        return size() == 0;
    }

    bool full() const
    {
        return size() == capacity();
    }

public:
    // forward iterator creation methods.
    iterator begin()
    {
        return (iterator)this->data_;
    }
    const_iterator begin() const
    {
        return (const_iterator)this->data_;
    }
    iterator end()
    {
        return begin() + size();
    }
    const_iterator end() const
    {
        return begin() + size();
    }

    // reverse iterator creation methods.
    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }
    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }

    size_type size_in_bytes() const
    {
        return size() * sizeof(T);
    }

    size_t capacity_in_bytes() const
    {
        return capacity() * sizeof(T);
    }

    // Return a pointer to the buffer, even if empty().
    pointer data()
    {
        return pointer(begin());
    }
    // Return a pointer to the buffer, even if empty().
    const_pointer data() const
    {
        return const_pointer(begin());
    }

    reference operator[](size_type idx)
    {
        return begin()[idx];
    }
    const_reference operator[](size_type idx) const
    {
        return begin()[idx];
    }

    reference At(s32 i)
    {
        return data_[i];
    }

    const_reference At(s32 i) const
    {
        return data_[i];
    }

    reference front()
    {
        return begin()[0];
    }
    const_reference front() const
    {
        return begin()[0];
    }

    reference back()
    {
        return end()[-1];
    }
    const_reference back() const
    {
        return end()[-1];
    }

protected:
    void set_size(size_t n)
    {
        len_ = n;
    }

    T *data_;
    s32 capacity_;
    s32 len_;
};

// gcc-4.9.4 doesn't has std::is_trivially_copy_constructible
template <typename T, bool = std::is_trivial<T>::value &&
                             std::is_trivially_destructible<T>::value>
class StaticVectorTemplateBase : public StaticVectorTemplateCommon<T>
{
    using SuperClass = StaticVectorTemplateCommon<T>;

public:
    using iterator = typename SuperClass::iterator;
    using const_iterator = typename SuperClass::const_iterator;
    using reference = typename SuperClass::reference;
    using const_reference = typename SuperClass::const_reference;
    using size_type = typename SuperClass::size_type;

protected:
    using SuperClass::SuperClass;
    StaticVectorTemplateBase(const StaticVectorTemplateBase&) = default;
    ~StaticVectorTemplateBase() = default;

    static void destroy_range(T *s, T *e)
    {
        while (s != e)
        {
            --e;
            e->~T();
        }
    }

    // Move the range [s, e) onto the uninitialized memory starting with
    // "dest", constructing elements as needed.
    template <typename It1, typename It2>
    static void uninitialized_move(It1 s, It1 e, It2 dest)
    {
        std::uninitialized_copy(std::make_move_iterator(s),
                                std::make_move_iterator(e), dest);
    }

    // Copy the range [s, e) onto the uninitialized memory starting with
    // "dest", constructing elements as needed.
    template <typename It1, typename It2>
    static void uninitialized_copy(It1 s, It1 e, It2 dest)
    {
        std::uninitialized_copy(s, e, dest);
    }

public:
    reference reserve_back()
    {
        assert (this->size() < this->capacity());
        iterator it = this->end();

        ::new ((void*) it) T { };
        this->set_size(this->size() + 1);

        return *it;
    }

    void pop_back()
    {
        this->set_size(this->size() - 1);
        this->end()->~T();
    }
};

template <typename T>
class StaticVectorTemplateBase<T, true> : public StaticVectorTemplateCommon<T>
{
    using SuperClass = StaticVectorTemplateCommon<T>;

public:
    using iterator = typename SuperClass::iterator;
    using const_iterator = typename SuperClass::const_iterator;
    using reference = typename SuperClass::reference;
    using const_reference = typename SuperClass::const_reference;
    using size_type = typename SuperClass::size_type;

protected:
    using SuperClass::SuperClass;
    StaticVectorTemplateBase(const StaticVectorTemplateBase&) = default;
    ~StaticVectorTemplateBase() = default;

protected:
    static void destroy_range(T *, T *)
    {
    }

    // Move the range [s, e) onto the uninitialized memory
    // starting with "dest", constructing elements into it as needed.
    template <typename It1, typename It2>
    static void uninitialized_move(It1 s, It1 e, It2 dest)
    {
        // Just do a copy.
        uninitialized_copy(s, e, dest);
    }

    // Copy the range [s, e) onto the uninitialized memory
    // starting with "dest", constructing elements into it as needed.
    template <typename It1, typename It2>
    static void uninitialized_copy(It1 s, It1 e, It2 dest)
    {
        std::uninitialized_copy(s, e, dest);
    }

    /// Copy the range [s, e) onto the uninitialized memory
    /// starting with "dest", constructing elements into it as needed.
    template <typename T1, typename T2>
    static void uninitialized_copy(
        T1 *s, T1 *e, T2 *dest,
        std::enable_if_t<std::is_same<std::remove_const_t<T1>, T2>::value> * = nullptr)
    {
        // Use memcpy for PODs iterated by pointers (which includes StaticVector
        // iterators): std::uninitialized_copy optimizes to memmove, but we can
        // use memcpy here. Note that s and e are iterators and thus might be
        // invalid for memcpy if they are equal.
        if (s != e)
        {
            memcpy(reinterpret_cast<void *>(dest), s, (e - s) * sizeof(T));
        }
    }

public:
    void pop_back()
    {
        this->set_size(this->size() - 1);
    }
};

/// This class consists of common code factored out of the StaticVector class to
/// reduce code duplication based on the StaticVector 'NUM' template parameter.
template <typename T>
class StaticVectorImpl : public StaticVectorTemplateBase<T>
{
    using SuperClass = StaticVectorTemplateBase<T>;

public:
    using size_type = typename SuperClass::size_type;
    using iterator = typename SuperClass::iterator;
    using const_iterator = typename SuperClass::const_iterator;
    using reference = typename SuperClass::reference;
    using const_reference = typename SuperClass::const_reference;
    using reverse_iterator = typename SuperClass::reverse_iterator;
    using const_reverse_iterator = typename SuperClass::const_reverse_iterator;

public:
    using SuperClass::SuperClass;
    StaticVectorImpl(const StaticVectorImpl&) = default;
    ~StaticVectorImpl() = default;

    void clear()
    {
        this->destroy_range(this->begin(), this->end());
        this->set_size(0);
    }
    s32 push_back(const_reference elt) { return Add(elt); }
    s32 Add(const_reference elt)
    {
        if ((this->full()))
        {
            return -1;
        }
        std::uninitialized_fill(this->end(), this->end() + 1, elt);
        this->set_size(this->size() + 1);
        return 0;
    }
    
    iterator Add()
    {
        if (this->full())
        {
            return nullptr;
        }

        iterator ptr = this->end();
        ::new ((void*) ptr) T { };
        this->set_size(this->size() + 1);
        return ptr;
    }

    template <typename in_iter,
              typename = std::enable_if_t<std::is_convertible<
                  typename std::iterator_traits<in_iter>::iterator_category,
                  std::input_iterator_tag>::value>>
    void assign(in_iter in_start, in_iter in_end)
    {
        clear();
        append(in_start, in_end);
    }

    void resize(size_type n)
    {
        if (n < this->size())
        {
            this->destroy_range(this->begin() + n, this->end());
            this->set_size(n);
        }
        else if (n > this->size())
        {
            if (n > this->capacity())
            {
                n = this->capacity();
            }
            for (auto s = this->end(), e = this->begin() + n; s != e; ++s)
            {
                new (&*s) T();
            }
            this->set_size(n);
        }
    }

    void resize(size_type n, const_reference v)
    {
        if (n < this->size())
        {
            this->destroy_range(this->begin() + n, this->end());
            this->set_size(n);
        }
        else if (n > this->size())
        {
            if (this->capacity() < n)
            {
                n = this->capacity();
            }

            std::uninitialized_fill(this->end(), this->begin() + n, v);
            this->set_size(n);
        }
    }

    T pop_back_val()
    {
        T result = ::std::move(this->back());
        this->pop_back();
        return result;
    }

    void remove_no_order(iterator it)
    {
        if (it != this->end())
        {
            std::swap(*it, this->back());
        }
        this->pop_back();
    }

    bool remove(const_reference v)
    {
        auto it = find(v);
        if (it != this->end())
        {
            erase(it);
            return true;
        }
        return false;
    }

    s32 insert(iterator it, const_reference v)
    {
        if ((this->full()))
        {
            return -1;
        }

        if (it == this->end())
        {
            return this->Add(v);
        }

        ::new ((void *)this->end()) T(std::move(this->back()));
        std::move_backward(it, this->end() - 1, this->end());
        this->set_size(this->size() + 1);
        *it = v;

        return 0;
    }

    // For convenience only
    iterator find(const_reference t)
    {
        return std::find(this->begin(), this->end(), t);
    }

    const_iterator find(const_reference t) const
    {
        return std::find(this->begin(), this->end(), t);
    }

    reverse_iterator rfind(const_reference t)
    {
        return std::find(this->rbegin(), this->rend(), t);
    }

    const_reverse_iterator rfind(const_reference t) const
    {
        return std::find(this->rbegin(), this->rend(), t);
    }

    iterator erase(iterator s)
    {
        iterator it = std::move(s + 1, this->end(), s);
        this->destroy_range(it, this->end());
        this->set_size(it - this->begin());
        return s;
    }

    iterator erase(iterator s, iterator e)
    {
        iterator it = std::move(e, this->end(), s);
        this->destroy_range(it, this->end());
        this->set_size(it - this->begin());
        return s;
    }

    // The caller should ensure there's free space.
    template <typename... ArgTypes>
    reference emplace_back(ArgTypes &&...Args)
    {
        // assert(!this->full());
        ::new ((void *)this->end()) T { std::forward<ArgTypes>(Args)... };
        this->set_size(this->size() + 1);
        return this->back();
    }

    // Not sure if it's meaningful to customize these operators
    StaticVectorImpl &operator=(const StaticVectorImpl &rhs);
    StaticVectorImpl &operator=(StaticVectorImpl &&rhs);

    bool operator==(const StaticVectorImpl &rhs) const
    {
        if (this->size() != rhs.size())
        {
            return false;
        }

        return std::equal(this->begin(), this->end(), rhs.begin());
    }

    bool operator!=(const StaticVectorImpl &rhs) const
    {
        return !(*this == rhs);
    }

private:
    template <typename in_iter,
              typename = std::enable_if_t<std::is_convertible<
                  typename std::iterator_traits<in_iter>::iterator_category,
                  std::input_iterator_tag>::value> >
    void append(in_iter in_start, in_iter in_end)
    {
        size_type n = std::distance(in_start, in_end);
        size_type new_size = this->size() + n;
        if (new_size > this->capacity())
        {
            return;
        }

        this->uninitialized_copy(in_start, in_end, this->end());
        this->set_size(new_size);
    }
};

template <typename T>
StaticVectorImpl<T> &StaticVectorImpl<T>::operator=(const StaticVectorImpl<T> &rhs)
{
    // Avoid self-assignment.
    if (this == &rhs)
    {
        return *this;
    }

    s32 rhs_size = rhs.size();
    s32 cur_size = this->size();
    if (cur_size >= rhs_size)
    {
        // Assign common elements.
        iterator new_end;
        if (rhs_size)
        {
            new_end = std::copy(rhs.begin(), rhs.begin() + rhs_size, this->begin());
        }
        else
        {
            new_end = this->begin();
        }

        // Destroy excess elements.
        this->destroy_range(new_end, this->end());
        this->set_size(rhs_size);
        return *this;
    }

    if (cur_size)
    {
        // Otherwise, use assignment for the already-constructed elements.
        std::copy(rhs.begin(), rhs.begin() + cur_size, this->begin());
    }

    this->uninitialized_copy(rhs.begin() + cur_size, rhs.end(),
                             this->begin() + cur_size);
    this->set_size(rhs_size);
    return *this;
}

template <typename T>
StaticVectorImpl<T> &StaticVectorImpl<T>::operator=(StaticVectorImpl<T> &&rhs)
{
    if (this == &rhs)
    {
        return *this;
    }

    // If we already have sufficient space, assign the common elements, then
    // destroy any excess.
    s32 rhs_size = rhs.size();
    s32 cur_size = this->size();
    if (cur_size >= rhs_size)
    {
        // Assign common elements.
        iterator new_end = this->begin();
        if (rhs_size)
        {
            new_end = std::move(rhs.begin(), rhs.end(), new_end);
        }

        // Destroy excess elements and trim the bounds.
        this->destroy_range(new_end, this->end());
        this->set_size(rhs_size);

        rhs.clear();

        return *this;
    }

    if (cur_size != 0)
    {
        // Otherwise, use assignment for the already-constructed elements.
        std::move(rhs.begin(), rhs.begin() + cur_size, this->begin());
    }

    // Move-construct the new elements in place.
    this->uninitialized_move(rhs.begin() + cur_size, rhs.end(),
                             this->begin() + cur_size);

    this->set_size(rhs_size);

    rhs.clear();
    return *this;
}

template <typename T, s32 MaxSize, bool = std::is_trivial<T>::value &&
                             std::is_trivially_destructible<T>::value>
class StaticVector : public StaticVectorImpl<T>
{
public:
    StaticVector()
        : StaticVectorImpl<T>(eles_, MaxSize)
    {
    }

    ~StaticVector()
    {
        this->destroy_range(this->begin(), this->end());
    }

    StaticVector(const StaticVector &rhs)
        : StaticVectorImpl<T>(eles_, MaxSize)
    {
        if (!rhs.empty())
        {
            StaticVectorImpl<T>::operator=(rhs);
        }
    }

    const StaticVector &operator=(const StaticVector &rhs)
    {
        StaticVectorImpl<T>::operator=(rhs);
        return *this;
    }

    const StaticVector &operator=(StaticVector &&rhs)
    {
        StaticVectorImpl<T>::operator=(::std::move(rhs));
        return *this;
    }

private:
    T eles_[MaxSize];
};

template <typename T, s32 MaxSize>
class StaticVector<T, MaxSize, true> : public StaticVectorImpl<T>
{
public:
    StaticVector()
        : StaticVectorImpl<T>(eles_, MaxSize)
    {
    }

private:
    T eles_[MaxSize];
};


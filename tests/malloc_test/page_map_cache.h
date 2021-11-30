#pragma once

// A safe way of doing "(1 << n) - 1" -- without worrying about overflow
// Note this will all be resolved to a constant expression at compile-time
#define SAFE_N_ONES_(IntType, N)                                     \
  ( (N) == 0 ? 0 : ((static_cast<IntType>(1) << ((N)-1))-1 +    \
                    (static_cast<IntType>(1) << ((N)-1))) )


template<s32 KEY_BITS, typename T>
class PageMapCache
{
public:
	typedef u64 K;
	typedef u64 V;

	static const s32 kCacheHashBits = 16; // hash值对应的位数
	static const s32 kValueBits = 7; // 留给size class的位数
	static const s32 kTBits = 8 * sizeof(T); // T类型占用的Bits数量

	static const s32 kUpperBits = KEY_BITS;

	static const K kKeyMask = SAFE_N_ONES_(K, KEY_BITS);

	static const T kUpperMask = SAFE_N_ONES_(T, kUpperBits) << kValueBits;

	static const V kValueMask = SAFE_N_ONES_(V, kValueBits);

public:
	explicit PageMapCache(V init_value)
	{


		Clear(init_value);
	}

	s32 Put(K key, V value)
	{
		if (key == 0 || key != (key & kKeyMask))
		{
			error_tlog("invalid key.");
			return -1;
		}

		if (value != (value & kValueMask))
		{
			error_tlog("invalid value.");
			return -2;
		}

		u64 hash_value = Hash(key);
		array_[hash_value] = KeyToUpper(key) | value;

		return 0;
	}

	bool Has(K key) const
	{
		if (key == 0 || key != (key & kKeyMask))
		{
			error_tlog("invalid key.");
			return false;
		}

		u64 hash_value = Hash(key);
		bool is_match = IsKeyMatch(array_[hash_value], key);
		return is_match;
	}

	V GetOrDefault(K key, V default_value)
	{
		if (key == 0 || key != (key & kKeyMask))
		{
			error_tlog("invalid key.");
			return default_value;
		}

		u64 hash_value = Hash(key);
		bool is_match = IsKeyMatch(array_[hash_value], key);
		if (is_match)
		{
			V value = EntryToValue(array_[hash_value]);
			return value;
		}

		return default_value;
	}

	void Clear(V value)
	{
		if (value != (value & kValueMask))
		{
			error_tlog("invalid value.");
			return;
		}

		for (s32 i = 0; i < 1 << kCacheHashBits; ++i)
		{
			array_[i] = KeyToUpper(i) | value;
		}
	}

private:
	typedef T UPPER; // 一个key被分为2个部分，1部分是hash的索引,1-kCacheHashBits, 剩下的高位部分就是Upper
	V EntryToValue(T t) { return t & kValueMask; }
	inline UPPER KeyToUpper(K k)
	{
		return static_cast<T>(k) << kValueBits; // 直接左移value bits即可
	}
	inline u64 Hash(K key)
	{
		return static_cast<u64>(key & SAFE_N_ONES_(u64, kCacheHashBits));
	}
	bool IsKeyMatch(T entry, K key)
	{
		return ((entry >> kValueBits) == key);
	}

private:
	T array_[1 << kCacheHashBits]; // hash映射到的数组
};
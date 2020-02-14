#pragma once

#include <assert.h>
#include <string>
#include <vector>
#include <stack>
#include <list>
#include <map>
#include <unordered_map>
#include <memory>

namespace flame
{
	typedef char*				charptr;
	typedef wchar_t*			wcharptr;
	typedef unsigned char		uchar;
	typedef unsigned short		ushort;
	typedef unsigned int		uint;
	typedef unsigned long		ulong;
	typedef long long			longlong;
	typedef unsigned long long  ulonglong;
	typedef void*				voidptr;

	const auto INVALID_POINTER = (void*)0x7fffffffffffffff;

	template <class T>
	constexpr uint array_size(const T& a)
	{
		return sizeof(a) / sizeof(a[0]);
	}

	template <uint N>
	struct EnsureConstU
	{
		static const uint value = N;
	};

	template <class CH>
	struct _SAL // str and len
	{
		uint l;
		const CH* s;

		_SAL(uint l, const CH* s) :
			l(l),
			s(s)
		{
		}
	};

#define FLAME_SAL_S(x) EnsureConstU<__f_strlen(x)>::value, x
#define FLAME_SAL(n, x) _SAL n(FLAME_SAL_S(x))

	template <class CH>
	constexpr uint __f_strlen(const CH* str)
	{
		auto p = str;
		while (*p)
			p++;
		return p - str;
	}

	inline constexpr uint hash_update(uint h, uint v)
	{
		return h ^ (v + 0x9e3779b9 + (h << 6) + (h >> 2));
	}

	template <class CH>
	constexpr uint hash_str(const CH* str, uint seed)
	{
		return 0 == *str ? seed : hash_str(str + 1, hash_update(seed, *str));
	}

#define FLAME_HASH(x) (hash_str(x, 0))

#define FLAME_CHASH(x) (EnsureConstU<hash_str(x, 0)>::value)

	template <class F>
	void* f2v(F f) // function to void pointer
	{
		union
		{
			F f;
			void* p;
		}cvt;
		cvt.f = f;
		return cvt.p;
	}

	template <class F>
	F p2f(void* p) // void pointer to function
	{
		union
		{
			void* p;
			F f;
		}cvt;
		cvt.p = p;
		return cvt.f;
	}

	template <class F, class ...Args>
	auto cf(F f, Args... args) // call function
	{
		return (*f)(args...);
	}

	struct __f_Dummy
	{
	};
	typedef void(__f_Dummy::* MF_v_v)();
	typedef void(__f_Dummy::* MF_v_vp)(void*);
	typedef void(__f_Dummy::* MF_v_u)(uint);
	typedef void(__f_Dummy::* MF_v_vp_u)(void*, uint);
	typedef void* (__f_Dummy::* MF_vp_v)();
	typedef void* (__f_Dummy::* MF_vp_vp)(void*);
	typedef bool(__f_Dummy::* MF_b_v)();

	template <class F, class ...Args>
	auto cmf(F f, void* p, Args... args) // call member function at an address
	{
		return (*((__f_Dummy*)p).*f)(args...);
	}

	template <class T>
	typename std::enable_if<std::is_pod<T>::value, void*>::type cf2v() // ctor function to void pointer
	{
		return nullptr;
	}

	template <class T>
	typename std::enable_if<!std::is_pod<T>::value, void*>::type cf2v() // ctor function to void pointer
	{
		struct Wrap : T
		{
			void ctor()
			{
				new(this) T;
			}
		};
		return f2v(&Wrap::ctor);
	}

	template <class T>
	typename std::enable_if<std::is_pod<T>::value, void*>::type df2v() // dtor function to void pointer
	{
		return nullptr;
	}

	template <class T>
	typename std::enable_if<!std::is_pod<T>::value, void*>::type df2v() // dtor function to void pointer
	{
		struct Wrap : T
		{
			void dtor()
			{
				(*this).~Wrap();
			}
		};
		return f2v(&Wrap::dtor);
	}

	inline bool not_null_equal(void* a, void* b)
	{
		return a && a == b;
	}

	template <class T>
	std::pair<T*, uchar> only_not_null(T* a, T* b)
	{
		if (a && !b)
			return std::make_pair(a, 0);
		if (!a && b)
			return std::make_pair(b, 1);
		return std::make_pair(nullptr, 0);
	}

	template <class T>
	bool is_one_of(T v, const std::initializer_list<T>& l)
	{
		for (auto& i : l)
		{
			if (v == i)
				return true;
		}
		return false;
	}
}

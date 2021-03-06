#pragma once

#include <stdarg.h>
#include <assert.h>
#include <string>
#include <array>
#include <vector>
#include <span>
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
	typedef long long			int64;
	typedef unsigned long long  uint64;
	typedef void*				voidptr;

	const auto INVALID_POINTER = (void*)0x7fffffffffffffff;

	template <class T>
	constexpr uint array_size(const T& a)
	{
		return sizeof(a) / sizeof(a[0]);
	}


	template <class T>
	void* var_end(T* p)
	{
		return (char*)p + sizeof(T);
	}

	template <uint N>
	struct EnsureConstU
	{
		static const uint value = N;
	};

	template <class T>
	void erase_if(std::vector<T>& vec, T v)
	{
		auto it = std::find(vec.begin(), vec.end(), v);
		if (it != vec.end())
			vec.erase(it);
	}

	template <class T>
	void erase_if(std::vector<std::unique_ptr<T>>& vec, T* v)
	{
		auto it = std::find_if(vec.begin(), vec.end(), [&](const auto& t) {
			return t.get() == v;
		});
		if (it != vec.end())
			vec.erase(it);
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

	typedef void* (*F_vp_v)();

	template <class F, class ...Args>
	auto cf(F f, Args... args) // call function
	{
		return (*f)(args...);
	}

	struct __Dummy__
	{
	};
	typedef void(__Dummy__::* MF_v_v)();
	typedef void(__Dummy__::* MF_v_vp)(void*);
	typedef void(__Dummy__::* MF_v_vp_u)(void*, uint);
	typedef void(__Dummy__::* MF_v_b)(bool);
	typedef void(__Dummy__::* MF_v_i)(int);
	typedef void(__Dummy__::* MF_v_u)(uint);
	typedef void(__Dummy__::* MF_v_f)(float);
	typedef void(__Dummy__::* MF_v_c)(uchar);
	typedef void(__Dummy__::* MF_v_cp_i)(char*, int);
	typedef void(__Dummy__::* MF_v_wp_i)(wchar_t*, int);
	typedef void* (__Dummy__::* MF_vp_v)();
	typedef void* (__Dummy__::* MF_vp_vp)(void*);

	template <class F, class ...Args>
	auto cmf(F f, void* p, Args... args) // call member function at an address
	{
		return (*((__Dummy__*)p).*f)(args...);
	}

	struct Setter
	{
		void* o;
		void* f;

		virtual ~Setter() {}
		virtual void set(const void* v) = 0;
	};

	template <class T>
	struct Setter_t;

	template <>
	struct Setter_t<bool> : Setter
	{
		static void set_s(void* o, void* f, bool v)
		{
			cmf(p2f<MF_v_b>(f), o, v);
		}

		void set(const void* v) override
		{
			set_s(o, f, *(bool*)v);
		}
	};

	template <>
	struct Setter_t<int> : Setter
	{
		static void set_s(void* o, void* f, int v)
		{
			cmf(p2f<MF_v_i>(f), o, v);
		}

		void set(const void* v) override
		{
			set_s(o, f, *(int*)v);
		}
	};

	template <>
	struct Setter_t<uint> : Setter
	{
		static void set_s(void* o, void* f, uint v)
		{
			cmf(p2f<MF_v_u>(f), o, v);
		}

		void set(const void* v) override
		{
			set_s(o, f, *(uint*)v);
		}
	};

	template <>
	struct Setter_t<float> : Setter
	{
		static void set_s(void* o, void* f, float v)
		{
			cmf(p2f<MF_v_f>(f), o, v);
		}

		void set(const void* v) override
		{
			set_s(o, f, *(float*)v);
		}
	};

	template <>
	struct Setter_t<uchar> : Setter
	{
		static void set_s(void* o, void* f, uchar v)
		{
			cmf(p2f<MF_v_c>(f), o, v);
		}

		void set(const void* v) override
		{
			set_s(o, f, *(uchar*)v);
		}
	};

	struct CountDown
	{
		bool is_frame;
		union
		{
			uint frames;
			float time;
		}v;

		CountDown() :
			is_frame(true)
		{
			v.frames = 0;
		}

		CountDown(uint frames) :
			is_frame(true)
		{
			v.frames = frames;
		}

		CountDown(float time) :
			is_frame(false)
		{
			v.time = time;
		}
	};
}

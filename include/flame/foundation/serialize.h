#pragma once

#include <flame/foundation/foundation.h>

#include <pugixml.hpp>
#include <nlohmann/json.hpp>

namespace flame
{
	inline std::string to_string(int v)
	{
		char buf[20];
		sprintf(buf, "%d", v);
		return buf;
	}

	inline std::string to_string(uint v)
	{
		char buf[20];
		sprintf(buf, "%d", v);
		return buf;
	}

	inline std::string to_string(float v, int precision = 6)
	{
		char buf[20];
		sprintf(buf, "%.*f", precision, v);
		return buf;
	}

	inline std::string to_string(uchar v)
	{
		char buf[20];
		sprintf(buf, "%d", v);
		return buf;
	}

	template<uint N>
	inline std::string to_string(const Vec<N, uint>& v)
	{
		auto ret = to_string(v[0]);
		for (auto i = 1; i < N; i++)
			ret += ";" + to_string(v[i]);
		return ret;
	}

	template<uint N>
	inline std::string to_string(const Vec<N, int>& v)
	{
		auto ret = to_string(v[0]);
		for (auto i = 1; i < N; i++)
			ret += ";" + to_string(v[i]);
		return ret;
	}

	template<uint N>
	inline std::string to_string(const Vec<N, float>& v, int precision = 6)
	{
		auto ret = to_string(v[0], precision);
		for (auto i = 1; i < N; i++)
			ret += ";" + to_string(v[i], precision);
		return ret;
	}

	template<uint N>
	inline std::string to_string(const Vec<N, uchar>& v)
	{
		auto ret = to_string(v[0]);
		for (auto i = 1; i < N; i++)
			ret += ";" + to_string(v[i]);
		return ret;
	}

	inline std::wstring to_wstring(int v)
	{
		wchar_t buf[20];
		swprintf(buf, L"%d", v);
		return buf;
	}

	inline std::wstring to_wstring(uint v)
	{
		wchar_t buf[20];
		swprintf(buf, L"%d", v);
		return buf;
	}


	inline std::wstring to_wstring(float v, int precision = 6)
	{
		wchar_t buf[20];
		swprintf(buf, L"%.*f", precision, v);
		return buf;
	}

	inline std::wstring to_wstring(uchar v)
	{
		wchar_t buf[20];
		swprintf(buf, L"%d", v);
		return buf;
	}

	template<uint N>
	inline std::wstring to_wstring(const Vec<N, uint>& v)
	{
		auto ret = to_wstring(v[0]);
		for (auto i = 1; i < N; i++)
			ret += L";" + to_wstring(v[i]);
		return ret;
	}

	template<uint N>
	inline std::wstring to_wstring(const Vec<N, int>& v)
	{
		auto ret = to_wstring(v[0]);
		for (auto i = 1; i < N; i++)
			ret += L";" + to_wstring(v[i]);
		return ret;
	}

	template<uint N>
	inline std::wstring to_wstring(const Vec<N, float>& v, int precision = 6)
	{
		auto ret = to_wstring(v[0], precision);
		for (auto i = 1; i < N; i++)
			ret += L";" + to_wstring(v[i], precision);
		return ret;
	}

	template<uint N>
	inline std::wstring to_wstring(const Vec<N, uchar>& v)
	{
		auto ret = to_wstring(v[0]);
		for (auto i = 1; i < N; i++)
			ret += L";" + to_wstring(v[i]);
		return ret;
	}

	inline Vec2f stof2(const char* s)
	{
		Vec2f ret;
		sscanf(s, "%f;%f", &ret.x(), &ret.y());
		return ret;
	}

	inline Vec3f stof3(const char* s)
	{
		Vec3f ret;
		sscanf(s, "%f;%f;%f", &ret.x(), &ret.y(), &ret.z());
		return ret;
	}

	inline Vec4f stof4(const char* s)
	{
		Vec4f ret;
		sscanf(s, "%f;%f;%f;%f", &ret.x(), &ret.y(), &ret.z(), &ret.w());
		return ret;
	}

	inline Vec2u stou2(const char* s)
	{
		Vec2u ret;
		sscanf(s, "%u;%u", &ret.x(), &ret.y());
		return ret;
	}

	inline Vec3u stou3(const char* s)
	{
		Vec3u ret;
		sscanf(s, "%u;%u;%u", &ret.x(), &ret.y(), &ret.z());
		return ret;
	}

	inline Vec4u stou4(const char* s)
	{
		Vec4u ret;
		sscanf(s, "%u;%u;%u;%u", &ret.x(), &ret.y(), &ret.z(), &ret.w());
		return ret;
	}

	inline Vec2i stoi2(const char* s)
	{
		Vec2i ret;
		sscanf(s, "%d;%d", &ret.x(), &ret.y());
		return ret;
	}

	inline Vec3i stoi3(const char* s)
	{
		Vec3i ret;
		sscanf(s, "%d;%d;%d", &ret.x(), &ret.y(), &ret.z());
		return ret;
	}

	inline Vec4i stoi4(const char* s)
	{
		Vec4i ret;
		sscanf(s, "%d;%d;%d;%d", &ret.x(), &ret.y(), &ret.z(), &ret.w());
		return ret;
	}

	inline Vec2c stoc2(const char* s)
	{
		Vec2i ret;
		sscanf(s, "%d;%d", &ret.x(), &ret.y());
		return Vec2c(ret);
	}

	inline Vec3c stoc3(const char* s)
	{
		Vec3i ret;
		sscanf(s, "%d;%d;%d", &ret.x(), &ret.y(), &ret.z());
		return Vec3c(ret);
	}

	inline Vec4c stoc4(const char* s)
	{
		Vec4i ret;
		sscanf(s, "%d;%d;%d;%d", &ret.x(), &ret.y(), &ret.z(), &ret.w());
		return Vec4c(ret);
	}

	template<class T>
	T sto(const char* s); 

	template<>
	inline int sto<int>(const char* s)
	{
		return std::stoi(s);
	}

	template<>
	inline uint sto<uint>(const char* s)
	{
		return std::stoul(s);
	}

	template<>
	inline float sto<float>(const char* s)
	{
		return std::stof(s);
	}

	template<>
	inline uchar sto<uchar>(const char* s)
	{
		return std::stoul(s);
	}

	inline Vec2f stof2(const wchar_t* s)
	{
		Vec2f ret;
		swscanf(s, L"%f;%f", &ret.x(), &ret.y());
		return ret;
	}

	inline Vec3f stof3(const wchar_t* s)
	{
		Vec3f ret;
		swscanf(s, L"%f;%f;%f", &ret.x(), &ret.y(), &ret.z());
		return ret;
	}

	inline Vec4f stof4(const wchar_t* s)
	{
		Vec4f ret;
		swscanf(s, L"%f;%f;%f;%f", &ret.x(), &ret.y(), &ret.z(), &ret.w());
		return ret;
	}

	inline Vec2u stou2(const wchar_t* s)
	{
		Vec2u ret;
		swscanf(s, L"%u;%u", &ret.x(), &ret.y());
		return ret;
	}

	inline Vec3u stou3(const wchar_t* s)
	{
		Vec3u ret;
		swscanf(s, L"%u;%u;%u", &ret.x(), &ret.y(), &ret.z());
		return ret;
	}

	inline Vec4u stou4(const wchar_t* s)
	{
		Vec4u ret;
		swscanf(s, L"%u;%u;%u;%u", &ret.x(), &ret.y(), &ret.z(), &ret.w());
		return ret;
	}

	inline Vec2i stoi2(const wchar_t* s)
	{
		Vec2i ret;
		swscanf(s, L"%d;%d", &ret.x(), &ret.y());
		return ret;
	}

	inline Vec3i stoi3(const wchar_t* s)
	{
		Vec3i ret;
		swscanf(s, L"%d;%d;%d", &ret.x(), &ret.y(), &ret.z());
		return ret;
	}

	inline Vec4i stoi4(const wchar_t* s)
	{
		Vec4i ret;
		swscanf(s, L"%d;%d;%d;%d", &ret.x(), &ret.y(), &ret.z(), &ret.w());
		return ret;
	}

	inline Vec2c stoc2(const wchar_t* s)
	{
		Vec2i ret;
		swscanf(s, L"%d;%d", &ret.x(), &ret.y());
		return Vec2c(ret);
	}

	inline Vec3c stoc3(const wchar_t* s)
	{
		Vec3i ret;
		swscanf(s, L"%d;%d;%d", &ret.x(), &ret.y(), &ret.z());
		return Vec3c(ret);
	}

	inline Vec4c stoc4(const wchar_t* s)
	{
		Vec4i ret;
		swscanf(s, L"%d;%d;%d;%d", &ret.x(), &ret.y(), &ret.z(), &ret.w());
		return Vec4c(ret);
	}

	template<class T>
	T sto(const wchar_t* s);

	template<>
	inline int sto<int>(const wchar_t* s)
	{
		return std::stoi(s);
	}

	template<>
	inline uint sto<uint>(const wchar_t* s)
	{
		return std::stoul(s);
	}

	template<>
	inline float sto<float>(const wchar_t* s)
	{
		return std::stof(s);
	}

	template<>
	inline uchar sto<uchar>(const wchar_t* s)
	{
		return std::stoul(s);
	}

	enum TypeTag$
	{
		TypeEnumSingle,
		TypeEnumMulti,
		TypeData,
		TypePointer
	};

	inline char type_tag(TypeTag$ tag)
	{
		static char names[] = {
			'S',
			'M',
			'D',
			'P'
		};
		return names[tag];
	}

	struct EnumInfo;
	struct VariableInfo;
	struct FunctionInfo;
	struct UdtInfo;
	struct TypeinfoDatabase;

	typedef EnumInfo* EnumInfoPtr;
	typedef VariableInfo* VariableInfoPtr;
	typedef FunctionInfo* FunctionInfoPtr;
	typedef UdtInfo* UdtInfoPtr;
	typedef TypeinfoDatabase* TypeinfoDatabasePtr;

	// type name archive:
	// ， no space
	// ， 'unsigned ' will be replaced to 'u'
	// ， '< ' will be replaced to '('
	// ， '> ' will be replaced to ')'
	// ， ', ' will be replaced to '+'

	inline std::string tn_c2a(const std::string& name) // type name code to archive
	{
		auto ret = name;
		for (auto& ch : ret)
		{
			if (ch == '<')
				ch = '(';
			else if (ch == '>')
				ch = ')';
			else if (ch == ',')
				ch = '+';
		}
		return ret;
	}

	inline std::string tn_a2c(const std::string& name) // type name archive to code
	{
		auto ret = name;
		for (auto& ch : ret)
		{
			if (ch == '(')
				ch = '<';
			else if (ch == ')')
				ch = '>';
			else if (ch == '+')
				ch = ',';
		}
		return ret;
	}

	struct TypeInfo
	{
		FLAME_FOUNDATION_EXPORTS TypeTag$ tag() const;
		FLAME_FOUNDATION_EXPORTS bool is_attribute() const;
		FLAME_FOUNDATION_EXPORTS bool is_array() const;
		FLAME_FOUNDATION_EXPORTS const char* base_name() const;
		FLAME_FOUNDATION_EXPORTS const char* name() const; // tag[A][V]#base, order matters
		FLAME_FOUNDATION_EXPORTS uint base_hash() const;
		FLAME_FOUNDATION_EXPORTS uint hash() const;

		inline static uint get_hash(TypeTag$ tag, const std::string& base_name, bool is_attribute = false, bool is_array = false)
		{
			std::string name;
			name = type_tag(tag);
			if (is_attribute)
				name += "A";
			if (is_array)
				name += "V";
			name += "#" + base_name;
			return H(name.c_str());
		}

		FLAME_FOUNDATION_EXPORTS static const TypeInfo* get(TypeTag$ tag, const char* base_name, bool is_attribute = false, bool is_array = false);
		FLAME_FOUNDATION_EXPORTS static const TypeInfo* get(const char* str);

		inline std::string serialize(const std::vector<TypeinfoDatabase*>& dbs, const void* src, int precision) const;
		inline void unserialize(const std::vector<TypeinfoDatabase*>& dbs, const std::string& src, void* dst) const;
		inline void copy_from(const void* src, void* dst) const;
	};

	inline uint data_size(uint type_hash)
	{
		switch (type_hash)
		{
		case cH("bool"):
			return sizeof(bool);
		case cH("int"):
		case cH("uint"):
			return sizeof(int);
		case cH("Vec(1+int)"):
		case cH("Vec(1+uint)"):
			return sizeof(Vec1i);
		case cH("Vec(2+int)"):
		case cH("Vec(2+uint)"):
			return sizeof(Vec2i);
		case cH("Vec(3+int)"):
		case cH("Vec(3+uint)"):
			return sizeof(Vec3i);
		case cH("Vec(4+int)"):
		case cH("Vec(4+uint)"):
			return sizeof(Vec4i);
		case cH("longlong"):
		case cH("ulonglong"):
			return sizeof(longlong);
		case cH("float"):
			return sizeof(float);
		case cH("Vec(1+float)"):
			return sizeof(Vec1f);
		case cH("Vec(2+float)"):
			return sizeof(Vec2f);
		case cH("Vec(3+float)"):
			return sizeof(Vec3f);
		case cH("Vec(4+float)"):
			return sizeof(Vec4f);
		case cH("uchar"):
			return sizeof(uchar);
		case cH("Vec(1+uchar)"):
			return sizeof(Vec1c);
		case cH("Vec(2+uchar)"):
			return sizeof(Vec2c);
		case cH("Vec(3+uchar)"):
			return sizeof(Vec3c);
		case cH("Vec(4+uchar)"):
			return sizeof(Vec4c);
		case cH("StringA"):
			return sizeof(StringA);
		case cH("StringW"):
			return sizeof(StringW);
		default:
			assert(0);
		}
	}

	inline void data_copy(uint type_hash, const void* src, void* dst, uint size = 0)
	{
		switch (type_hash)
		{
		case cH("StringA"):
			*(StringA*)dst = *(StringA*)src;
			return;
		case cH("StringW"):
			*(StringW*)dst = *(StringW*)src;
			return;
		}

		memcpy(dst, src, size ? size : data_size(type_hash));
	}

	inline void data_dtor(uint type_hash, void* p)
	{
		switch (type_hash)
		{
		case cH("StringA"):
			((StringA*)p)->~String();
			return;
		case cH("StringW"):
			((StringW*)p)->~String();
			return;
		}
	}

	struct VariableInfo
	{
		FLAME_FOUNDATION_EXPORTS const TypeInfo* type() const;
		FLAME_FOUNDATION_EXPORTS const char* name() const;
		FLAME_FOUNDATION_EXPORTS uint name_hash() const;
		FLAME_FOUNDATION_EXPORTS const char* decoration() const;
		FLAME_FOUNDATION_EXPORTS uint offset() const;
		FLAME_FOUNDATION_EXPORTS uint size() const;
		FLAME_FOUNDATION_EXPORTS const char* default_value() const;
	};

	struct EnumItem
	{
		FLAME_FOUNDATION_EXPORTS const char* name() const;
		FLAME_FOUNDATION_EXPORTS int value() const;
	};

	struct EnumInfo
	{
		FLAME_FOUNDATION_EXPORTS TypeinfoDatabase* db() const;

		FLAME_FOUNDATION_EXPORTS const char* name() const;

		FLAME_FOUNDATION_EXPORTS uint item_count() const;
		FLAME_FOUNDATION_EXPORTS EnumItem* item(int idx) const;
		FLAME_FOUNDATION_EXPORTS EnumItem* find_item(const char* name, int *out_idx = nullptr) const;
		FLAME_FOUNDATION_EXPORTS EnumItem* find_item(int value, int* out_idx = nullptr) const;
		FLAME_FOUNDATION_EXPORTS EnumItem* add_item(const char* name, int value);
	};

	struct FunctionInfo
	{
		FLAME_FOUNDATION_EXPORTS TypeinfoDatabase* db() const;

		FLAME_FOUNDATION_EXPORTS const char* name() const;
		FLAME_FOUNDATION_EXPORTS void* rva() const;
		FLAME_FOUNDATION_EXPORTS const TypeInfo* return_type() const;

		FLAME_FOUNDATION_EXPORTS uint parameter_count() const;
		FLAME_FOUNDATION_EXPORTS const TypeInfo* parameter_type(uint idx) const;
		FLAME_FOUNDATION_EXPORTS void add_parameter(const TypeInfo* type);

	};

	struct UdtInfo
	{
		FLAME_FOUNDATION_EXPORTS TypeinfoDatabase* db() const;

		FLAME_FOUNDATION_EXPORTS const TypeInfo* type() const;
		FLAME_FOUNDATION_EXPORTS uint size() const;

		FLAME_FOUNDATION_EXPORTS uint variable_count() const;
		FLAME_FOUNDATION_EXPORTS VariableInfo* variable(uint idx) const;
		FLAME_FOUNDATION_EXPORTS VariableInfo* find_variable(const char* name, int *out_idx = nullptr) const;
		FLAME_FOUNDATION_EXPORTS VariableInfo* add_variable(const TypeInfo* type, const char* name, const char* decoration, uint offset, uint size);

		FLAME_FOUNDATION_EXPORTS uint function_count() const;
		FLAME_FOUNDATION_EXPORTS FunctionInfo* function(uint idx) const;
		FLAME_FOUNDATION_EXPORTS FunctionInfo* find_function(const char* name, int *out_idx = nullptr) const;
		FLAME_FOUNDATION_EXPORTS FunctionInfo* add_function(const char* name, void* rva, const TypeInfo* return_type);
	};

	/*
		something end with '$[a]' means it is reflectable
		the 'a' is called decoration, and is optional
		if first char of member name is '_', then the '_' will be ignored in reflection

		such as:
			struct Apple$ // mark this will be collected by typeinfogen
			{
				float size$; // mark this member will be collected
				Vec3f color$i; // mark this member will be collected, and its decoration is 'i'
				float _1$i; // mark this member will be collected, and reflected name is '1'
			};

		the decoration can be one or more chars, and order doesn't matter

		currently, the following attributes are used by typeinfogen, others are free to use:
			'm' for enum variable, means it can hold combination of the enum
			'c' for function, means to collect the code of the function
	*/

	struct TypeinfoDatabase
	{
		FLAME_FOUNDATION_EXPORTS const wchar_t* module_name() const;

		FLAME_FOUNDATION_EXPORTS Array<EnumInfo*> get_enums();
		FLAME_FOUNDATION_EXPORTS EnumInfo* find_enum(uint name_hash);
		FLAME_FOUNDATION_EXPORTS EnumInfo* add_enum(const char* name);

		FLAME_FOUNDATION_EXPORTS Array<FunctionInfo*> get_functions();
		FLAME_FOUNDATION_EXPORTS FunctionInfo* find_function(uint name_hash);
		FLAME_FOUNDATION_EXPORTS FunctionInfo* add_function(const char* name, void* rva, const TypeInfo* return_type);

		FLAME_FOUNDATION_EXPORTS Array<UdtInfo*> get_udts();
		FLAME_FOUNDATION_EXPORTS UdtInfo* find_udt(uint name_hash);
		FLAME_FOUNDATION_EXPORTS UdtInfo* add_udt(const TypeInfo* type, uint size);

		FLAME_FOUNDATION_EXPORTS static TypeinfoDatabase* collect(uint owned_dbs_count, TypeinfoDatabase** owned_dbs, const wchar_t* module_filename, const wchar_t* pdb_filename = nullptr);
		FLAME_FOUNDATION_EXPORTS static TypeinfoDatabase* load(uint owned_dbs_count, TypeinfoDatabase** owned_dbs, const wchar_t* typeinfo_filename);
		FLAME_FOUNDATION_EXPORTS static void save(uint owned_dbs_count, TypeinfoDatabase** owned_dbs, TypeinfoDatabase* db);
		FLAME_FOUNDATION_EXPORTS static void destroy(TypeinfoDatabase* db);
	};

	inline EnumInfo* find_enum(const std::vector<TypeinfoDatabase*>& dbs, uint name_hash)
	{
		for (auto db : dbs)
		{
			auto info = db->find_enum(name_hash);
			if (info)
				return info;
		}
		return nullptr;
	}

	inline UdtInfo* find_udt(const std::vector<TypeinfoDatabase*>& dbs, uint name_hash)
	{
		for (auto db : dbs)
		{
			auto info = db->find_udt(name_hash);
			if (info)
				return info;
		}
		return nullptr;
	}

	inline FunctionInfo* find_function(const std::vector<TypeinfoDatabase*>& dbs, uint name_hash)
	{
		for (auto db : dbs)
		{
			auto info = db->find_function(name_hash);
			if (info)
				return info;
		}
		return nullptr;
	}

	std::string TypeInfo::serialize(const std::vector<TypeinfoDatabase*>& dbs, const void* src, int precision) const
	{
		if (is_attribute())
			src = (char*)src + sizeof(AttributeBase);

		switch (tag())
		{
		case TypeEnumSingle:
		{
			auto e = find_enum(dbs, base_hash());
			assert(e);
			return e->find_item(*(int*)src)->name();
		}
		case TypeEnumMulti:
		{
			std::string str;
			auto e = find_enum(dbs, base_hash());
			assert(e);
			auto v = *(int*)src;
			for (auto i = 0; i < e->item_count(); i++)
			{
				if ((v & 1) == 1)
				{
					if (!str.empty())
						str += ";";
					str += e->find_item(1 << i)->name();
				}
				v >>= 1;
			}
			return str;
		}
		case TypeData:
			switch (base_hash())
			{
			case cH("bool"):
				return *(bool*)src ? "1" : "0";
			case cH("int"):
				return std::to_string(*(int*)src);
			case cH("Vec(1+int)"):
				return to_string(*(Vec1i*)src);
			case cH("Vec(2+int)"):
				return to_string(*(Vec2i*)src);
			case cH("Vec(3+int)"):
				return to_string(*(Vec3i*)src);
			case cH("Vec(4+int)"):
				return to_string(*(Vec4i*)src);
			case cH("uint"):
				return std::to_string(*(uint*)src);
			case cH("Vec(1+uint)"):
				return to_string(*(Vec1u*)src);
			case cH("Vec(2+uint)"):
				return to_string(*(Vec2u*)src);
			case cH("Vec(3+uint)"):
				return to_string(*(Vec3u*)src);
			case cH("Vec(4+uint)"):
				return to_string(*(Vec4u*)src);
			case cH("ulonglong"):
				return std::to_string(*(ulonglong*)src);
			case cH("float"):
				return to_string(*(float*)src, precision);
			case cH("Vec(1+float)"):
				return to_string(*(Vec1f*)src, precision);
			case cH("Vec(2+float)"):
				return to_string(*(Vec2f*)src, precision);
			case cH("Vec(3+float)"):
				return to_string(*(Vec3f*)src, precision);
			case cH("Vec(4+float)"):
				return to_string(*(Vec4f*)src, precision);
			case cH("uchar"):
				return std::to_string(*(uchar*)src);
			case cH("Vec(1+uchar)"):
				return to_string(*(Vec1c*)src);
			case cH("Vec(2+uchar)"):
				return to_string(*(Vec2c*)src);
			case cH("Vec(3+uchar)"):
				return to_string(*(Vec3c*)src);
			case cH("Vec(4+uchar)"):
				return to_string(*(Vec4c*)src);
			case cH("std::string"):
				return *(std::string*)src;
			case cH("std::wstring"):
				return w2s(*(std::wstring*)src);
			case cH("StringA"):
				return ((StringA*)src)->str();
			case cH("StringW"):
				return w2s(((StringW*)src)->str());
			default:
				assert(0);
			}
		}
	}

	void TypeInfo::unserialize(const std::vector<TypeinfoDatabase*>& dbs, const std::string& src, void* dst) const
	{
		if (is_attribute())
			dst = (char*)dst + sizeof(AttributeBase);

		switch (tag())
		{
		case TypeEnumSingle:
		{
			auto e = find_enum(dbs, base_hash());
			assert(e);
			e->find_item(src.c_str(), (int*)dst);
		}
			return;
		case TypeEnumMulti:
		{
			auto v = 0;
			auto e = find_enum(dbs, base_hash());
			assert(e);
			auto sp = ssplit(src, ';');
			for (auto& t : sp)
				v |= e->find_item(t.c_str())->value();
			*(int*)dst = v;
		}
			return;
		case TypeData:
			switch (base_hash())
			{
			case cH("bool"):
				*(bool*)dst = (src != "0");
				break;
			case cH("int"):
				*(int*)dst = std::stoi(src);
				break;
			case cH("Vec(1+int)"):
				*(Vec1u*)dst = std::stoi(src.c_str());
				break;
			case cH("Vec(2+int)"):
				*(Vec2u*)dst = stoi2(src.c_str());
				break;
			case cH("Vec(3+int)"):
				*(Vec3u*)dst = stoi3(src.c_str());
				break;
			case cH("Vec(4+int)"):
				*(Vec4u*)dst = stoi4(src.c_str());
				break;
			case cH("uint"):
				*(uint*)dst = std::stoul(src);
				break;
			case cH("Vec(1+uint)"):
				*(Vec1u*)dst = std::stoul(src.c_str());
				break;
			case cH("Vec(2+uint)"):
				*(Vec2u*)dst = stou2(src.c_str());
				break;
			case cH("Vec(3+uint)"):
				*(Vec3u*)dst = stou3(src.c_str());
				break;
			case cH("Vec(4+uint)"):
				*(Vec4u*)dst = stou4(src.c_str());
				break;
			case cH("ulonglong"):
				*(ulonglong*)dst = std::stoull(src);
				break;
			case cH("float"):
				*(float*)dst = std::stof(src.c_str());
				break;
			case cH("Vec(1+float)"):
				*(Vec1f*)dst = std::stof(src.c_str());
				break;
			case cH("Vec(2+float)"):
				*(Vec2f*)dst = stof2(src.c_str());
				break;
			case cH("Vec(3+float)"):
				*(Vec3f*)dst = stof3(src.c_str());
				break;
			case cH("Vec(4+float)"):
				*(Vec4f*)dst = stof4(src.c_str());
				break;
			case cH("uchar"):
				*(uchar*)dst = std::stoul(src);
				break;
			case cH("Vec(1+uchar)"):
				*(Vec1c*)dst = std::stoul(src.c_str());
				break;
			case cH("Vec(2+uchar)"):
				*(Vec2c*)dst = stoc2(src.c_str());
				break;
			case cH("Vec(3+uchar)"):
				*(Vec3c*)dst = stoc3(src.c_str());
				break;
			case cH("Vec(4+uchar)"):
				*(Vec4c*)dst = stoc4(src.c_str());
				break;
			case cH("StringA"):
				*(StringA*)dst = src;
				break;
			case cH("StringW"):
				*(StringW*)dst = s2w(src);
				break;
			default:
				assert(0);
			}
			return;
		}
	}

	void TypeInfo::copy_from(const void* src, void* dst) const
	{
		if (is_attribute())
			dst = (char*)dst + sizeof(AttributeBase);

		if (tag() == TypeData)
			data_copy(base_hash(), src, dst);
		else if (tag() == TypeEnumSingle || tag() == TypeEnumMulti)
			memcpy(dst, src, sizeof(int));
		else if (tag() == TypePointer)
			memcpy(dst, src, sizeof(void*));
	}
}


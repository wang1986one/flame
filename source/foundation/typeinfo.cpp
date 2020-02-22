#include <flame/serialize.h>
#include "typeinfo_private.h"

#include <Windows.h>

namespace flame
{
	TypeTag TypeInfo::tag() const
	{
		return ((TypeInfoPrivate*)this)->tag;
	}

	bool TypeInfo::is_array() const
	{
		return ((TypeInfoPrivate*)this)->is_array;
	}

	const char* TypeInfo::base_name() const
	{
		return ((TypeInfoPrivate*)this)->base_name.c_str();
	}

	const char* TypeInfo::name() const
	{
		return ((TypeInfoPrivate*)this)->name.c_str();
	}

	uint TypeInfo::base_hash() const
	{
		return ((TypeInfoPrivate*)this)->base_hash;
	}

	uint TypeInfo::hash() const
	{
		return ((TypeInfoPrivate*)this)->hash;
	}

	static std::vector<std::unique_ptr<TypeInfoPrivate>> type_infos;

	const TypeInfo* TypeInfo::get(TypeTag tag, const char* base_name, bool is_array)
	{
		for (auto& i : type_infos)
		{
			if (i->tag == tag && i->base_name == base_name && i->is_array == is_array)
				return i.get();
		}
		auto i = new TypeInfoPrivate(tag, base_name, is_array);
		type_infos.emplace_back(i);
		return i;
	}

	const TypeInfo* TypeInfo::get(const char* str)
	{
		TypeTag tag;
		std::string base_name;
		bool is_array;
		break_str(str, tag, base_name, is_array);
		return TypeInfo::get(tag, base_name.c_str(), is_array);
	}

	const TypeInfo* VariableInfo::type() const
	{
		return ((VariableInfoPrivate*)this)->type;
	}

	const char* VariableInfo::name() const
	{
		return ((VariableInfoPrivate*)this)->name.c_str();
	}

	uint VariableInfo::name_hash() const
	{
		return ((VariableInfoPrivate*)this)->name_hash;
	}

	uint VariableInfo::offset() const
	{
		return ((VariableInfoPrivate*)this)->offset;
	}

	uint VariableInfo::size() const
	{
		return ((VariableInfoPrivate*)this)->size;
	}

	uint VariableInfo::flags() const
	{
		return ((VariableInfoPrivate*)this)->flags;
	}

	const char* VariableInfo::default_value() const
	{
		return ((VariableInfoPrivate*)this)->default_value.c_str();
	}

	const char* EnumItem::name() const
	{
		return ((EnumItemPrivate*)this)->name.c_str();
	}

	int EnumItem::value() const
	{
		return ((EnumItemPrivate*)this)->value;
	}

	TypeinfoDatabase* EnumInfo::db() const
	{
		return ((EnumInfoPrivate*)this)->db;
	}

	const char* EnumInfo::name() const
	{
		return ((EnumInfoPrivate*)this)->name.c_str();
	}

	uint EnumInfo::item_count() const
	{
		return ((EnumInfoPrivate*)this)->items.size();
	}

	EnumItem* EnumInfo::item(int idx) const
	{
		return ((EnumInfoPrivate*)this)->items[idx].get();
	}

	EnumItem* EnumInfo::find_item(const char* _name, int* out_idx) const
	{
		auto name = std::string(_name);
		auto& items = ((EnumInfoPrivate*)this)->items;
		for (auto i = 0; i < items.size(); i++)
		{
			if (items[i]->name == name)
			{
				if (out_idx)
					*out_idx = items[i]->value;
				return items[i].get();
			}
		}
		if (out_idx)
			*out_idx = -1;
		return nullptr;
	}

	EnumItem* EnumInfo::find_item(int value, int* out_idx) const
	{
		auto& items = ((EnumInfoPrivate*)this)->items;
		for (auto i = 0; i < items.size(); i++)
		{
			if (items[i]->value == value)
			{
				if (out_idx)
					*out_idx = i;
				return items[i].get();
			}
		}
		if (out_idx)
			*out_idx = -1;
		return nullptr;
	}

	TypeinfoDatabase* FunctionInfo::db() const
	{
		return ((FunctionInfoPrivate*)this)->db;
	}

	const char* FunctionInfo::name() const
	{
		return ((FunctionInfoPrivate*)this)->name.c_str();
	}

	void* FunctionInfo::rva() const
	{
		return ((FunctionInfoPrivate*)this)->rva;
	}

	const TypeInfo* FunctionInfo::return_type() const
	{
		return ((FunctionInfoPrivate*)this)->return_type;
	}

	uint FunctionInfo::parameter_count() const
	{
		return ((FunctionInfoPrivate*)this)->parameter_types.size();
	}

	const TypeInfo* FunctionInfo::parameter_type(uint idx) const
	{
		return ((FunctionInfoPrivate*)this)->parameter_types[idx];
	}

	TypeinfoDatabase* UdtInfo::db() const
	{
		return ((UdtInfoPrivate*)this)->db;
	}

	const TypeInfo* UdtInfo::type() const
	{
		return ((UdtInfoPrivate*)this)->type;
	}

	uint UdtInfo::size() const
	{
		return ((UdtInfoPrivate*)this)->size;
	}

	uint UdtInfo::variable_count() const
	{
		return ((UdtInfoPrivate*)this)->variables.size();
	}

	VariableInfo* UdtInfo::variable(uint idx) const
	{
		return ((UdtInfoPrivate*)this)->variables[idx].get();
	}

	VariableInfo* UdtInfo::find_variable(const char* name, int* out_idx) const
	{
		return ((UdtInfoPrivate*)this)->find_variable(name, out_idx);
	}

	uint UdtInfo::function_count() const
	{
		return ((UdtInfoPrivate*)this)->functions.size();
	}

	FunctionInfo* UdtInfo::function(uint idx) const
	{
		return ((UdtInfoPrivate*)this)->functions[idx].get();
	}

	FunctionInfo* UdtInfo::find_function(const char* name, int* out_idx) const
	{
		return ((UdtInfoPrivate*)this)->find_function(name, out_idx);
	}

	static std::vector<TypeinfoDatabasePrivate*> global_dbs;
	uint extra_global_db_count;
	TypeinfoDatabase* const* extra_global_dbs;

	uint global_db_count()
	{
		return global_dbs.size();
	}

	TypeinfoDatabase* global_db(uint idx)
	{
		return global_dbs[idx];
	}

	void* TypeinfoDatabase::module() const
	{
		return ((TypeinfoDatabasePrivate*)this)->module;
	}

	const wchar_t* TypeinfoDatabase::module_name() const
	{
		return ((TypeinfoDatabasePrivate*)this)->module_name.c_str();
	}

	template <class T, class U>
	Array<T*> get_typeinfo_objects(const std::map<uint, std::unique_ptr<U>>& map)
	{
		auto ret = Array<T*>();
		ret.resize(map.size());
		auto i = 0;
		for (auto it = map.begin(); it != map.end(); it++)
		{
			ret[i] = it->second.get();
			i++;
		}
		return ret;
	}

	template <class T>
	T* find_typeinfo_object(const std::map<uint, std::unique_ptr<T>>& map, uint name_hash)
	{
		auto it = map.find(name_hash);
		if (it == map.end())
			return nullptr;
		return it->second.get();
	}

	Array<EnumInfo*> TypeinfoDatabase::get_enums()
	{
		return get_typeinfo_objects<EnumInfo>(((TypeinfoDatabasePrivate*)this)->enums);
	}

	EnumInfo* TypeinfoDatabase::find_enum(uint name_hash)
	{
		return find_typeinfo_object(((TypeinfoDatabasePrivate*)this)->enums, name_hash);
	}

	Array<UdtInfo*> TypeinfoDatabase::get_udts()
	{
		return get_typeinfo_objects<UdtInfo>(((TypeinfoDatabasePrivate*)this)->udts);
	}

	UdtInfo* TypeinfoDatabase::find_udt(uint name_hash)
	{
		return find_typeinfo_object(((TypeinfoDatabasePrivate*)this)->udts, name_hash);
	}

	TypeinfoDatabase* TypeinfoDatabase::load(const wchar_t* module_filename, bool add_to_global, bool load_with_module)
	{
		auto typeinfo_path = std::filesystem::path(module_filename);
		typeinfo_path.replace_extension(L".typeinfo");
		if (!std::filesystem::exists(typeinfo_path) || std::filesystem::last_write_time(typeinfo_path) < std::filesystem::last_write_time(module_filename))
		{
			auto typeinfogen_path = std::filesystem::path(getenv("FLAME_PATH"));
			typeinfogen_path = typeinfogen_path / L"bin/typeinfogen.exe";
			exec_and_redirect_to_std_output(nullptr, (wchar_t*)(typeinfogen_path.wstring() + L" " + module_filename).c_str());
		}

		pugi::xml_document file;
		pugi::xml_node file_root;
		if (!file.load_file(typeinfo_path.c_str()) || (file_root = file.first_child()).name() != std::string("typeinfo"))
		{
			assert(0);
			return nullptr;
		}

		auto db = new TypeinfoDatabasePrivate;
		db->module_name = module_filename;
		extra_global_db_count = 1;
		extra_global_dbs = (TypeinfoDatabase**)&db;

		for (auto n_enum : file_root.child("enums"))
		{
			auto e = db->add_enum(n_enum.attribute("name").value());

			for (auto n_item : n_enum.child("items"))
				e->add_item(n_item.attribute("name").value(), n_item.attribute("value").as_int());
		}

		for (auto n_udt : file_root.child("udts"))
		{
			auto u = db->add_udt(TypeInfo::get(n_udt.attribute("name").value()), n_udt.attribute("size").as_uint());

			for (auto n_variable : n_udt.child("variables"))
			{
				auto v = (VariableInfoPrivate*)u->add_variable(TypeInfo::get(n_variable.attribute("type").value()), n_variable.attribute("name").value(),
					n_variable.attribute("flags").as_uint(), n_variable.attribute("offset").as_uint(), n_variable.attribute("size").as_uint());
				v->default_value = n_variable.attribute("default_value").value();
			}

			for (auto n_function : n_udt.child("functions"))
			{
				auto f = u->add_function(n_function.attribute("name").value(), (void*)n_function.attribute("rva").as_uint(), TypeInfo::get(n_function.attribute("return_type").value()));
				for (auto n_parameter : n_function.child("parameters"))
					f->add_parameter(TypeInfo::get(n_parameter.attribute("type").value()));
			}
		}

		extra_global_db_count = 0;
		extra_global_dbs = nullptr;

		if (load_with_module)
			db->module = load_module(db->module_name.c_str());
		if (add_to_global)
			global_dbs.push_back(db);

		return db;
	}

	void TypeinfoDatabase::destroy(TypeinfoDatabase* db)
	{
		delete (TypeinfoDatabasePrivate*)db;
	}
}
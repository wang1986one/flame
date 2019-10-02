#pragma once

#include <flame/foundation/foundation.h>

namespace flame
{
	/*
		- A blueprint(BP) is a scene that represents relations between objects.
		- An object is called node and associated with an udt.
		- The reflected members with attribute 'i' or 'o' will be as inputs or outpus.
		- All inputs and outputs must be Attribute[*]<T> type.
		- The udt must have a update function, the function return nothing and takes no parameters
		- Address in BP: [node_id].[varible_name]
		  you can use address to find an object in BP, e.g.
		  'a'     for node
		  'a.b'   for node input or output
		- A BP file is basically a XML file
	*/

	struct SerializableNode;
	struct VariableInfo;
	struct UdtInfo;
	struct TypeinfoDatabase;

	namespace graphics
	{
		struct Device;
	}

	struct BP
	{
		struct Node;

		struct Module
		{
			FLAME_FOUNDATION_EXPORTS const std::wstring& filename() const;
			FLAME_FOUNDATION_EXPORTS void* module() const;
			FLAME_FOUNDATION_EXPORTS TypeinfoDatabase* db() const;
			Vec2f pos;

			void* user_data;
		};

		struct Import
		{
			FLAME_FOUNDATION_EXPORTS BP* bp() const;
			FLAME_FOUNDATION_EXPORTS const std::string& id() const;
			FLAME_FOUNDATION_EXPORTS void set_id(const std::string& id);
			Vec2f pos;

			void* user_data;
		};

		struct Slot
		{
			enum Type
			{
				Input,
				Output
			};

			FLAME_FOUNDATION_EXPORTS Type type() const;
			FLAME_FOUNDATION_EXPORTS Node* node() const;
			FLAME_FOUNDATION_EXPORTS VariableInfo* vi() const;

			FLAME_FOUNDATION_EXPORTS int frame() const;
			FLAME_FOUNDATION_EXPORTS void set_frame(int frame);
			FLAME_FOUNDATION_EXPORTS void* raw_data() const;
			FLAME_FOUNDATION_EXPORTS void* data() const;
			FLAME_FOUNDATION_EXPORTS void set_data(const void* data);

			void* data_p()
			{
				return *(void**)data();
			}

			void set_data_i(int i)
			{
				set_data(&i);
			}

			void set_data_p(const void* p)
			{
				set_data(&p);
			}

			FLAME_FOUNDATION_EXPORTS uint link_count() const;
			FLAME_FOUNDATION_EXPORTS Slot* link(int idx = 0) const;
			FLAME_FOUNDATION_EXPORTS bool link_to(Slot* target);

			FLAME_FOUNDATION_EXPORTS Mail<std::string> get_address() const;

			void* user_data;
		};

		struct Node
		{
			FLAME_FOUNDATION_EXPORTS BP* bp() const;
			FLAME_FOUNDATION_EXPORTS const std::string& id() const;
			FLAME_FOUNDATION_EXPORTS void set_id(const std::string& id);
			FLAME_FOUNDATION_EXPORTS UdtInfo* udt() const;
			Vec2f pos;

			FLAME_FOUNDATION_EXPORTS uint input_count() const;
			FLAME_FOUNDATION_EXPORTS Slot* input(uint idx) const;
			FLAME_FOUNDATION_EXPORTS uint output_count() const;
			FLAME_FOUNDATION_EXPORTS Slot* output(uint idx) const;

			FLAME_FOUNDATION_EXPORTS Slot* find_input(const std::string& name) const;
			FLAME_FOUNDATION_EXPORTS Slot* find_output(const std::string& name) const;

			void* user_data;
		};

		struct Export
		{
			FLAME_FOUNDATION_EXPORTS Slot* slot() const;
			FLAME_FOUNDATION_EXPORTS const std::string& alias() const;

			void* user_data;
		};

		struct Environment
		{
			std::wstring path;
			std::vector<TypeinfoDatabase*> dbs;
			graphics::Device* graphics_device;
		};

		graphics::Device* graphics_device;

		FLAME_FOUNDATION_EXPORTS uint module_count() const;
		FLAME_FOUNDATION_EXPORTS Module* module(uint idx) const;
		FLAME_FOUNDATION_EXPORTS Module* self_module() const;
		FLAME_FOUNDATION_EXPORTS Module* add_module(const std::wstring& filename);
		FLAME_FOUNDATION_EXPORTS void remove_module(Module* m);

		FLAME_FOUNDATION_EXPORTS uint impt_count() const;
		FLAME_FOUNDATION_EXPORTS Import* impt(uint idx) const;
		FLAME_FOUNDATION_EXPORTS Import* add_impt(const std::wstring& filename, const std::string& id);
		FLAME_FOUNDATION_EXPORTS void remove_impt(Import* e);
		FLAME_FOUNDATION_EXPORTS Import* find_impt(const std::string& id) const;

		FLAME_FOUNDATION_EXPORTS uint node_count() const;
		FLAME_FOUNDATION_EXPORTS Node* node(uint idx) const;
		FLAME_FOUNDATION_EXPORTS Node* add_node(const std::string& type_name, const std::string& id);
		FLAME_FOUNDATION_EXPORTS void remove_node(Node* n);
		FLAME_FOUNDATION_EXPORTS Node* find_node(const std::string& id) const;
		FLAME_FOUNDATION_EXPORTS Slot* find_input(const std::string& address) const;
		FLAME_FOUNDATION_EXPORTS Slot* find_output(const std::string& address) const;
		
		Vec2f expts_node_pos;
		FLAME_FOUNDATION_EXPORTS uint expt_count() const;
		FLAME_FOUNDATION_EXPORTS Export* expt(uint idx) const;
		FLAME_FOUNDATION_EXPORTS Export* add_expt(Slot* output, const std::string& alias);
		FLAME_FOUNDATION_EXPORTS void remove_expt(Export* e);
		FLAME_FOUNDATION_EXPORTS Export* find_expt(const std::string& alias) const;

		FLAME_FOUNDATION_EXPORTS void clear(); // all nodes and links

		FLAME_FOUNDATION_EXPORTS void update();

		FLAME_FOUNDATION_EXPORTS static BP* create();
		FLAME_FOUNDATION_EXPORTS static BP* create_from_file(const std::wstring& filename, bool no_compile = false);
		FLAME_FOUNDATION_EXPORTS static void save_to_file(BP* bp, const std::wstring& filename);
		FLAME_FOUNDATION_EXPORTS static void destroy(BP* bp);
	};

	FLAME_FOUNDATION_EXPORTS const BP::Environment& bp_env();
}


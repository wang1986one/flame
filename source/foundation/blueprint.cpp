// MIT License
// 
// Copyright (c) 2018 wjs
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <flame/foundation/serialize.h>
#include <flame/foundation/blueprint.h>

namespace flame
{
	struct BPPrivate;
	struct NodePrivate;
	struct InputPrivate;
	struct OutputPrivate;

	struct ItemPrivate : BP::Item
	{
		InputPrivate *parent_i;
		OutputPrivate *parent_o;
		CommonData data;

		ItemPrivate *link;

		inline ItemPrivate(InputPrivate *_parent_i, OutputPrivate *_parent_o);
		inline bool set_link(ItemPrivate *target);

		inline String get_address() const;
	};

	struct InputPrivate : BP::Input
	{
		NodePrivate *node;
		VariableInfo *variable_info;
		std::vector<std::unique_ptr<ItemPrivate>> items;

		inline bool is_array() { auto tag = variable_info->tag(); return tag == VariableTagArrayOfVariable || tag == VariableTagArrayOfPointer; }
		inline InputPrivate(NodePrivate *_node, VariableInfo *_variable_info);
		inline ItemPrivate *array_insert_item(int idx);
		inline void array_remove_item(int idx);
		inline void array_clear();

		inline int find_item(const ItemPrivate *item)
		{
			for (auto i = 0; i < items.size(); i++)
			{
				if (items[i].get() == item)
					return i;
			}
			return -1;
		}
	};

	struct OutputPrivate : BP::Output
	{
		NodePrivate *node;
		VariableInfo *variable_info;
		std::unique_ptr<ItemPrivate> item;

		inline OutputPrivate(NodePrivate *_node, VariableInfo *_variable_info);
	};

	struct NodePrivate : BP::Node
	{
		BPPrivate *bp;
		std::string id;
		UdtInfo *udt;
		std::vector<std::unique_ptr<InputPrivate>> inputs;
		std::vector<std::unique_ptr<OutputPrivate>> outputs;
		bool enable;

		bool updated;
		void* dummy; // represents the object

		inline NodePrivate(BPPrivate *_bp, const std::string &_id, UdtInfo *_udt);
		inline ~NodePrivate();

		inline void *_find_input_or_output(const std::string &name, int &input_or_output /* 0 or 1 */) const;
		inline InputPrivate* find_input(const std::string &name) const;
		inline OutputPrivate* find_output(const std::string &name) const;

		inline void report_order(); // use by BP's prepare update, report update order from dependencies
		inline void update();
	};

	struct BPPrivate : BP
	{
		std::wstring filename;

		std::vector<std::unique_ptr<NodePrivate>> nodes;
		std::vector<NodePrivate*> update_list;

		inline NodePrivate *add_node(const char *id, UdtInfo *udt);
		inline void remove_node(NodePrivate *n);

		inline NodePrivate *find_node(const std::string &id) const;
		inline InputPrivate *find_input(const std::string &address) const;
		inline OutputPrivate *find_output(const std::string &address) const;
		inline ItemPrivate *find_item(const std::string &address) const;

		inline void clear();

		inline void prepare();
		inline void unprepare();

		inline void update();

		inline void load(const wchar_t *filename);
		inline void save(const wchar_t *filename);
		inline void tobin();
	};

	ItemPrivate::ItemPrivate(InputPrivate *_parent_i, OutputPrivate *_parent_o) :
		parent_i(_parent_i),
		parent_o(_parent_o),
		link(nullptr)
	{
		data = (parent_i ? parent_i->variable_info : parent_o->variable_info)->default_value();
	}

	bool ItemPrivate::set_link(ItemPrivate *target)
	{
		if (!parent_i)
			return false;
		if (target && !typefmt_compare(data.fmt, target->data.fmt))
			return false;
		if (link)
			link->link = nullptr;
		link = target;
		return true;
	}

	String ItemPrivate::get_address() const
	{
		if (parent_i)
			return parent_i->node->id + "." + parent_i->variable_info->name() + "." + to_stdstring(parent_i->find_item(this));
		else if (parent_o)
			return parent_o->node->id + "." + parent_o->variable_info->name();
		return "";
	}

	InputPrivate::InputPrivate(NodePrivate *_node, VariableInfo *_variable_info) :
		node(_node),
		variable_info(_variable_info)
	{
		if (!is_array())
			/* so we need an item stands for the content */
			items.emplace_back(new ItemPrivate(this, nullptr));
	}

	ItemPrivate *InputPrivate::array_insert_item(int idx)
	{
		if (!is_array())
			return nullptr;
		auto item = new ItemPrivate(this, nullptr);
		items.emplace(items.begin() + idx, item);
		return item;
	}

	void InputPrivate::array_remove_item(int idx)
	{
		if (!is_array())
			return;
		items.erase(items.begin() + idx);
	}

	void InputPrivate::array_clear()
	{
		if (!is_array())
			return;
		items.clear();
	}

	OutputPrivate::OutputPrivate(NodePrivate *_node, VariableInfo *_variable_info) :
		node(_node),
		variable_info(_variable_info),
		item(new ItemPrivate(nullptr, this))
	{
	}

	NodePrivate::NodePrivate(BPPrivate *_bp, const std::string &_id, UdtInfo *_udt) :
		bp(_bp),
		id(_id),
		udt(_udt),
		enable(true),
		updated(false),
		dummy(nullptr)
	{
		for (auto i = 0; i < udt->item_count(); i++)
		{
			auto v = udt->item(i);
			auto attr = std::string(v->attribute());
			if (attr.find('i') != std::string::npos)
				inputs.emplace_back(new InputPrivate(this, v));
			if (attr.find('o') != std::string::npos)
				outputs.emplace_back(new OutputPrivate(this, v));
		}
	}

	NodePrivate::~NodePrivate()
	{
		for (auto &output : outputs)
		{
			auto link = output->item->link;
			if (link)
				link->link = nullptr;
		}
	}

	void *NodePrivate::_find_input_or_output(const std::string &name, int &input_or_output) const
	{
		auto udt_item_idx = udt->find_item_i(name.c_str());
		if (udt_item_idx < 0)
			return nullptr;
		auto udt_item = udt->item(udt_item_idx);
		auto udt_item_attribute = std::string(udt_item->attribute());
		if (udt_item_attribute.find('i') != std::string::npos)
		{
			for (auto &input : inputs)
			{
				if (name == input->variable_info->name())
				{
					input_or_output = 0;
					return input.get();
				}
			}
		}
		else if (udt_item_attribute.find('o') != std::string::npos)
		{
			for (auto &output : outputs)
			{
				if (name == output->variable_info->name())
				{
					input_or_output = 1;
					return output.get();
				}
			}
		}
	}

	InputPrivate* NodePrivate::find_input(const std::string &name) const
	{
		int input_or_output;
		auto ret = _find_input_or_output(name, input_or_output);
		return input_or_output == 0 ? (InputPrivate*)ret : nullptr;
	}

	OutputPrivate* NodePrivate::find_output(const std::string &name) const
	{
		int input_or_output;
		auto ret = _find_input_or_output(name, input_or_output);
		return input_or_output == 1 ? (OutputPrivate*)ret : nullptr;
	}

	void NodePrivate::report_order()
	{
		if (updated)
			return;

		for (auto &input : inputs)
		{
			for (auto &i : input->items)
			{
				auto link = i->link;
				if (link)
				{
					auto n = link->parent_o->node;
					if (!n->updated)
						n->report_order();
				}
			}
		}

		bp->update_list.push_back(this);

		updated = true;
	}

	void NodePrivate::update()
	{
		if (updated)
			return;

		for (auto &input : inputs)
		{
			auto v = input->variable_info;
			if (!input->is_array())
			{
				for (auto &i : input->items)
					v->set(i->link ? &i->link->data : &i->data, dummy, true, -1);
			}
			else
			{
				v->array_resize(input->items.size(), dummy, true);
				auto idx = 0;
				for (auto &i : input->items)
				{
					v->set(i->link ? &i->link->data : &i->data, dummy, true, idx);
					idx++;
				}
			}
		}

		auto update_function_rva = udt->update_function_rva();
		if (update_function_rva)
			run_module_function_member_void_void(udt->module_name(), update_function_rva, dummy);

		for (auto &output : outputs)
		{
			auto v = output->variable_info;
			auto i = output->item.get();
			v->get(dummy, true, -1, &i->data);
		}

		updated = true;
	}

	NodePrivate *BPPrivate::add_node(const char *id, UdtInfo *udt)
	{
		std::string s_id;
		if (id)
		{
			s_id = id;
			if (find_node(s_id))
				return nullptr;
		}
		else
		{
			for (auto i = 0; i < nodes.size() + 1; i++)
			{
				s_id = "node_" + to_stdstring(i);
				if (find_node(s_id))
					continue;
			}
		}
		auto n = new NodePrivate(this, s_id, udt);
		nodes.emplace_back(n);
		return n;
	}

	void BPPrivate::remove_node(NodePrivate *n)
	{
		for (auto it = nodes.begin(); it != nodes.end(); it++)
		{
			if ((*it).get() == n)
			{
				nodes.erase(it);
				return;
			}
		}
	}

	NodePrivate *BPPrivate::find_node(const std::string &id) const
	{
		for (auto &n : nodes)
		{
			if (n->id == id)
				return n.get();
		}
		return nullptr;
	}

	InputPrivate *BPPrivate::find_input(const std::string &address) const
	{
		auto sp = string_split(address, '.');
		auto n = find_node(sp[0]);
		if (!n)
			return nullptr;
		int input_or_output;
		auto ret = (InputPrivate*)n->_find_input_or_output(sp[1], input_or_output);
		return input_or_output == 0 ? ret : nullptr;
	}

	OutputPrivate *BPPrivate::find_output(const std::string &address) const
	{
		auto sp = string_split(address, '.');
		auto n = find_node(sp[0]);
		if (!n)
			return nullptr;
		int input_or_output;
		auto ret = (OutputPrivate*)n->_find_input_or_output(sp[1], input_or_output);
		return input_or_output == 1 ? ret : nullptr;
	}

	ItemPrivate *BPPrivate::find_item(const std::string &address) const
	{
		auto sp = string_split(address, '.');
		if (sp.size() < 2)
			return nullptr;
		uint index = 0;
		if (sp.size() >= 3)
			index = std::stoi(sp[2]);
		auto n = find_node(sp[0]);
		if (!n)
			return nullptr;
		int input_or_output;
		auto ret = n->_find_input_or_output(sp[1], input_or_output);
		if (input_or_output == 0)
		{
			auto input = (InputPrivate*)ret;
			return input->items[index].get();
		}
		else /*if (input_or_output == 1)*/
		{
			auto output = (OutputPrivate*)ret;
			return output->item.get();
		}

		return nullptr;
	}

	void BPPrivate::clear()
	{
		nodes.clear();
	}

	void BPPrivate::prepare()
	{
		for (auto &n : nodes)
		{
			if (!n->dummy)
			{
				auto udt = n->udt;
				auto size = udt->size();
				n->dummy = malloc(size);
				memset(n->dummy, 0, size);
				udt->construct(n->dummy);
			}
		}

		update_list.clear();
		for (auto &n : nodes)
			n->updated = false;
		for (auto &n : nodes)
			n->report_order();
	}

	void BPPrivate::unprepare()
	{
		for (auto &n : nodes)
		{
			if (n->dummy)
			{
				auto udt = n->udt;
				udt->destruct(n->dummy);
				free(n->dummy);
				n->dummy = nullptr;
			}
		}

		update_list.clear();
	}

	void BPPrivate::update()
	{
		if (update_list.empty())
		{
			printf("no nodes or didn't call 'prepare'\n");
			return;
		}

		for (auto &n : update_list)
			n->updated = false;

		for (auto &n : update_list)
			n->update();
	}

	void BPPrivate::tobin()
	{
		if (filename.empty())
		{
			printf("you need to do 'save' first\n");
			return;
		}
		if (update_list.empty())
		{
			printf("no nodes or didn't call 'prepare'\n");
			return;
		}

		std::string code;

		auto using_graphics_module = false;
		auto using_sound_module = false;
		auto using_universe_module = false;
		auto using_physics_module = false;

		for (auto& n : nodes)
		{
			auto module_name = std::wstring(n->udt->module_name());
			if (module_name == L"flame_graphics.dll")
				using_graphics_module = true;
			else if (module_name == L"flame_sound.dll")
				using_sound_module = true;
			else if (module_name == L"flame_universe.dll")
				using_universe_module = true;
			else if (module_name == L"flame_physics.dll")
				using_physics_module = true;
		}

		code += "#include <flame/foundation/foundation.h>\n";
		if (using_graphics_module)
			code += "#include <flame/graphics/all.h>\n";

		code += "\nusing namespace flame;\n\n";

		auto define_variable = [](const std::string &id_prefix, VariableInfo *v) {
			auto id = id_prefix + v->name();
			auto type = std::string(v->type_name());
			if (v->tag() == VariableTagPointer)
				type += "*";
			if (v->tag() == VariableTagArrayOfVariable || v->tag() == VariableTagArrayOfPointer)
				type = "Array<" + type + ">";
			return type + " " + id + ";\n";
		};

		auto set_variable_value = [](const std::string &id_prefix, VariableInfo *v, int item_index, const CommonData& data) {
			auto id = id_prefix + v->name();
			std::string value;
			switch (v->type_hash())
			{
			case cH("bool"):
				value = data.v.i[0] ? "true" : "false";
				break;
			case cH("uint"):
				value = to_stdstring((uint)data.v.i[0]);
				break;
			case cH("int"):
				value = to_stdstring(data.v.i[0]);
				break;
			case cH("Ivec2"):
				value = "Ivec2(" + to_stdstring(data.v.i[0]) + ", " + to_stdstring(data.v.i[1]) + ")";
				break;
			case cH("Ivec3"):
				value = "Ivec3(" + to_stdstring(data.v.i[0]) + ", " + to_stdstring(data.v.i[1]) + ", " + to_stdstring(data.v.i[2]) + ")";
				break;
			case cH("Ivec4"):
				value = "Ivec4(" + to_stdstring(data.v.i[0]) + ", " + to_stdstring(data.v.i[1]) + ", " + to_stdstring(data.v.i[2]) + ", " + to_stdstring(data.v.i[3]) + ")";
				break;
			case cH("float"):
				value = to_stdstring(data.v.f[0]);
				break;
			case cH("Vec2"):
				value = "Vec2(" + to_stdstring(data.v.f[0]) + ", " + to_stdstring(data.v.f[1]) + ")";
				break;
			case cH("Vec3"):
				value = "Vec3(" + to_stdstring(data.v.f[0]) + ", " + to_stdstring(data.v.f[1]) + ", " + to_stdstring(data.v.f[2]) + ")";
				break;
			case cH("Vec4"):
				value = "Vec4(" + to_stdstring(data.v.f[0]) + ", " + to_stdstring(data.v.f[1]) + ", " + to_stdstring(data.v.f[2]) + ", " + to_stdstring(data.v.f[3]) + ")";
				break;
			case cH("Bvec2"):
				value = "Bvec2(" + to_stdstring(data.v.b[0]) + ", " + to_stdstring(data.v.b[1]) + ")";
				break;
			case cH("Bvec3"):
				value = "Bvec3(" + to_stdstring(data.v.b[0]) + ", " + to_stdstring(data.v.b[1]) + ", " + to_stdstring(data.v.b[2]) + ")";
				break;
			case cH("Bvec4"):
				value = "Bvec4(" + to_stdstring(data.v.b[0]) + ", " + to_stdstring(data.v.b[1]) + ", " + to_stdstring(data.v.b[2]) + ", " + to_stdstring(data.v.b[3]) + ")";
				break;
			case cH("void"):
				value = "nullptr";
				break;
			}
			if (v->tag() == VariableTagArrayOfVariable || v->tag() == VariableTagArrayOfPointer)
				id += "[" + to_stdstring(item_index) + "]";
			return id + " = " + value + ";\n";
		};

		for (auto& n : update_list)
		{
			auto id_prefix = n->id + "_";
			for (auto& input : n->inputs)
				code += define_variable(id_prefix, input->variable_info);
			for (auto& output : n->outputs)
				code += define_variable(id_prefix, output->variable_info);
		}

		code += "\n __declspec(dllexport) void initialize()\n{\n";
		for (auto& n : update_list)
		{
			auto id_prefix = n->id + "_";
			for (auto& input : n->inputs)
			{
				auto v = input->variable_info;
				if (v->tag() == VariableTagArrayOfVariable || v->tag() == VariableTagArrayOfPointer)
					code += "\t" + id_prefix + v->name() + ".resize("+ to_stdstring((int)input->items.size()) + ");\n";
				auto idx = 0;
				for (auto& i : input->items)
				{
					code += "\t" + set_variable_value(id_prefix, v, idx, i->data);
					idx++;
				}
			}
			for (auto& output : n->outputs)
				code += "\t" + set_variable_value(id_prefix, output->variable_info, -1, output->item->data);
		}
		code += "}\n";

		code += "\n __declspec(dllexport) void update()\n{\n";
		std::regex reg_nl(R"(\bNL\b)");
		std::regex reg_tab(R"(\bTAB\b)");
		std::regex reg_variable(R"(\b([\w]+)\$\w*\b)");
		for (auto& n : update_list)
		{
			auto id_prefix = n->id + "_";
			auto udt = n->udt;
			auto code_function_rva = udt->code_function_rva();
			auto fmt_id = id_prefix + "$1";

			for (auto& input : n->inputs)
			{
				auto v = input->variable_info;
				auto idx = 0;
				for (auto& i : input->items)
				{
					auto link = i->link;
					if (link)
					{
						auto dst_id = id_prefix + v->name();
						if (v->tag() == VariableTagArrayOfVariable || v->tag() == VariableTagArrayOfPointer)
							dst_id += "[" + to_stdstring(idx) + "]";
						auto output = link->parent_o;
						code += "\t" + dst_id + " = " + output->node->id + "_" + output->variable_info->name() + ";\n";
					}
					idx++;
				}
			}

			if (code_function_rva)
			{
				auto str = std::string(run_module_function_member_constcharp_void(udt->module_name(), code_function_rva, nullptr).v);
				str = std::regex_replace(str, reg_nl, "\n");
				str = std::regex_replace(str, reg_tab, "\t");
				str = std::regex_replace(str, reg_variable, fmt_id);
				code += str + "\n";
			}
			code += "\n";
		}
		code += "}\n";

		code += "\n";

		auto cpp_filename = ext_replace(filename, L".cpp");
		std::ofstream file(cpp_filename);
		file << code;
		file.close();

		std::vector<std::wstring> libraries;
		libraries.push_back(L"flame_foundation.lib");
		if (using_graphics_module)
			libraries.push_back(L"flame_graphics.lib");
		if (using_sound_module)
			libraries.push_back(L"flame_sound.lib");
		if (using_universe_module)
			libraries.push_back(L"flame_universe.lib");
		if (using_physics_module)
			libraries.push_back(L"flame_physics.lib");

		auto output = compile_to_dll({ cpp_filename }, libraries, ext_replace(filename, L".dll"));

		printf("compiling:\n%s\n\n", output.v);
	}
	
	void BPPrivate::load(const wchar_t *_filename)
	{
		filename = _filename;

		auto file = SerializableNode::create_from_xml(filename);
		if (!file)
			return;

		for (auto i_n = 0; i_n < file->node_count(); i_n++)
		{
			auto n_node = file->node(i_n);
			if (n_node->name() == "node")
			{
				auto type = n_node->find_attr("type")->value();
				auto id = n_node->find_attr("id")->value();

				auto udt = find_udt(H(type.c_str()));
				if (!udt)
					continue;
				auto n = new NodePrivate(this, id, udt);

				for (auto i_i = 0; i_i < n_node->node_count(); i_i++)
				{
					auto n_input = n_node->node(i_i);
					if (n_input->name() == "input")
					{
						auto name = n_input->find_attr("name")->value();
						auto udt_item_idx = udt->find_item_i(name.c_str());
						if (udt_item_idx >= 0)
						{
							auto udt_item = udt->item(udt_item_idx);
							if (udt_item->tag() != VariableTagPointer && udt_item->tag() != VariableTagArrayOfPointer)
							{
								if (std::string(udt_item->attribute()).find('i') != std::string::npos)
								{
									for (auto &input : n->inputs)
									{
										if (name == input->variable_info->name())
										{
											input->items.clear();
											for (auto i_v = 0; i_v < n_input->node_count(); i_v++)
											{
												auto n_item = n_input->node(i_v);
												if (n_item->name() == "item")
												{
													auto v = new ItemPrivate(input.get(), nullptr);
													input->variable_info->unserialize_value(n_item->find_attr("value")->value(), &v->data.v, false, -1);
													input->items.emplace_back(v);
												}
											}
											break;
										}
									}
								}
							}
						}
					}
				}

				nodes.emplace_back(n);
			}
			else if (n_node->name() == "link")
			{
				auto i = find_item(n_node->find_attr("in")->value());
				auto o = find_item(n_node->find_attr("out")->value());
				if (o && i)
					i->set_link(o);
			}
		}

		SerializableNode::destroy(file);
	}

	void BPPrivate::save(const wchar_t *_filename)
	{
		filename = _filename;

		auto file = SerializableNode::create("BP");

		for (auto &n : nodes)
		{
			auto n_node = file->new_node("node");
			n_node->new_attr("type", n->udt->name());
			n_node->new_attr("id", n->id);
			for (auto &input : n->inputs)
			{
				auto v = input->variable_info;
				if (v->tag() != VariableTagPointer && v->tag() != VariableTagArrayOfPointer)
				{
					auto n_input = n_node->new_node("input");
					n_input->new_attr("name", input->variable_info->name());
					for (auto &i : input->items)
					{
						auto n_item = n_input->new_node("item");
						n_item->new_attr("value", input->variable_info->serialize_value(&i->data.v, false, -1, 2).v);
					}
				}
			}
		}
		for (auto &n : nodes)
		{
			for (auto &input : n->inputs)
			{
				auto idx = 0;
				for (auto &i : input->items)
				{
					auto o = i->link;
					if (o)
					{
						auto n_link = file->new_node("link");
						n_link->new_attr("in", i->get_address().v);
						n_link->new_attr("out", o->get_address().v);
					}
					idx++;
				}
			}
		}

		file->save_xml(filename);
		SerializableNode::destroy(file);
	}

	BP::Input *BP::Item::parent_i() const
	{
		return ((ItemPrivate*)this)->parent_i;
	}

	BP::Output *BP::Item::parent_o() const
	{
		return ((ItemPrivate*)this)->parent_o;
	}

	CommonData &BP::Item::data()
	{
		return ((ItemPrivate*)this)->data;
	}

	void BP::Item::set_data(const CommonData &d)
	{
		((ItemPrivate*)this)->data = d;
	}

	BP::Item *BP::Item::link() const
	{
		return ((ItemPrivate*)this)->link;
	}

	bool BP::Item::set_link(BP::Item *target)
	{
		return ((ItemPrivate*)this)->set_link((ItemPrivate*)target);
	}

	String BP::Item::get_address() const
	{
		return ((ItemPrivate*)this)->get_address();
	}

	BP::Node *BP::Input::node() const
	{
		return ((InputPrivate*)this)->node;
	}

	VariableInfo *BP::Input::variable_info() const
	{
		return ((InputPrivate*)this)->variable_info;
	}

	int BP::Input::array_item_count() const
	{
		return ((InputPrivate*)this)->items.size();
	}

	BP::Item *BP::Input::array_item(int idx) const
	{
		return ((InputPrivate*)this)->items[idx].get();
	}

	BP::Item *BP::Input::array_insert_item(int idx)
	{
		return ((InputPrivate*)this)->array_insert_item(idx);
	}

	void BP::Input::array_remove_item(int idx)
	{
		((InputPrivate*)this)->array_remove_item(idx);
	}

	void BP::Input::array_clear() const
	{
		((InputPrivate*)this)->array_clear();
	}

	BP::Node *BP::Output::node() const
	{
		return ((OutputPrivate*)this)->node;
	}

	VariableInfo *BP::Output::variable_info() const
	{
		return ((OutputPrivate*)this)->variable_info;
	}

	BP::Item *BP::Output::item() const
	{
		return ((OutputPrivate*)this)->item.get();
	}

	BP *BP::Node::bp() const
	{
		return ((NodePrivate*)this)->bp;
	}

	const char *BP::Node::id() const
	{
		return ((NodePrivate*)this)->id.c_str();
	}

	UdtInfo *BP::Node::udt() const
	{
		return ((NodePrivate*)this)->udt;
	}

	int BP::Node::input_count() const
	{
		return ((NodePrivate*)this)->inputs.size();
	}

	BP::Input *BP::Node::input(int idx) const
	{
		return ((NodePrivate*)this)->inputs[idx].get();
	}

	int BP::Node::output_count() const
	{
		return ((NodePrivate*)this)->outputs.size();
	}

	BP::Output *BP::Node::output(int idx) const
	{
		return ((NodePrivate*)this)->outputs[idx].get();
	}

	BP::Input *BP::Node::find_input(const char *name) const
	{
		return ((NodePrivate*)this)->find_input(name);
	}

	BP::Output *BP::Node::find_output(const char *name) const
	{
		return ((NodePrivate*)this)->find_output(name);
	}

	bool BP::Node::enable() const
	{
		return ((NodePrivate*)this)->enable;
	}

	void BP::Node::set_enable(bool enable) const
	{
		((NodePrivate*)this)->enable = enable;
	}

	int BP::node_count() const
	{
		return ((BPPrivate*)this)->nodes.size();
	}

	BP::Node *BP::node(int idx) const
	{
		return ((BPPrivate*)this)->nodes[idx].get();
	}

	BP::Node *BP::add_node(const char *id, UdtInfo *udt)
	{
		return ((BPPrivate*)this)->add_node(id, udt);
	}

	void BP::remove_node(BP::Node *n)
	{
		((BPPrivate*)this)->remove_node((NodePrivate*)n);
	}

	BP::Node *BP::find_node(const char *id) const
	{
		return ((BPPrivate*)this)->find_node(id);
	}

	BP::Input *BP::find_input(const char *address) const
	{
		return ((BPPrivate*)this)->find_input(address);
	}

	BP::Output *BP::find_output(const char *address) const
	{
		return ((BPPrivate*)this)->find_output(address);
	}

	BP::Item *BP::find_item(const char *adress) const
	{
		return ((BPPrivate*)this)->find_item(adress);
	}

	void BP::clear()
	{
		((BPPrivate*)this)->clear();
	}

	void BP::prepare()
	{
		((BPPrivate*)this)->prepare();
	}

	void BP::unprepare()
	{
		((BPPrivate*)this)->unprepare();
	}

	void BP::update()
	{
		((BPPrivate*)this)->update();
	}

	void BP::tobin()
	{
		((BPPrivate*)this)->tobin();
	}

	void BP::save(const wchar_t *filename)
	{
		((BPPrivate*)this)->save(filename);
	}

	BP *BP::create()
	{
		return new BPPrivate();
	}

	BP *BP::create_from_file(const wchar_t *filename)
	{
		if (!std::filesystem::exists(filename))
			return nullptr;

		auto bp = new BPPrivate();
		bp->load(filename);
		return bp;
	}

	void BP::destroy(BP *bp)
	{
		delete(BPPrivate*)bp;
	}

#define CODE_vec1 \
			v$o = v$i;
#define CODE_vec2 \
			v$o.x = x$i; NL\
			v$o.y = y$i;
#define CODE_vec3 \
			v$o.x = x$i; NL\
			v$o.y = y$i; NL\
			v$o.z = z$i;
#define CODE_vec4 \
			v$o.x = x$i; NL\
			v$o.y = y$i; NL\
			v$o.z = z$i; NL\
			v$o.w = w$i;

	void BP_Int::update()
	{
		CODE_vec1
	}

	const char* BP_Int::code()
	{
		return FLAME_STR(CODE_vec1);
	}

	BP_Int bp_int_unused;

	void BP_Float::update()
	{
		CODE_vec1
	}

	const char* BP_Float::code()
	{
		return FLAME_STR(CODE_vec1);
	}

	BP_Float bp_float_unused;

	void BP_Bool::update()
	{
		CODE_vec1
	}

	const char* BP_Bool::code()
	{
		return FLAME_STR(CODE_vec1);
	}

	BP_Bool bp_bool_unused;

	void BP_Vec2::update()
	{
#define NL
		CODE_vec2
#undef NL
	}

	const char* BP_Vec2::code()
	{
		return FLAME_STR(CODE_vec2);
	}

	BP_Vec2 bp_vec2_unused;

	void BP_Vec3::update()
	{
#define NL
		CODE_vec3
#undef NL
	}

	const char* BP_Vec3::code()
	{
		return FLAME_STR(CODE_vec3);
	}

	BP_Vec3 bp_vec3_unused;

	void BP_Vec4::update()
	{
#define NL
		CODE_vec4
#undef NL
	}

	const char* BP_Vec4::code()
	{
		return FLAME_STR(CODE_vec4);
	}

	BP_Vec4 bp_vec4_unused;

	void BP_Ivec2::update()
	{
#define NL
		CODE_vec2
#undef NL
	}

	const char* BP_Ivec2::code()
	{
		return FLAME_STR(CODE_vec2);
	}

	BP_Ivec2 bp_ivec2_unused;

	void BP_Ivec3::update()
	{
#define NL
		CODE_vec3
#undef NL
	}

	const char* BP_Ivec3::code()
	{
		return FLAME_STR(CODE_vec3);
	}

	BP_Ivec3 bp_ivec3_unused;

	void BP_Ivec4::update()
	{
#define NL
		CODE_vec4
#undef NL
	}

	const char* BP_Ivec4::code()
	{
		return FLAME_STR(CODE_vec4);
	}

	BP_Ivec4 bp_ivec4_unused;

#undef CODE_vec1
#undef CODE_vec2
#undef CODE_vec3
#undef CODE_vec4

}


#include <flame/foundation/serialize.h>
#include <flame/foundation/blueprint.h>
#include <flame/graphics/device.h>
#include <flame/graphics/commandbuffer.h>
#include <flame/graphics/image.h>
#include <flame/graphics/shader.h>
#include <flame/graphics/font.h>
#include <flame/universe/default_style.h>
#include <flame/universe/topmost.h>
#include <flame/universe/systems/event_dispatcher.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/text.h>
#include <flame/universe/components/edit.h>
#include <flame/universe/components/checkbox.h>
#include <flame/universe/components/combobox.h>
#include <flame/universe/components/event_receiver.h>
#include <flame/universe/components/aligner.h>
#include <flame/universe/components/layout.h>
#include <flame/universe/components/scrollbar.h>
#include <flame/universe/components/menu.h>
#include <flame/universe/components/list.h>
#include <flame/universe/components/style.h>
#include <flame/universe/components/splitter.h>
#include <flame/universe/components/window.h>
#include <flame/universe/components/custom_draw.h>

#include "../renderpath/canvas_make_cmd/canvas.h"

#include "../app.h"
#include "../data_tracker.h"
#include "blueprint_editor.h"
#include "console.h"
#include "image_viewer.h"

template<class T>
void create_edit(Entity* parent, BP::Slot* input)
{
	auto& data = *(T*)input->data();

	auto e_edit = create_drag_edit(app.font_atlas_pixel, 0.9f, std::is_floating_point<T>::value);
	parent->add_child(e_edit);
	{
		struct Capture
		{
			BP::Slot* input;
			cText* drag_text;
		}capture;
		capture.input = input;
		capture.drag_text = e_edit->child(1)->get_component(cText);
		e_edit->child(0)->get_component(cText)->data_changed_listeners.add([](void* c, Component* t, uint hash, void*) {
			auto& capture = *(Capture*)c;
			if (hash == cH("text"))
			{
				auto& text = ((cText*)t)->text();
				auto data = sto_s<T>(text.c_str());
				capture.input->set_data(&data);
				capture.drag_text->set_text(text);
			}
		}, new_mail(&capture));
	}

	auto c_tracker = new_u_object<cDigitalDataTracker<T>>();
	c_tracker->data = &data;
	parent->add_component(c_tracker);
}

template<uint N, class T>
void create_vec_edit(Entity* parent, BP::Slot* input)
{
	auto& data = *(Vec<N, T>*)input->data();

	struct Capture
	{
		BP::Slot* input;
		uint i;
		cText* drag_text;
	}capture;
	capture.input = input;
	for (auto i = 0; i < N; i++)
	{
		auto e_edit = create_drag_edit(app.font_atlas_pixel, 0.9f, std::is_floating_point<T>::value);
		parent->add_child(wrap_standard_text(e_edit, false, app.font_atlas_pixel, 0.9f, s2w(Vec<N, T>::coord_name(i))));
		capture.i = i;
		capture.drag_text = e_edit->child(1)->get_component(cText);
		e_edit->child(0)->get_component(cText)->data_changed_listeners.add([](void* c, Component* t, uint hash, void*) {
			auto& capture = *(Capture*)c;
			if (hash == cH("text"))
			{
				auto& text = ((cText*)t)->text();
				auto data = *(Vec<N, T>*)capture.input->data();
				data[capture.i] = sto_s<T>(text.c_str());
				capture.input->set_data(&data);
				capture.drag_text->set_text(text);
			}
		}, new_mail(&capture));
	}

	auto c_tracker = new_u_object<cDigitalVecDataTracker<N, T>>();
	c_tracker->data = &data;
	parent->add_component(c_tracker);
}

namespace flame
{
	struct DstImage$
	{
		AttributeV<Vec2u> size$i;

		AttributeP<void> img$o;
		AttributeE<TargetType$> type$o;
		AttributeP<void> view$o;

		AttributeV<uint> idx$o;

		__declspec(dllexport) DstImage$()
		{
			size$i.v = Vec2u(800, 600);
		}

		__declspec(dllexport) void update$()
		{
			if (size$i.frame > img$o.frame)
			{
				if (idx$o.v > 0)
					app.canvas->set_image(idx$o.v, nullptr);
				if (img$o.v)
					Image::destroy((Image*)img$o.v);
				if (view$o.v)
					Imageview::destroy((Imageview*)view$o.v);
				auto d = Device::default_one();
				if (d && size$i.v.x() > 0 && size$i.v.y() > 0)
				{
					img$o.v = Image::create(d, Format_R8G8B8A8_UNORM, size$i.v, 1, 1, SampleCount_1, ImageUsage$(ImageUsageTransferDst | ImageUsageAttachment | ImageUsageSampled));
					((Image*)img$o.v)->init(Vec4c(0, 0, 0, 255));
				}
				else
					img$o.v = nullptr;
				type$o.v = TargetImageview;
				type$o.frame = size$i.frame;
				if (img$o.v)
				{
					view$o.v = Imageview::create((Image*)img$o.v);
					idx$o.v = app.canvas->set_image(-1, (Imageview*)view$o.v);
				}
				img$o.frame = size$i.frame;
				view$o.frame = size$i.frame;
				idx$o.frame = size$i.frame;
			}
		}

		__declspec(dllexport) ~DstImage$()
		{
			if (idx$o.v > 0)
				app.canvas->set_image(idx$o.v, nullptr);
			if (img$o.v)
				Image::destroy((Image*)img$o.v);
			if (view$o.v)
				Imageview::destroy((Imageview*)view$o.v);
		}
	};

	struct CmdBufs$
	{
		AttributeV<std::vector<void*>> out$o;

		__declspec(dllexport) CmdBufs$()
		{
		}

		__declspec(dllexport) void update$()
		{
			if (out$o.frame == -1)
			{
				for (auto i = 0; i < out$o.v.size(); i++)
					Commandbuffer::destroy((Commandbuffer*)out$o.v[i]);
				auto d = Device::default_one();
				if (d)
				{
					out$o.v.resize(1);
					out$o.v[0] = Commandbuffer::create(d->gcp);
				}
				else
					out$o.v.clear();
				out$o.frame = 0;
			}

			app.extra_cbs.push_back((Commandbuffer*)out$o.v[0]);
		}

		__declspec(dllexport) ~CmdBufs$()
		{
			for (auto i = 0; i < out$o.v.size(); i++)
				Commandbuffer::destroy((Commandbuffer*)out$o.v[i]);
		}
	};
}

const auto dot_path = s2w(GRAPHVIZ_PATH) + L"/bin/dot.exe";

struct cBPEditor : Component
{
	cCustomDraw* custom_draw;

	std::wstring filename;
	std::wstring filepath;
	BP* bp;
	bool locked;

	Entity* e_add_node_menu;
	cEdit* add_node_menu_filter;
	Vec2f add_pos;
	Entity* e_base;
	Entity* e_slot_menu;
	cDockerTab* console_tab;

	enum SelType
	{
		SelAir,
		SelModule,
		SelPackage,
		SelNode,
		SelSlot,
		SelLink
	}sel_type_;
	union
	{
		BP::Module* m;
		BP::Package* p;
		BP::Node* n;
		BP::Slot* s;
		BP::Slot* l;
	}selected_;

	BP::Slot* dragging_slot;

	bool running;

	std::vector<std::pair<cElement*, uint>> notification;

	cBPEditor() :
		Component("BPEditor")
	{
		bp = nullptr;
		locked = false;

		console_tab = nullptr;
	}

	~cBPEditor()
	{
		BP::destroy(bp);

		if (console_tab)
		{
			looper().add_delay_event([](void* c) {
				auto tab = *(cDockerTab**)c;
				tab->take_away(true);
			}, new_mail_p(console_tab));
		}
	}

	void deselect()
	{
		switch (sel_type_)
		{
		case SelModule:
		case SelPackage:
		case SelNode:
			((Entity*)selected_.m->user_data)->get_component(cElement)->frame_thickness = 0.f;
			break;
		}

		sel_type_ = SelAir;
		selected_.n = nullptr;
	}

	void select(BP::Module* m)
	{
		deselect();
		sel_type_ = SelModule;
		selected_.m = m;
		((Entity*)selected_.m->user_data)->get_component(cElement)->frame_thickness = 4.f;
	}

	void select(BP::Package* p)
	{
		deselect();
		sel_type_ = SelPackage;
		selected_.p = p;
		((Entity*)selected_.m->user_data)->get_component(cElement)->frame_thickness = 4.f;
	}

	void select(BP::Node* n)
	{
		deselect();
		sel_type_ = SelNode;
		selected_.n = n;
		((Entity*)selected_.m->user_data)->get_component(cElement)->frame_thickness = 4.f;
	}

	void select_s(BP::Slot* s)
	{
		deselect();
		sel_type_ = SelSlot;
		selected_.s = s;
	}

	void select_l(BP::Slot* s)
	{
		deselect();
		sel_type_ = SelLink;
		selected_.l = s;
	}

	void reset_add_node_menu_filter()
	{
		add_node_menu_filter->text->set_text(L"");

		for (auto i = 1; i < e_add_node_menu->child_count(); i++)
			e_add_node_menu->child(i)->set_visibility(true);
	}

	void refresh_add_node_menu()
	{
		e_add_node_menu->remove_child((Entity*)FLAME_INVALID_POINTER);

		std::vector<UdtInfo*> all_udts;
		for (auto db : bp->dbs())
		{
			auto udts = db->get_udts();
			for (auto i = 0; i < udts.p->size(); i++)
			{
				auto u = udts.p->at(i);
				if (u->name().find('(') != std::string::npos)
					continue;
				{
					auto f = u->find_function("update");
					if (!(f && f->return_type()->equal(TypeTagVariable, cH("void")) && f->parameter_count() == 0))
						continue;
				}
				auto no_input_output = true;
				for (auto i = 0; i < u->variable_count(); i++)
				{
					auto v = u->variable(i);
					if (v->decoration().find('i') != std::string::npos || v->decoration().find('o') != std::string::npos)
					{
						no_input_output = false;
						break;
					}
				}
				if (!no_input_output)
					all_udts.push_back(u);
			}
			delete_mail(udts);
		}
		std::sort(all_udts.begin(), all_udts.end(), [](UdtInfo* a, UdtInfo* b) {
			return a->name() < b->name();
		});
		{
			auto e_edit = create_standard_edit(150.f, app.font_atlas_pixel, 1.f);
			auto item = wrap_standard_text(e_edit, true, app.font_atlas_pixel, 1.f, Icon_SEARCH);
			e_add_node_menu->add_child(item);

			e_edit->get_component(cText)->data_changed_listeners.add([](void* c, Component* t, uint hash, void*) {
				auto menu = *(Entity**)c;
				auto& str = ((cText*)t)->text();
				for (auto i = 1; i < menu->child_count(); i++)
				{
					auto item = menu->child(i);
					item->set_visibility(str[0] ? (item->get_component(cText)->text().find(str) != std::string::npos) : true);
				}
			}, new_mail_p(e_add_node_menu));

			add_node_menu_filter = e_edit->get_component(cEdit);
		}
		for (auto udt : all_udts)
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, s2w(udt->name()));
			e_add_node_menu->add_child(e_item);
			struct Capture
			{
				cBPEditor* e;
				UdtInfo* u;
			}capture;
			capture.e = this;
			capture.u = udt;
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto& capture = *(Capture*)c;

				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					capture.e->reset_add_node_menu_filter();

					capture.e->add_node(capture.u->name(), "", capture.e->add_pos);
				}
			}, new_mail(&capture));
		}
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"template..");
			e_add_node_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;

				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					editor->reset_add_node_menu_filter();

					popup_input_dialog(editor->entity, L"template", [](void* c, bool ok, const std::wstring& text) {
						auto editor = *(cBPEditor**)c;
						auto bp = editor->bp;
						auto name = w2s(text);

						if (bp->self_module() && bp->self_module()->db()->find_udt(H(name.c_str())))
							editor->add_node(name, "", editor->add_pos);
						else
						{
							if (editor->running)
								editor->add_notification(L"Cannot Add New Template Node While Running");
							else
							{
								struct Capture
								{
									cBPEditor* e;
									std::string n;
								}capture;
								capture.e = editor;
								capture.n = name;
								popup_confirm_dialog(editor->entity, L"Save BP first?", [](void* c, bool ok) {
									auto& capture = *(Capture*)c;
									auto bp = capture.e->bp;

									if (ok)
										BP::save_to_file(bp, capture.e->filename);

									auto file = SerializableNode::create_from_xml_file(capture.e->filename);
									auto n_nodes = file->find_node("nodes");
									auto n_node = n_nodes->new_node("node");
									n_node->new_attr("type", capture.n);
									{
										std::string id;
										for (auto i = 0; i < bp->node_count() + 1; i++)
										{
											id = "node_" + std::to_string(i);
											if (!bp->find_node(id))
												break;
										}
										n_node->new_attr("id", id);
									}
									n_node->new_attr("pos", to_string(capture.e->add_pos));
									SerializableNode::save_to_xml_file(file, capture.e->filename);
									SerializableNode::destroy(file);

									capture.e->load(capture.e->filename, false);
								}, new_mail(&capture));
							}
						}
					}, new_mail_p(editor));
				}
			}, new_mail_p(this));
		}
	}

	Entity* create_module_entity(BP::Module* m);
	Entity* create_node_entity(BP::Node* n);
	Entity* create_package_entity(BP::Package* p);

	void link_test_nodes()
	{
		auto m = bp->find_module(L"editor.exe");
		if (!m)
		{
			m = bp->add_module(L"editor.exe");
			m->pos = Vec2f(-200.f, 0.f);
			m->dont_save = true;
		}
		auto n_dst = bp->find_node("test_dst");
		if (!n_dst)
		{
			n_dst = bp->add_node(cH("DstImage"), "test_dst");
			n_dst->pos = Vec2f(0.f, -200.f);
			n_dst->dont_save = true;
		}
		{
			auto s = bp->find_input("*.rt_dst.type");
			if (s)
				s->link_to(n_dst->find_output("type"));
		}
		{
			auto s = bp->find_input("*.rt_dst.v");
			if (s)
				s->link_to(n_dst->find_output("view"));
		}
		auto n_cbs = bp->find_node("test_cbs");
		if (!n_cbs)
		{
			n_cbs = bp->add_node(cH("CmdBufs"), "test_cbs");
			n_cbs->pos = Vec2f(200.f, -200.f);
			n_cbs->dont_save = true;
		}
		{
			auto s = bp->find_input("*.make_cmd.cbs");
			if (s)
				s->link_to(n_cbs->find_output("out"));
		}
	}

	void load(const std::wstring& _filename, bool no_compile)
	{
		filename = _filename;
		filepath = std::filesystem::path(filename).parent_path().wstring();
		if (bp)
			BP::destroy(bp);
		bp = BP::create_from_file(filename, no_compile);
		link_test_nodes();

		refresh_add_node_menu();

		e_base->remove_child((Entity*)FLAME_INVALID_POINTER);

		for (auto i = 0; i < bp->module_count(); i++)
			create_module_entity(bp->module(i));
		for (auto i = 0; i < bp->package_count(); i++)
			create_package_entity(bp->package(i));
		for (auto i = 0; i < bp->node_count(); i++)
			create_node_entity(bp->node(i));

		deselect();
		dragging_slot = nullptr;
		running = false;
	}

	void set_add_pos_center()
	{
		add_pos = e_base->parent()->get_component(cElement)->size_ * 0.5f - e_base->get_component(cElement)->pos_;
	}

	BP::Node* add_node(const std::string& type_name, const std::string& id, const Vec2f& pos)
	{
		auto n = bp->add_node(H(type_name.c_str()), id);
		n->pos = pos;
		create_node_entity(n);
		return n;
	}

	void remove_module(BP::Module* m)
	{
		struct Capture
		{
			cBPEditor* e;
			BP::Module* m;
		}capture;
		capture.e = this;
		capture.m = m;
		looper().add_delay_event([](void* c) {
			auto& capture = *(Capture*)c;
			auto bp = capture.e->bp;

			auto m_db = capture.m->db();
			for (auto i = 0; i < bp->node_count(); i++)
			{
				auto n = bp->node(i);
				if (n->udt()->db() == m_db)
				{
					auto e = (Entity*)n->user_data;
					e->parent()->remove_child(e);
				}
			}
			auto e = (Entity*)capture.m->user_data;
			e->parent()->remove_child(e);

			bp->remove_module(capture.m);
			capture.e->refresh_add_node_menu();
		}, new_mail(&capture));
	}

	void remove_package(BP::Package* p)
	{
		struct Capture
		{
			cBPEditor* e;
			BP::Package* p;
		}capture;
		capture.e = this;
		capture.p = p;
		looper().add_delay_event([](void* c) {
			auto& capture = *(Capture*)c;
			auto e = (Entity*)capture.p->user_data;
			e->parent()->remove_child(e);
			capture.e->bp->remove_package(capture.p);
			capture.e->refresh_add_node_menu();
		}, new_mail(&capture));
	}

	bool remove_node(BP::Node* n)
	{
		if (selected_.n->id() == "test_dst" || selected_.n->id() == "test_cbs")
			return false;
		looper().add_delay_event([](void* c) {
			auto n = *(BP::Node**)c;
			auto e = (Entity*)n->user_data;
			e->parent()->remove_child(e);
			n->parent()->remove_node(n);
		}, new_mail_p(n));
		return true;
	}

	void duplicate_selected()
	{
		switch (sel_type_)
		{
		case SelNode:
		{
			auto n = bp->add_node(H(selected_.n->udt()->name().c_str()), "");
			n->pos = add_pos;
			for (auto i = 0; i < n->input_count(); i++)
			{
				auto src = selected_.n->input(i);
				auto dst = n->input(i);
				dst->set_data(src->data());
				dst->link_to(src->link(0));
			}
			create_node_entity(n);
		}
			break;
		}
	}

	void delete_selected()
	{
		switch (sel_type_)
		{
		case SelModule:
			if (selected_.m->filename() == L"bp.dll")
				add_notification(L"Cannot Remove Self Module");
			else if (selected_.m->filename() == L"flame_foundation.dll")
				add_notification(L"Cannot Remove 'foundation' Module");
			else if (selected_.m->filename() == L"editor.exe")
				add_notification(L"Cannot Remove 'this' Module");
			else
			{
				std::wstring str;
				auto m_db = selected_.m->db();
				for (auto i = 0; i < bp->node_count(); i++)
				{
					auto n = bp->node(i);
					auto udt = n->udt();
					if (udt->db() == m_db)
						str += L"id: " + s2w(n->id()) + L", type: " + s2w(udt->name()) + L"\n";
				}

				struct Capture
				{
					cBPEditor* e;
					BP::Module* m;
				}capture;
				capture.e = this;
				capture.m = selected_.m;
				popup_confirm_dialog(entity, L"The node(s):\n" + str + L"will be removed, sure to remove module?", [](void* c, bool yes) {
					auto& capture = *(Capture*)c;

					if (yes)
						capture.e->remove_module(capture.m);
				}, new_mail(&capture));
			}
			break;
		case SelPackage:
			break;
		case SelNode:
			if (!remove_node(selected_.n))
				add_notification(L"Cannot Remove Test Nodes");
			break;
		case SelLink:
			selected_.l->link_to(nullptr);
			break;
		}

		deselect();
	}

	void update_gv()
	{
		auto gv_filename = filepath + L"/bp.gv";
		if (!std::filesystem::exists(gv_filename) || std::filesystem::last_write_time(gv_filename) < std::filesystem::last_write_time(filename))
		{
			if (GRAPHVIZ_PATH == std::string(""))
				assert(0);

			std::string gv = "digraph bp {\nrankdir=LR\nnode [shape = Mrecord];\n";
			for (auto i = 0; i < bp->node_count(); i++)
			{
				auto src = bp->node(i);
				auto& name = src->id();

				auto str = "\t" + name + " [label = \"" + name + "|" + src->udt()->name() + "|{{";
				for (auto j = 0; j < src->input_count(); j++)
				{
					auto input = src->input(j);
					auto& name = input->vi()->name();
					str += "<" + name + ">" + name;
					if (j != src->input_count() - 1)
						str += "|";
				}
				str += "}|{";
				for (auto j = 0; j < src->output_count(); j++)
				{
					auto output = src->output(j);
					auto& name = output->vi()->name();
					str += "<" + name + ">" + name;
					if (j != src->output_count() - 1)
						str += "|";
				}
				str += "}}\"];\n";

				gv += str;
			}
			for (auto i = 0; i < bp->node_count(); i++)
			{
				auto src = bp->node(i);

				for (auto j = 0; j < src->input_count(); j++)
				{
					auto input = src->input(j);
					if (input->link())
					{
						auto in_addr = input->get_address();
						auto out_addr = input->link()->get_address();
						auto in_sp = string_split(*in_addr.p, '.');
						auto out_sp = string_split(*out_addr.p, '.');
						delete_mail(in_addr);
						delete_mail(out_addr);

						gv += "\t" + out_sp[0] + ":" + out_sp[1] + " -> " + in_sp[0] + ":" + in_sp[1] + ";\n";
					}
				}
			}
			gv += "}\n";

			std::ofstream file(gv_filename);
			file << gv;
			file.close();
		}
	}

	bool generate_graph_image()
	{
		update_gv();
		auto png_filename = filepath + L"/bp.png";
		if (!std::filesystem::exists(png_filename) || std::filesystem::last_write_time(png_filename) < std::filesystem::last_write_time(filename))
			exec(dot_path, L"-Tpng " + filepath + L"/bp.gv -y -o " + png_filename, true);

		return std::filesystem::exists(png_filename);
	}

	bool auto_set_layout()
	{
		update_gv();
		auto graph_filename = filepath + L"/bp.graph";
		if (!std::filesystem::exists(graph_filename) || std::filesystem::last_write_time(graph_filename) < std::filesystem::last_write_time(filename))
			exec(dot_path, L"-Tplain" + filepath + L"/bp.gv -y -o " + graph_filename, true);
		if (!std::filesystem::exists(graph_filename))
			return false;

		auto str = get_file_string(L"bp.graph.txt");
		for (auto it = str.begin(); it != str.end(); )
		{
			if (*it == '\\')
			{
				it = str.erase(it);
				if (it != str.end())
				{
					if (*it == '\r')
					{
						it = str.erase(it);
						if (it != str.end() && *it == '\n')
							it = str.erase(it);
					}
					else if (*it == '\n')
						it = str.erase(it);
				}
			}
			else
				it++;
		}

		std::regex reg_node(R"(node ([\w]+) ([\d\.]+) ([\d\.]+))");
		std::smatch match;
		while (std::regex_search(str, match, reg_node))
		{
			auto n = bp->find_node(match[1].str().c_str());
			if (n)
			{
				n->pos = Vec2f(std::stof(match[2].str().c_str()), std::stof(match[3].str().c_str())) * 100.f;
				((Entity*)n->user_data)->get_component(cElement)->set_pos(n->pos);
			}

			str = match.suffix();
		}

		return true;
	}

	void add_notification(const std::wstring& text)
	{
		auto e_notification = Entity::create();
		entity->add_child(e_notification);
		{
			auto c_element = cElement::create();
			c_element->pos_.y() = notification.size() * (default_style.font_size + 20.f);
			c_element->inner_padding_ = Vec4f(8.f);
			c_element->color = Vec4c(0, 0, 0, 255);
			e_notification->add_component(c_element);
			notification.emplace_back(c_element, 180);

			auto c_text = cText::create(app.font_atlas_pixel);
			c_text->color = Vec4c(255);
			c_text->set_text(text);
			e_notification->add_component(c_text);
		}
	}

	void on_component_added(Component* c) override
	{
		if (c->name_hash == cH("cCustomDraw"))
		{
			custom_draw = (cCustomDraw*)c;
			custom_draw->cmds.add([](void* c, graphics::Canvas* canvas) {
				(*(cBPEditor**)c)->draw(canvas);
			}, new_mail_p(this));
		}
	}

	void draw(graphics::Canvas* canvas)
	{
		if (running)
			bp->update();

		for (auto it = notification.begin(); it != notification.end(); )
		{
			it->second--;
			if (it->second == 0)
			{
				for (auto _it = it + 1; _it != notification.end(); _it++)
					_it->first->set_y(it->first->pos_.y());
				entity->remove_child(it->first->entity);
				it = notification.erase(it);
			}
			else
			{
				if (it->second < 60)
					it->first->alpha = it->second / 60.f;
				it++;
			}
		}
	}
};

struct cBP : Component
{
	cElement* element;
	cEventReceiver* event_receiver;
	cElement* base_element;
	cCustomDraw* custom_draw;

	cText* scale_text;
	cBPEditor* editor;

	float bezier_extent;

	cBP() :
		Component("cBP")
	{
	}

	void on_component_added(Component* c) override;
	void draw(graphics::Canvas* canvas);
};

struct cBPModule : Component
{
	cElement* element;
	cEventReceiver* event_receiver;

	cBPEditor* editor;
	BP::Module* m;

	cBPModule() :
		Component("cBPModule")
	{
	}

	void on_component_added(Component* c) override
	{
		if (c->name_hash == cH("cElement"))
		{
			element = (cElement*)c;
			element->data_changed_listeners.add([](void* c, Component* e, uint hash, void*) {
				if (hash == cH("pos"))
					(*(cBPModule**)c)->m->pos = ((cElement*)e)->pos_;
			}, new_mail_p(this));
		}
		else if (c->name_hash == cH("cEventReceiver"))
		{
			event_receiver = (cEventReceiver*)c;
			event_receiver->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto thiz = *(cBPModule**)c;
				if (is_mouse_down(action, key, true) && key == Mouse_Left)
					thiz->editor->select(thiz->m);
			}, new_mail_p(this));
		}
	}
};

struct cBPPackage : Component
{
	cElement* element;
	cEventReceiver* event_receiver;

	cBPEditor* editor;
	BP::Package* p;

	cBPPackage() :
		Component("cBPPackage")
	{
	}

	void on_component_added(Component* c) override
	{
		if (c->name_hash == cH("cElement"))
		{
			element = (cElement*)c;
			element->data_changed_listeners.add([](void* c, Component* e, uint hash, void*) {
				if (hash == cH("pos"))
					(*(cBPModule**)c)->m->pos = ((cElement*)e)->pos_;
			}, new_mail_p(this));
		}
		else if (c->name_hash == cH("cEventReceiver"))
		{
			event_receiver = (cEventReceiver*)c;
			event_receiver->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto thiz = *(cBPPackage**)c;
				if (is_mouse_down(action, key, true) && key == Mouse_Left)
					thiz->editor->select(thiz->p);
			}, new_mail_p(this));
		}
	}
};

struct cBPNode : Component
{
	cElement* element;
	cEventReceiver* event_receiver;

	cBPEditor* editor;
	BP::Node* n;

	cBPNode() :
		Component("cBPNode")
	{
	}

	void on_component_added(Component* c) override
	{
		if (c->name_hash == cH("cElement"))
		{
			element = (cElement*)c;
			element->data_changed_listeners.add([](void* c, Component* e, uint hash, void*) {
				if (hash == cH("pos"))
					(*(cBPModule**)c)->m->pos = ((cElement*)e)->pos_;
			}, new_mail_p(this));
		}
		else if (c->name_hash == cH("cEventReceiver"))
		{
			event_receiver = (cEventReceiver*)c;
			event_receiver->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto thiz = *(cBPNode**)c;
				if (is_mouse_down(action, key, true) && key == Mouse_Left)
					thiz->editor->select(thiz->n);
			}, new_mail_p(this));
		}
	}
};

struct cBPSlot : Component
{
	cElement* element;
	cEventReceiver* event_receiver;
	cDataTracker* tracker;

	cBPEditor* editor;
	BP::Slot* s;

	cBPSlot() :
		Component("cBPSlot")
	{
		element = nullptr;
		event_receiver = nullptr;
		tracker = nullptr;
	}

	void on_component_added(Component* c) override
	{
		if (c->name_hash == cH("cElement"))
			element = (cElement*)c;
		else if (c->name_hash == cH("cEventReceiver"))
		{
			event_receiver = (cEventReceiver*)c;
			if (s->type() == BP::Slot::Input)
			{
				event_receiver->drag_hash = cH("input_slot");
				event_receiver->set_acceptable_drops({ cH("output_slot") });
			}
			else
			{
				event_receiver->drag_hash = cH("output_slot");
				event_receiver->set_acceptable_drops({ cH("input_slot") });
			}

			event_receiver->drag_and_drop_listeners.add([](void* c, DragAndDrop action, cEventReceiver* er, const Vec2i& pos) {
				auto thiz = *(cBPSlot**)c;
				if (action == DragStart)
				{
					thiz->editor->dragging_slot = thiz->s;
					if (thiz->s->type() == BP::Slot::Input)
						thiz->s->link_to(nullptr);
				}
				else if (action == DragEnd)
					thiz->editor->dragging_slot = nullptr;
				else if (action == Dropped)
				{
					auto oth = er->entity->get_component(cBPSlot)->s;
					if (thiz->s->type() == BP::Slot::Input)
						thiz->s->link_to(oth);
					else
						oth->link_to(thiz->s);
				}
			}, new_mail_p(this));

			event_receiver->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto thiz = *(cBPSlot**)c;
				auto editor = thiz->editor;

				if (is_mouse_down(action, key, true) && key == Mouse_Right)
				{
					editor->select_s(thiz->s);
					popup_menu(editor->e_slot_menu, app.root, (Vec2f)pos);
				}
			}, new_mail_p(this));
		}
	}
};

void cBP::on_component_added(Component* c)
{
	if (c->name_hash == cH("cElement"))
		element = (cElement*)c;
	else if (c->name_hash == cH("cEventReceiver"))
	{
		event_receiver = (cEventReceiver*)c;
		event_receiver->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
			auto thiz = *(cBP**)c;
			auto editor = thiz->editor;

			if (is_mouse_down(action, key, true) && key == Mouse_Left)
			{
				editor->deselect();

				auto bp = editor->bp;

				for (auto i = 0; i < bp->node_count(); i++)
				{
					auto n = bp->node(i);
					for (auto j = 0; j < n->input_count(); j++)
					{
						auto input = n->input(j);
						auto output = input->link(0);
						if (output)
						{
							auto e1 = ((cBPSlot*)output->user_data)->element;
							auto e2 = ((cBPSlot*)input->user_data)->element;
							auto p1 = e1->global_pos + e1->global_size * 0.5f;
							auto p2 = e2->global_pos + e2->global_size * 0.5f;

							if (distance((Vec2f)pos, bezier_closest_point((Vec2f)pos, p1, p1 + Vec2f(thiz->bezier_extent, 0.f), p2 - Vec2f(thiz->bezier_extent, 0.f), p2, 4, 7)) < 3.f * thiz->element->global_scale)
								editor->select_l(input);
						}
					}
				}
			}
			else if (is_mouse_up(action, key, true) && key == Mouse_Right)
			{
				popup_menu(editor->e_add_node_menu, app.root, (Vec2f)pos);
				editor->add_pos = pos - thiz->element->global_pos;
			}
		}, new_mail_p(this));
	}
	else if (c->name_hash == cH("cCustomDraw"))
	{
		custom_draw = (cCustomDraw*)c;
		custom_draw->cmds.add([](void* c, graphics::Canvas* canvas) {
			(*(cBP**)c)->draw(canvas);
		}, new_mail_p(this));
	}
}

void cBP::draw(graphics::Canvas* canvas)
{
	bezier_extent = 50.f * base_element->global_scale;

	auto bp = editor->bp;
	const auto show_link = [&](BP::Slot* input, BP::Slot* output) {
		if (output->parent()->parent() == bp)
		{
			auto e1 = ((cBPSlot*)output->user_data)->element;
			auto e2 = ((cBPSlot*)input->user_data)->element;
			auto p1 = e1->global_pos + e1->global_size * 0.5f;
			auto p2 = e2->global_pos + e2->global_size * 0.5f;

			std::vector<Vec2f> points;
			path_bezier(points, p1, p1 + Vec2f(bezier_extent, 0.f), p2 - Vec2f(bezier_extent, 0.f), p2);
			canvas->stroke(points, editor->selected_.l == input ? Vec4c(255, 255, 50, 255) : Vec4c(100, 100, 120, 255), 3.f * base_element->global_scale);
		}
	};
	for (auto i = 0; i < bp->package_count(); i++)
	{
		auto p = bp->package(i);
		auto pbp = p->bp();
		for (auto j = 0; j < pbp->input_export_count(); j++)
		{
			auto input = pbp->input_export(j);
			auto output = input->link(0);
			if (output)
				show_link(input, output);
		}
	}
	for (auto i = 0; i < bp->node_count(); i++)
	{
		auto n = bp->node(i);
		for (auto j = 0; j < n->input_count(); j++)
		{
			auto input = n->input(j);
			auto output = input->link(0);
			if (output)
				show_link(input, output);
		}
	}
	if (editor->dragging_slot)
	{
		auto e = ((cBPSlot*)editor->dragging_slot->user_data)->element;
		auto p1 = e->global_pos + e->global_size * 0.5f;
		auto p2 = Vec2f(event_receiver->dispatcher->mouse_pos);

		std::vector<Vec2f> points;
		path_bezier(points, p1, p1 + Vec2f(editor->dragging_slot->type() == BP::Slot::Output ? bezier_extent : -bezier_extent, 0.f), p2, p2);
		canvas->stroke(points, Vec4c(255, 255, 50, 255), 3.f * base_element->global_scale);
	}
}

Entity* cBPEditor::create_module_entity(BP::Module* m)
{
	auto e_module = Entity::create();
	e_base->add_child(e_module);
	m->user_data = e_module;
	{
		auto c_element = cElement::create();
		c_element->pos_ = m->pos;
		c_element->color = Vec4c(255, 200, 190, 200);
		c_element->frame_color = Vec4c(252, 252, 50, 200);
		e_module->add_component(c_element);

		e_module->add_component(cEventReceiver::create());

		auto c_layout = cLayout::create(LayoutVertical);
		c_layout->fence = 1;
		e_module->add_component(c_layout);

		e_module->add_component(cMoveable::create());

		auto c_module = new_u_object<cBPModule>();
		c_module->editor = this;
		c_module->m = m;
		e_module->add_component(c_module);
	}
	{
		auto e_content = Entity::create();
		e_module->add_child(e_content);
		{
			auto c_element = cElement::create();
			c_element->inner_padding_ = Vec4f(8.f);
			e_content->add_component(c_element);

			auto c_layout = cLayout::create(LayoutVertical);
			c_layout->item_padding = 4.f;
			e_content->add_component(c_layout);
		}

		auto e_text_filename = Entity::create();
		e_content->add_child(e_text_filename);
		{
			auto c_element = cElement::create();
			c_element->inner_padding_ = Vec4f(4.f, 2.f, 4.f, 2.f);
			e_text_filename->add_component(c_element);

			auto c_text = cText::create(app.font_atlas_pixel);
			c_text->set_text(m->filename());
			e_text_filename->add_component(c_text);
		}

		auto e_text_type = Entity::create();
		e_content->add_child(e_text_type);
		{
			e_text_type->add_component(cElement::create());

			auto c_text = cText::create(app.font_atlas_pixel);
			c_text->set_text(L"module");
			c_text->color = Vec4c(50, 50, 50, 255);
			e_text_type->add_component(c_text);
		}
	}

	auto e_bring_to_front = Entity::create();
	e_module->add_child(e_bring_to_front);
	{
		e_bring_to_front->add_component(cElement::create());

		auto c_event_receiver = cEventReceiver::create();
		c_event_receiver->penetrable = true;
		e_bring_to_front->add_component(c_event_receiver);

		auto c_aligner = cAligner::create();
		c_aligner->width_policy_ = SizeFitParent;
		c_aligner->height_policy_ = SizeFitParent;
		e_bring_to_front->add_component(c_aligner);

		e_bring_to_front->add_component(cBringToFront::create());
	}

	return e_module;
}

Entity* cBPEditor::create_package_entity(BP::Package* p)
{
	auto e_package = Entity::create();
	e_base->add_child(e_package);
	p->user_data = e_package;
	{
		auto c_element = cElement::create();
		c_element->pos_ = p->pos;
		c_element->color = Vec4c(190, 255, 200, 200);
		c_element->frame_color = Vec4c(252, 252, 50, 200);
		e_package->add_component(c_element);

		e_package->add_component(cEventReceiver::create());

		auto c_layout = cLayout::create(LayoutVertical);
		c_layout->fence = 1;
		e_package->add_component(c_layout);

		e_package->add_component(cMoveable::create());

		auto c_package = new_u_object<cBPPackage>();
		c_package->editor = this;
		c_package->p = p;
		e_package->add_component(c_package);
	}
	{
		auto e_content = Entity::create();
		e_package->add_child(e_content);
		{
			auto c_element = cElement::create();
			c_element->inner_padding_ = Vec4f(8.f);
			e_content->add_component(c_element);

			auto c_layout = cLayout::create(LayoutVertical);
			c_layout->item_padding = 4.f;
			e_content->add_component(c_layout);
		}

		auto e_text_id = Entity::create();
		e_content->add_child(e_text_id);
		{
			auto c_element = cElement::create();
			c_element->inner_padding_ = Vec4f(4.f, 2.f, 4.f, 2.f);
			e_text_id->add_component(c_element);

			auto c_text = cText::create(app.font_atlas_pixel);
			c_text->set_text(s2w(p->id()));
			c_text->data_changed_listeners.add([](void* c, Component* t, uint hash, void*) {
				(*(BP::Package**)c)->set_id(w2s(((cText*)t)->text()));
			}, new_mail_p(p));
			e_text_id->add_component(c_text);

			e_text_id->add_component(cEventReceiver::create());

			e_text_id->add_component(cCustomDraw::create());

			e_text_id->add_component(cEdit::create());
		}

		auto e_main = Entity::create();
		e_content->add_child(e_main);
		{
			e_main->add_component(cElement::create());

			auto c_aligner = cAligner::create();
			c_aligner->width_policy_ = SizeGreedy;
			e_main->add_component(c_aligner);

			auto c_layout = cLayout::create(LayoutHorizontal);
			c_layout->item_padding = 16.f;
			e_main->add_component(c_layout);
		}

		auto bp = p->bp();

		auto e_left = Entity::create();
		e_main->add_child(e_left);
		{
			e_left->add_component(cElement::create());

			auto c_aligner = cAligner::create();
			c_aligner->width_policy_ = SizeGreedy;
			e_left->add_component(c_aligner);

			e_left->add_component(cLayout::create(LayoutVertical));

			for (auto j = 0; j < bp->input_export_count(); j++)
			{
				auto s = bp->input_export(j);

				auto e_input = Entity::create();
				e_left->add_child(e_input);
				{
					e_input->add_component(cElement::create());

					auto c_layout = cLayout::create(LayoutVertical);
					c_layout->item_padding = 2.f;
					e_input->add_component(c_layout);
				}

				auto e_title = Entity::create();
				e_input->add_child(e_title);
				{
					e_title->add_component(cElement::create());

					e_title->add_component(cLayout::create(LayoutHorizontal));

					auto e_slot = Entity::create();
					e_title->add_child(e_slot);
					{
						auto c_element = cElement::create();
						auto r = default_style.font_size;
						c_element->size_ = r;
						c_element->roundness = r * 0.5f;
						c_element->color = Vec4c(200, 200, 200, 255);
						e_slot->add_component(c_element);

						e_slot->add_component(cEventReceiver::create());

						auto c_slot = new_u_object<cBPSlot>();
						c_slot->editor = this;
						c_slot->s = s;
						e_slot->add_component(c_slot);
						s->user_data = c_slot;
					}

					auto e_text = Entity::create();
					e_title->add_child(e_text);
					{
						e_text->add_component(cElement::create());

						auto c_text = cText::create(app.font_atlas_pixel);
						auto addr = s->get_address();
						c_text->set_text(s2w(*addr.p));
						delete_mail(addr);
						e_text->add_component(c_text);
					}
				}
			}
		}

		auto e_right = Entity::create();
		e_main->add_child(e_right);
		{
			e_right->add_component(cElement::create());

			e_right->add_component(cLayout::create(LayoutVertical));

			for (auto j = 0; j < bp->output_export_count(); j++)
			{
				auto s = bp->output_export(j);

				auto e_title = Entity::create();
				e_right->add_child(e_title);
				{
					e_title->add_component(cElement::create());

					auto c_aligner = cAligner::create();
					c_aligner->x_align_ = AlignxRight;
					e_title->add_component(c_aligner);

					e_title->add_component(cLayout::create(LayoutHorizontal));

					auto e_text = Entity::create();
					e_title->add_child(e_text);
					{
						e_text->add_component(cElement::create());

						auto c_text = cText::create(app.font_atlas_pixel);
						auto addr = s->get_address();
						c_text->set_text(s2w(*addr.p));
						delete_mail(addr);
						e_text->add_component(c_text);
					}

					auto e_slot = Entity::create();
					e_title->add_child(e_slot);
					{
						auto c_element = cElement::create();
						auto r = default_style.font_size;
						c_element->size_ = r;
						c_element->roundness = r * 0.5f;
						c_element->color = Vec4c(200, 200, 200, 255);
						e_slot->add_component(c_element);

						e_slot->add_component(cEventReceiver::create());

						auto c_slot = new_u_object<cBPSlot>();
						c_slot->editor = this;
						c_slot->s = s;
						e_slot->add_component(c_slot);
						s->user_data = c_slot;
					}
				}
			}
		}
	}

	auto e_bring_to_front = Entity::create();
	e_package->add_child(e_bring_to_front);
	{
		e_bring_to_front->add_component(cElement::create());

		auto c_event_receiver = cEventReceiver::create();
		c_event_receiver->penetrable = true;
		e_bring_to_front->add_component(c_event_receiver);

		auto c_aligner = cAligner::create();
		c_aligner->width_policy_ = SizeFitParent;
		c_aligner->height_policy_ = SizeFitParent;
		e_bring_to_front->add_component(c_aligner);

		e_bring_to_front->add_component(cBringToFront::create());
	}

	return e_package;
}

Entity* cBPEditor::create_node_entity(BP::Node* n)
{
	auto e_node = Entity::create();
	e_base->add_child(e_node);
	n->user_data = e_node;
	{
		auto c_element = cElement::create();
		c_element->pos_ = n->pos;
		c_element->color = Vec4c(255, 255, 255, 200);
		c_element->frame_color = Vec4c(252, 252, 50, 200);
		e_node->add_component(c_element);

		e_node->add_component(cEventReceiver::create());

		auto c_layout = cLayout::create(LayoutVertical);
		c_layout->fence = 1;
		e_node->add_component(c_layout);

		e_node->add_component(cMoveable::create());

		auto c_node = new_u_object<cBPNode>();
		c_node->editor = this;
		c_node->n = n;
		e_node->add_component(c_node);
	}
	{
		auto e_content = Entity::create();
		e_node->add_child(e_content);
		{
			auto c_element = cElement::create();
			c_element->inner_padding_ = Vec4f(8.f);
			e_content->add_component(c_element);

			auto c_layout = cLayout::create(LayoutVertical);
			c_layout->item_padding = 4.f;
			e_content->add_component(c_layout);
		}

		auto e_text_id = Entity::create();
		e_content->add_child(e_text_id);
		{
			auto c_element = cElement::create();
			c_element->inner_padding_ = Vec4f(4.f, 2.f, 4.f, 2.f);
			e_text_id->add_component(c_element);

			auto c_text = cText::create(app.font_atlas_pixel);
			c_text->font_size_ = default_style.font_size * 1.5f;
			c_text->set_text(s2w(n->id()));
			c_text->data_changed_listeners.add([](void* c, Component* t, uint hash, void*) {
				(*(BP::Node**)c)->set_id(w2s(((cText*)t)->text()));
			}, new_mail_p(n));
			e_text_id->add_component(c_text);

			e_text_id->add_component(cEventReceiver::create());

			e_text_id->add_component(cCustomDraw::create());

			e_text_id->add_component(cEdit::create());

		}

		auto e_text_type = Entity::create();
		e_content->add_child(e_text_type);
		{
			e_text_type->add_component(cElement::create());

			auto c_text = cText::create(app.font_atlas_pixel);
			auto udt = n->udt();
			auto module_name = std::filesystem::path(udt->db()->module_name());
			if (module_name.parent_path() != L"")
				module_name = module_name.lexically_relative(std::filesystem::path(filename).parent_path());
			c_text->set_text(module_name.wstring() + L"\n" + s2w(udt->name()));
			c_text->color = Vec4c(50, 50, 50, 255);
			e_text_type->add_component(c_text);
		}

		auto udt_name = n->udt()->name();
		if (udt_name == "DstImage")
		{
			auto e_show = create_standard_button(app.font_atlas_pixel, 0.9f, L"Show");
			e_content->add_child(e_show);
			e_show->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				if (is_mouse_clicked(action, key))
					open_image_viewer(*(uint*)(*(BP::Node**)c)->find_output("idx")->data(), Vec2f(1495.f, 339.f));
			}, new_mail_p(n));
		}
		else if (udt_name == "graphics::Shader")
		{
			auto e_edit = create_standard_button(app.font_atlas_pixel, 0.9f, L"Edit Shader");
			e_content->add_child(e_edit);

			struct Capture
			{
				cBPEditor* e;
				BP::Node* n;
			}capture;
			capture.e = this;
			capture.n = n;
			e_edit->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto& capture = *(Capture*)c;

				if (is_mouse_clicked(action, key))
				{
					capture.e->locked = true;
					auto t = create_topmost(capture.e->entity, false, false, true, Vec4c(255, 255, 255, 235), true);
					{
						t->get_component(cElement)->inner_padding_ = Vec4f(4.f);

						auto c_layout = cLayout::create(LayoutVertical);
						c_layout->width_fit_children = false;
						c_layout->height_fit_children = false;
						t->add_component(c_layout);
					}

					auto e_buttons = Entity::create();
					t->add_child(e_buttons);
					{
						e_buttons->add_component(cElement::create());

						auto c_layout = cLayout::create(LayoutHorizontal);
						c_layout->item_padding = 4.f;
						e_buttons->add_component(c_layout);
					}

					auto e_back = create_standard_button(app.font_atlas_pixel, 1.f, L"Back");
					e_buttons->add_child(e_back);
					{
						e_back->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
							if (is_mouse_clicked(action, key))
							{
								auto editor = *(cBPEditor**)c;
								destroy_topmost(editor->entity, false);
								editor->locked = false;
							}
						}, new_mail_p(capture.e));
					}

					auto e_compile = create_standard_button(app.font_atlas_pixel, 1.f, L"Compile");
					e_buttons->add_child(e_compile);

					auto e_text_tip = Entity::create();
					e_buttons->add_child(e_text_tip);
					{
						e_text_tip->add_component(cElement::create());

						auto c_text = cText::create(app.font_atlas_pixel);
						c_text->set_text(L"(Do update first to get popper result)");
						e_text_tip->add_component(c_text);
					}

					auto e_main = Entity::create();
					t->add_child(e_main);
					{
						e_main->add_component(cElement::create());

						auto c_aligner = cAligner::create();
						c_aligner->width_policy_ = SizeFitParent;
						c_aligner->height_policy_ = SizeFitParent;
						e_main->add_component(c_aligner);

						auto c_layout = cLayout::create(LayoutVertical);
						c_layout->width_fit_children = false;
						c_layout->height_fit_children = false;
						e_main->add_component(c_layout);
					}


					auto filename = *(std::wstring*)capture.n->find_input("filename")->data();
					auto prefix = *(std::string*)capture.n->find_input("prefix")->data();
					auto inputs = (AttributeP<std::vector<void*>>*)capture.n->find_input("inputs")->raw_data();
					auto outputs = (AttributeP < std::vector<void*>>*)capture.n->find_input("outputs")->raw_data();
					auto pll = (Pipelinelayout*)capture.n->find_input("pll")->data_p();
					auto autogen_code = *(bool*)capture.n->find_input("autogen_code")->data();

					{
						auto e_text_view = Entity::create();
						{
							auto c_element = cElement::create();
							c_element->clip_children = true;
							e_text_view->add_component(c_element);

							auto c_aligner = cAligner::create();
							c_aligner->width_policy_ = SizeFitParent;
							c_aligner->height_policy_ = SizeFitParent;
							e_text_view->add_component(c_aligner);

							auto c_layout = cLayout::create(LayoutVertical);
							c_layout->width_fit_children = false;
							c_layout->height_fit_children = false;
							e_text_view->add_component(c_layout);
						}

						auto e_text = Entity::create();
						e_text_view->add_child(e_text);
						{
							e_text->add_component(cElement::create());

							auto c_text = cText::create(app.font_atlas_pixel);
							auto _prefix = s2w(prefix);
							if (autogen_code)
							{
								auto code = get_shader_autogen_code(shader_stage_from_filename(filename), get_attribute_vec(*inputs), get_attribute_vec(*outputs), pll);
								_prefix += s2w(*code.p);
								delete_mail(code);
							}
							c_text->set_text(_prefix);
							c_text->auto_width_ = false;
							e_text->add_component(c_text);
						}

						auto e_scrollbar_container = wrap_standard_scrollbar(e_text_view, ScrollbarVertical, true, default_style.font_size);
						e_scrollbar_container->get_component(cAligner)->height_factor_ = 1.f / 3.f;
						e_main->add_child(e_scrollbar_container);
					}

					auto e_spliter = Entity::create();
					e_main->add_child(e_spliter);
					{
						auto c_element = cElement::create();
						c_element->size_.y() = 8.f;
						e_spliter->add_component(c_element);

						e_spliter->add_component(cEventReceiver::create());

						e_spliter->add_component(cStyleColor::create(Vec4c(0), default_style.frame_color_hovering, default_style.frame_color_active));

						auto c_splitter = cSplitter::create();
						c_splitter->type = SplitterVertical;
						e_spliter->add_component(c_splitter);

						auto c_aligner = cAligner::create();
						c_aligner->width_policy_ = SizeFitParent;
						e_spliter->add_component(c_aligner);
					}

					{
						auto e_text_view = Entity::create();
						{
							auto c_element = cElement::create();
							c_element->clip_children = true;
							e_text_view->add_component(c_element);

							auto c_aligner = cAligner::create();
							c_aligner->width_policy_ = SizeFitParent;
							c_aligner->height_policy_ = SizeFitParent;
							e_text_view->add_component(c_aligner);

							auto c_layout = cLayout::create(LayoutVertical);
							c_layout->width_fit_children = false;
							c_layout->height_fit_children = false;
							e_text_view->add_component(c_layout);
						}

						auto e_text = Entity::create();
						e_text_view->add_child(e_text);
						{
							e_text->add_component(cElement::create());

							auto c_text = cText::create(app.font_atlas_pixel);
							auto file = get_file_string(capture.e->filepath + L"/" + filename);
							c_text->set_text(s2w(file));
							c_text->auto_width_ = false;
							e_text->add_component(c_text);

							e_text->add_component(cEventReceiver::create());

							e_text->add_component(cEdit::create());

							auto c_aligner = cAligner::create();
							c_aligner->width_policy_ = SizeFitParent;
							e_text->add_component(c_aligner);

							{
								struct _Capture
								{
									cBPEditor* e;
									BP::Node* n;
									cText* t;
								}_capture;
								_capture.e = capture.e;
								_capture.n = capture.n;
								_capture.t = c_text;
								e_compile->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
									auto& capture = *(_Capture*)c;
									if (is_mouse_clicked(action, key))
									{
										auto i_filename = capture.n->find_input("filename");
										std::ofstream file(capture.e->filepath + L"/" + *(std::wstring*)i_filename->data());
										auto str = w2s(capture.t->text());
										str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
										file.write(str.c_str(), str.size());
										file.close();
										i_filename->set_frame(looper().frame);
									}
								}, new_mail(&_capture));
							}
						}

						auto e_scrollbar_container = wrap_standard_scrollbar(e_text_view, ScrollbarVertical, true, default_style.font_size);
						e_scrollbar_container->get_component(cAligner)->height_factor_ = 2.f / 3.f;
						e_main->add_child(e_scrollbar_container);
					}
				}
			}, new_mail(&capture));
		}

		auto e_main = Entity::create();
		e_content->add_child(e_main);
		{
			e_main->add_component(cElement::create());

			auto c_aligner = cAligner::create();
			c_aligner->width_policy_ = SizeGreedy;
			e_main->add_component(c_aligner);

			auto c_layout = cLayout::create(LayoutHorizontal);
			c_layout->item_padding = 16.f;
			e_main->add_component(c_layout);

			auto e_left = Entity::create();
			e_main->add_child(e_left);
			{
				e_left->add_component(cElement::create());

				auto c_aligner = cAligner::create();
				c_aligner->width_policy_ = SizeGreedy;
				e_left->add_component(c_aligner);

				e_left->add_component(cLayout::create(LayoutVertical));

				for (auto j = 0; j < n->input_count(); j++)
				{
					auto input = n->input(j);

					auto e_input = Entity::create();
					e_left->add_child(e_input);
					{
						e_input->add_component(cElement::create());

						auto c_layout = cLayout::create(LayoutVertical);
						c_layout->item_padding = 2.f;
						e_input->add_component(c_layout);
					}

					auto e_title = Entity::create();
					e_input->add_child(e_title);
					{
						e_title->add_component(cElement::create());

						e_title->add_component(cLayout::create(LayoutHorizontal));
					}

					auto e_slot = Entity::create();
					e_title->add_child(e_slot);
					{
						auto c_element = cElement::create();
						auto r = default_style.font_size;
						c_element->size_ = r;
						c_element->roundness = r * 0.5f;
						c_element->color = bp->find_input_export(input) != -1 ? Vec4c(200, 40, 20, 255) : Vec4c(200, 200, 200, 255);
						e_slot->add_component(c_element);

						e_slot->add_component(cEventReceiver::create());
					}
					auto c_slot = new_u_object<cBPSlot>();
					c_slot->editor = this;
					c_slot->s = input;
					e_slot->add_component(c_slot);
					input->user_data = c_slot;

					auto e_text = Entity::create();
					e_title->add_child(e_text);
					{
						e_text->add_component(cElement::create());

						auto c_text = cText::create(app.font_atlas_pixel);
						c_text->set_text(s2w(input->vi()->name()));
						e_text->add_component(c_text);
					}

					auto e_data = Entity::create();
					e_input->add_child(e_data);
					{
						auto c_element = cElement::create();
						c_element->inner_padding_ = Vec4f(default_style.font_size, 0.f, 0.f, 0.f);
						e_data->add_component(c_element);

						auto c_layout = cLayout::create(LayoutVertical);
						c_layout->item_padding = 2.f;
						e_data->add_component(c_layout);
					}

					auto type = input->vi()->type();
					switch (type->tag())
					{
					case TypeTagAttributeES:
					{
						auto info = find_enum(bp->dbs(), type->hash());
						create_enum_combobox(info, 120.f, app.font_atlas_pixel, 0.9f, e_data);

						struct Capture
						{
							BP::Slot* input;
							EnumInfo* e;
						}capture;
						capture.input = input;
						capture.e = info;
						e_data->child(0)->get_component(cCombobox)->data_changed_listeners.add([](void* c, Component* cb, uint hash, void*) {
							auto& capture = *(Capture*)c;
							if (hash == cH("index"))
							{
								auto v = capture.e->item(((cCombobox*)cb)->idx)->value();
								capture.input->set_data(&v);
							}
						}, new_mail(&capture));

						auto c_tracker = new_u_object<cEnumSingleDataTracker>();
						c_tracker->data = input->data();
						c_tracker->info = info;
						e_data->add_component(c_tracker);
					}
						break;
					case TypeTagAttributeEM:
					{
						auto v = *(int*)input->data();

						auto info = find_enum(bp->dbs(), type->hash());

						create_enum_checkboxs(info, app.font_atlas_pixel, 0.9f, e_data);
						for (auto k = 0; k < info->item_count(); k++)
						{
							auto item = info->item(k);

							struct Capture
							{
								BP::Slot* input;
								int v;
							}capture;
							capture.input = input;
							capture.v = item->value();
							e_data->child(k)->child(0)->get_component(cCheckbox)->data_changed_listeners.add([](void* c, Component* cb, uint hash, void*) {
								auto& capture = *(Capture*)c;
								if (hash == cH("checked"))
								{
									auto v = *(int*)capture.input->data();
									if (((cCheckbox*)cb)->checked)
										v |= capture.v;
									else
										v &= ~capture.v;
									capture.input->set_data(&v);
								}
							}, new_mail(&capture));
						}

						auto c_tracker = new_u_object<cEnumMultiDataTracker>();
						c_tracker->data = input->data();
						c_tracker->info = info;
						e_data->add_component(c_tracker);
					}
						break;
					case TypeTagAttributeV:
						switch (type->hash())
						{
						case cH("bool"):
						{
							auto e_checkbox = create_standard_checkbox();
							e_data->add_child(e_checkbox);

							e_checkbox->get_component(cCheckbox)->data_changed_listeners.add([](void* c, Component* cb, uint hash, void*) {
								if (hash == cH("checked"))
								{
									auto input = *(BP::Slot**)c;
									auto v = (((cCheckbox*)cb)->checked) ? 1 : 0;
									input->set_data(&v);
								}
							}, new_mail_p(input));

							auto c_tracker = new_u_object<cBoolDataTracker>();
							c_tracker->data = input->data();
							e_data->add_component(c_tracker);
						}
							break;
						case cH("int"):
							create_edit<int>(e_data, input);
							break;
						case cH("Vec(2+int)"):
							create_vec_edit<2, int>(e_data, input);
							break;
						case cH("Vec(3+int)"):
							create_vec_edit<3, int>(e_data, input);
							break;
						case cH("Vec(4+int)"):
							create_vec_edit<4, int>(e_data, input);
							break;
						case cH("uint"):
							create_edit<uint>(e_data, input);
							break;
						case cH("Vec(2+uint)"):
							create_vec_edit<2, uint>(e_data, input);
							break;
						case cH("Vec(3+uint)"):
							create_vec_edit<3, uint>(e_data, input);
							break;
						case cH("Vec(4+uint)"):
							create_vec_edit<4, uint>(e_data, input);
							break;
						case cH("float"):
							create_edit<float>(e_data, input);
							break;
						case cH("Vec(2+float)"):
							create_vec_edit<2, float>(e_data, input);
							break;
						case cH("Vec(3+float)"):
							create_vec_edit<3, float>(e_data, input);
							break;
						case cH("Vec(4+float)"):
							create_vec_edit<4, float>(e_data, input);
							break;
						case cH("uchar"):
							create_edit<uchar>(e_data, input);
							break;
						case cH("Vec(2+uchar)"):
							create_vec_edit<2, uchar>(e_data, input);
							break;
						case cH("Vec(3+uchar)"):
							create_vec_edit<3, uchar>(e_data, input);
							break;
						case cH("Vec(4+uchar)"):
							create_vec_edit<4, uchar>(e_data, input);
							break;
						case cH("std::basic_string(char)"):
						{
							auto e_edit = create_standard_edit(50.f, app.font_atlas_pixel, 0.9f);
							e_data->add_child(e_edit);
							e_edit->get_component(cText)->data_changed_listeners.add([](void* c, Component* t, uint hash, void*) {
								if (hash == cH("text"))
								{
									auto str = w2s(((cText*)t)->text());
									(*(BP::Slot**)c)->set_data(&str);
								}
							}, new_mail_p(input));

							auto c_tracker = new_u_object<cStringDataTracker>();
							c_tracker->data = input->data();
							e_data->add_component(c_tracker);
						}
							break;
						case cH("std::basic_string(wchar_t)"):
						{
							auto e_edit = create_standard_edit(50.f, app.font_atlas_pixel, 0.9f);
							e_data->add_child(e_edit);
							e_edit->get_component(cText)->data_changed_listeners.add([](void* c, Component* t, uint hash, void*) {
								if (hash == cH("text"))
									(*(BP::Slot**)c)->set_data(&((cText*)t)->text());
							}, new_mail_p(input));

							auto c_tracker = new_u_object<cWStringDataTracker>();
							c_tracker->data = input->data();
							e_data->add_component(c_tracker);
						}
							break;
						}
						break;
					}

					c_slot->tracker = e_data->get_component(cDataTracker);
				}
			}

			auto e_right = Entity::create();
			e_main->add_child(e_right);
			{
				e_right->add_component(cElement::create());

				e_right->add_component(cLayout::create(LayoutVertical));

				for (auto j = 0; j < n->output_count(); j++)
				{
					auto output = n->output(j);

					auto e_title = Entity::create();
					e_right->add_child(e_title);
					{
						e_title->add_component(cElement::create());

						auto c_aligner = cAligner::create();
						c_aligner->x_align_ = AlignxRight;
						e_title->add_component(c_aligner);

						e_title->add_component(cLayout::create(LayoutHorizontal));
					}

					auto e_text = Entity::create();
					e_title->add_child(e_text);
					{
						e_text->add_component(cElement::create());

						auto c_text = cText::create(app.font_atlas_pixel);
						c_text->set_text(s2w(output->vi()->name()));
						e_text->add_component(c_text);
					}

					auto e_slot = Entity::create();
					e_title->add_child(e_slot);
					{
						auto c_element = cElement::create();
						auto r = default_style.font_size;
						c_element->size_ = r;
						c_element->roundness = r * 0.5f;
						c_element->color = bp->find_output_export(output) != -1 ? Vec4c(200, 40, 20, 255) : Vec4c(200, 200, 200, 255);
						e_slot->add_component(c_element);

						e_slot->add_component(cEventReceiver::create());

						auto c_slot = new_u_object<cBPSlot>();
						c_slot->editor = this;
						c_slot->s = output;
						e_slot->add_component(c_slot);
						output->user_data = c_slot;
					}
				}
			}
		}
	}

	auto e_bring_to_front = Entity::create();
	e_node->add_child(e_bring_to_front);
	{
		e_bring_to_front->add_component(cElement::create());

		auto c_event_receiver = cEventReceiver::create();
		c_event_receiver->penetrable = true;
		e_bring_to_front->add_component(c_event_receiver);

		auto c_aligner = cAligner::create();
		c_aligner->width_policy_ = SizeFitParent;
		c_aligner->height_policy_ = SizeFitParent;
		e_bring_to_front->add_component(c_aligner);

		e_bring_to_front->add_component(cBringToFront::create());
	}

	return e_node;
}

void open_blueprint_editor(const std::wstring& filename, bool no_compile, const Vec2f& pos)
{
	auto e_container = get_docker_container_model()->copy();
	app.root->add_child(e_container);
	{
		auto c_element = e_container->get_component(cElement);
		c_element->pos_ = pos;
		c_element->size_.x() = 1483.f;
		c_element->size_.y() = 711.f;
	}

	auto e_docker = get_docker_model()->copy();
	e_container->add_child(e_docker, 0);

	e_docker->child(0)->add_child(create_standard_docker_tab(app.font_atlas_pixel, L"Blueprint Editor", app.root));

	auto e_page = get_docker_page_model()->copy();
	{
		auto c_layout = cLayout::create(LayoutVertical);
		c_layout->width_fit_children = false;
		c_layout->height_fit_children = false;
		c_layout->fence = 3;
		e_page->add_component(c_layout);

		e_page->add_component(cCustomDraw::create());
	}
	e_docker->child(1)->add_child(e_page);

	auto c_editor = new_u_object<cBPEditor>();
	e_page->add_component(c_editor);

	auto e_menubar = create_standard_menubar();
	e_page->add_child(e_menubar);
	{
		auto e_menu = create_standard_menu();
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Save");
			e_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;

				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					editor->bp->save_to_file(editor->bp, editor->filename);
				}
			}, new_mail_p(c_editor));
		}
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Reload");
			e_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;
				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);
					if (editor->running)
						editor->add_notification(L"Cannot Reload While Running");
					else
						editor->load(editor->filename, false);
				}
			}, new_mail_p(c_editor));
		}
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Reload (No Compile)");
			e_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;

				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					if (editor->running)
						editor->add_notification(L"Cannot Reload While Running");
					else
						editor->load(editor->filename, true);
				}
			}, new_mail_p(c_editor));
		}
		auto e_menu_btn = create_standard_menu_button(app.font_atlas_pixel, 1.f, L"Blueprint", app.root, e_menu, true, SideS, true, false, true, nullptr);
		e_menubar->add_child(e_menu_btn);
	}
	{
		auto e_menu = create_standard_menu();
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Module");
			e_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;
				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					if (editor->running)
						editor->add_notification(L"Cannot Add Module While Running");
					else
					{
						popup_input_dialog(editor->entity, L"module", [](void* c, bool ok, const std::wstring& text) {
							auto editor = *(cBPEditor**)c;
							auto bp = editor->bp;

							if (ok)
							{
								auto m = bp->add_module(text);
								if (!m)
									editor->add_notification(L"Add Module Failed");
								else
								{
									m->pos = editor->add_pos;
									editor->create_module_entity(m);
									editor->refresh_add_node_menu();
								}
							}
						}, new_mail_p(editor));
					}
				}
			}, new_mail_p(c_editor));
		}
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Package");
			e_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;
				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					if (editor->running)
						editor->add_notification(L"Cannot Add Package While Running");
					else
					{
						popup_input_dialog(editor->entity, L"bp", [](void* c, bool ok, const std::wstring& text) {
							auto editor = *(cBPEditor**)c;
							auto bp = editor->bp;

							if (ok)
							{
								auto p = bp->add_package(text, "");
								if (!p)
									editor->add_notification(L"Add Package Failed");
								else
								{
									p->pos = editor->add_pos;
									editor->create_package_entity(p);
									editor->refresh_add_node_menu();
								}
							}
						}, new_mail_p(editor));
					}
				}
			}, new_mail_p(c_editor));
		}
		{
			auto e_sub_menu = create_standard_menu();
			c_editor->e_add_node_menu = e_sub_menu;
			e_menu->add_child(create_standard_menu_button(app.font_atlas_pixel, 1.f, L"Nodes", app.root, e_sub_menu, true, SideE, false, true, false, Icon_CARET_RIGHT));
		}
		auto e_menu_btn = create_standard_menu_button(app.font_atlas_pixel, 1.f, L"Add", app.root, e_menu, true, SideS, true, false, true, nullptr);
		e_menubar->add_child(e_menu_btn);
		struct Capture
		{
			cBPEditor* e;
			cMenuButton* b;
		}capture;
		capture.e = c_editor;
		capture.b = e_menu_btn->get_component(cMenuButton);
		e_menu_btn->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
			auto& capture = *(Capture*)c;

			if (capture.b->can_open(action, key))
			{
				auto base = capture.e->e_base;
				capture.e->set_add_pos_center();
			}
		}, new_mail(&capture));
	}
	{
		auto e_menu = create_standard_menu();
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Duplicate");
			e_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;

				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					editor->set_add_pos_center();
					editor->duplicate_selected();
				}
			}, new_mail_p(c_editor));
		}
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Delete");
			e_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;

				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					editor->delete_selected();
				}
			}, new_mail_p(c_editor));
		}
		auto e_menu_btn = create_standard_menu_button(app.font_atlas_pixel, 1.f, L"Edit", app.root, e_menu, true, SideS, true, false, true, nullptr);
		e_menubar->add_child(e_menu_btn);
	}
	{
		auto e_menu = create_standard_menu();
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Generate Graph Image");
			e_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;

				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					editor->generate_graph_image();
				}
			}, new_mail_p(c_editor));
		}
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Auto Set Layout");
			e_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;

				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					editor->auto_set_layout();
				}
			}, new_mail_p(c_editor));
		}
		{
			auto e_item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Link Test Nodes");
			e_menu->add_child(e_item);
			e_item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;

				if (is_mouse_clicked(action, key))
				{
					destroy_topmost(app.root);

					editor->link_test_nodes();
				}
			}, new_mail_p(c_editor));
		}
		auto e_menu_btn = create_standard_menu_button(app.font_atlas_pixel, 1.f, L"Tools", app.root, e_menu, true, SideS, true, false, true, nullptr);
		e_menubar->add_child(e_menu_btn);
	}

	auto e_clipper = Entity::create();
	e_page->add_child(e_clipper);
	{
		auto c_element = cElement::create();
		c_element->clip_children = true;
		e_clipper->add_component(c_element);

		auto c_aligner = cAligner::create();
		c_aligner->width_policy_ = SizeFitParent;
		c_aligner->height_policy_ = SizeFitParent;
		e_clipper->add_component(c_aligner);

		e_clipper->add_component(cLayout::create(LayoutFree));
	}

	auto e_scene = Entity::create();
	e_clipper->add_child(e_scene);
	{
		e_scene->add_component(cElement::create());

		e_scene->add_component(cEventReceiver::create());

		auto c_aligner = cAligner::create();
		c_aligner->width_policy_ = SizeFitParent;
		c_aligner->height_policy_ = SizeFitParent;
		e_scene->add_component(c_aligner);

		e_scene->add_component(cLayout::create(LayoutFree));

		e_scene->add_component(cCustomDraw::create());

		auto c_bp = new_u_object<cBP>();
		c_bp->editor = c_editor;
		e_scene->add_component(c_bp);
	}

	auto c_bp = e_scene->get_component(cBP);

	auto e_base = Entity::create();
	e_scene->add_child(e_base);
	{
		auto c_element = cElement::create();
		e_base->add_component(c_element);
		c_bp->base_element = c_element;
	}
	c_editor->e_base = e_base;

	{
		auto e_menu = create_standard_menu();
		c_editor->e_slot_menu = e_menu;
		{
			auto item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Add To Exports");
			e_menu->add_child(item);
			item->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto editor = *(cBPEditor**)c;
				if (is_mouse_down(action, key, true) && key == Mouse_Left)
				{
					destroy_topmost(app.root);
					auto s = editor->selected_.s;
					if (s->type() == BP::Slot::Input)
						editor->bp->add_input_export(s);
					else
						editor->bp->add_output_export(s);
					((cBPSlot*)s->user_data)->element->color = Vec4c(200, 40, 20, 255);
				}
			}, new_mail_p(c_editor));
		}
		{
			auto item = create_standard_menu_item(app.font_atlas_pixel, 1.f, L"Remove From Exports");
			e_menu->add_child(item);
		}
	}

	auto e_overlayer = Entity::create();
	e_scene->add_child(e_overlayer);
	{
		e_overlayer->add_component(cElement::create());

		auto c_event_receiver = cEventReceiver::create();
		c_event_receiver->penetrable = true;
		c_event_receiver->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
			auto c_bp = *(cBP**)c;
			if (is_mouse_scroll(action, key))
			{
				auto s = clamp(c_bp->base_element->scale_ + (pos.x() > 0.f ? 0.1f : -0.1f), 0.1f, 2.f);
				c_bp->base_element->set_scale(s);
				c_bp->scale_text->set_text(std::to_wstring(int(s * 100)) + L"%");
			}
			else if (is_mouse_move(action, key))
			{
				auto ed = c_bp->event_receiver->dispatcher;
				if ((ed->key_states[Key_Ctrl] & KeyStateDown) && (ed->mouse_buttons[Mouse_Left] & KeyStateDown))
					c_bp->base_element->set_pos(Vec2f(pos), true);
			}
		}, new_mail_p(c_bp));
		e_overlayer->add_component(c_event_receiver);

		auto c_aligner = cAligner::create();
		c_aligner->width_policy_ = SizeFitParent;
		c_aligner->height_policy_ = SizeFitParent;
		e_overlayer->add_component(c_aligner);
	}

	c_editor->load(filename, no_compile);

	auto e_run = create_standard_button(app.font_atlas_pixel, 1.f, L"Run");;
	e_clipper->add_child(e_run);
	{
		auto c_event_receiver = e_run->get_component(cEventReceiver);
		struct Capture
		{
			cBPEditor* e;
			cText* t;
		}capture;
		capture.e = c_editor;
		capture.t = e_run->get_component(cText);
		c_event_receiver->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
			auto& capture = *(Capture*)c;

			if (is_mouse_clicked(action, key))
			{
				capture.e->running = !capture.e->running;
				capture.t->set_text(capture.e->running ? L"Pause" : L"Run");

				if (capture.e->running)
					capture.e->bp->time = 0.f;
			}
		}, new_mail(&capture));
	}

	auto e_scale = Entity::create();
	{
		e_scale->add_component(cElement::create());
		
		auto c_text = cText::create(app.font_atlas_pixel);
		c_text->set_text(L"100%");
		e_scale->add_component(c_text);

		auto c_aligner = cAligner::create();
		c_aligner->x_align_ = AlignxLeft;
		c_aligner->y_align_ = AlignyBottom;
		e_scale->add_component(c_aligner);
	}
	e_clipper->add_child(e_scale);

	c_bp->scale_text = e_scale->get_component(cText);

	auto console_page = open_console([](void* c, const std::wstring& cmd, cConsole* console) {
		auto editor = *(cBPEditor**)c;
		auto& filename = editor->filename;
		auto bp = editor->bp;
		auto& dbs = editor->bp->dbs();
		auto tokens = string_split(cmd);

		if (editor->locked)
		{
			console->print(L"bp is locked");
			return;
		}

		auto set_data = [&](const std::string& address, const std::string& value) {
			auto i = bp->find_input(address.c_str());
			if (i)
			{
				auto v = i->vi();
				auto type = v->type();
				auto value_before = serialize_value(dbs, type->tag(), type->hash(), i->raw_data(), 2);
				auto data = new char[v->size()];
				unserialize_value(dbs, type->tag(), type->hash(), value, data);
				i->set_data((char*)data + sizeof(AttributeBase));
				((cBPSlot*)i->user_data)->tracker->update_view();
				delete data;
				auto value_after = serialize_value(dbs, type->tag(), type->hash(), i->raw_data(), 2);
				console->print(L"set value: " + s2w(address) + L", " + s2w(*value_before.p) + L" -> " + s2w(*value_after.p));
				delete_mail(value_before);
				delete_mail(value_after);
			}
			else
				console->print(L"input not found");
		};

		if (tokens[0] == L"help")
		{
			console->print(
				L"  help - show this help\n"
				"  show udts - show all available udts (see blueprint.h for more details)\n"
				"  show udt [udt_name] - show an udt\n"
				"  show nodes - show all nodes\n"
				"  show node [id] - show a node\n"
				"  show graph - use Graphviz to show graph\n"
				"  add node [type_name] [id] - add a node (id of '-' means don't care)\n"
				"  add link [out_adress] [in_adress] - add a link\n"
				"  remove node [id] - remove a node\n"
				"  remove link [in_adress] - remove a link\n"
				"  set [in_adress] [value] - set value for input\n"
				"  update - update this blueprint\n"
				"  save - save this blueprint\n"
				"  auto-set-layout - set nodes' positions using 'bp.png' and 'bp.graph.txt', need do show graph first"
			);
		}
		else if (tokens[0] == L"show")
		{
			if (tokens[1] == L"udts")
			{
				std::vector<UdtInfo*> all_udts;
				for (auto db : dbs)
				{
					auto udts = db->get_udts();
					for (auto i = 0; i < udts.p->size(); i++)
						all_udts.push_back(udts.p->at(i));
					delete_mail(udts);
				}
				std::sort(all_udts.begin(), all_udts.end(), [](UdtInfo* a, UdtInfo* b) {
					return a->name() < b->name();
				});
				for (auto udt : all_udts)
					console->print(s2w(udt->name()));
			}
			else if (tokens[1] == L"udt")
			{
				auto udt = find_udt(dbs, H(w2s(tokens[2]).c_str()));
				if (udt)
				{
					console->print(s2w(udt->name()));
					std::vector<VariableInfo*> inputs;
					std::vector<VariableInfo*> outputs;
					for (auto i_i = 0; i_i < udt->variable_count(); i_i++)
					{
						auto vari = udt->variable(i_i);
						auto attribute = std::string(vari->decoration());
						if (attribute.find('i') != std::string::npos)
							inputs.push_back(vari);
						if (attribute.find('o') != std::string::npos)
							outputs.push_back(vari);
					}
					console->print(L"[In]");
					for (auto& i : inputs)
						console->print(L"name:" + s2w(i->name()) + L" decoration:" + s2w(i->decoration()) + L" tag:" + s2w(get_name(i->type()->tag())) + L" type:" + s2w(i->type()->name()));
					console->print(L"[Out]");
					for (auto& o : outputs)
						console->print(L"name:" + s2w(o->name()) + L" decoration:" + s2w(o->decoration()) + L" tag:" + s2w(get_name(o->type()->tag())) + L" type:" + s2w(o->type()->name()));
				}
				else
					console->print(L"udt not found");
			}
			else if (tokens[1] == L"nodes")
			{
				for (auto i = 0; i < bp->node_count(); i++)
				{
					auto n = bp->node(i);
					console->print(L"id:" + s2w(n->id()) + L" type:" + s2w(n->udt()->name()));
				}
			}
			else if (tokens[1] == L"node")
			{
				auto n = bp->find_node(w2s(tokens[2]).c_str());
				if (n)
				{
					console->print(L"[In]");
					for (auto i = 0; i < n->input_count(); i++)
					{
						auto input = n->input(i);
						auto v = input->vi();
						auto type = v->type();
						console->print(s2w(v->name()));
						Mail<std::string> link_address;
						if (input->link())
							link_address = input->link()->get_address();
						console->print(L"[" + (link_address.p ? s2w(*link_address.p) : L"") + L"]");
						delete_mail(link_address);
						auto str = serialize_value(dbs, type->tag(), type->hash(), input->raw_data(), 2);
						console->print(std::wstring(L"   ") + (str.p->empty() ? L"-" : s2w(*str.p)));
						delete_mail(str);
					}
					console->print(L"[Out]");
					for (auto i = 0; i < n->output_count(); i++)
					{
						auto output = n->output(i);
						auto v = output->vi();
						auto type = v->type();
						console->print(s2w(v->name()));
						auto str = serialize_value(dbs, type->tag(), type->hash(), output->raw_data(), 2);
						console->print(std::wstring(L"   ") + (str.p->empty() ? L"-" : s2w(*str.p)));
						delete_mail(str);
					}
				}
				else
					console->print(L"node not found");
			}
			else if (tokens[1] == L"graph")
			{
				if (!editor->generate_graph_image())
				{
					exec(L"bp.png", L"", false);
					console->print(L"ok");
				}
				else
					console->print(L"bp.png not found, perhaps Graphviz is not available");
			}
			else
				console->print(L"unknow object to show");
		}
		else if (tokens[0] == L"add")
		{
			if (tokens[1] == L"node")
			{
				auto n = editor->add_node(w2s(tokens[2]), tokens[3] == L"-" ? "" : w2s(tokens[3]), Vec2f(0.f));
				if (n)
					console->print(L"node added: " + s2w(n->id()));
				else
					console->print(L"bad udt name or id already exist");
			}
			else if (tokens[1] == L"link")
			{
				auto out = bp->find_output(w2s(tokens[2]));
				auto in = bp->find_input(w2s(tokens[3]));
				if (out && in)
				{
					in->link_to(out);
					auto out_addr = in->link()->get_address();
					auto in_addr = in->get_address();
					console->print(L"link added: " + s2w(*out_addr.p) + L" -> " + s2w(*in_addr.p));
					delete_mail(out_addr);
					delete_mail(in_addr);
				}
				else
					console->print(L"wrong address");
			}
			else
				console->print(L"unknow object to add");
		}
		else if (tokens[0] == L"remove")
		{
			if (tokens[1] == L"node")
			{
				auto n = bp->find_node(w2s(tokens[2]));
				if (n)
				{
					if (!editor->remove_node(n))
						printf("cannot remove test nodes\n");
					else
						console->print(L"node removed: " + tokens[2]);
				}
				else
					console->print(L"node not found");
			}
			else if (tokens[1] == L"link")
			{
				auto i = bp->find_input(w2s(tokens[3]));
				if (i)
				{
					i->link_to(nullptr);
					console->print(L"link removed: " + tokens[2]);
				}
				else
					console->print(L"input not found");
			}
			else
				console->print(L"unknow object to remove");
		}
		else if (tokens[0] == L"set")
			set_data(w2s(tokens[1]), w2s(tokens[2]));
		else if (tokens[0] == L"update")
		{
			bp->update();
			console->print(L"BP updated");
		}
		else if (tokens[0] == L"save")
		{
			BP::save_to_file(bp, filename);
			console->print(L"file saved");
		}
		else if (tokens[0] == L"auto-set-layout")
		{
			if (editor->auto_set_layout())
				console->print(L"ok");
			else
				console->print(L"bp.graph.txt not found");
		}
		else
			console->print(L"unknow command");
	}, new_mail_p(c_editor), [](void* c) {
		auto editor = *(cBPEditor**)c;
		editor->console_tab = nullptr;
	}, new_mail_p(c_editor), filename + L":", Vec2f(1495.f, 10.f));
	c_editor->console_tab = console_page->parent()->parent()->child(0)->child(0)->get_component(cDockerTab);
}

#include <flame/universe/utils/typeinfo.h>
#include "app.h"

static Vec4c unselected_col = Vec4c(0, 0, 0, 255);
static Vec4c selected_col = Vec4c(0, 0, 145, 255);

static const wchar_t* type_prefix(TypeTag t, bool is_array = false)
{
	switch (t)
	{
	case TypeEnumSingle:
		return L"Enum Single\n";
	case TypeEnumMulti:
		return L"Enum Multi\n";
	case TypeData:
		return is_array ? L"Array\n" : L"Data\n";
	case TypePointer:
		return is_array ? L"Array Pointer\n" : L"Pointer\n";
	}
	return L"";
}

static Vec4c type_color(TypeTag t)
{
	switch (t)
	{
	case TypeEnumSingle:
		return Vec4c(23, 160, 93, 255);
	case TypeEnumMulti:
		return Vec4c(23, 160, 93, 255);
	case TypeData:
		return Vec4c(40, 58, 228, 255);
	case TypePointer:
		return Vec4c(239, 94, 41, 255);
	}
	return Vec4c(0);
}

const auto slot_bezier_extent = 50.f;

struct cSlot : Component
{
	cElement* element;
	cEventReceiver* event_receiver;
	cDataTracker* tracker;

	BP::Slot* s;

	bool dragging;

	Entity* tip_info;
	Entity* tip_link;

	cSlot() :
		Component("cSlot")
	{
		element = nullptr;
		event_receiver = nullptr;
		tracker = nullptr;

		dragging = false;

		tip_info = nullptr;
		tip_link = nullptr;
	}

	~cSlot() override
	{
		clear_tips();
	}

	void clear_tips()
	{
		if (tip_info)
		{
			auto e = tip_info;
			tip_info = nullptr;
			looper().add_event([](void* c, bool*) {
				auto e = *(Entity**)c;
				e->parent()->remove_child(e);
			}, Mail::from_p(e));
		}
	}

	void on_component_added(Component* c) override
	{
		if (c->name_hash == FLAME_CHASH("cElement"))
			element = (cElement*)c;
		else if (c->name_hash == FLAME_CHASH("cEventReceiver"))
		{
			event_receiver = (cEventReceiver*)c;
			if (s->io() == BP::Slot::In)
			{
				event_receiver->drag_hash = FLAME_CHASH("input_slot");
				event_receiver->set_acceptable_drops(1, &FLAME_CHASH("output_slot"));
			}
			else
			{
				event_receiver->drag_hash = FLAME_CHASH("output_slot");
				event_receiver->set_acceptable_drops(1, &FLAME_CHASH("input_slot"));
			}
			event_receiver->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
				auto thiz = *(cSlot**)c;
				if (thiz->dragging)
				{
					if (is_mouse_scroll(action, key) || is_mouse_move(action, key))
						app.editor->edt.event_receiver->on_mouse(action, key, pos);
				}
				return true;
			}, Mail::from_p(this));
			event_receiver->drag_and_drop_listeners.add([](void* c, DragAndDrop action, cEventReceiver* er, const Vec2i& pos) {
				auto thiz = *(cSlot**)c;
				auto element = thiz->element;
				auto s = thiz->s;
				auto is_in = s->io() == BP::Slot::In;

				if (action == DragStart)
				{
					thiz->dragging = true;
					app.editor->dragging_slot = s;
					app.deselect();
				}
				else if (action == DragOvering)
				{
					app.editor->dragging_slot_pos = Vec2f(pos);
					app.s_2d_renderer->pending_update = true;
				}
				else if (action == DragEnd)
				{
					thiz->dragging = false;
					if (!er)
						app.editor->show_add_node_menu(Vec2f(pos));
					else
						app.editor->dragging_slot = nullptr;
					app.s_2d_renderer->pending_update = true;
				}
				else if (action == BeingOverStart)
				{
					auto oth = er->entity->get_component(cSlot)->s;
					auto ok = (is_in ? BP::Slot::can_link(s->type(), oth->type()) : BP::Slot::can_link(oth->type(), s->type()));

					element->set_frame_color(ok ? Vec4c(0, 255, 0, 255) : Vec4c(255, 0, 0, 255));
					element->set_frame_thickness(2.f);

					if (!thiz->tip_link)
					{
						utils::push_parent(app.root);
						thiz->tip_link = utils::e_begin_layout(LayoutVertical, 4.f);
						auto c_element = thiz->tip_link->get_component(cElement);
						c_element->pos = thiz->element->global_pos + Vec2f(thiz->element->global_size.x(), -8.f);
						c_element->pivot = Vec2f(0.5f, 1.f);
						c_element->padding = 4.f;
						c_element->frame_thickness = 2.f;
						c_element->color = Vec4c(200, 200, 200, 255);
						c_element->frame_color = Vec4c(0, 0, 0, 255);
						{
							auto s_type = s->type();
							auto o_type = oth->type();
							std::wstring str;
							if (is_in)
								str = s2w(o_type->name()) + (ok ? L"  =>  " : L"  ��>  ") + s2w(s_type->name());
							else
								str = s2w(s_type->name()) + (ok ? L"  =>  " : L"  ��>  ") + s2w(o_type->name());
							utils::e_text(str.c_str())->get_component(cText)->color_ = ok ? Vec4c(0, 128, 0, 255) : Vec4c(255, 0, 0, 255);
						}
						utils::e_end_layout();
						utils::pop_parent();
					}
				}
				else if (action == BeingOverEnd)
				{
					element->set_frame_thickness(0.f);

					if (thiz->tip_link)
					{
						auto e = thiz->tip_link;
						thiz->tip_link = nullptr;
						looper().add_event([](void* c, bool*) {
							auto e = *(Entity**)c;
							e->parent()->remove_child(e);
						}, Mail::from_p(e));
					}
				}
				else if (action == BeenDropped)
				{
					auto oth = er->entity->get_component(cSlot)->s;
					if (s->io() == BP::Slot::In)
						app.set_links({ {s, oth} });
					else
						app.set_links({ {oth, s} });
				}

				return true;
			}, Mail::from_p(this));
			event_receiver->hover_listeners.add([](void* c, bool hovering) {
				auto thiz = *(cSlot**)c;
				auto s = thiz->s;
				auto is_in = s->io() == BP::Slot::In;

				if (!hovering)
					thiz->clear_tips();
				else
				{
					if (!thiz->tip_info)
					{
						utils::push_parent(app.root);
							thiz->tip_info = utils::e_begin_layout(LayoutVertical, 8.f);
							auto c_element = thiz->tip_info->get_component(cElement);
							c_element->pos = thiz->element->global_pos + Vec2f(is_in ? -8.f : thiz->element->global_size.x() + 8.f, 0.f);
							c_element->pivot = Vec2f(is_in ? 1.f : 0.f , 0.f);
							c_element->padding = 4.f;
							c_element->frame_thickness = 2.f;
							c_element->color = Vec4c(200, 200, 200, 255);
							c_element->frame_color = Vec4c(0, 0, 0, 255);

							auto type = s->type();
							auto tag = type->tag();
							utils::e_text((type_prefix(tag, type->is_array()) + s2w(type->base_name())).c_str())->get_component(cText)->color_ = type_color(tag);
							auto fail_message = s2w(s->fail_message());
							if (!fail_message.empty())
								utils::e_text(fail_message.c_str())->get_component(cText)->color_ = Vec4c(255, 0, 0, 255);

							utils::e_end_layout();
						utils::pop_parent();
					}
				}

				return true;
			}, Mail::from_p(this));
		}
	}
};

static const wchar_t* node_type_prefix(char t)
{
	switch (t)
	{
	case 'S':
		return type_prefix(TypeEnumSingle);
	case 'M':
		return type_prefix(TypeEnumMulti);
	case 'V':
		return L"Variable\n";
	case 'A':
		return L"Array\n";
	}
	return L"";
}

static Vec4c node_type_color(char t)
{
	switch (t)
	{
	case 'S':
		return type_color(TypeEnumSingle);
	case 'M':
		return type_color(TypeEnumMulti);
	case 'V':
		return Vec4c(0, 128, 0, 255);
	case 'A':
		return Vec4c(0, 0, 255, 255);
	}
	return Vec4c(128, 60, 220, 255);
}

struct cNode : Component
{
	cElement* element;
	cEventReceiver* event_receiver;

	BP::Node* n;
	char n_type;
	std::wstring n_name;

	bool moved;

	Entity* tip;

	cNode() :
		Component("cNode")
	{
		moved = false;

		tip = nullptr;
	}

	~cNode() override
	{
		clear_tips();
	}

	void clear_tips()
	{
		if (tip)
		{
			auto e = tip;
			tip = nullptr;
			looper().add_event([](void* c, bool*) {
				auto e = *(Entity**)c;
				e->parent()->remove_child(e);
			}, Mail::from_p(e));
		}
	}

	void on_component_added(Component* c) override
	{
		if (c->name_hash == FLAME_CHASH("cElement"))
			element = (cElement*)c;
		else if (c->name_hash == FLAME_CHASH("cEventReceiver"))
		{
			event_receiver = (cEventReceiver*)c;
			event_receiver->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
				auto thiz = *(cNode**)c;
				if (is_mouse_down(action, key, true) && key == Mouse_Left)
				{
					auto n = thiz->n;
					for (auto& s : app.selected_nodes)
					{
						if (n == s)
						{
							n = nullptr;
							break;
						}
					}
					if (n)
						app.select({ n });
				}
				else if (is_mouse_move(action, key) && utils::is_active(thiz->event_receiver))
				{
					for (auto& s : app.selected_nodes)
					{
						auto e = ((Entity*)s->user_data)->get_component(cElement);
						e->add_pos((Vec2f)pos / e->global_scale);
					}
					thiz->moved = true;
				}
				return true;
			}, Mail::from_p(this));
			event_receiver->hover_listeners.add([](void* c, bool hovering) {
				auto thiz = *(cNode**)c;
				if (!hovering)
					thiz->clear_tips();
				else
				{
					if (!thiz->tip)
					{
						utils::push_parent(app.root);
							thiz->tip = utils::e_begin_layout(LayoutVertical, 4.f);
							auto c_element = thiz->tip->get_component(cElement);
							c_element->pos = thiz->element->global_pos - Vec2f(0.f, 8.f);
							c_element->pivot = Vec2f(0.f, 1.f);
							c_element->padding = 4.f;
							c_element->frame_thickness = 2.f;
							c_element->color = Vec4c(200, 200, 200, 255);
							c_element->frame_color = Vec4c(0, 0, 0, 255);
								auto n = thiz->n;
								std::wstring str;
								auto udt = n->udt();
								if (udt)
									str = L"UDT (" + std::wstring(udt->db()->module_name()) + L")\n" + thiz->n_name;
								else
									str = node_type_prefix(thiz->n_type) + thiz->n_name;
								str += L"\nID: " + s2w(n->id());
								utils::e_text(str.c_str())->get_component(cText)->color_ = node_type_color(thiz->n_type);
							utils::e_end_layout();
						utils::pop_parent();
					}
				}
				return true;
			}, Mail::from_p(this));
			event_receiver->state_listeners.add([](void* c, EventReceiverState) {
				auto thiz = *(cNode**)c;
				if (thiz->moved && !utils::is_active(thiz->event_receiver))
				{
					std::vector<Vec2f> poses;
					for (auto& s : app.selected_nodes)
						poses.push_back(((Entity*)s->user_data)->get_component(cElement)->pos);
					app.set_nodes_pos(app.selected_nodes, poses);
					thiz->moved = false;
				}
				return true;
			}, Mail::from_p(this));
		}
	}
};

cEditor::cEditor() :
	Component("cEditor")
{
	auto e_page = utils::e_begin_docker_page(L"Editor").second;
	{
		auto c_layout = utils::c_layout(LayoutVertical);
		c_layout->width_fit_children = false;
		c_layout->height_fit_children = false;
		c_layout->fence = 2;
	}
	e_page->add_component(this);

	edt.create([](void*, const Vec4f& r) {
		if (r.x() == r.z() && r.y() == r.z())
			app.deselect();
		else
		{
			std::vector<BP::Node*> nodes;
			for (auto i = 0; i < app.bp->node_count(); i++)
			{
				auto n = app.bp->node(i);
				auto e = ((Entity*)n->user_data)->get_component(cElement);
				if (rect_overlapping(r, rect(e->global_pos, e->global_size)))
					nodes.push_back(n);
			}
			if (!nodes.empty())
			{
				app.select(nodes);
				return;
			}

			std::vector<BP::Slot*> links;

			Vec2f lines[8];
			lines[0] = Vec2f(r.x(), r.y());
			lines[1] = Vec2f(r.z(), r.y());
			lines[2] = Vec2f(r.x(), r.y());
			lines[3] = Vec2f(r.x(), r.w());
			lines[4] = Vec2f(r.z(), r.w());
			lines[5] = Vec2f(r.x(), r.w());
			lines[6] = Vec2f(r.z(), r.w());
			lines[7] = Vec2f(r.z(), r.y());

			auto& edt = app.editor->edt;

			auto range = rect(edt.element->global_pos, edt.element->global_size);
			auto scale = edt.base->global_scale;
			auto extent = slot_bezier_extent * scale;
			for (auto i = 0; i < app.bp->node_count(); i++)
			{
				auto n = app.bp->node(i);
				for (auto j = 0; j < n->input_count(); j++)
				{
					auto input = n->input(j);
					auto output = input->link();
					if (!output)
						continue;
					auto p1 = ((cSlot*)output->user_data)->element->center();
					auto p4 = ((cSlot*)input->user_data)->element->center();
					auto p2 = p1 + Vec2f(extent, 0.f);
					auto p3 = p4 - Vec2f(extent, 0.f);
					auto bb = rect_from_points(p1, p2, p3, p4);
					if (rect_overlapping(bb, range))
					{
						std::vector<Vec2f> points;
						path_bezier(points, p1, p2, p3, p4);
						auto ok = false;
						for (auto k = 0; k < points.size() - 1; k++)
						{
							for (auto m = 0; m < 8; m += 2)
							{
								if (segment_intersect(lines[m], lines[m + 1], points[k], points[k + 1]))
								{
									links.push_back(input);
									ok = true;
									break;
								}
								if (ok)
									break;
							}
							if (ok)
								break;
						}
					}
				}
			}

			if (!links.empty())
				app.select(links);
		}
	}, Mail());
	edt.element->cmds.add([](void*, graphics::Canvas* canvas) {
		auto& edt = app.editor->edt;

		auto scale = edt.base->global_scale;
		auto extent = slot_bezier_extent * scale;
		auto range = rect(edt.element->global_pos, edt.element->global_size);
		auto line_width = 3.f * scale;
		for (auto i = 0; i < app.bp->node_count(); i++)
		{
			auto n = app.bp->node(i);
			for (auto j = 0; j < n->input_count(); j++)
			{
				auto input = n->input(j);
				auto output = input->link();
				if (!output)
					continue;
				auto e1 = ((cSlot*)output->user_data)->element;
				auto e2 = ((cSlot*)input->user_data)->element;
				if (e1->entity->global_visibility && e2->entity->global_visibility)
				{
					auto p1 = e1->center();
					auto p4 = e2->center();
					auto p2 = p1 + Vec2f(extent, 0.f);
					auto p3 = p4 - Vec2f(extent, 0.f);
					auto bb = rect_from_points(p1, p2, p3, p4);
					if (rect_overlapping(bb, range))
					{
						std::vector<Vec2f> points;
						path_bezier(points, p1, p2, p3, p4);
						auto selected = false;
						for (auto& s : app.selected_links)
						{
							if (s == input)
							{
								selected = true;
								break;
							}
						}
						canvas->stroke(points.size(), points.data(), selected ? selected_col : Vec4c(100, 100, 120, 255), line_width);
					}
				}
			}
		}
		auto ds = app.editor->dragging_slot;
		if (ds)
		{
			auto e1 = ((cSlot*)ds->user_data)->element;
			if (e1->entity->global_visibility)
			{
				auto p1 = e1->center();
				auto p4 = app.editor->dragging_slot_pos;
				auto is_in = ds->io() == BP::Slot::In;
				auto p2 = p1 + Vec2f(is_in ? -extent : extent, 0.f);
				auto p3 = p4 + Vec2f(is_in ? extent : -extent, 0.f);
				auto bb = rect_from_points(p1, p2, p3, p4);
				if (rect_overlapping(bb, range))
				{
					std::vector<Vec2f> points;
					path_bezier(points, p1, p2, p3, p4);
					canvas->stroke(points.size(), points.data(), selected_col, line_width);
				}
			}
		}
		return true;
	}, Mail());
	edt.event_receiver->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
		if (is_mouse_up(action, key, true) && key == Mouse_Right)
		{
			if (!app.editor->edt.moved)
				app.editor->show_add_node_menu(Vec2f(pos));
		}
		return true;
	}, Mail());

	utils::e_end_docker_page();

	dragging_slot = nullptr;

	for (auto i = 0; i < app.bp->node_count(); i++)
		on_add_node(app.bp->node(i));
}

cEditor::~cEditor()
{
	app.editor = nullptr;
}

void cEditor::on_deselect()
{
	for (auto& s : app.selected_nodes)
	{
		auto e = (Entity*)s->user_data;
		if (e)
			e->get_component(cElement)->set_frame_color(unselected_col);
	}
}

void cEditor::on_select()
{
	for (auto& s : app.selected_nodes)
	{
		auto e = (Entity*)s->user_data;
		if (e)
			e->get_component(cElement)->set_frame_color(selected_col);
	}
}

void cEditor::on_id_changed(BP::Node* n)
{
	((Entity*)n->user_data)->get_component(cNode)->clear_tips();
}

void cEditor::on_pos_changed(BP::Node* n)
{
	((Entity*)n->user_data)->get_component(cElement)->set_pos(n->pos);
}

template <class T>
void create_edit(cEditor* editor, BP::Slot* input)
{
	utils::e_drag_edit();

	utils::current_parent()->add_component(new_object<cDigitalDataTracker<T>>(input->data(), [](void* c, T v, bool exit_editing) {
		if (exit_editing)
			app.set_data(*(BP::Slot**)c, &v, true);
		else
			(*(BP::Slot**)c)->set_data(&v);
	}, Mail::from_p(input)));
}

template <uint N, class T>
void create_vec_edit(cEditor* editor, BP::Slot* input)
{
	for (auto i = 0; i < N; i++)
	{
		utils::e_begin_layout(LayoutHorizontal, 4.f);
		utils::e_drag_edit();
		utils::e_text(s2w(Vec<N, T>::coord_name(i)).c_str());
		utils::e_end_layout();
	}

	utils::current_parent()->add_component(new_object<cDigitalVecDataTracker<N, T>>(input->data(), [](void* c, const Vec<N, T>& v, bool exit_editing) {
		if (exit_editing)
			app.set_data(*(BP::Slot**)c, (void*)&v, true);
		else
			(*(BP::Slot**)c)->set_data((void*)&v);
	}, Mail::from_p(input)));
}

void cEditor::on_add_node(BP::Node* n)
{
	auto e_node = Entity::create();
	utils::set_current_entity(e_node);
	n->user_data = e_node;
	{
		auto c_element = utils::c_element();
		c_element->pos = n->pos;
		c_element->roundness = 8.f;
		c_element->roundness_lod = 2;
		c_element->frame_thickness = 4.f;
		c_element->color = Vec4c(255, 255, 255, 200);
		c_element->frame_color = unselected_col;
		utils::c_event_receiver();
		utils::c_layout(LayoutVertical)->fence = 1;
	}
		auto c_node = new_object<cNode>();
		c_node->n = n;
		{
			std::string parameters;
			c_node->n_type = BP::type_from_node_name(n->type(), parameters);
			c_node->n_name = c_node->n_type ? s2w(parameters) : s2w(n->type());
		}
		e_node->add_component(c_node);
		utils::e_begin_popup_menu(false);
			utils::e_menu_item(L"Change ID", [](void* c) {
				auto n = *(BP::Node**)c;
				utils::e_input_dialog(L"ID", [](void* c, bool ok, const wchar_t* text) {
					if (ok && text[0])
						app.set_node_id(*(BP::Node**)c, w2s(text));
				}, Mail::from_p(n), s2w(n->id()).c_str());
			}, Mail::from_p(n));
			utils::e_menu_item(L"Duplicate", [](void* c) {
			}, Mail::from_p(n));
			utils::e_menu_item(L"Delete", [](void* c) {
			}, Mail::from_p(n));
		utils::e_end_popup_menu();
	utils::push_parent(e_node);
		utils::e_begin_layout(LayoutVertical, 4.f)->get_component(cElement)->padding = Vec4f(8.f);
			utils::push_style_1u(utils::FontSize, 20);
			utils::e_begin_layout(LayoutHorizontal, 4.f);
				if (c_node->n_type == 0)
				{
					auto str = s2w(n->type());
					auto last_colon = str.find_last_of(L':');
					if (last_colon != std::wstring::npos)
						str = std::wstring(str.begin() + last_colon + 1, str.end());
					auto e_text = utils::e_text(str.c_str());
					e_text->get_component(cElement)->padding = Vec4f(4.f, 2.f, 4.f, 2.f);
					e_text->get_component(cText)->color_ = node_type_color(c_node->n_type);
				}
			utils::e_end_layout();
			utils::pop_style(utils::FontSize);

			std::string type = n->type();

			utils::e_begin_layout(LayoutHorizontal, 16.f);
			utils::c_aligner(SizeGreedy, SizeFixed);

				utils::e_begin_layout(LayoutVertical);
				utils::c_aligner(SizeGreedy, SizeFixed);
					for (auto i = 0; i < n->input_count(); i++)
					{
						auto input = n->input(i);

						utils::e_begin_layout(LayoutVertical, 2.f);

						utils::e_begin_layout(LayoutHorizontal);
						utils::e_empty();
						{
							auto c_element = utils::c_element();
							auto r = utils::style_1u(utils::FontSize);
							c_element->size = r;
							c_element->roundness = r * 0.4f;
							c_element->roundness_lod = 2;
							c_element->color = Vec4c(200, 200, 200, 255);
						}
						{
							auto c_text = utils::c_text();
							c_text->color_ = Vec4c(255, 0, 0, 255);
							c_text->auto_width_ = false;
							c_text->auto_height_ = false;
						}
						utils::c_event_receiver();
						auto c_slot = new_object<cSlot>();
						c_slot->s = input;
						input->user_data = c_slot;
						utils::current_entity()->add_component(c_slot);
						utils::e_begin_popup_menu(false);
							utils::e_menu_item(L"Break Link(s)", [](void* c) {
							}, Mail::from_p(input));
							utils::e_menu_item(L"Reset Value", [](void* c) {
							}, Mail::from_p(input));
							if (c_node->n_type == 'A')
							{
								utils::e_menu_item(L"Remove Slot", [](void* c) {
									auto input = *(BP::Slot**)c;
									auto n = input->node();
									if (n->input_count() == 1)
										return;
									app.deselect();
									auto idx = input->index();
									std::string type = n->type();
									std::string id = n->id();
									auto left_pos = type.find('(');
									auto plus_pos = type.find('+');
									auto size = std::stoi(std::string(type.begin() + left_pos + 1, type.begin() + plus_pos));
									type = std::string(type.begin(), type.begin() + left_pos + 1) + std::to_string(size - 1) + std::string(type.begin() + plus_pos, type.end());
									NodeDesc d;
									d.type = type;
									d.id = "";
									d.pos = n->pos;
									auto nn = app.add_node(d);
									for (auto i = 0; i < n->input_count(); i++)
									{
										if (i == idx)
											continue;
										auto src = n->input(i);
										auto dst = nn->input(i > idx ? i - 1 : i);
										dst->set_data(src->data());
										dst->link_to(src->link());
									}
									for (auto i = 0; i < n->output_count(); i++)
									{
										auto src = n->output(i);
										auto dst = nn->output(i);
										for (auto j = 0; j < src->link_count(); j++)
											src->link(j)->link_to(dst);
									}
									app.remove_nodes({ n });
									nn->set_id(id.c_str());
								}, Mail::from_p(input));
							}
						utils::e_end_popup_menu();

						utils::e_text(s2w(input->name()).c_str())->get_component(cText)->color_ = type_color(input->type()->tag());
						utils::e_end_layout();

						auto type = input->type();
						auto tag = type->tag();
						if (!input->link() && tag != TypePointer)
						{
							auto e_data = utils::e_begin_layout(LayoutVertical, 2.f);
							e_data->get_component(cElement)->padding = Vec4f(utils::style_1u(utils::FontSize), 0.f, 0.f, 0.f);

							std::vector<TypeinfoDatabase*> dbs;
							dbs.resize(app.bp->library_count());
							for (auto i = 0; i < dbs.size(); i++)
								dbs[i] = app.bp->library(i)->db();
							extra_global_db_count = dbs.size();
							extra_global_dbs = dbs.data();
							auto base_hash = type->base_hash();

							switch (tag)
							{
							case TypeEnumSingle:
							{
								auto info = find_enum(base_hash);
								utils::create_enum_combobox(info);

								e_data->add_component(new_object<cEnumSingleDataTracker>(input->data(), info, [](void* c, int v) {
									app.set_data(*(BP::Slot**)c, &v, true);
								}, Mail::from_p(input)));
							}
							break;
							case TypeEnumMulti:
							{
								auto info = find_enum(base_hash);
								utils::create_enum_checkboxs(info);

								e_data->add_component(new_object<cEnumMultiDataTracker>(input->data(), info, [](void* c, int v) {
									app.set_data(*(BP::Slot**)c, &v, true);
								}, Mail::from_p(input)));
							}
							break;
							case TypeData:
								switch (base_hash)
								{
								case FLAME_CHASH("bool"):
									utils::e_checkbox(L"");

									e_data->add_component(new_object<cBoolDataTracker>(input->data(), [](void* c, bool v) {
										app.set_data(*(BP::Slot**)c, &v, true);
									}, Mail::from_p(input)));
									break;
								case FLAME_CHASH("int"):
									create_edit<int>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(2+int)"):
									create_vec_edit<2, int>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(3+int)"):
									create_vec_edit<3, int>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(4+int)"):
									create_vec_edit<4, int>(this, input);
									break;
								case FLAME_CHASH("uint"):
									create_edit<uint>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(2+uint)"):
									create_vec_edit<2, uint>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(3+uint)"):
									create_vec_edit<3, uint>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(4+uint)"):
									create_vec_edit<4, uint>(this, input);
									break;
								case FLAME_CHASH("float"):
									create_edit<float>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(2+float)"):
									create_vec_edit<2, float>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(3+float)"):
									create_vec_edit<3, float>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(4+float)"):
									create_vec_edit<4, float>(this, input);
									break;
								case FLAME_CHASH("uchar"):
									create_edit<uchar>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(2+uchar)"):
									create_vec_edit<2, uchar>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(3+uchar)"):
									create_vec_edit<3, uchar>(this, input);
									break;
								case FLAME_CHASH("flame::Vec(4+uchar)"):
									create_vec_edit<4, uchar>(this, input);
									break;
								case FLAME_CHASH("flame::StringA"):
									utils::e_edit(50.f);

									e_data->add_component(new_object<cStringADataTracker>(input->data(), [](void* c, const char* v) {
										app.set_data(*(BP::Slot**)c, (void*)v, true);
									}, Mail::from_p(input)));
									break;
								case FLAME_CHASH("flame::StringW"):
									utils::e_edit(50.f);

									e_data->add_component(new_object<cStringWDataTracker>(input->data(), [](void* c, const wchar_t* v) {
										app.set_data(*(BP::Slot**)c, (void*)v, true);
									}, Mail::from_p(input)));
									break;
								}
								break;
							}
							extra_global_db_count = 0;
							extra_global_dbs = nullptr;
							utils::e_end_layout();

							c_slot->tracker = e_data->get_component(cDataTracker);
						}

						utils::e_end_layout();

					}
				utils::e_end_layout();

				utils::e_begin_layout(LayoutVertical);
				utils::c_aligner(SizeGreedy, SizeFixed);
					for (auto i = 0; i < n->output_count(); i++)
					{
						auto output = n->output(i);

						utils::e_begin_layout(LayoutHorizontal);
						utils::c_aligner(AlignxRight, AlignyFree);
						utils::e_text(s2w(output->name()).c_str())->get_component(cText)->color_ = type_color(output->type()->tag());

						utils::e_empty();
						{
							auto c_element = utils::c_element();
							auto r = utils::style_1u(utils::FontSize);
							c_element->size = r;
							c_element->roundness = r * 0.4f;
							c_element->roundness_lod = 2;
							c_element->color = Vec4c(200, 200, 200, 255);
						}
						utils::c_event_receiver();
						auto c_slot = new_object<cSlot>();
						c_slot->s = output;
						utils::current_entity()->add_component(c_slot);
						output->user_data = c_slot;
						utils::e_begin_popup_menu(false);
							utils::e_menu_item(L"Break Link(s)", [](void* c) {
							}, Mail::from_p(output));
						utils::e_end_popup_menu();
						utils::e_end_layout();
					}
				utils::e_end_layout();
			utils::e_end_layout();

			if (c_node->n_type == 'A')
			{
				auto c_element = utils::e_button(L"+", [](void* c) {
					auto n = *(BP::Node**)c;
					app.deselect();
					std::string type = n->type();
					std::string id = n->id();
					{
						auto left_pos = type.find('(');
						auto plus_pos = type.find('+');
						auto size = std::stoi(std::string(type.begin() + left_pos + 1, type.begin() + plus_pos));
						type = std::string(type.begin(), type.begin() + left_pos + 1) + std::to_string(size + 1) + std::string(type.begin() + plus_pos, type.end());
						NodeDesc d;
						d.type = type;
						d.id = "";
						d.pos = n->pos;
						auto nn = app.add_node(d);
						for (auto i = 0; i < n->input_count(); i++)
						{
							auto src = n->input(i);
							auto dst = nn->input(i);
							dst->set_data(src->data());
							dst->link_to(src->link());
						}
						for (auto i = 0; i < n->output_count(); i++)
						{
							auto src = n->output(i);
							auto dst = nn->output(i);
							for (auto j = 0; j < src->link_count(); j++)
								src->link(j)->link_to(dst);
						}
						app.remove_nodes({ n });
						nn->set_id(id.c_str());
					}
				}, Mail::from_p(n))->get_component(cElement);
				c_element->padding = Vec4f(5.f, 2.f, 5.f, 2.f);
				c_element->roundness = 8.f;
				c_element->roundness_lod = 2;
				utils::c_aligner(AlignxMiddle, AlignyFree);
			}

		utils::e_end_layout();
		utils::e_empty();
		utils::c_element();
		utils::c_event_receiver()->pass_checkers.add([](void*, cEventReceiver*, bool* pass) {
			*pass = true;
			return true;
		}, Mail());
		utils::c_aligner(SizeFitParent, SizeFitParent);
		utils::c_bring_to_front();
	utils::pop_parent();

	looper().add_event([](void* c, bool*) {
		app.editor->edt.base->entity->add_child(*(Entity**)c);
	}, Mail::from_p(e_node));
}

void cEditor::on_remove_node(BP::Node* n)
{
	auto e = (Entity*)n->user_data;
	e->parent()->remove_child(e);
}

void cEditor::on_data_changed(BP::Slot* s)
{
	((cSlot*)s->user_data)->tracker->update_view();
}

void cEditor::show_add_node_menu(const Vec2f& pos)
{
	const TypeInfo* type;
	TypeTag tag;
	const char* base_name;
	uint base_hash;
	bool is_array;
	if (dragging_slot)
	{
		type = dragging_slot->type();
		tag = type->tag();
		base_name = type->base_name();
		base_hash = type->base_hash();
		is_array = type->is_array();
	}

	std::vector<std::pair<UdtInfo*, VariableInfo*>> node_types;
	auto add_udt = [&](UdtInfo* u) {
		{
			auto f = find_not_null_and_only(u->find_function("update"), u->find_function("active_update"));
			if (!f.first)
				return;
			if (!check_function(f.first, "D#void", { "D#uint" }))
				return;
		}
		for (auto i = 0; i < u->variable_count(); i++)
		{
			auto v = u->variable(i);
			auto flags = v->flags();
			if (flags & VariableFlagInput)
			{
				if (!dragging_slot)
				{
					node_types.emplace_back(u, nullptr);
					return;
				}
				if (dragging_slot->io() == BP::Slot::Out)
				{
					if (BP::Slot::can_link(v->type(), dragging_slot->type()))
					{
						node_types.emplace_back(u, v);
						return;
					}
				}
			}
			if (flags & VariableFlagOutput)
			{
				if (!dragging_slot)
				{
					node_types.emplace_back(u, nullptr);
					return;
				}
				if (dragging_slot->io() == BP::Slot::In)
				{
					if (BP::Slot::can_link(dragging_slot->type(), v->type()))
					{
						node_types.emplace_back(u, v);
						return;
					}
				}
			}
		}
	};
	for (auto i = 0; i < global_db_count(); i++)
	{
		auto udts = global_db(i)->get_udts();
		for (auto j = 0; j < udts.s; j++)
			add_udt(udts.v[j]);
	}
	for (auto i = 0; i < app.bp->library_count(); i++)
	{
		auto udts = app.bp->library(i)->db()->get_udts();
		for (auto j = 0; j < udts.s; j++)
			add_udt(udts.v[j]);
	}
	std::sort(node_types.begin(), node_types.end(), [](const auto& a, const auto& b) {
		return std::string(a.first->type()->name()) < std::string(b.first->type()->name());
	});

	utils::push_parent(utils::add_layer(app.root, ""));
		utils::e_empty()->on_removed_listeners.add([](void*) {
			app.editor->dragging_slot = nullptr;
			return true;
		}, Mail());
		utils::next_element_pos = pos;
		auto c_element = utils::c_element();
		c_element->padding = 4.f;
		c_element->frame_thickness = 2.f;
		c_element->color = utils::style_4c(utils::BackgroundColor);
		c_element->frame_color = utils::style_4c(utils::ForegroundColor);
		utils::c_layout(LayoutVertical)->item_padding = 4.f;
		utils::push_parent(utils::current_entity());
			if (dragging_slot)
				utils::e_text((L"Filtered For: " + s2w(type->name())).c_str())->get_component(cText)->color_ = type_color(tag);
			utils::e_begin_layout(LayoutHorizontal, 4.f);
				utils::e_text(Icon_SEARCH);
				auto c_text_search = utils::e_edit(300.f)->get_component(cText);
			utils::e_end_layout();
			utils::e_begin_scroll_view1(ScrollbarVertical, Vec2f(0.f, 300.f), 4.f);
			utils::c_aligner(SizeGreedy, SizeFixed);
				auto e_list = utils::e_begin_list(true, 0.f);
					struct Capture
					{
						Entity* l;
						Vec2f p;

						void show_enums(TypeTag tag)
						{
							std::vector<EnumInfo*> enum_infos;
							for (auto i = 0; i < global_db_count(); i++)
							{
								auto enums = global_db(i)->get_enums();
								for (auto j = 0; j < enums.s; j++)
									enum_infos.push_back(enums.v[j]);
							}
							for (auto i = 0; i < app.bp->library_count(); i++)
							{
								auto enums = app.bp->library(i)->db()->get_enums();
								for (auto j = 0; j < enums.s; j++)
									enum_infos.push_back(enums.v[j]);
							}
							std::sort(enum_infos.begin(), enum_infos.end(), [](EnumInfo* a, EnumInfo* b) {
								return std::string(a->name()) < std::string(b->name());
							});

							struct Capture
							{
								TypeTag t;
								const char* s;
								Vec2f p;
							}capture;
							capture.t = tag;
							capture.p = p;
							l->remove_children(0, -1);
							for (auto ei : enum_infos)
							{
								capture.s = ei->name();
								utils::push_parent(l);
								utils::e_menu_item(s2w(ei->name()).c_str(), [](void* c) {
									auto& capture = *(Capture*)c;
									NodeDesc d;
									d.type = capture.t == TypeEnumSingle ? "EnumSingle(" : "EnumMulti(";
									d.type += capture.s;
									d.type += ")";
									d.id = "";
									d.pos = capture.p;
									app.add_node(d);
								}, Mail::from_t(&capture));
								utils::pop_parent();
							}
						}
					}capture;
					capture.l = e_list;
					capture.p = (Vec2f(pos) - edt.base->global_pos) / (edt.scale_level * 0.1f);
					if (!dragging_slot)
					{
						utils::e_menu_item(L"Enum Single", [](void* c) {
							auto& capture = *(Capture*)c;
							looper().add_event([](void* c, bool*) {
								auto& capture = *(Capture*)c;
								capture.show_enums(TypeEnumSingle);
							}, Mail::from_t(&capture));
						}, Mail::from_t(&capture), false);
						utils::e_menu_item(L"Enum Multi", [](void* c) {
							auto& capture = *(Capture*)c;
							looper().add_event([](void* c, bool*) {
								auto& capture = *(Capture*)c;
								capture.show_enums(TypeEnumMulti);
							}, Mail::from_t(&capture));
						}, Mail::from_t(&capture), false);
						utils::e_menu_item(L"Variable", [](void* c) {
							auto& capture = *(Capture*)c;
							looper().add_event([](void* c, bool*) {
								auto& capture = *(Capture*)c;
								struct _Capture
								{
									const char* s;
									Vec2f p;
								}_capture;
								_capture.p = capture.p;
								capture.l->remove_children(0, -1);
								for (auto t : basic_types())
								{
									_capture.s = t;
									utils::push_parent(capture.l);
									utils::e_menu_item(s2w(t).c_str(), [](void* c) {
										auto& capture = *(_Capture*)c;
										NodeDesc d;
										d.type = "Variable(";
										d.type += capture.s;
										d.type += ")";
										d.id = "";
										d.pos = capture.p;
										app.add_node(d);
									}, Mail::from_t(&_capture));
									utils::pop_parent();
								}
							}, Mail::from_t(&capture));
						}, Mail::from_t(&capture), false);
						utils::e_menu_item(L"Array", [](void* c) {
							auto& capture = *(Capture*)c;
							looper().add_event([](void* c, bool*) {
								auto& capture = *(Capture*)c;
								struct _Capture
								{
									const char* s;
									Vec2f p;
								}_capture;
								_capture.p = capture.p;
								capture.l->remove_children(0, -1);
								for (auto t : basic_types())
								{
									_capture.s = t;
									utils::push_parent(capture.l);
									utils::e_menu_item(s2w(t).c_str(), [](void* c) {
										auto& capture = *(_Capture*)c;
										NodeDesc d;
										d.type = "Array(1+";
										d.type += capture.s;
										d.type += ")";
										d.id = "";
										d.pos = capture.p;
										app.add_node(d);
									}, Mail::from_t(&_capture));
									utils::pop_parent();
								}
							}, Mail::from_t(&capture));
						}, Mail::from_t(&capture), false);
					}
					else
					{
						if (tag == TypeEnumSingle || tag == TypeEnumMulti)
						{
							struct _Capture
							{
								TypeTag t;
								const char* s;
								Vec2f p;
							}_capture;
							_capture.t = tag;
							_capture.p = capture.p;
							_capture.s = base_name;
							utils::e_menu_item(((tag == TypeEnumSingle ? L"Enum Single: " : L"Enum Multi: ") + s2w(base_name)).c_str(), [](void* c) {
								auto& capture = *(_Capture*)c;
								NodeDesc d;
								d.type = capture.t == TypeEnumSingle ? "EnumSingle(" : "EnumMulti(";
								d.type += capture.s;
								d.type += ")";
								d.id = "";
								d.pos = capture.p;
								auto n = app.add_node(d);
								auto s = app.editor->dragging_slot;
								if (s)
								{
									if (s->io() == BP::Slot::In)
										_set_link(s, n->find_output("out"));
									else
										_set_link(n->find_input("in"), s);
								}
							}, Mail::from_t(&_capture));
						}
						else if (tag == TypeData && !is_array)
						{
							if (basic_type_size(base_hash))
							{
								struct _Capture
								{
									const char* s;
									Vec2f p;
								}_capture;
								_capture.p = capture.p;
								_capture.s = base_name;
								utils::e_menu_item((L"Variable: " + s2w(base_name)).c_str(), [](void* c) {
									auto& capture = *(_Capture*)c;
									NodeDesc d;
									d.type = "Variable(";
									d.type += capture.s;
									d.type += ")";
									d.id = "";
									d.pos = capture.p;
									auto n = app.add_node(d);
									auto s = app.editor->dragging_slot;
									if (s)
									{
										if (s->io() == BP::Slot::In)
											_set_link(s, n->find_output("out"));
										else
											_set_link(n->find_input("in"), s);
									}
								}, Mail::from_t(&_capture));
							}
						}
						else if (tag == TypePointer && is_array)
						{
							struct _Capture
							{
								const char* s;
								Vec2f p;
							}_capture;
							_capture.p = capture.p;
							_capture.s = base_name;
							utils::e_menu_item((L"Array: " + s2w(base_name)).c_str(), [](void* c) {
								auto& capture = *(_Capture*)c;
								NodeDesc d;
								d.type = "Array(1+";
								d.type += capture.s;
								d.type += ")";
								d.id = "";
								d.pos = capture.p;
								auto n = app.add_node(d);
								auto s = app.editor->dragging_slot;
								if (s)
								{
									if (s->io() == BP::Slot::In)
										_set_link(s, n->find_output("out"));
									else
										_set_link(n->find_input("0"), s);
								}
							}, Mail::from_t(&_capture));
						}
					}
					{
						struct Capture
						{
							const char* us;
							const char* vs;
							Vec2f p;
						}capture;
						capture.p = (Vec2f(pos) - edt.base->global_pos) / (edt.scale_level * 0.1f);
						for (auto& t : node_types)
						{
							capture.us = t.first->type()->name() + 2;
							capture.vs = t.second ? t.second->name() : nullptr;
							utils::e_menu_item(s2w(t.first->type()->name()).c_str(), [](void* c) {
								auto& capture = *(Capture*)c;
								NodeDesc d;
								d.type = capture.us;
								d.id = "";
								d.pos = capture.p;
								auto n = app.add_node(d);
								auto s = app.editor->dragging_slot;
								if (s)
								{
									if (s->io() == BP::Slot::In)
										_set_link(s, n->find_output(capture.vs));
									else
										_set_link(n->find_input(capture.vs), s);
								}
							}, Mail::from_t(&capture));
						}
					}
				utils::e_end_list();
			utils::e_end_scroll_view1();
		utils::pop_parent();
	utils::pop_parent();

	{
		struct Capture
		{
			Entity* l;
			cText* t;
		}capture;
		capture.l = e_list;
		capture.t = c_text_search;
		c_text_search->data_changed_listeners.add([](void* c, uint hash, void*) {
			auto& capture = *(Capture*)c;
			std::wstring str = capture.t->text();
			for (auto i = 0; i < capture.l->child_count(); i++)
			{
				auto item = capture.l->child(i);
				item->set_visible(str[0] ? (std::wstring(item->get_component(cText)->text()).find(str) != std::string::npos) : true);
			}
			return true;
		}, Mail::from_t(&capture));
	}
}

void cEditor::clear_failed_flags()
{
	for (auto i = 0; i < app.bp->node_count(); i++)
	{
		auto n = app.bp->node(i);
		for (auto j = 0; j < n->input_count(); j++)
		{
			auto in = n->input(j);
			((cSlot*)in->user_data)->entity->get_component(cText)->set_text(L"");
		}
	}
}

void cEditor::set_failed_flags()
{
	for (auto i = 0; i < app.bp->node_count(); i++)
	{
		auto n = app.bp->node(i);
		for (auto j = 0; j < n->input_count(); j++)
		{
			auto in = n->input(j);
			if (in->fail_message()[0])
				((cSlot*)in->user_data)->entity->get_component(cText)->set_text(Icon_TIMES);
		}
	}
}

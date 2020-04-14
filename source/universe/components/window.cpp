#include <flame/graphics/font.h>
#include <flame/graphics/canvas.h>
#include <flame/universe/systems/event_dispatcher.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/text.h>
#include "event_receiver_private.h"
#include <flame/universe/components/aligner.h>
#include <flame/universe/components/layout.h>
#include <flame/universe/components/list.h>
#include <flame/universe/components/style.h>
#include <flame/universe/components/splitter.h>
#include <flame/universe/components/window.h>
#include <flame/universe/utils/event.h>
#include <flame/universe/utils/layer.h>
#include <flame/universe/utils/style.h>
#include <flame/universe/utils/window.h>

namespace flame
{
	struct cMoveablePrivate : cMoveable
	{
		void* mouse_listener;

		cMoveablePrivate()
		{
			element = nullptr;
			event_receiver = nullptr;

			mouse_listener = nullptr;
		}

		~cMoveablePrivate()
		{
			if (!entity->dying_)
				event_receiver->mouse_listeners.remove(mouse_listener);
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cElement"))
				element = (cElement*)c;
			else if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				mouse_listener = event_receiver->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
					auto thiz = *(cMoveablePrivate**)c;
					if (utils::is_active(thiz->event_receiver) && is_mouse_move(action, key))
						thiz->element->add_pos((Vec2f)pos / thiz->element->global_scale, thiz);
					return true;
				}, Mail::from_p(this));
			}
		}
	};

	cMoveable* cMoveable::create()
	{
		return new cMoveablePrivate;
	}
	struct cBringToFrontPrivate : cBringToFront
	{
		void* mouse_listener;

		cBringToFrontPrivate()
		{
			event_receiver = nullptr;

			mouse_listener = nullptr;
		}

		~cBringToFrontPrivate()
		{
			if (!entity->dying_)
				event_receiver->mouse_listeners.remove(mouse_listener);
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				mouse_listener = event_receiver->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
					if (is_mouse_down(action, key, true) && key == Mouse_Left)
					{
						auto thiz = *(cBringToFrontPrivate**)c;
						auto l = thiz->entity->parent()->parent()->last_child(0);
						if (!l || !SUS::starts_with(l->name(), "layer_"))
						{
							looper().add_event([](void* c, bool*) {
								auto p = (*(Entity**)c)->parent();
								p->parent()->reposition_child(p, -1);
							}, Mail::from_p(thiz->entity));
						}
					}
					return true;
				}, Mail::from_p(this));
			}
		}
	};

	cBringToFront* cBringToFront::create()
	{
		return new cBringToFrontPrivate;
	}

	struct cSizeDraggerPrivate : cSizeDragger
	{
		void* mouse_listener;
		void* state_listener;

		cSizeDraggerPrivate()
		{
			event_receiver = nullptr;
			p_element = nullptr;

			mouse_listener = nullptr;
			state_listener = nullptr;
		}

		~cSizeDraggerPrivate()
		{
			if (!entity->dying_)
			{
				event_receiver->mouse_listeners.remove(mouse_listener);
				event_receiver->state_listeners.remove(state_listener);
			}
		}

		void on_added() override
		{
			auto p = entity->parent();
			if (p)
				p_element = p->get_component(cElement);
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				mouse_listener = event_receiver->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
					auto thiz = (*(cSizeDraggerPrivate**)c);
					if (is_mouse_move(action, key) && utils::is_active(thiz->event_receiver))
						thiz->p_element->add_size(Vec2f(pos));
					return true;
				}, Mail::from_p(this));
				state_listener = event_receiver->state_listeners.add([](void* c, EventReceiverState s) {
					sEventDispatcher::current()->window->set_cursor(s ? CursorSizeNWSE : CursorArrow);
					return true;
				}, Mail::from_p(this));
			}
		}
	};

	cSizeDragger* cSizeDragger::create()
	{
		return new cSizeDraggerPrivate;
	}

	struct cDockerTabPrivate : cDockerTab
	{
		void* mouse_listener;
		void* drag_and_drop_listener;
		Vec2f drop_pos;
		void* draw_cmd;

		struct DropTip
		{
			bool highlighted;
			Vec2f pos;
			Vec2f size;
		};
		std::vector<DropTip> drop_tips;
		cEventReceiver* overing;

		cDockerTabPrivate()
		{
			element = nullptr;
			event_receiver = nullptr;
			list_item = nullptr;

			root = nullptr;

			floating = false;
			page = nullptr;
			page_element = nullptr;

			mouse_listener = nullptr;
			drag_and_drop_listener = nullptr;
			drop_pos = Vec2f(0.f);
			draw_cmd = nullptr;

			overing = nullptr;
		}

		~cDockerTabPrivate()
		{
			if (!entity->dying_)
			{
				element->cmds.remove(draw_cmd);
				event_receiver->mouse_listeners.remove(mouse_listener);
				event_receiver->drag_and_drop_listeners.remove(drag_and_drop_listener);
			}
		}

		void take_away(bool close)
		{
			auto tabbar = entity->parent();
			if (tabbar->name_hash() != FLAME_CHASH("docker_tabbar"))
				return;

			auto docker = tabbar->parent();
			auto pages = docker->child(1);
			page = pages->child(entity->index_);
			page_element = page->get_component(cElement);
			auto page_aligner = page->get_component(cAligner);
			auto list = list_item->list;
			list_item->list = nullptr;

			if (tabbar->child_count() > 1 && list && list->selected == entity)
				list->set_selected(list->entity->child(0));

			if (close)
			{
				pages->remove_child(page); // remove tab first will lead variable 'page' to be trash data
				tabbar->remove_child(entity);
			}
			else
			{
				auto dp = event_receiver->dispatcher;

				tabbar->remove_child(entity, false);
				pages->remove_child(page, false);
				page->set_visible(true);
				element->set_pos(element->global_pos);
				element->set_alpha(0.5f);
				page_element->set_pos(Vec2f(element->pos.x(), element->pos.y() + element->global_size.y() + 8.f));
				page_element->set_alpha(0.5f);
				page_aligner->set_width_policy(SizeFixed);
				page_aligner->set_height_policy(SizeFixed);
				root->add_child(page);
				root->add_child(entity);

				dp->focusing = event_receiver;
				dp->focusing_state = FocusingAndDragging;
			}

			if (tabbar->child_count() == 0)
			{
				auto p = docker->parent();
				if (p)
				{
					if (p->name_hash() == FLAME_CHASH("docker_floating_container"))
						p->parent()->remove_child(p);
					else if (p->name_hash() == FLAME_CHASH("docker_static_container"))
						p->remove_children(0, -1);
					else if (p->name_hash() == FLAME_CHASH("docker_layout"))
					{
						auto oth_docker = p->child(docker->index_ == 0 ? 2 : 0);
						p->remove_child(oth_docker, false);
						auto pp = p->parent();
						auto idx = p->index_;
						pp->remove_child(p);
						pp->add_child(oth_docker, idx);
						if (pp->name_hash() == FLAME_CHASH("docker_floating_container"))
						{
							auto ca = oth_docker->get_component(cAligner);
							ca->set_x_align(AlignxLeft);
							ca->set_y_align(AlignyTop);
							ca->set_using_padding(true);
						}
					}
				}
			}
		}

		void on_added() override
		{
			auto p = entity->parent();
			if (p && p->child_count() == 1)
				p->get_component(cDockerTabbar)->list->set_selected(entity);
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cElement"))
			{
				element = (cElement*)c;
				draw_cmd = element->cmds.add([](void* c, graphics::Canvas* canvas) {
					(*(cDockerTabPrivate**)c)->draw(canvas);
					return true;
				}, Mail::from_p(this));
			}
			else if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				event_receiver->drag_hash = FLAME_CHASH("cDockerTab");
				mouse_listener = event_receiver->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
					auto thiz = (*(cDockerTabPrivate**)c);
					if (is_mouse_move(action, key) && utils::is_dragging(thiz->event_receiver) && thiz->page)
					{
						thiz->element->add_pos(Vec2f(pos));
						thiz->page_element->add_pos(Vec2f(pos));
					}
					return true;
				}, Mail::from_p(this));

				drag_and_drop_listener = event_receiver->drag_and_drop_listeners.add([](void* c, DragAndDrop action, cEventReceiver* er, const Vec2i& pos) {
					auto thiz = (*(cDockerTabPrivate**)c);
					if (action == DragStart)
					{
						thiz->floating = true;
						looper().add_event([](void* c, bool*) {
							(*(cDockerTabPrivate**)c)->take_away(false);
						}, Mail::from_p(thiz));
					}
					else if (action == DragEnd)
					{
						if (!er || thiz->floating)
						{
							thiz->drop_pos = pos;
							looper().add_event([](void* c, bool*) {
								auto thiz = (*(cDockerTabPrivate**)c);

								auto e_tab = thiz->entity;
								auto e_page = thiz->page;
								auto page_element = thiz->page_element;
								auto page_aligner = e_page->get_component(cAligner);

								auto e_container = Entity::create();
								utils::make_docker_floating_container(e_container, thiz->drop_pos, page_element->size);
								thiz->root->add_child(e_container);

								auto e_docker = Entity::create();
								utils::make_docker(e_docker);
								e_container->add_child(e_docker, 0);

								auto e_tabbar = e_docker->child(0);
								auto e_pages = e_docker->child(1);

								thiz->root->remove_child(e_tab, false);
								thiz->root->remove_child(e_page, false);
								thiz->list_item->list = e_tabbar->get_component(cList);
								e_tabbar->add_child(e_tab);
								auto element = thiz->element;
								element->set_pos(Vec2f(0.f));
								element->set_alpha(1.f);
								e_pages->add_child(e_page);
								page_element->set_pos(Vec2f(0.f));
								page_element->set_alpha(1.f);
								page_aligner->set_width_policy(SizeFitParent);
								page_aligner->set_height_policy(SizeFitParent);
								thiz->page = nullptr;
								thiz->page_element = nullptr;
							}, Mail::from_p(thiz));
						}
					}
					return true;
				}, Mail::from_p(this));
			}
			else if (c->name_hash == FLAME_CHASH("cListItem"))
				list_item = (cListItem*)c;
		}

		void draw(graphics::Canvas* canvas)
		{
			if (!overing || event_receiver->dispatcher->drag_overing != overing)
			{
				drop_tips.clear();
				overing = nullptr;
			}
			else
			{
				if (!element->clipped)
				{
					for (auto p : drop_tips)
					{
						std::vector<Vec2f> points;
						path_rect(points, p.pos, p.size);
						canvas->fill(points.size(), points.data(), p.highlighted ? Vec4c(60, 90, 210, 255) : Vec4c(50, 80, 200, 200));
					}
				}
			}
		}
	};

	void cDockerTab::take_away(bool close)
	{
		((cDockerTabPrivate*)this)->take_away(close);
	}

	cDockerTab* cDockerTab::create()
	{
		return new cDockerTabPrivate;
	}

	struct cDockerTabbarPrivate : cDockerTabbar
	{
		void* drag_and_drop_listener;
		cDockerTab* drop_tab;
		uint drop_idx;
		void* selected_changed_listener;

		cDockerTabbarPrivate()
		{
			element = nullptr;
			event_receiver = nullptr;
			list = nullptr;

			drag_and_drop_listener = nullptr;
			drop_tab = nullptr;
			drop_idx = 0;
		}

		~cDockerTabbarPrivate()
		{
			if (!entity->dying_)
			{
				event_receiver->drag_and_drop_listeners.remove(drag_and_drop_listener);
				list->data_changed_listeners.remove(selected_changed_listener);
			}
		}

		uint calc_pos(float x, float* out)
		{
			for (auto i = 0; i < entity->child_count(); i++)
			{
				auto element = entity->child(i)->get_component(cElement);
				auto half = element->global_pos.x() + element->size.x() * 0.5f;
				if (x >= element->global_pos.x() && x < half)
				{
					if (out)
						*out = element->global_pos.x();
					return i;
				}
				if (x >= half && x < element->global_pos.x() + element->size.x())
				{
					if (out)
						*out = element->global_pos.x() + element->size.x();
					return i + 1;
				}
			}
			if (out)
			{
				auto element = entity->child(entity->child_count() - 1)->get_component(cElement);
				*out = element->global_pos.x() + element->global_size.x();
			}
			return entity->child_count();
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cElement"))
				element = (cElement*)c;
			else if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				event_receiver->set_acceptable_drops(1, &FLAME_CHASH("cDockerTab"));
				drag_and_drop_listener = event_receiver->drag_and_drop_listeners.add([](void* c, DragAndDrop action, cEventReceiver* er, const Vec2i& pos) {
					auto thiz = (*(cDockerTabbarPrivate**)c);
					if (thiz->entity->child_count() > 0) // a valid docker tabbar must have at least one item
					{
						if (action == BeingOvering)
						{
							float show_drop_pos;
							auto idx = thiz->calc_pos(pos.x(), &show_drop_pos);
							if (idx == thiz->entity->child_count())
								show_drop_pos -= 10.f;
							else if (idx != 0)
								show_drop_pos -= 5.f;
							auto drop_tab = (cDockerTabPrivate*)er->entity->get_component(cDockerTab);
							drop_tab->drop_tips.clear();
							drop_tab->overing = thiz->event_receiver;
							cDockerTabPrivate::DropTip primitive;
							primitive.highlighted = false;
							primitive.pos = Vec2f(show_drop_pos, thiz->element->global_pos.y());
							primitive.size = Vec2f(10.f, thiz->element->global_size.y());
							drop_tab->drop_tips.push_back(primitive);
						}
						else if (action == BeenDropped)
						{
							thiz->drop_tab = er->entity->get_component(cDockerTab);
							thiz->drop_idx = thiz->calc_pos(pos.x(), nullptr);
							thiz->drop_tab->floating = false;
							looper().add_event([](void* c, bool*) {
								auto thiz = *(cDockerTabbarPrivate**)c;
								auto tabbar = thiz->entity;
								auto tab = thiz->drop_tab;
								auto e_tab = tab->entity;
								auto e_page = tab->page;
								auto page_element = tab->page_element;
								auto page_aligner = e_page->get_component(cAligner);

								tab->root->remove_child(e_tab, false);
								tab->root->remove_child(e_page, false);
								tab->list_item->list = thiz->list;
								tabbar->add_child(e_tab, thiz->drop_idx);
								tabbar->parent()->child(1)->add_child(e_page);

								tab->element->set_alpha(1.f);
								page_element->set_pos(Vec2f(0.f));
								page_element->set_alpha(1.f);
								page_aligner->set_width_policy(SizeFitParent);
								page_aligner->set_height_policy(SizeFitParent);

								thiz->drop_tab->page = nullptr;
								thiz->drop_tab->page_element = nullptr;
								thiz->drop_tab = nullptr;
								thiz->drop_idx = 0;

								thiz->list->set_selected(e_tab);
							}, Mail::from_p(thiz));
						}
					}
					return true;
				}, Mail::from_p(this));
			}
			else if (c->name_hash == FLAME_CHASH("cList"))
			{
				list = (cList*)c;
				selected_changed_listener = list->data_changed_listeners.add([](void* c, uint hash, void*) {
					auto thiz = (*(cDockerTabbarPrivate**)c);
					if (hash == FLAME_CHASH("selected"))
					{
						auto tabbar = thiz->entity;
						auto docker = tabbar->parent();
						auto pages = docker->child(1);
						if (pages->child_count() > 0)
						{
							auto idx = thiz->list->selected->index_;
							for (auto i = 0; i < pages->child_count(); i++)
								pages->child(i)->set_visible(false);
							pages->child(idx)->set_visible(true);
						}
					}
					return true;
				}, Mail::from_p(this));
			}
		}
	};

	cDockerTabbar* cDockerTabbar::create()
	{
		return new cDockerTabbarPrivate;
	}

	struct cDockerPagesPrivate : cDockerPages 
	{
		void* drag_and_drop_listener;
		cDockerTab* drop_tab;
		Side dock_side;

		cDockerPagesPrivate()
		{
			element = nullptr;
			event_receiver = nullptr;

			drag_and_drop_listener = nullptr;
			drop_tab = nullptr;
			dock_side = Outside;
		}

		~cDockerPagesPrivate()
		{
			if (!entity->dying_)
				event_receiver->drag_and_drop_listeners.remove(drag_and_drop_listener);
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cElement"))
				element = (cElement*)c;
			else if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				event_receiver->set_acceptable_drops(1, &FLAME_CHASH("cDockerTab"));
				drag_and_drop_listener = event_receiver->drag_and_drop_listeners.add([](void* c, DragAndDrop action, cEventReceiver* er, const Vec2i& pos) {
					auto thiz = (*(cDockerPagesPrivate**)c);
					if (action == BeingOvering)
					{
						thiz->dock_side = Outside;
						auto center = thiz->element->center();
						if (rect_contains(Vec4f(center + Vec2f(-25.f, -25.f), center + Vec2f(25.f, 25.f)), (Vec2f)pos))
							thiz->dock_side = SideCenter;
						else if (rect_contains(Vec4f(center + Vec2f(-55.f, -25.f), center + Vec2f(-30.f, 25.f)), (Vec2f)pos))
							thiz->dock_side = SideW;
						else if (rect_contains(Vec4f(center + Vec2f(30.f, -25.f), center + Vec2f(55.f, 25.f)), (Vec2f)pos))
							thiz->dock_side = SideE;
						else if (rect_contains(Vec4f(center + Vec2f(-25.f, -55.f), center + Vec2f(25.f, -30.f)), (Vec2f)pos))
							thiz->dock_side = SideN;
						else if (rect_contains(Vec4f(center + Vec2f(-25.f, 30.f), center + Vec2f(25.f, 55.f)), (Vec2f)pos))
							thiz->dock_side = SideS;

						auto drop_tab = (cDockerTabPrivate*)er->entity->get_component(cDockerTab);
						drop_tab->drop_tips.clear();
						drop_tab->overing = thiz->event_receiver;
						{
							cDockerTabPrivate::DropTip primitive;
							primitive.highlighted = thiz->dock_side == SideCenter;
							primitive.pos = center + Vec2f(-25.f);
							primitive.size = Vec2f(50.f, 50.f);
							std::vector<Vec2f> points;
							drop_tab->drop_tips.push_back(primitive);
						}
						{
							cDockerTabPrivate::DropTip primitive;
							primitive.highlighted = thiz->dock_side == SideW;
							primitive.pos = center + Vec2f(-55.f, -25.f);
							primitive.size = Vec2f(25.f, 50.f);
							std::vector<Vec2f> points;
							drop_tab->drop_tips.push_back(primitive);
						}
						{
							cDockerTabPrivate::DropTip primitive;
							primitive.highlighted = thiz->dock_side == SideE;
							primitive.pos = center + Vec2f(30.f, -25.f);
							primitive.size = Vec2f(25.f, 50.f);
							std::vector<Vec2f> points;
							drop_tab->drop_tips.push_back(primitive);
						}
						{
							cDockerTabPrivate::DropTip primitive;
							primitive.highlighted = thiz->dock_side == SideN;
							primitive.pos = center + Vec2f(-25.f, -55.f);
							primitive.size = Vec2f(50.f, 25.f);
							std::vector<Vec2f> points;
							drop_tab->drop_tips.push_back(primitive);
						}
						{
							cDockerTabPrivate::DropTip primitive;
							primitive.highlighted = thiz->dock_side == SideS;
							primitive.pos = center + Vec2f(-25.f, 30.f);
							primitive.size = Vec2f(50.f, 25.f);
							std::vector<Vec2f> points;
							drop_tab->drop_tips.push_back(primitive);
						}
					}
					else if (action == BeenDropped)
					{
						if (thiz->dock_side == SideCenter)
						{
							auto tabbar_er = (cEventReceiverPrivate*)thiz->entity->parent()->child(0)->get_component(cEventReceiver);
							tabbar_er->on_drag_and_drop(BeenDropped, er, Vec2i(0, 99999));
						}
						else if (thiz->dock_side != Outside)
						{
							thiz->drop_tab = er->entity->get_component(cDockerTab);
							thiz->drop_tab->floating = false;
							looper().add_event([](void* c, bool*) {
								auto thiz = (*(cDockerPagesPrivate**)c);
								auto tab = thiz->drop_tab;
								auto e_tab = tab->entity;
								auto e_page = tab->page;
								auto page_element = tab->page_element;
								auto page_aligner = e_page->get_component(cAligner);
								auto docker = thiz->entity->parent();
								auto docker_element = docker->get_component(cElement);
								auto docker_aligner = docker->get_component(cAligner);
								auto p = docker->parent();
								auto docker_idx = docker->index_; LayoutFree;
								auto layout = Entity::create();
								utils::make_docker_layout(layout, (thiz->dock_side == SideW || thiz->dock_side == SideE) ? LayoutHorizontal : LayoutVertical);

								if (is_one_of(p->name_hash(), { FLAME_CHASH("docker_floating_container"), FLAME_CHASH("docker_static_container") }))
								{
									auto aligner = docker->get_component(cAligner);
									aligner->set_x_align(AlignxFree);
									aligner->set_y_align(AlignyFree);
								}
								else
								{
									auto p_element = p->get_component(cElement);
									auto layout_element = layout->get_component(cElement);

									layout_element->set_size(p_element->size);

									auto aligner = layout->get_component(cAligner);
									aligner->set_x_align(AlignxFree);
									aligner->set_y_align(AlignyFree);
									aligner->set_width_factor(p_element->size.x());
									aligner->set_height_factor(p_element->size.y());

									{
										auto oth = p->child(docker_idx == 0 ? 2 : 0);
										auto element = oth->get_component(cElement);
										auto aligner = oth->get_component(cAligner);
										aligner->set_width_factor(element->size.x());
										aligner->set_height_factor(element->size.y());
									}
								}
								p->remove_child(docker, false);
								p->add_child(layout, docker_idx);

								auto new_docker = Entity::create();
								utils::make_docker(new_docker);
								auto new_docker_element = new_docker->get_component(cElement);
								auto new_docker_aligner = new_docker->get_component(cAligner);
								{
									auto ca = new_docker->get_component(cAligner);
									ca->x_align_ = AlignxFree;
									ca->y_align_ = AlignyFree;
									ca->using_padding_ = false;
								}
								auto new_tabbar = new_docker->child(0);
								auto new_pages = new_docker->child(1);

								tab->root->remove_child(e_tab, false);
								tab->root->remove_child(e_page, false);
								tab->list_item->list = (cList*)new_tabbar->get_component(cList);
								new_tabbar->add_child(e_tab);
								new_pages->add_child(e_page);

								tab->element->set_alpha(1.f);
								page_element->set_pos(Vec2f(0.f));
								page_element->set_alpha(1.f);
								page_aligner->set_width_policy(SizeFitParent);
								page_aligner->set_height_policy(SizeFitParent);

								auto e_splitter = layout->child(0);
								auto splitter_element = e_splitter->get_component(cElement);
								auto splitter = e_splitter->get_component(cSplitter);
								if (thiz->dock_side == SideW || thiz->dock_side == SideE)
								{
									layout->get_component(cLayout)->type = LayoutHorizontal;
									splitter->type = SplitterHorizontal;
									e_splitter->get_component(cAligner)->set_height_policy(SizeFitParent);

									auto w = (docker_element->size.x() - splitter_element->size.x()) * 0.5f;
									docker_element->set_width(w);
									new_docker_element->set_width(w);
									docker_aligner->set_width_factor(w);
									new_docker_aligner->set_width_factor(w);

									auto h = docker_element->size.y() * 0.5f;
									docker_element->set_height(h);
									new_docker_element->set_height(h);
									docker_aligner->set_height_factor(h);
									new_docker_aligner->set_height_factor(h);
								}
								else
								{
									layout->get_component(cLayout)->type = LayoutVertical;
									splitter->type = SplitterVertical;
									e_splitter->get_component(cAligner)->set_width_policy(SizeFitParent);

									auto w = docker_element->size.x();
									docker_element->set_width(w);
									new_docker_element->set_width(w);
									docker_aligner->set_width_factor(w);
									new_docker_aligner->set_width_factor(w);

									auto h = (docker_element->size.y() - splitter_element->size.y()) * 0.5f;
									docker_element->set_height(h);
									new_docker_element->set_height(h);
									docker_aligner->set_height_factor(h);
									new_docker_aligner->set_height_factor(h);
								}

								if (thiz->dock_side == SideW)
								{
									layout->add_child(new_docker, 0);
									layout->add_child(docker, 2);
								}
								else if (thiz->dock_side == SideE)
								{
									layout->add_child(docker, 0);
									layout->add_child(new_docker, 2);
								}
								else if (thiz->dock_side == SideN)
								{
									layout->add_child(new_docker, 0);
									layout->add_child(docker, 2);
								}
								else if (thiz->dock_side == SideS)
								{
									layout->add_child(docker, 0);
									layout->add_child(new_docker, 2);
								}
							}, Mail::from_p(thiz));
						}
					}
					return true;
				}, Mail::from_p(this));
			}
		}
	};

	cDockerPages* cDockerPages::create()
	{
		return new cDockerPagesPrivate;
	}

	struct cDockerStaticContainerPrivate : cDockerStaticContainer
	{
		void* drag_and_drop_listener;
		cDockerTab* drop_tab;
		Side dock_side;

		cDockerStaticContainerPrivate()
		{
			element = nullptr;
			event_receiver = nullptr;

			drag_and_drop_listener = nullptr;
			drop_tab = nullptr;

		}

		~cDockerStaticContainerPrivate()
		{
			if (!entity->dying_)
				event_receiver->drag_and_drop_listeners.remove(drag_and_drop_listener);
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cElement"))
				element = (cElement*)c;
			else if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				event_receiver->set_acceptable_drops(1, &FLAME_CHASH("cDockerTab"));
				drag_and_drop_listener = event_receiver->drag_and_drop_listeners.add([](void* c, DragAndDrop action, cEventReceiver* er, const Vec2i& pos) {
					auto thiz = (*(cDockerStaticContainerPrivate**)c);
					if (action == BeingOvering)
					{
						thiz->dock_side = Outside;
						auto center = thiz->element->center();
						if (rect_contains(Vec4f(center + Vec2f(-25.f, -25.f), center + Vec2f(25.f, 25.f)), (Vec2f)pos))
							thiz->dock_side = SideCenter;

						auto drop_tab = (cDockerTabPrivate*)er->entity->get_component(cDockerTab);
						drop_tab->drop_tips.clear();
						drop_tab->overing = thiz->event_receiver;
						{
							cDockerTabPrivate::DropTip primitive;
							primitive.highlighted = thiz->dock_side == SideCenter;
							primitive.pos = center + Vec2f(-25.f);
							primitive.size = Vec2f(50.f, 50.f);
							std::vector<Vec2f> points;
							drop_tab->drop_tips.push_back(primitive);
						}
					}
					else if (action == BeenDropped)
					{
						if (thiz->dock_side == SideCenter)
						{
							thiz->drop_tab = er->entity->get_component(cDockerTab);
							thiz->drop_tab->floating = false;
							looper().add_event([](void* c, bool*) {
								auto thiz = (*(cDockerStaticContainerPrivate**)c);
								auto tab = thiz->drop_tab;
								auto e_tab = tab->entity;
								auto e_page = tab->page;
								auto page_element = tab->page_element;
								auto page_aligner = e_page->get_component(cAligner);

								auto new_docker = Entity::create();
								utils::make_docker(new_docker);
								{
									auto ca = new_docker->get_component(cAligner);
									ca->x_align_ = AlignxFree;
									ca->y_align_ = AlignyFree;
									ca->using_padding_ = false;
								}
								auto new_tabbar = new_docker->child(0);
								auto new_pages = new_docker->child(1);
								thiz->entity->add_child(new_docker);

								tab->root->remove_child(e_tab, false);
								tab->root->remove_child(e_page, false);
								tab->list_item->list = (cList*)new_tabbar->get_component(cList);
								new_tabbar->add_child(e_tab);
								new_pages->add_child(e_page);

								tab->element->set_alpha(1.f);
								page_element->set_pos(Vec2f(0.f));
								page_element->set_alpha(1.f);
								page_aligner->set_width_policy(SizeFitParent);
								page_aligner->set_height_policy(SizeFitParent);
							}, Mail::from_p(thiz));
						}
					}
					return true;
				}, Mail::from_p(this));
			}
		}
	};

	cDockerStaticContainer* cDockerStaticContainer::create()
	{
		return new cDockerStaticContainerPrivate;
	}
}

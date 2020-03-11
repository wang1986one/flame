#include <flame/serialize.h>
#include <flame/universe/world.h>
#include "event_dispatcher_private.h"
#include <flame/universe/components/element.h>
#include "../components/event_receiver_private.h"

namespace flame
{
	sEventDispatcherPrivate::sEventDispatcherPrivate()
	{
		window = nullptr;
		key_listener = nullptr;
		mouse_listener = nullptr;

		hovering = nullptr;
		focusing = nullptr;
		focusing_state = FocusingNormal;
		drag_overing = nullptr;

		next_focusing = (cEventReceiver*)INVALID_POINTER;

		char_input_compelete = true;
		for (auto i = 0; i < array_size(key_states); i++)
			key_states[i] = KeyStateUp;

		mouse_pos = Vec2i(0);
		mouse_pos_prev = Vec2i(0);
		mouse_disp = Vec2i(0);
		mouse_scroll = 0;
		for (auto i = 0; i < array_size(mouse_buttons); i++)
			mouse_buttons[i] = KeyStateUp;
		dbclick_timer = -1.f;

		active_pos = Vec2i(0);
	}

	sEventDispatcherPrivate::~sEventDispatcherPrivate()
	{
		if (window)
		{
			window->key_listeners.remove(key_listener);
			window->mouse_listeners.remove(mouse_listener);
		}
	}

	void sEventDispatcherPrivate::on_receiver_removed(cEventReceiver* er)
	{
		if (er == focusing)
			focusing = nullptr;
		if (er == hovering)
			hovering = nullptr;
		if (er == drag_overing)
			drag_overing = nullptr;
		er->state = EventReceiverNormal;
		er->state_listeners.call(EventReceiverNormal);
	}

	void sEventDispatcherPrivate::on_added()
	{
		window = (SysWindow*)world_->find_object(FLAME_CHASH("SysWindow"), 0);
		if (window)
		{

			key_listener = window->key_listeners.add([](void* c, KeyStateFlags action, int value) {
				auto thiz = *(sEventDispatcherPrivate**)c;

				if (action == KeyStateNull)
				{
					if (!thiz->char_input_compelete && !thiz->char_inputs.empty())
					{
						std::string ansi;
						ansi += thiz->char_inputs.back();
						ansi += value;
						auto wstr = a2w(ansi);
						thiz->char_inputs.back() = wstr[0];
						thiz->char_input_compelete = true;
					}
					else
					{
						thiz->char_inputs.push_back(value);
						if (value >= 0x80)
							thiz->char_input_compelete = false;
					}
				}
				else
				{
					thiz->key_states[value] = action | KeyStateJust;
					if (action == KeyStateDown)
						thiz->keydown_inputs.push_back((Key)value);
					else if (action == KeyStateUp)
						thiz->keyup_inputs.push_back((Key)value);
				}

				thiz->pending_update = true;

				return true;
			}, new_mail_p(this));

			mouse_listener = window->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
				auto thiz = *(sEventDispatcherPrivate**)c;

				if (action == KeyStateNull)
				{
					if (key == Mouse_Middle)
						thiz->mouse_scroll = pos.x();
					else if (key == Mouse_Null)
						thiz->mouse_pos = pos;
				}
				else
				{
					thiz->mouse_buttons[key] = action | KeyStateJust;
					thiz->mouse_pos = pos;
				}

				thiz->pending_update = true;

				return true;
			}, new_mail_p(this));
		}
	}

	void sEventDispatcherPrivate::dispatch_mouse(EntityPrivate* e)
	{
		for (auto it = e->children.rbegin(); it != e->children.rend(); it++)
		{
			auto c = it->get();
			if (c->global_visibility_)
				dispatch_mouse(c);
		}

		if (!pass)
			return;

		auto er = (cEventReceiverPrivate*)e->get_component(cEventReceiver);
		if (!er)
			return;

		auto mouse_contained = (!er->element->cliped && rect_contains(er->element->cliped_rect, Vec2f(mouse_pos)));
		if ((er == focusing && focusing_state != FocusingNormal) || mouse_contained)
		{
			//if (!ban && !thiz->hovering)
			//	thiz->hovering = er;

			//auto prev_drag_overing = drag_overing;
			//auto prev_dragging = (!focusing || focusing_state != FocusingAndDragging) ? nullptr : focusing;

			hovering = mouse_contained ? er : nullptr;

			if (is_mouse_down((KeyState)mouse_buttons[Mouse_Left], Mouse_Left, true))
			{
				focusing = nullptr;

				if (hovering)
				{
					focusing = hovering;
					focusing_state = FocusingAndActive;
					active_pos = mouse_pos;
				}
			}

			//if (prev_focusing != focusing)
			//{
			//	key_dispatch_list.clear();

			//	if (prev_focusing)
			//		prev_focusing->focus_listeners.call(false);
			//	if (focusing)
			//	{
			//		auto e = focusing->entity;
			//		while (e)
			//		{
			//			auto er = e->get_component(cEventReceiver);
			//			if (er && er->accept_key)
			//				key_dispatch_list.push_back(er);
			//			e = e->parent();
			//		}

			//		focusing->focus_listeners.call(true);
			//	}
			//}

			if (mouse_disp != 0)
				((cEventReceiverPrivate*)er)->on_mouse(KeyStateNull, Mouse_Null, mouse_disp);
			if (mouse_scroll != 0)
				((cEventReceiverPrivate*)er)->on_mouse(KeyStateNull, Mouse_Middle, Vec2i(mouse_scroll, 0));
			for (auto i = 0; i < array_size(mouse_buttons); i++)
			{
				auto s = mouse_buttons[i];
				if (s & KeyStateJust)
					((cEventReceiverPrivate*)er)->on_mouse(s, (MouseKey)i, mouse_pos);
			}

			pass = false;
		}
	}

	void sEventDispatcherPrivate::update(Entity* root)
	{
		if (!pending_update)
			return;
		pending_update = false;

		mouse_disp = mouse_pos - mouse_pos_prev;

		auto prev_hovering = hovering;
		auto prev_focusing = focusing;
		auto prev_focusing_state = focusing_state;
		hovering = nullptr;

		if (next_focusing != INVALID_POINTER)
		{
			focusing = next_focusing;
			next_focusing = (cEventReceiver*)INVALID_POINTER;
		}

		if (focusing)
		{
			if (!focusing->entity->global_visibility_)
				focusing = nullptr;
			else if (focusing_state != FocusingNormal && ((KeyState)mouse_buttons[Mouse_Left] & KeyStateUp))
				focusing_state = FocusingNormal;
		}

		if (!focusing)
			dbclick_timer = -1.f;
		if (dbclick_timer > 0.f)
		{
			dbclick_timer -= looper().delta_time;
			if (dbclick_timer <= 0.f)
				dbclick_timer = -1.f;
		}

		drag_overing = nullptr;
		if (focusing && focusing_state == FocusingAndActive)
		{
			if (focusing->drag_hash)
			{
				if (mouse_disp != 0 && (abs(mouse_pos.x() - active_pos.x()) > 4.f || abs(mouse_pos.y() - active_pos.y()) > 4.f))
					focusing_state = FocusingAndDragging;
			}
		}

		pass = true;
		dispatch_mouse((EntityPrivate*)root);

		if (focusing && is_mouse_up(mouse_buttons[Mouse_Left], Mouse_Left, true) && rect_contains(focusing->element->cliped_rect, Vec2f(mouse_pos)))
		{
			auto disp = mouse_pos - active_pos;
			auto db = dbclick_timer > 0.f;
			((cEventReceiverPrivate*)focusing)->on_mouse(KeyStateDown | KeyStateUp | (db ? KeyStateDouble : 0), Mouse_Null, disp);
			if (db)
				dbclick_timer = -1.f;
			else
				dbclick_timer = 0.5f;
		}

		if (prev_hovering != hovering)
		{
			if (prev_hovering)
			{
				prev_hovering->state = EventReceiverNormal;
				prev_hovering->state_listeners.call(EventReceiverNormal);
			}
			if (hovering)
			{
				hovering->state = (hovering == focusing && focusing_state != FocusingNormal) ? EventReceiverActive : EventReceiverHovering;
				hovering->state_listeners.call(hovering->state);
			}
		}
		else if (hovering && prev_focusing_state != focusing_state)
		{
			hovering->state = focusing_state != FocusingNormal ? EventReceiverActive : EventReceiverHovering;
			hovering->state_listeners.call(hovering->state);
		}

		//if (!prev_dragging && focusing && focusing_state == FocusingAndDragging)
		//	((cEventReceiverPrivate*)focusing)->on_drag_and_drop(DragStart, nullptr, mouse_pos);
		//else if (prev_dragging && (!focusing || focusing_state != FocusingAndDragging))
		//{
		//	if (prev_drag_overing)
		//		((cEventReceiverPrivate*)prev_drag_overing)->on_drag_and_drop(Dropped, prev_dragging, mouse_pos);
		//	((cEventReceiverPrivate*)prev_dragging)->on_drag_and_drop(DragEnd, prev_drag_overing, mouse_pos);
		//}
		//if (drag_overing)
		//	((cEventReceiverPrivate*)drag_overing)->on_drag_and_drop(DragOvering, focusing, mouse_pos);

		//if (thiz->focusing && thiz->focusing_state == FocusingAndDragging && !thiz->drag_overing && er != thiz->focusing)
		//{
		//	auto hash = thiz->focusing->drag_hash;
		//	for (auto h : er->acceptable_drops)
		//	{
		//		if (h == hash)
		//		{
		//			thiz->drag_overing = er;
		//			break;
		//		}
		//	}
		//}

		//for (auto er : key_dispatch_list)
		//{
		//	for (auto& code : keydown_inputs)
		//		((cEventReceiverPrivate*)er)->on_key(KeyStateDown, code);
		//	for (auto& code : keyup_inputs)
		//		((cEventReceiverPrivate*)er)->on_key(KeyStateUp, code);
		//	for (auto& ch : char_inputs)
		//		((cEventReceiverPrivate*)er)->on_key(KeyStateNull, ch);
		//}

		keydown_inputs.clear();
		keyup_inputs.clear();
		char_inputs.clear();
		for (int i = 0; i < array_size(key_states); i++)
			key_states[i] &= ~KeyStateJust;

		mouse_pos_prev = mouse_pos;
		mouse_scroll = 0;
		for (auto i = 0; i < array_size(mouse_buttons); i++)
			mouse_buttons[i] &= ~KeyStateJust;
	}

	sEventDispatcher* sEventDispatcher::create()
	{
		return new sEventDispatcherPrivate();
	}
}

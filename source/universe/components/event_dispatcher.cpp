#include <flame/universe/entity.h>
#include <flame/universe/components/element.h>
#include "event_receiver_private.h"
#include <flame/universe/components/event_dispatcher.h>

namespace flame
{
	struct cEventDispatcherPrivate : cEventDispatcher
	{
		Window* window;
		void* key_listener;
		void* mouse_listener;

		std::vector<Key> keydown_inputs;
		std::vector<Key> keyup_inputs;
		std::vector<wchar_t> char_inputs;
		bool char_input_compelete;

		cEventReceiver* potential_dbclick_er;
		float potential_dbclick_time;

		std::vector<cEventReceiver*> hovers;

		cEventDispatcherPrivate(Window* window) :
			window(window)
		{
			hovering = nullptr;
			focusing = nullptr;
			drag_overing = nullptr;

			char_input_compelete = true;
			for (auto i = 0; i < FLAME_ARRAYSIZE(key_states); i++)
				key_states[i] = KeyStateUp;

			mouse_pos = Vec2i(0);
			mouse_pos_prev = Vec2i(0);
			mouse_disp = Vec2i(0);
			mouse_scroll = 0;
			for (auto i = 0; i < FLAME_ARRAYSIZE(mouse_buttons); i++)
				mouse_buttons[i] = KeyStateUp;
			potential_dbclick_er = nullptr;
			potential_dbclick_time = 0.f;

			key_listener = window->add_key_listener([](void* c, KeyState action, Key key) {
				auto thiz = *(cEventDispatcherPrivate**)c;

				if (action == KeyStateNull)
				{
					auto value = (int)key;
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
					thiz->key_states[key] = action | KeyStateJust;
					if (action == KeyStateDown)
						thiz->keydown_inputs.push_back(key);
					else if (action == KeyStateUp)
						thiz->keyup_inputs.push_back(key);
				}
				
			}, new_mail_p(this));

			mouse_listener = window->add_mouse_listener([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
				auto thiz = *(cEventDispatcherPrivate * *)c;

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
			}, new_mail_p(this));
		}

		~cEventDispatcherPrivate() 
		{
			window->remove_key_listener(key_listener);
			window->remove_mouse_listener(mouse_listener);
		}

		void update()
		{
			mouse_disp = mouse_pos - mouse_pos_prev;
			if (potential_dbclick_er)
			{
				potential_dbclick_time += looper().delta_time;
				if (potential_dbclick_time > 0.5f)
				{
					potential_dbclick_er = nullptr;
					potential_dbclick_time = 0.f;
				}
			}

			if (focusing)
			{
				if (!focusing->entity->global_visible)
				{
					focusing->hovering = false;
					focusing->focusing = false;
					focusing->dragging = false;
					focusing = nullptr;
				}
				else if (focusing->dragging && is_mouse_up((KeyState)mouse_buttons[Mouse_Left], Mouse_Left))
					focusing->dragging = false;
			}

			auto prev_drag_overing = drag_overing;
			drag_overing = nullptr;
			if (focusing && focusing->dragging)
			{
				if (focusing->drag_hash != 0)
				{
					entity->traverse_backward([](void* c, Entity* e) {
						auto thiz = *(cEventDispatcherPrivate**)c;
						if (thiz->drag_overing)
							return;

						auto er = (cEventReceiverPrivate*)e->find_component(cH("EventReceiver"));
						if (er)
						{
							er->event_dispatcher = thiz;
							if (!er->element->cliped && !er->penetrable && er != thiz->focusing && er->element->contains(Vec2f(thiz->mouse_pos)) && !er->acceptable_drops.empty())
							{
								auto h = thiz->focusing->drag_hash;
								for (auto _h : er->acceptable_drops)
								{
									if (_h == h)
									{
										thiz->drag_overing = er;
										break;
									}
								}
							}
						}
					}, new_mail_p(this));
				}
			}
			else
			{
				if (hovering)
				{
					hovering->hovering = false;
					hovering = nullptr;
				}
				hovers.clear();
				entity->traverse_backward([](void* c, Entity* e) {
					auto thiz = *(cEventDispatcherPrivate**)c;
					if (thiz->hovering)
						return;

					auto er = (cEventReceiver*)e->find_component(cH("EventReceiver"));
					if (er)
					{
						er->event_dispatcher = thiz;
						if (!er->element->cliped && er->element->contains(Vec2f(thiz->mouse_pos)))
						{
							thiz->hovers.push_back(er);
							if (!er->penetrable)
							{
								er->hovering = true;
								thiz->hovering = er;
							}
						}
					}
				}, new_mail_p(this));
			}
			if (is_mouse_down((KeyState)mouse_buttons[Mouse_Left], Mouse_Left, true))
			{
				if (focusing)
				{
					focusing->focusing = false;
					focusing = nullptr;
				}
				if (hovering)
				{
					focusing = hovering;
					hovering->focusing = true;
					hovering->dragging = true;
				}
			}
			if (focusing)
			{
				for (auto& code : keydown_inputs)
					focusing->on_key(KeyStateDown, code);
				for (auto& code : keyup_inputs)
					focusing->on_key(KeyStateUp, code);
				for (auto& ch : char_inputs)
					focusing->on_key(KeyStateNull, ch);
			}
			for (auto it = hovers.rbegin(); it != hovers.rend(); it++)
			{
				auto er = *it;
				if (mouse_disp != 0)
					er->on_mouse(KeyStateNull, Mouse_Null, Vec2f(mouse_disp));
				for (auto i = 0; i < FLAME_ARRAYSIZE(mouse_buttons); i++)
				{
					auto s = mouse_buttons[i];
					if (s & KeyStateJust)
						er->on_mouse((KeyState)s, (MouseKey)i, Vec2f(mouse_pos));
				}
			}
			if (hovering)
			{
				if (is_mouse_up((KeyState)mouse_buttons[Mouse_Left], Mouse_Left, true))
				{
					if (hovering == focusing)
					{
						hovering->on_mouse(KeyState(KeyStateDown | KeyStateUp), Mouse_Null, Vec2f(mouse_pos));
						if (potential_dbclick_er == focusing)
						{
							hovering->on_mouse(KeyState(KeyStateDown | KeyStateUp | KeyStateDouble), Mouse_Null, Vec2f(mouse_pos));
							potential_dbclick_er = nullptr;
							potential_dbclick_time = 0.f;
						}
						else
							potential_dbclick_er = focusing;
					}
				}
			}

			keydown_inputs.clear();
			keyup_inputs.clear();
			char_inputs.clear();
			for (int i = 0; i < FLAME_ARRAYSIZE(key_states); i++)
				key_states[i] &= ~KeyStateJust;

			mouse_pos_prev = mouse_pos;
			mouse_scroll = 0;
			for (auto i = 0; i < FLAME_ARRAYSIZE(mouse_buttons); i++)
				mouse_buttons[i] &= ~KeyStateJust;
		}
	};

	cEventDispatcher::~cEventDispatcher()
	{
		((cEventDispatcherPrivate*)this)->~cEventDispatcherPrivate();
	}

	void cEventDispatcher::update()
	{
		((cEventDispatcherPrivate*)this)->update();
	}

	cEventDispatcher* cEventDispatcher::create(Window* window)
	{
		return new cEventDispatcherPrivate(window);
	}
}

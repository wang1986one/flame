#include <flame/foundation/window.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/event.h>
#include <flame/universe/components/ui.h>

namespace flame
{
	struct cUIPrivate : cUI
	{
		Window* window;

		Vec2i mouse_pos, mouse_pos_prev, mouse_disp;
		int mouse_scroll;
		int mouse_buttons[3];

		bool f_all_done;
		bool f_hovering, f_mljustdown, f_mljustup, f_mrjustdown, f_mrjustup;
		Vec2f f_mpos;
		Vec2i f_mdisp;
		int f_mscroll;

		cUIPrivate(Entity* e, Window* window) :
			cUI(e),
			window(window)
		{
			hovering = nullptr;
			focusing = nullptr;

			mouse_pos = Vec2i(0);
			mouse_pos_prev = Vec2i(0);
			mouse_disp = Vec2i(0);
			mouse_scroll = 0;

			for (auto i = 0; i < FLAME_ARRAYSIZE(mouse_buttons); i++)
				mouse_buttons[i] = KeyStateUp;

			if (window)
			{
				window->add_mouse_listener([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
					auto thiz = *(cUIPrivate * *)c;

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
		}

		void update()
		{
			mouse_disp = mouse_pos - mouse_pos_prev;

			f_mljustdown = is_mouse_down((KeyState)mouse_buttons[Mouse_Left], Mouse_Left, true);
			f_mljustup = is_mouse_up((KeyState)mouse_buttons[Mouse_Left], Mouse_Left, true);
			f_mrjustdown = is_mouse_down((KeyState)mouse_buttons[Mouse_Right], Mouse_Right, true);
			f_mrjustup = is_mouse_up((KeyState)mouse_buttons[Mouse_Right], Mouse_Right, true);
			f_mpos = mouse_pos;
			f_mdisp = mouse_disp;
			f_mscroll = mouse_scroll;

			if (focusing)
			{
				if (!focusing->entity->global_visible)
				{
					focusing->focusing = false;
					focusing->dragging = false;
					focusing = nullptr;
				}
				else if (!is_mouse_down((KeyState)mouse_buttons[Mouse_Left], Mouse_Left))
					focusing->dragging = false;
				else if (focusing->dragging && f_mdisp != 0)
					f_mdisp = Vec2i(0);
			}

			f_all_done = !f_mljustdown && !f_mljustup && !f_mrjustdown && !f_mrjustup && f_mdisp == 0 && f_mscroll == 0;

			entity->traverse_backward([](void* c, Entity* e) {
				auto thiz = *((cUIPrivate**)c);
				if (thiz->f_all_done)
					return;

				auto w = (cEvent*)e->find_component(cH("Event"));
				if (w)
				{
					auto mhover = w->contains(thiz->f_mpos);
					if (w->blackhole || mhover)
					{
						if (thiz->f_mljustdown)
						{
							thiz->f_mljustdown = false;
							thiz->focusing = w;
							w->focusing = true;
							if (mhover)
								w->dragging = true;
						}
						if (thiz->f_mljustup)
							thiz->f_mljustdown = false;
					}

					thiz->f_all_done = !thiz->f_mljustdown && !thiz->f_mljustup && 
						!thiz->f_mrjustdown && !thiz->f_mrjustup && thiz->f_mdisp == 0 && thiz->f_mscroll == 0;
				}
			}, new_mail_p(this));

			if (focusing && f_mljustdown)
			{
				focusing->focusing = false;
				focusing->dragging = false;
				focusing = nullptr;
			}

			for (auto i = 0; i < FLAME_ARRAYSIZE(mouse_buttons); i++)
				mouse_buttons[i] &= ~KeyStateJust;

			mouse_pos_prev = mouse_pos;
			mouse_scroll = 0;
		}
	};

	cUI::cUI(Entity* e) :
		Component("UI", e)
	{
	}

	cUI::~cUI()
	{
	}

	void cUI::update()
	{
		((cUIPrivate*)this)->update();
	}

	cUI* cUI::create(Entity* e, Window* window)
	{
		return new cUIPrivate(e, window);
	}
}

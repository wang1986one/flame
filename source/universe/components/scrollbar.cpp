#include <flame/universe/components/element.h>
#include <flame/universe/components/event_receiver.h>
#include <flame/universe/components/aligner.h>
#include <flame/universe/components/style.h>
#include <flame/universe/components/layout.h>
#include <flame/universe/components/scrollbar.h>
#include <flame/universe/ui/style_stack.h>

namespace flame
{
	struct cScrollbarPrivate : cScrollbar
	{
		cScrollbarPrivate()
		{
			element = nullptr;
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cElement"))
				element = (cElement*)c;
		}
	};

	cScrollbar* cScrollbar::create()
	{
		return new cScrollbarPrivate();
	}

	struct cScrollbarThumbPrivate : cScrollbarThumb
	{
		void* mouse_listener;
		void* parent_element_listener;
		void* target_element_listener;
		void* target_layout_listener;

		cScrollbarThumbPrivate(ScrollbarType _type)
		{
			element = nullptr;
			event_receiver = nullptr;
			scrollbar = nullptr;

			parent_element = nullptr;

			type = _type;
			target_layout = nullptr;
			step = 1.f;

			mouse_listener = nullptr;
			parent_element_listener = nullptr;
			target_element_listener = nullptr;
			target_layout_listener = nullptr;
		}

		~cScrollbarThumbPrivate()
		{
			if (!entity->dying_)
			{
				event_receiver->mouse_listeners.remove(mouse_listener);
				parent_element->data_changed_listeners.remove(parent_element_listener);
				target_layout->element->data_changed_listeners.remove(target_element_listener);
				target_layout->data_changed_listeners.remove(target_layout_listener);
			}
		}

		void on_added() override
		{
			auto parent = entity->parent();
			parent_element = parent->get_component(cElement);
			parent_element_listener = parent_element->data_changed_listeners.add([](void* c, Component* e, uint hash, void*) {
				if (hash == FLAME_CHASH("size"))
					(*(cScrollbarThumbPrivate**)c)->update(0.f);
			}, new_mail_p(this));
			scrollbar = parent->get_component(cScrollbar);
			target_layout = parent->parent()->child(0)->get_component(cLayout);
			target_element_listener = target_layout->element->data_changed_listeners.add([](void* c, Component* e, uint hash, void*) {
				if (hash == FLAME_CHASH("size"))
					(*(cScrollbarThumbPrivate**)c)->update(0.f);
			}, new_mail_p(this));
			target_layout_listener = target_layout->data_changed_listeners.add([](void* c, Component* e, uint hash, void*) {
				if (hash == FLAME_CHASH("content_size"))
					(*(cScrollbarThumbPrivate**)c)->update(0.f);
			}, new_mail_p(this));
			update(0.f);
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cElement"))
				element = (cElement*)c;
			else if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				mouse_listener = event_receiver->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
					auto thiz = (*(cScrollbarThumbPrivate**)c);
					if (thiz->event_receiver->active && is_mouse_move(action, key))
					{
						if (thiz->type == ScrollbarVertical)
							thiz->update(pos.y());
						else
							thiz->update(pos.x());
					}
				}, new_mail_p(this));
			}
		}

		void update(float v)
		{
			auto target_element = target_layout->element;
			if (type == ScrollbarVertical)
			{
				auto content_size = target_layout->content_size.y() + 20.f;
				if (target_element->size_.y() > 0.f)
				{
					if (content_size > target_element->size_.y())
						element->set_height(target_element->size_.y() / content_size * scrollbar->element->size_.y());
					else
						element->set_height(0.f);
				}
				else
					element->set_height(0.f);
				v += element->pos_.y();
				element->set_y(element->size_.y() > 0.f ? clamp(v, 0.f, scrollbar->element->size_.y() - element->size_.y()) : 0.f);
				target_layout->set_y_scroll_offset(-int(element->pos_.y() / scrollbar->element->size_.y() * content_size / step) * step);
			}
			else
			{
				auto content_size = target_layout->content_size.x() + 20.f;
				if (target_element->size_.x() > 0.f)
				{
					if (content_size > target_element->size_.x())
						element->set_width(target_element->size_.x() / content_size * scrollbar->element->size_.x());
					else
						element->set_width(0.f);
				}
				else
					element->set_width(0.f);
				v += element->pos_.x();
				element->set_x(element->size_.x() > 0.f ? clamp(v, 0.f, scrollbar->element->size_.x() - element->size_.x()) : 0.f);
				target_layout->set_x_scroll_offset(-int(element->pos_.x() / scrollbar->element->size_.x() * content_size / step) * step);
			}
			v = 0.f;
		}
	};

	void cScrollbarThumb::update(float v)
	{
		((cScrollbarThumbPrivate*)this)->update(v);
	}

	cScrollbarThumb* cScrollbarThumb::create(ScrollbarType type)
	{
		return new cScrollbarThumbPrivate(type);
	}
}

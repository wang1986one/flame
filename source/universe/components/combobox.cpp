#include "../universe_private.h"
#include <flame/graphics/font.h>
#include <flame/universe/default_style.h>
#include <flame/universe/topmost.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/text.h>
#include <flame/universe/components/menu.h>
#include <flame/universe/components/event_receiver.h>
#include <flame/universe/components/style.h>
#include <flame/universe/components/aligner.h>
#include <flame/universe/components/combobox.h>

namespace flame
{
	struct cComboboxItemPrivate : cComboboxItem
	{
		void* mouse_listener;

		cComboboxItemPrivate()
		{
			event_receiver = nullptr;
			style = nullptr;
			combobox = nullptr;

			unselected_color_normal = default_style.frame_color_normal;
			unselected_color_hovering = default_style.frame_color_hovering;
			unselected_color_active = default_style.frame_color_active;
			selected_color_normal = default_style.selected_color_normal;
			selected_color_hovering = default_style.selected_color_hovering;
			selected_color_active = default_style.selected_color_active;

			idx = -1;

			mouse_listener = nullptr;
		}

		~cComboboxItemPrivate()
		{
			if (!entity->dying_)
				event_receiver->mouse_listeners.remove(mouse_listener);
		}

		void do_style(bool selected)
		{
			if (!selected)
			{
				if (style)
				{
					style->color_normal = unselected_color_normal;
					style->color_hovering = unselected_color_hovering;
					style->color_active = unselected_color_active;
					style->style();
				}
			}
			else
			{
				if (style)
				{
					style->color_normal = selected_color_normal;
					style->color_hovering = selected_color_hovering;
					style->color_active = selected_color_active;
					style->style();
				}
			}
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				mouse_listener = event_receiver->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
					if (is_mouse_down(action, key, true) && key == Mouse_Left)
					{
						auto thiz = *(cComboboxItemPrivate**)c;
						thiz->combobox->set_index(thiz->idx);
						destroy_topmost(thiz->combobox->menu_button->root);
					}
				}, new_mail_p(this));
			}
			else if (c->name_hash == FLAME_CHASH("cStyleColor"))
			{
				style = (cStyleColor*)c;
				do_style(false);
			}
		}
	};

	cComboboxItem* cComboboxItem::create()
	{
		return new cComboboxItemPrivate();
	}

	struct cComboboxPrivate : cCombobox
	{
		cComboboxPrivate()
		{
			text = nullptr;
			menu_button = nullptr;

			idx = -1;
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cText"))
				text = (cText*)c;
			else if (c->name_hash == FLAME_CHASH("cMenuButton"))
			{
				menu_button = (cMenuButton*)c;
				auto menu = menu_button->menu;
				for (auto i = 0; i < menu->child_count(); i++)
					menu->child(i)->get_component(cComboboxItem)->combobox = this;
			}
		}
	};

	void cCombobox::set_index(int _idx, bool trigger_changed)
	{
		auto menu = menu_button->menu;
		if (idx != -1)
		{
			auto comboboxitem = (cComboboxItemPrivate*)menu->child(idx)->get_component(cComboboxItem);
			if (comboboxitem)
				comboboxitem->do_style(false);
		}
		idx = _idx;
		if (idx < 0)
			text->set_text(L"");
		else
		{
			auto selected = menu->child(idx);
			text->set_text(selected->get_component(cText)->text());
			{
				auto comboboxitem = (cComboboxItemPrivate*)selected->get_component(cComboboxItem);
				if (comboboxitem)
					comboboxitem->do_style(true);
			}
		}
		if (trigger_changed)
			data_changed(FLAME_CHASH("index"), nullptr);
	}

	cCombobox* cCombobox::create()
	{
		return new cComboboxPrivate;
	}

	Entity* create_standard_combobox(float width, graphics::FontAtlas* font_atlas, float font_size_scale, Entity* root, uint item_count, const wchar_t* const* items)
	{
		auto e_menu = create_standard_menu();
		for (auto i = 0; i < item_count; i++)
		{
			auto e_item = create_standard_menu_item(font_atlas, font_size_scale, items[i]);
			e_menu->add_child(e_item);

			auto c_combobox_item = cComboboxItem::create();
			c_combobox_item->idx = i;
			e_item->add_component(c_combobox_item);
		}

		auto e_combobox = create_standard_menu_button(font_atlas, font_size_scale, L"", root, e_menu, false, SideS, true, false, false, Icon_ANGLE_DOWN);
		{
			auto c_element = e_combobox->get_component(cElement);
			c_element->size_.x() = width + 8.f;
			c_element->size_.y() = default_style.font_size * font_size_scale + 4.f;
			c_element->frame_color_ = default_style.text_color_normal;
			c_element->frame_thickness_ = 2.f;

			e_combobox->get_component(cText)->auto_width_ = false;
			
			e_combobox->add_component(cCombobox::create());
		}

		return e_combobox;
	}
}

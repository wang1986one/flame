#pragma once

#include <flame/universe/component.h>

namespace flame
{
	struct cElement;
	struct cEventReceiver;
	struct cStyleBgCol;
	struct cList;

	struct cListItem : Component
	{
		cElement* element;
		cEventReceiver* event_receiver;
		cStyleBgCol* style;
		cList* list;

		Vec4c unselected_color_normal;
		Vec4c unselected_color_hovering;
		Vec4c unselected_color_active;
		Vec4c selected_color_normal;
		Vec4c selected_color_hovering;
		Vec4c selected_color_active;

		cListItem() :
			Component("ListItem")
		{
		}

		FLAME_UNIVERSE_EXPORTS virtual ~cListItem() override;

		FLAME_UNIVERSE_EXPORTS virtual void on_added() override;

		FLAME_UNIVERSE_EXPORTS virtual void update() override;

		FLAME_UNIVERSE_EXPORTS static cListItem* create();
	};

	struct cList : Component
	{
		Entity* select;

		cList() :
			Component("List")
		{
		}

		FLAME_UNIVERSE_EXPORTS virtual ~cList() override;

		FLAME_UNIVERSE_EXPORTS virtual void update() override;

		FLAME_UNIVERSE_EXPORTS static cList* create();
	};
}

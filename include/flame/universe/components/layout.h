#pragma once

#include <flame/universe/component.h>

namespace flame
{
	struct cElement;
	struct cAligner;

	struct cLayout : Component
	{
		cElement* element;
		cAligner* aligner;

		LayoutType$ type;
		float item_padding;
		bool width_fit_children;
		bool height_fit_children;

		float scroll_offset;

		cLayout() :
			Component("Layout")
		{
		}

		FLAME_UNIVERSE_EXPORTS virtual ~cLayout() override;

		FLAME_UNIVERSE_EXPORTS virtual void start() override;

		FLAME_UNIVERSE_EXPORTS virtual void update() override;

		FLAME_UNIVERSE_EXPORTS static cLayout* create();
	};
}

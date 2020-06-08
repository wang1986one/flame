#pragma once

#include <flame/universe/components/element.h>

namespace flame
{
	namespace graphics
	{
		struct Canvas;
	}

	struct cElementPrivate : cElement
	{
		cElementPrivate();
		~cElementPrivate();
		void calc_geometry();
		void draw(graphics::Canvas* canvas);
		void on_event(EntityEvent e, void* t) override;
	};
}

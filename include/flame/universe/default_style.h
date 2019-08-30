#pragma once

#include <flame/math.h>
#include <flame/universe/universe.h>

namespace flame
{
	struct DefaultStyle
	{
		Vec4c text_color_normal;
		Vec4c text_color_else;
		Vec4c window_color;
		Vec4c frame_color_normal;
		Vec4c frame_color_hovering;
		Vec4c frame_color_active;
		Vec4c button_color_normal;
		Vec4c button_color_hovering;
		Vec4c button_color_active;
		Vec4c header_color_normal;
		Vec4c header_color_hovering;
		Vec4c header_color_active;
		Vec4c selected_color_normal;
		Vec4c selected_color_hovering;
		Vec4c selected_color_active;
		float sdf_scale;

		FLAME_UNIVERSE_EXPORTS DefaultStyle();
		FLAME_UNIVERSE_EXPORTS void set_to_light();
		FLAME_UNIVERSE_EXPORTS void set_to_dark();
	};

	FLAME_UNIVERSE_EXPORTS extern DefaultStyle default_style;
}

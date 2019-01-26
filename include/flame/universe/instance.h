// MIT License
// 
// Copyright (c) 2018 wjs
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <flame/window.h>
#include <flame/graphics/graphics.h>
#include <flame/ui/ui.h>

namespace flame
{
	struct Window;
	struct FontAtlas;
	
	namespace graphics
	{
		struct Device;
		struct Imageview;
		struct Swapchain;
	}

	namespace ui
	{
		struct Widget;
		struct Canvas;

		const auto MaxImageviewCount = 128;

		FLAME_UI_EXPORTS void init(graphics::Device *d, graphics::SampleCount sample_count = graphics::SampleCount_1);
		FLAME_UI_EXPORTS void deinit();

		enum DefaultStyle
		{
			DefaultStyleDark,
			DefaultStyleLight
		};

		struct Instance
		{
			int key_states[Key_count];

			Ivec2 mouse_pos;
			Ivec2 mouse_disp;
			int mouse_scroll;

			int mouse_buttons[MouseKey_count];

			bool processed_mouse_input;
			bool processed_keyboard_input;

			// mouse just down
			inline bool just_down_M(int idx)
			{
				return mouse_buttons[idx] == (KeyStateJust | KeyStateDown);
			}

			// mouse just up
			inline bool just_up_M(int idx)
			{
				return mouse_buttons[idx] == (KeyStateJust | KeyStateUp);
			}

			// mouse pressing
			inline bool pressing_M(int idx)
			{
				return (mouse_buttons[idx] & KeyStateDown) != 0;
			}

			// key just down
			inline bool just_down_K(Key k)
			{
				return key_states[k] == (KeyStateJust | KeyStateDown);
			}

			// key just up
			inline bool just_up_K(Key k)
			{
				return key_states[k] == (KeyStateJust | KeyStateUp);
			}

			// key pressing
			inline bool pressing_K(Key k)
			{
				return (key_states[k] & KeyStateDown) != 0;
			}

			FLAME_UI_EXPORTS void set_default_style(DefaultStyle s);

			FLAME_UI_EXPORTS Ivec2 size() const;

			FLAME_UI_EXPORTS void on_key(KeyState action, int value);
			FLAME_UI_EXPORTS void on_mouse(KeyState action, MouseKey key, const Ivec2 &pos);
			FLAME_UI_EXPORTS void on_resize(const Ivec2 &size);

			FLAME_UI_EXPORTS graphics::Imageview *imageview(int index);
			FLAME_UI_EXPORTS void set_imageview(int index, graphics::Imageview *v);

			FLAME_UI_EXPORTS Widget *root();
			FLAME_UI_EXPORTS Widget *hovering_widget();
			FLAME_UI_EXPORTS Widget *focus_widget();
			FLAME_UI_EXPORTS Widget *key_focus_widget();
			FLAME_UI_EXPORTS Widget *dragging_widget();
			FLAME_UI_EXPORTS Widget *popup_widget();
			FLAME_UI_EXPORTS void set_hovering_widget(Widget *w);
			FLAME_UI_EXPORTS void set_focus_widget(Widget *w);
			FLAME_UI_EXPORTS void set_key_focus_widget(Widget *w);
			FLAME_UI_EXPORTS void set_dragging_widget(Widget *w);
			FLAME_UI_EXPORTS void set_popup_widget(Widget *w, bool modual = false);
			FLAME_UI_EXPORTS void close_popup();

			FLAME_UI_EXPORTS void begin(float elp_time);
			FLAME_UI_EXPORTS void end(Canvas *canvas, const Vec2 &show_off = Vec2(0.f));

			FLAME_UI_EXPORTS float total_time() const;

			FLAME_UI_EXPORTS static Instance *create(Window *w = nullptr);
			FLAME_UI_EXPORTS static void destroy(Instance *i);
		};

		typedef Instance* InstancePtr;
	}
}
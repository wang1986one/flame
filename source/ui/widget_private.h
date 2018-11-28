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

#include <flame/ui/widget.h>
#include <flame/ui/style.h>
#include <flame/ui/animation.h>
#include <flame/ui/instance.h>

#include <tuple>
#include <vector>

namespace flame
{
	namespace ui
	{
		struct WidgetPrivate : Widget
		{
			Instance *instance;
			WidgetPrivate *parent;
			int layer;

			std::vector<std::pair<uint, Function*>> delay_listener_remove;

			std::vector<std::tuple<WidgetPrivate*, int, int, Function*>> delay_adds;
			std::vector<std::pair<int, int>> delay_removes_by_idx;
			std::vector<WidgetPrivate*> delay_removes_by_ptr;
			std::vector<std::pair<int, int>> delay_takes_by_idx;
			std::vector<WidgetPrivate*> delay_takes_by_ptr;
			std::vector<std::tuple<int, int, int>> delay_takes;
			std::vector<std::tuple<int, int, int>> delay_clears;

			WidgetPrivate(Instance *ui);
			~WidgetPrivate();

			void set_width(float x, Widget *sender = nullptr);
			void set_height(float y, Widget *sender = nullptr);
			void set_size(const Vec2 &v, Widget *sender = nullptr);

			void set_visibility(bool v);

			void add_child(WidgetPrivate *w, int layer = 0, int pos = -1, bool delay = false, PF pf = nullptr, const std::vector<CommonData> &capt = {});
			void remove_child(int layer, int idx, bool delay = false);
			void remove_child(WidgetPrivate *w, bool delay = false);
			void take_child(int layer, int idx, bool delay = false);
			void take_child(WidgetPrivate *w, bool delay = false);
			void clear_children(int layer, int begin, int end, bool delay = false);
			void take_children(int layer, int begin, int end, bool delay = false);
			void remove_from_parent(bool delay = false);
			void take_from_parent(bool delay = false);
			int find_child(WidgetPrivate *w);

			void arrange();

			void add_draw_command(PF pf, const std::vector<CommonData> &capt);

			void add_style(PF pf, const std::vector<CommonData> &capt);

			void add_animation(PF pf, const std::vector<CommonData> &capt);

			void remove_animations();

			void on_draw(Canvas *c, const Vec2 &off, float scl);
			void on_mouseenter();
			void on_mouseleave();
			void on_lmousedown(const Vec2 &mpos);
			void on_rmousedown(const Vec2 &mpos);
			void on_mousemove(const Vec2 &disp);
			void on_clicked();
			void on_doubleclicked();
			void on_mousescroll(int scroll);
			void on_keydown(int code);
			void on_keyup(int code);
			void on_char(wchar_t ch);
			void on_drop(Widget *src);

			void report_changed() const;

			Function *add_listener(uint type, PF pf, const std::vector<CommonData> &capt);
			void remove_listener(uint type, Function *f, bool delay = false);

			void add_data_storages(const char *fmt);
			void add_string_storages(int count);
		};
	}
}


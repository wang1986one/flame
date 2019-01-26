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

#include <flame/file.h>
#include <flame/serialize.h>
#include <flame/system.h>
#include <flame/font.h>

#include <flame/ui/icon.h>
#include <flame/ui/style.h>
#include <flame/ui/canvas.h>
#include "widget_private.h"
#include "instance_private.h"

#include <Windows.h>
#include <assert.h>

namespace flame
{
	namespace ui
	{
		const Vec2 hidden_pos(9999.f);

		inline WidgetPrivate::WidgetPrivate(Instance *ins)
		{
			instance = ins;
			parent = nullptr;
			layer = 0;
		}

		inline WidgetPrivate::~WidgetPrivate()
		{
			for (auto i = 0; i < styles$.size; i++)
				Function::destroy(styles$[i]);
			for (auto i = 0; i < animations$.size; i++)
				Function::destroy(animations$[i]);

			if (this == instance->hovering_widget())
				instance->set_hovering_widget(nullptr);
			if (this == instance->focus_widget())
				instance->set_focus_widget(nullptr);
			if (this == instance->key_focus_widget())
				instance->set_key_focus_widget(nullptr);
			if (this == instance->dragging_widget())
				instance->set_dragging_widget(nullptr);
			if (this == instance->popup_widget())
				instance->set_popup_widget(nullptr);
		}

		inline void WidgetPrivate::set_width(float x, Widget *sender)
		{
			if (size$.x == x)
				return;
			size$.x = x;
			if (sender != this)
				arrange();
			if (parent && parent != sender)
				parent->arrange();
		}

		inline void WidgetPrivate::set_height(float y, Widget *sender)
		{
			if (size$.y == y)
				return;
			size$.y = y;
			if (sender != this)
				arrange();
			if (parent && parent != sender)
				parent->arrange();
		}

		inline void WidgetPrivate::set_size(const Vec2 &v, Widget *sender)
		{
			auto changed = false;
			auto do_arrange = false;
			if (size$.x != v.x)
			{
				size$.x = v.x;
				do_arrange |= (size_policy_hori$ == SizeFitLayout && sender != this);
				changed = true;
			}
			if (size$.y != v.y)
			{
				size$.y = v.y;
				do_arrange |= (size_policy_vert$ == SizeFitLayout && sender != this);
				changed = true;
			}
			if (!changed)
				return;
			if (do_arrange)
				arrange();
			if (parent && parent != sender)
				parent->arrange();
		}

		inline void WidgetPrivate::set_visibility(bool v)
		{
			if (visible$ == v)
				return;

			visible$ = v;
			if (!visible$)
				remove_animations();
			arrange();
			if (parent)
				parent->arrange();
		}

		inline void WidgetPrivate::add_child(WidgetPrivate *w, int layer, int pos, bool delay, bool modual)
		{
			if (delay)
			{
				delay_adds.emplace_back(w, layer, pos, modual);
				return;
			}

			auto &children = layer == 0 ? children_1$ : children_2$;
			if (pos < 0)
				pos = children.size + pos + 1;
			children.insert(pos, w);

			w->parent = this;
			w->layer = layer;

			arrange();

			for (auto i = 0; i < child_listeners$.size; i++)
			{
				auto f = child_listeners$[i];

				auto p = (ChildListenerParm&)f->p;
				p.op() = ChildAdd;
				p.src() = w;
				f->exec();
			}
		}

		inline void WidgetPrivate::remove_child(int layer, int idx, bool delay)
		{
			if (delay)
			{
				delay_removes_by_idx.emplace_back(layer, idx);
				return;
			}

			auto &children = layer == 0 ? children_1$ : children_2$;
			Widget::destroy(children[idx]);
			children.remove(idx);

			arrange();
		}

		inline void WidgetPrivate::remove_child(WidgetPrivate *w, bool delay)
		{
			if (delay)
			{
				delay_removes_by_ptr.push_back(w);
				return;
			}

			auto &children = layer == 0 ? children_1$ : children_2$;
			auto idx = children.find(w);
			if (idx != -1)
			{
				Widget::destroy(children[idx]);
				children.remove(idx);

				arrange();
			}
		}

		inline void WidgetPrivate::take_child(int layer, int idx, bool delay)
		{
			if (delay)
			{
				delay_takes_by_idx.emplace_back(layer, idx);
				return;
			}

			auto &children = layer == 0 ? children_1$ : children_2$;
			children.remove(idx);

			remove_animations();

			arrange();
		}

		inline void WidgetPrivate::take_child(WidgetPrivate *w, bool delay)
		{
			if (delay)
			{
				delay_takes_by_ptr.push_back(w);
				return;
			}

			auto &children = layer == 0 ? children_1$ : children_2$;
			auto idx = children.find(w);
			if (idx != -1)
			{
				children.remove(idx);

				arrange();
			}
		}

		inline void WidgetPrivate::clear_children(int layer, int begin, int end, bool delay)
		{
			if (delay)
			{
				delay_clears.emplace_back(layer, begin, end);
				return;
			}

			auto &children = layer == 0 ? children_1$ : children_2$;
			if (end < 0)
				end = children.size + end + 1;
			for (auto i = begin; i < end; i++)
				Widget::destroy(children[i]);
			children.remove(begin, end - begin);

			arrange();
		}

		inline void WidgetPrivate::take_children(int layer, int begin, int end, bool delay)
		{
			if (delay)
			{
				delay_takes.emplace_back(layer, begin, end);
				return;
			}

			auto &children = layer == 0 ? children_1$ : children_2$;
			if (end == -1)
				end = children.size;
			for (auto i = begin; i < end; i++)
				((WidgetPrivate*)children[i])->remove_animations();
			children.remove(begin, end - begin);

			arrange();
		}

		inline void WidgetPrivate::remove_from_parent(bool delay)
		{
			if (delay)
			{
				if (parent)
					parent->remove_child(this, true);
				return;
			}

			if (parent)
				parent->remove_child(this);
		}

		inline void WidgetPrivate::take_from_parent(bool delay)
		{
			if (delay)
			{
				if (parent)
					parent->take_child(this, true);
				return;
			}

			if (parent)
				parent->take_child(this);
		}

		inline int WidgetPrivate::find_child(WidgetPrivate *w)
		{
			auto &children = layer == 0 ? children_1$ : children_2$;
			return children.find(w);
		}

		inline void WidgetPrivate::set_to_foreground()
		{
			auto &list = layer == 0 ? parent->children_1$ : parent->children_2$;
			for (auto i = list.find(this); i < list.size - 1; i++)
				list[i] = list[i + 1];
			list[list.size - 1] = this;
		}

		inline void WidgetPrivate::arrange()
		{
			switch (layout_type$)
			{
			case LayoutVertical:
			{
				if (size_policy_hori$ == SizeFitChildren || size_policy_hori$ == SizeGreedy)
				{
					auto width = 0;
					for (auto i_c = 0; i_c < children_1$.size; i_c++)
					{
						auto c = children_1$[i_c];

						if (!c->visible$)
							continue;

						width = max(width, c->size$.x);
					}

					width += inner_padding$[0] + inner_padding$[1];
					if (size_policy_hori$ == SizeFitChildren)
						set_width(width, this);
					else if (size_policy_hori$ == SizeGreedy)
					{
						if (width > size$.x)
							set_width(width, this);
					}
				}
				if (size_policy_vert$ == SizeFitChildren || size_policy_vert$ == SizeGreedy)
				{
					auto height = 0;
					for (auto i_c = 0; i_c < children_1$.size; i_c++)
					{
						auto c = children_1$[i_c];

						if (!c->visible$)
							continue;

						height += c->size$.y + item_padding$;
					}
					height -= item_padding$;
					content_size = height;

					height += inner_padding$[2] + inner_padding$[3];
					if (size_policy_vert$ == SizeFitChildren)
						set_height(height, this);
					else if (size_policy_vert$ == SizeGreedy)
					{
						if (height > size$.y)
							set_height(height, this);
					}
				}
				else if (size_policy_vert$ == SizeFitLayout)
				{
					auto cnt = 0;
					auto height = 0;
					for (auto i_c = 0; i_c < children_1$.size; i_c++)
					{
						auto c = children_1$[i_c];

						if (!c->visible$)
							continue;

						if (c->size_policy_vert$ == SizeFitLayout)
							cnt++;
						else
							height += c->size$.y;
						height += item_padding$;
					}
					height -= item_padding$;
					content_size = height;

					height = max(0, (size$.y - inner_padding$[2] - inner_padding$[3] - height) / cnt);

					for (auto i_c = 0; i_c < children_1$.size; i_c++)
					{
						auto c = children_1$[i_c];

						if (!c->visible$)
							continue;

						if (c->size_policy_vert$ == SizeFitLayout)
							c->set_height(height, this);
					}
				}

				auto width = size$.x - inner_padding$[0] - inner_padding$[1];
				auto height = size$.y - inner_padding$[2] - inner_padding$[3];

				auto content_size = get_content_size();
				scroll_offset$ = content_size > height ? clamp((float)scroll_offset$, height - content_size, 0.f) : 0.f;

				auto y = inner_padding$[2] + scroll_offset$;

				for (auto i_c = 0; i_c < children_1$.size; i_c++)
				{
					auto c = children_1$[i_c];

					if (!c->visible$)
						continue;

					if (c->size_policy_hori$ == SizeFitLayout)
						c->set_width(width, this);
					else if (c->size_policy_hori$ == SizeGreedy)
					{
						if (width > c->size$.x)
							c->set_width(width, this);
					}

					switch (c->align$)
					{
					case AlignLittleEnd:
						c->pos$ = Vec2(inner_padding$[0] + c->layout_padding$, y);
						break;
					case AlignLargeEnd:
						c->pos$ = Vec2(size$.x - inner_padding$[1] - c->size$.x - c->layout_padding$, y);
						break;
					case AlignMiddle:
						c->pos$ = Vec2((size$.x - inner_padding$[0] - inner_padding$[1] - c->size$.x) * 0.5f + inner_padding$[0], y);
						break;
					}

					y += c->size$.y + item_padding$;
				}
			}
				break;
			case LayoutHorizontal:
			{
				if (size_policy_hori$ == SizeFitChildren || size_policy_hori$ == SizeGreedy)
				{
					auto width = 0;
					for (auto i_c = 0; i_c < children_1$.size; i_c++)
					{
						auto c = children_1$[i_c];

						if (!c->visible$)
							continue;

						width += c->size$.x + item_padding$;
					}
					width -= item_padding$;
					content_size = width;

					width += inner_padding$[0] + inner_padding$[1];
					if (size_policy_hori$ == SizeFitChildren)
						set_width(width, this);
					else if (size_policy_hori$ == SizeGreedy)
					{
						if (width > size$.x)
							set_width(width, this);
					}
				}
				else if (size_policy_hori$ == SizeFitLayout)
				{
					auto cnt = 0;
					auto width = 0;
					for (auto i_c = 0; i_c < children_1$.size; i_c++)
					{
						auto c = children_1$[i_c];

						if (!c->visible$)
							continue;

						if (c->size_policy_hori$ == SizeFitLayout)
							cnt++;
						else
							width += c->size$.x;
						width += item_padding$;
					}
					width -= item_padding$;
					content_size = width;

					width = max(0, (size$.x - inner_padding$[0] - inner_padding$[1] - width) / cnt);

					for (auto i_c = 0; i_c < children_1$.size; i_c++)
					{
						auto c = children_1$[i_c];

						if (!c->visible$)
							continue;

						if (c->size_policy_hori$ == SizeFitLayout)
							c->set_width(width, this);
					}
				}
				if (size_policy_vert$ == SizeFitChildren || size_policy_vert$ == SizeGreedy)
				{
					auto height = 0;
					for (auto i_c = 0; i_c < children_1$.size; i_c++)
					{
						auto c = children_1$[i_c];

						if (!c->visible$)
							continue;

						height = max(height, c->size$.y);
					}

					height += inner_padding$[2] + inner_padding$[3];
					if (size_policy_vert$ == SizeFitChildren)
						set_height(height, this);
					else if (size_policy_vert$ == SizeGreedy)
					{
						if (height > size$.y)
							set_height(height, this);
					}
				}

				auto height = size$.y - inner_padding$[2] - inner_padding$[3];
				auto x = inner_padding$[0];
				for (auto i_c = 0; i_c < children_1$.size; i_c++)
				{
					auto c = children_1$[i_c];

					if (!c->visible$)
						continue;

					if (c->size_policy_vert$ == SizeFitLayout)
						c->set_height(height, this);
					else if (c->size_policy_vert$ == SizeGreedy)
					{
						if (height > c->size$.y)
							c->set_height(height, this);
					}

					switch (c->align$)
					{
					case AlignLittleEnd:
						c->pos$ = Vec2(x, inner_padding$[2] + c->layout_padding$);
						break;
					case AlignLargeEnd:
						c->pos$ = Vec2(x, size$.y - inner_padding$[3] - c->size$.y - c->layout_padding$);
						break;
					case AlignMiddle:
						c->pos$ = Vec2(x, (size$.y - inner_padding$[2] - inner_padding$[3] - c->size$.y) * 0.5f + inner_padding$[2]);
						break;
					}

					x += c->size$.x + item_padding$;
				}
			}
				break;
			case LayoutGrid:
			{
				auto pos = Vec2(inner_padding$[0], inner_padding$[2]);

				auto cnt = 0;
				auto line_height = 0.f;
				auto max_width = 0.f;
				for (auto i_c = 0; i_c < children_1$.size; i_c++)
				{
					auto c = children_1$[i_c];

					c->pos$ = pos;
					line_height = max(line_height, c->size$.y);

					pos.x += c->size$.x + item_padding$;
					max_width = max(max_width, pos.x);
					cnt++;
					if (cnt >= grid_hori_count$)
					{
						pos.x = inner_padding$[0];
						pos.y += line_height + item_padding$;
						cnt = 0;
						line_height = 0.f;
					}
				}

				if (size_policy_hori$ == SizeFitChildren)
					set_width(max(max_width - item_padding$, 0.f), this);
				if (size_policy_vert$ == SizeFitChildren)
					set_height(max(pos.y - item_padding$, 0.f), this);
			}
				break;
			}

			for (auto i_c = 0; i_c < children_2$.size; i_c++)
			{
				auto c = children_2$[i_c];

				switch (c->align$)
				{
				case AlignLeft:
					c->pos$ = Vec2(inner_padding$[0] + c->layout_padding$,
						(size$.y - inner_padding$[2] - inner_padding$[3] - c->size$.y) * 0.5f + inner_padding$[2]);
					break;
				case AlignRight:
					if (c->size_policy_vert$ == SizeFitLayout)
						c->size$.y = size$.y - inner_padding$[2] - inner_padding$[3];
					c->pos$ = Vec2(size$.x - inner_padding$[1] - c->size$.x - c->layout_padding$,
						(size$.y - inner_padding$[2] - inner_padding$[3] - c->size$.y) * 0.5f + inner_padding$[2]);
					break;
				case AlignTop:
					c->pos$ = Vec2((size$.x - inner_padding$[0] - inner_padding$[1] - c->size$.x) * 0.5f + inner_padding$[0],
						inner_padding$[2] + c->layout_padding$);
					break;
				case AlignBottom:
					c->pos$ = Vec2((size$.x - inner_padding$[0] - inner_padding$[1] - c->size$.x) * 0.5f + inner_padding$[0],
						size$.y - inner_padding$[3] - c->size$.y - c->layout_padding$);
					break;
				case AlignLeftTop:
					c->pos$ = Vec2(inner_padding$[0] + c->layout_padding$,
						inner_padding$[2] + c->layout_padding$);
					break;
				case AlignLeftBottom:
					c->pos$ = Vec2(inner_padding$[0] + c->layout_padding$,
						size$.y - inner_padding$[3] - c->size$.y - c->layout_padding$);
					break;
				case AlignRightTop:
					c->pos$ = Vec2(size$.x - inner_padding$[1] - c->size$.x - c->layout_padding$,
						inner_padding$[2] + c->layout_padding$);
					break;
				case AlignRightBottom:
					c->pos$ = Vec2(size$.x - inner_padding$[1] - c->size$.x - c->layout_padding$,
						size$.y - inner_padding$[3] - c->size$.y - c->layout_padding$);
					break;
				case AlignLeftNoPadding:
					c->pos$ = Vec2(c->layout_padding$, (size$.y - c->size$.y) * 0.5f);
					break;
				case AlignRightNoPadding:
					c->pos$ = Vec2(size$.x - c->size$.x - c->layout_padding$, (size$.y - c->size$.y) * 0.5f);
					break;
				case AlignTopNoPadding:
					c->pos$ = Vec2((size$.x - c->size$.x) * 0.5f, c->layout_padding$);
					break;
				case AlignBottomNoPadding:
					c->pos$ = Vec2((size$.x - c->size$.x) * 0.5f, size$.y - c->size$.y - c->layout_padding$);
					break;
				case AlignLeftTopNoPadding:
					c->pos$ = Vec2(c->layout_padding$, c->layout_padding$);
					break;
				case AlignLeftBottomNoPadding:
					c->pos$ = Vec2(c->layout_padding$, size$.y - c->size$.y - c->layout_padding$);
					break;
				case AlignRightTopNoPadding:
					c->pos$ = Vec2(size$.x - c->size$.x - c->layout_padding$, c->layout_padding$);
					break;
				case AlignRightBottomNoPadding:
					c->pos$ = size$ - c->size$ - Vec2(c->layout_padding$);
					break;
				case AlignCenter:
					c->pos$ = (size$ - c->size$) * 0.5f;
					break;
				case AlignLeftOutside:
					c->pos$ = Vec2(-c->size$.x, 0.f);
					break;
				case AlignRightOutside:
					c->pos$ = Vec2(size$.x, 0.f);
					break;
				case AlignTopOutside:
					c->pos$ = Vec2(0.f, -c->size$.y);
					break;
				case AlignBottomOutside:
					c->pos$ = Vec2(0.f, size$.y);
					break;
				}
			}
		}

		inline void WidgetPrivate::add_extra_draw(PF pf, const std::vector<CommonData> &_capt)
		{
			auto capt = _capt;
			capt.emplace(capt.begin(), this);
			extra_draws$.push_back(Function::create(pf, ExtraDrawParm::PARM_SIZE, capt));
		}

		inline void WidgetPrivate::add_style(int closet_id, PF pf, const std::vector<CommonData> &_capt, int pos)
		{
			auto capt = _capt;
			capt.emplace(capt.begin(), closet_id);
			capt.emplace(capt.begin(), this);
			if (pos == -1)
				pos = styles$.size;
			styles$.insert(pos, Function::create(pf, StyleParm::PARM_SIZE, capt));
		}

		inline void WidgetPrivate::remove_style(int idx)
		{
			auto s = styles$[idx];
			styles$.remove(idx);
			Function::destroy(s);
		}

		inline void WidgetPrivate::add_animation(float duration, int looping, PF pf, const std::vector<CommonData> &_capt)
		{
			auto capt = _capt;
			capt.emplace(capt.begin(), looping);
			capt.emplace(capt.begin(), duration);
			capt.emplace(capt.begin(), this);
			auto f = Function::create(pf, AnimationParm::PARM_SIZE, capt);
			auto &p = (AnimationParm&)f->p;
			p.time() = 0.f;
			animations$.push_back(f);
		}

		void WidgetPrivate::remove_animations()
		{
			for (auto i = 0; i < animations$.size; i++)
			{
				auto f = animations$[i];
				auto &p = (AnimationParm&)f->p;
				p.time() = -1.f;
				f->exec();
				Function::destroy(animations$[i]);
			}
			animations$.resize(0);
			for (auto i = 0; i < children_1$.size; i++)
				((WidgetPrivate*)children_1$[i])->remove_animations();
			for (auto i = 0; i < children_2$.size; i++)
				((WidgetPrivate*)children_2$[i])->remove_animations();
		}

		inline void WidgetPrivate::on_draw(Canvas *c, const Vec2 &off, float scl)
		{
			if (draw_default$)
			{
				auto p = (pos$ - Vec2(background_offset$[0], background_offset$[1])) * scl + off;
				auto ss = scl * scale$;
				auto s = (size$ + Vec2(background_offset$[0] + background_offset$[2], background_offset$[1] + background_offset$[3])) * ss;
				auto rr = background_round_radius$ * ss;

				if (background_shaow_thickness$ > 0.f)
				{
					c->add_rect_col2(p - Vec2(background_shaow_thickness$ * 0.5f), s + Vec2(background_shaow_thickness$), Bvec4(0, 0, 0, 128), Bvec4(0),
						background_shaow_thickness$, rr, background_round_flags$);
				}
				if (alpha$ > 0.f)
				{
					if (background_col$.w > 0)
						c->add_rect_filled(p, s, Bvec4(background_col$, alpha$), rr, background_round_flags$);
					if (background_frame_thickness$ > 0.f && background_frame_col$.w > 0)
						c->add_rect(p, s, Bvec4(background_frame_col$, alpha$), background_frame_thickness$, rr, background_round_flags$);
				}
			}
			for (auto i = 0; i < extra_draws$.size; i++)
			{
				auto f = extra_draws$[i];
				auto &p = (ExtraDrawParm&)f->p;
				p.canvas() = c;
				p.off() = off;
				p.scl() = scl;
				f->exec();
			}
		}

		void WidgetPrivate::on_focus(FocusType type, int focus_or_keyfocus)
		{
			for (auto i = 0; i < focus_listeners$.size; i++)
			{
				auto f = focus_listeners$[i];
				auto &p = (FoucusListenerParm&)f->p;
				p.type() = type;
				p.focus_or_keyfocus() = focus_or_keyfocus;
				f->exec();
			}
		}

		inline void WidgetPrivate::on_key(KeyState action, int value)
		{
			for (auto i = 0; i < key_listeners$.size; i++)
			{
				auto f = key_listeners$[i];
				auto &p = (KeyListenerParm&)f->p;
				p.action() = action;
				p.value() = value;
				f->exec();
			}
		}

		inline void WidgetPrivate::on_mouse(KeyState action, MouseKey key, const Vec2 &value)
		{
			for (auto i = 0; i < mouse_listeners$.size; i++)
			{
				auto f = mouse_listeners$[i];
				auto &p = (MouseListenerParm&)f->p;
				p.action() = action;
				p.key() = key;
				p.value() = value;
				f->exec();
			}
		}

		inline void WidgetPrivate::on_drop(Widget *src)
		{
			for (auto i = 0; i < drop_listeners$.size; i++)
			{
				auto f = drop_listeners$[i];
				auto &p = (DropListenerParm&)f->p;
				p.src() = src;
				f->exec();
			}
		}

		inline void WidgetPrivate::on_changed()
		{
			for (auto i = 0; i < changed_listeners$.size; i++)
			{
				auto f = changed_listeners$[i];
				auto &p = (ChangedListenerParm&)f->p;
				f->exec();
			}
		}

		inline Function *WidgetPrivate::add_listener(Listener l, PF pf, void *thiz, const std::vector<CommonData> &_capt)
		{
			auto parm_cnt = 0;
			Array<Function*> *list;

			switch (l)
			{
			case ListenerFocus:
				parm_cnt = FoucusListenerParm::PARM_SIZE;
				list = &focus_listeners$;
				break;
			case ListenerKey:
				parm_cnt = KeyListenerParm::PARM_SIZE;
				list = &key_listeners$;
				break;
			case ListenerMouse:
				parm_cnt = MouseListenerParm::PARM_SIZE;
				list = &mouse_listeners$;
				break;
			case ListenerDrop:
				parm_cnt = DropListenerParm::PARM_SIZE;
				list = &drop_listeners$;
				break;
			case ListenerChanged:
				parm_cnt = ChangedListenerParm::PARM_SIZE;
				list = &changed_listeners$;
				break;
			case ListenerChild:
				parm_cnt = ChildListenerParm::PARM_SIZE;
				list = &child_listeners$;
				break;
			default:
				assert(0);
				return nullptr;
			}

			auto capt = _capt;
			capt.emplace(capt.begin(), thiz);
			auto f = Function::create(pf, parm_cnt, capt);
			list->push_back(f);
			return f;
		}

		inline void WidgetPrivate::remove_listener(Listener l, Function *f, bool delay)
		{
			if (delay)
			{
				delay_listener_remove.emplace_back(l, f);
				return;
			}

			Array<Function*> *list;

			switch (l)
			{
			case ListenerFocus:
				list = &focus_listeners$;
				break;
			case ListenerKey:
				list = &key_listeners$;
				break;
			case ListenerMouse:
				list = &mouse_listeners$;
				break;
			case ListenerDrop:
				list = &drop_listeners$;
				break;
			case ListenerChanged:
				list = &changed_listeners$;
				break;
			case ListenerChild:
				list = &child_listeners$;
				break;
			default:
				assert(0);
				return;
			}

			list->remove(list->find(f));
			Function::destroy(f);
		}

		inline void WidgetPrivate::add_data_storages(const std::vector<CommonData> &datas)
		{
			if (datas.empty())
				return;

			auto original_size = data_storages$.size;
			data_storages$.resize(original_size + datas.size());
			auto d = &data_storages$[original_size];
			for (auto &t : datas)
			{
				*d = t;

				d++;
			}
		}

		inline void WidgetPrivate::add_string_storages(int count)
		{
			if (count == 0)
				return;
			string_storages$.resize(string_storages$.size + count);
		}

		void Widget::set_width(float x, Widget *sender)
		{
			((WidgetPrivate*)this)->set_width(x, sender);
		}

		void Widget::set_height(float y, Widget *sender)
		{
			((WidgetPrivate*)this)->set_height(y, sender);
		}

		void Widget::set_size(const Vec2 &v, Widget *sender)
		{
			((WidgetPrivate*)this)->set_size(v, sender);
		}

		void Widget::set_visibility(bool v)
		{
			((WidgetPrivate*)this)->set_visibility(v);
		}

		Instance *Widget::instance() const
		{
			return ((WidgetPrivate*)this)->instance;
		}

		Widget *Widget::parent() const
		{
			return ((WidgetPrivate*)this)->parent;
		}

		int Widget::layer() const
		{
			return ((WidgetPrivate*)this)->layer;
		}

		void Widget::add_child(Widget *w, int layer, int pos, bool delay, bool modual)
		{
			((WidgetPrivate*)this)->add_child((WidgetPrivate*)w, layer, pos, delay, modual);
		}

		void Widget::remove_child(int layer, int idx, bool delay)
		{
			((WidgetPrivate*)this)->remove_child(idx, delay);
		}

		void Widget::remove_child(Widget *w, bool delay)
		{
			((WidgetPrivate*)this)->remove_child((WidgetPrivate*)w, delay);
		}

		void Widget::take_child(int layer, int idx, bool delay)
		{
			((WidgetPrivate*)this)->take_child(layer, idx, delay);
		}

		void Widget::take_child(Widget *w, bool delay)
		{
			((WidgetPrivate*)this)->take_child((WidgetPrivate*)w, delay);
		}

		void Widget::clear_children(int layer, int begin, int end, bool delay)
		{
			((WidgetPrivate*)this)->clear_children(layer, begin, end, delay);
		}

		void Widget::take_children(int layer, int begin, int end, bool delay)
		{
			((WidgetPrivate*)this)->take_children(layer, begin, end, delay);
		}

		void Widget::remove_from_parent(bool delay)
		{
			((WidgetPrivate*)this)->remove_from_parent(delay);
		}

		void Widget::take_from_parent(bool delay)
		{
			((WidgetPrivate*)this)->take_from_parent(delay);
		}

		int Widget::find_child(Widget *w)
		{
			return ((WidgetPrivate*)this)->find_child((WidgetPrivate*)w);
		}

		void Widget::set_to_foreground()
		{
			((WidgetPrivate*)this)->set_to_foreground();
		}

		const auto scroll_spare_spacing = 20.f;

		float Widget::get_content_size() const
		{
			return content_size + scroll_spare_spacing;
		}

		void Widget::arrange()
		{
			((WidgetPrivate*)this)->arrange();
		}

		void Widget::add_extra_draw(PF pf, const std::vector<CommonData> &capt)
		{
			((WidgetPrivate*)this)->add_extra_draw(pf, capt);
		}

		void Widget::add_style(int closet_id, PF pf, const std::vector<CommonData> &capt, int pos)
		{
			((WidgetPrivate*)this)->add_style(closet_id, pf, capt, pos);
		}

		void Widget::remove_style(int idx)
		{
			((WidgetPrivate*)this)->remove_style(idx);
		}

		void Widget::add_animation(float duration, int looping, PF pf, const std::vector<CommonData> &capt)
		{
			((WidgetPrivate*)this)->add_animation(duration, looping, pf, capt);
		}

		void Widget::on_draw(Canvas *c, const Vec2 &off, float scl)
		{
			((WidgetPrivate*)this)->on_draw(c, off, scl);
		}

		void Widget::on_focus(FocusType type, int focus_or_keyfocus)
		{
			((WidgetPrivate*)this)->on_focus(type, focus_or_keyfocus);
		}

		void Widget::on_key(KeyState action, int value)
		{
			((WidgetPrivate*)this)->on_key(action, value);
		}

		void Widget::on_mouse(KeyState action, MouseKey key, const Vec2 &pos)
		{
			((WidgetPrivate*)this)->on_mouse(action, key, pos);
		}

		void Widget::on_drop(Widget *src)
		{
			((WidgetPrivate*)this)->on_drop(src);
		}

		void Widget::on_changed()
		{
			((WidgetPrivate*)this)->on_changed();
		}

		Function *Widget::add_listener(Listener l, PF pf, void *thiz, const std::vector<CommonData> &capt)
		{
			return ((WidgetPrivate*)this)->add_listener(l, pf, thiz, capt);
		}

		void Widget::remove_listener(Listener l, Function *f, bool delay)
		{
			((WidgetPrivate*)this)->remove_listener(l, f, delay);
		}

		void Widget::add_data_storages(const std::vector<CommonData> &datas)
		{
			((WidgetPrivate*)this)->add_data_storages(datas);
		}

		void Widget::add_string_storages(int count)
		{
			((WidgetPrivate*)this)->add_string_storages(count);
		}

		SerializableNode *Widget::save()
		{
			return SerializableNode::serialize(find_udt(cH("ui::Widget")), this, 1);
		}

		Widget *Widget::create(Instance *ins)
		{
			return new WidgetPrivate(ins);
		}

		void Widget::create_from_typeinfo(Instance *ins, VaribleInfo *info, void *p, Widget *dst)
		{
			switch (info->tag())
			{
			case VariableTagEnumSingle:
			{
				auto c = createT<wCombo>(ins, find_enum(info->type_hash()), (char*)p + info->offset());
				c->align$ = AlignLittleEnd;
				dst->add_child(c, 0, -1, true);
			}
				break;
			case VariableTagEnumMulti:
				break;
			case VariableTagVariable:
			{
				switch (info->type_hash())
				{
				case cH("bool"):
				{
					auto c = createT<wCheckbox>(ins, (char*)p + info->offset());
					c->align$ = AlignLittleEnd;
					dst->add_child(c, 0, -1, true);
				}
					break;
				case cH("uint"):
				{
					auto e = createT<wEdit>(ins, wEdit::TypeUint, (char*)p + info->offset());
					e->size_policy_hori$ = SizeFitLayout;
					e->align$ = AlignLittleEnd;
					e->set_size_by_width(10.f);
					dst->add_child(e, 0, -1, true);
				}
					break;
				case cH("int"):
				{
					auto e = createT<wEdit>(ins, wEdit::TypeInt, (char*)p + info->offset());
					e->size_policy_hori$ = SizeFitLayout;
					e->align$ = AlignLittleEnd;
					e->set_size_by_width(10.f);
					dst->add_child(e, 0, -1, true);
				}
					break;
				case cH("Ivec2"):
				{
					auto pp = (Ivec2*)((char*)p + info->offset());

					for (auto i_v = 0; i_v < 2; i_v++)
					{
						auto e  = createT<wEdit>(ins, wEdit::TypeInt, &((*pp)[i_v]));
						e->size_policy_hori$ = SizeFitLayout;
						e->align$ = AlignLittleEnd;
						e->set_size_by_width(10.f);
						dst->add_child(e, 0, -1, true);
					}
				}
					break;
				case cH("Ivec3"):
				{
					auto pp = (Ivec3*)((char*)p + info->offset());

					for (auto i_v = 0; i_v < 3; i_v++)
					{
						auto e = createT<wEdit>(ins, wEdit::TypeInt, &((*pp)[i_v]));
						e->size_policy_hori$ = SizeFitLayout;
						e->align$ = AlignLittleEnd;
						e->set_size_by_width(10.f);
						dst->add_child(e, 0, -1, true);
					}
				}
					break;
				case cH("Ivec4"):
				{
					auto pp = (Ivec4*)((char*)p + info->offset());

					for (auto i_v = 0; i_v < 4; i_v++)
					{
						auto e = createT<wEdit>(ins, wEdit::TypeInt, &((*pp)[i_v]));
						e->size_policy_hori$ = SizeFitLayout;
						e->align$ = AlignLittleEnd;
						e->set_size_by_width(10.f);
						dst->add_child(e, 0, -1, true);
					}
				}
					break;
				case cH("float"):
				{
					auto e = createT<wEdit>(ins, wEdit::TypeFloat, (char*)p + info->offset());
					e->size_policy_hori$ = SizeFitLayout;
					e->align$ = AlignLittleEnd;
					e->set_size_by_width(10.f);
					dst->add_child(e, 0, -1, true);
				}
					break;
				case cH("Vec2"):
				{
					auto pp = (Vec2*)((char*)p + info->offset());

					for (auto i_v = 0; i_v < 2; i_v++)
					{
						auto e = createT<wEdit>(ins, wEdit::TypeFloat, &((*pp)[i_v]));
						e->size_policy_hori$ = SizeFitLayout;
						e->align$ = AlignLittleEnd;
						e->set_size_by_width(10.f);
						dst->add_child(e, 0, -1, true);
					}
				}
					break;
				case cH("Vec3"):
				{
					auto pp = (Vec3*)((char*)p + info->offset());

					for (auto i_v = 0; i_v < 3; i_v++)
					{
						auto e = createT<wEdit>(ins, wEdit::TypeFloat, &((*pp)[i_v]));
						e->size_policy_hori$ = SizeFitLayout;
						e->align$ = AlignLittleEnd;
						e->set_size_by_width(10.f);
						dst->add_child(e, 0, -1, true);
					}
				}
					break;
				case cH("Vec4"):
				{
					auto pp = (Vec4*)((char*)p + info->offset());

					for (auto i_v = 0; i_v < 4; i_v++)
					{
						auto e = createT<wEdit>(ins, wEdit::TypeFloat, &((*pp)[i_v]));
						e->size_policy_hori$ = SizeFitLayout;
						e->align$ = AlignLittleEnd;
						e->set_size_by_width(10.f);
						dst->add_child(e, 0, -1, true);
					}
				}
					break;
				case cH("uchar"):
				{
					auto e = createT<wEdit>(ins, wEdit::TypeUchar, (char*)p + info->offset());
					e->size_policy_hori$ = SizeFitLayout;
					e->align$ = AlignLittleEnd;
					e->set_size_by_width(10.f);
					dst->add_child(e, 0, -1, true);
				}
					break;
				case cH("Bvec2"):
				{
					auto pp = (Bvec2*)((char*)p + info->offset());

					for (auto i_v = 0; i_v < 2; i_v++)
					{
						auto e = createT<wEdit>(ins, wEdit::TypeUchar, &((*pp)[i_v]));
						e->size_policy_hori$ = SizeFitLayout;
						e->align$ = AlignLittleEnd;
						e->set_size_by_width(10.f);
						dst->add_child(e, 0, -1, true);
					}
				}
					break;
				case cH("Bvec3"):
				{
					auto pp = (Bvec3*)((char*)p + info->offset());

					for (auto i_v = 0; i_v < 3; i_v++)
					{
						auto e = createT<wEdit>(ins, wEdit::TypeUchar, &((*pp)[i_v]));
						e->size_policy_hori$ = SizeFitLayout;
						e->align$ = AlignLittleEnd;
						e->set_size_by_width(10.f);
						dst->add_child(e, 0, -1, true);
					}
				}
					break;
				case cH("Bvec4"):
				{
					auto pp = (Bvec4*)((char*)p + info->offset());

					for (auto i_v = 0; i_v < 4; i_v++)
					{
						auto e = createT<wEdit>(ins, wEdit::TypeUchar, &((*pp)[i_v]));
						e->size_policy_hori$ = SizeFitLayout;
						e->align$ = AlignLittleEnd;
						e->set_size_by_width(10.f);
						dst->add_child(e, 0, -1, true);
					}
				}
					break;
				}
			}
				break;
			case VariableTagArrayOfVariable:
			{
			}
				break;
			case VariableTagArrayOfPointer:
			{
				auto &arr = *(Array<void*>*)((char*)p + info->offset());

				switch (info->type_hash())
				{
				case cH("Function"):
					for (auto i_i = 0; i_i < arr.size; i_i++)
					{
						auto f = (Function*)arr[i_i];
						auto r = find_registered_function(f->pf);

					}
					break;
				}
			}
				break;
			}
		}

		Widget *Widget::create_from_file(Instance *ins, SerializableNode *src)
		{
			FLAME_DATA_PACKAGE_BEGIN(ObjGeneratorData, SerializableNode::ObjGeneratorParm)
				FLAME_DATA_PACKAGE_CAPT(InstancePtr, ins, p)
			FLAME_DATA_PACKAGE_END

			return (Widget*)src->unserialize(find_udt(cH("ui::Widget")), [](const ParmPackage &_p) {
				auto &p = (ObjGeneratorData&)_p;

				auto w = (WidgetPrivate*)Widget::create(p.ins());
				p.out_obj() = w;

				if (p.parent())
				{
					w->parent = (WidgetPrivate*)p.parent();
					if (p.att_hash() == cH("children_1"))
						w->layer = 0;
					else /* if (att_hash == cH("children_2")) */
						w->layer = 1;
				}
			}, { ins });
		}

		void Widget::destroy(Widget *w)
		{
			delete(WidgetPrivate*)w;
		}

		void wLayout::init(LayoutType type, float item_padding)
		{
			class$ = "layout";
			add_data_storages({ });
			add_string_storages(STRING_SIZE);

			layout_type$ = type;
			item_padding$ = item_padding;
			event_attitude$ = EventIgnore;
			size_policy_hori$ = SizeFitChildren;
			size_policy_vert$ = SizeFitChildren;
		}

		FLAME_REGISTER_FUNCTION_BEG(CheckboxMouse, FLAME_GID(15432), Widget::MouseListenerParm)
			if (!(p.action() == (KeyStateDown | KeyStateUp) && p.key() == Mouse_Null))
				return;

			((wCheckbox*)p.thiz())->checked() = !((wCheckbox*)p.thiz())->checked();

			if (((wCheckbox*)p.thiz())->target())
				*(bool*)((wCheckbox*)p.thiz())->target() = ((wCheckbox*)p.thiz())->checked();

			p.thiz()->on_changed();
		FLAME_REGISTER_FUNCTION_END(CheckboxMouse)

		FLAME_REGISTER_FUNCTION_BEG(CheckboxExtraDraw, FLAME_GID(8818), Widget::ExtraDrawParm)
			p.canvas()->add_rect(p.thiz()->pos$ * p.scl() + p.off(), p.thiz()->size$ * p.scl(), p.thiz()->background_col$, 2.f * p.scl());
			if (((wCheckbox*)p.thiz())->checked())
				p.canvas()->add_rect_filled((p.thiz()->pos$ + 3.f) * p.scl() + p.off(), (p.thiz()->size$ - 6.f) * p.scl(), p.thiz()->background_col$);
		FLAME_REGISTER_FUNCTION_END(CheckboxExtraDraw)

		void wCheckbox::init(void *_target)
		{
			class$ = "checkbox";
			add_data_storages({ 0, _target });
			add_string_storages(STRING_SIZE);
			
			size$ = Vec2(share_data.font_atlas->pixel_height);
			background_col$ = Bvec4(255);

			if (target())
				checked() = *(bool*)target();

			draw_default$ = false;
			add_extra_draw(CheckboxExtraDraw::v, {});

			add_listener(ListenerMouse, CheckboxMouse::v, this, {});

			auto i = (InstancePrivate*)instance();
			add_style_background_color(this, 0, i->default_frame_col, i->default_frame_col_hovering, i->default_frame_col_active);
		}

		FLAME_REGISTER_FUNCTION_BEG(TextExtraDraw, FLAME_GID(9510), Widget::ExtraDrawParm)
			if (p.thiz()->alpha$ > 0.f && ((wText*)p.thiz())->text_col().w > 0.f)
			{
				auto _pos = (p.thiz()->pos$ + Vec2(p.thiz()->inner_padding$[0], p.thiz()->inner_padding$[2])) * p.scl() + p.off();
				if (((wText*)p.thiz())->sdf_scale() < 0.f)
					p.canvas()->add_text_stroke(_pos, Bvec4(((wText*)p.thiz())->text_col(), p.thiz()->alpha$), ((wText*)p.thiz())->text().v);
				else
					p.canvas()->add_text_sdf(_pos, Bvec4(((wText*)p.thiz())->text_col(), p.thiz()->alpha$), ((wText*)p.thiz())->text().v, ((wText*)p.thiz())->sdf_scale() * p.scl());
			}
		FLAME_REGISTER_FUNCTION_END(TextExtraDraw)

		void wText::init()
		{
			auto i = (InstancePrivate*)instance();

			class$ = "text";
			add_data_storages( { i->default_text_col, i->default_sdf_scale } );
			add_string_storages(STRING_SIZE);

			event_attitude$ = EventIgnore;

			text() = L"";

			add_extra_draw(TextExtraDraw::v, {});
		}

		void wText::set_size_auto()
		{
			auto v = Vec2(share_data.font_atlas->get_text_width(text().v), share_data.font_atlas->pixel_height);
			if (sdf_scale() > 0.f)
				v *= sdf_scale();
			v.x += inner_padding$[0] + inner_padding$[1];
			v.y += inner_padding$[2] + inner_padding$[3];
			set_size(v);
		}

		void wButton::init(const wchar_t *title)
		{
			wText::init();

			class$ = "button";
			add_data_storages({ });
			add_string_storages(STRING_SIZE);

			inner_padding$ = Vec4(4.f, 4.f, 2.f, 2.f);
			event_attitude$ = EventAccept;

			auto i = (InstancePrivate*)instance();
			add_style_background_color(this, 0, i->default_button_col, i->default_button_col_hovering, i->default_button_col_active);

			if (title)
			{
				text() = L"OK";
				set_size_auto();
			}
		}

		FLAME_REGISTER_FUNCTION_BEG(ToggleMouse, FLAME_GID(23140), Widget::MouseListenerParm)
			if (!(p.action() == (KeyStateDown | KeyStateUp) && p.key() == Mouse_Null))
				return;

			((wToggle*)p.thiz())->set_toggle(!((wToggle*)p.thiz())->toggled());
		FLAME_REGISTER_FUNCTION_END(ToggleMouse)

		void wToggle::init()
		{
			wText::init();

			class$ = "toggle";
			add_data_storages({ 0 });
			add_string_storages(STRING_SIZE);

			background_col$ = Bvec4(255, 255, 255, 255 * 0.7f);
			background_round_radius$ = share_data.font_atlas->pixel_height * 0.5f;
			background_offset$ = Vec4(background_round_radius$, 0.f, background_round_radius$, 0.f);
			background_round_flags$ = Rect::SideNW | Rect::SideNE | Rect::SideSW | Rect::SideSE;

			event_attitude$ = EventAccept;

			add_listener(ListenerMouse, ToggleMouse::v, this, {});
		}

		void wToggle::set_toggle(bool v)
		{
			toggled() = v;
			if (!v)
				closet_id$ = 0;
			else
				closet_id$ = 1;

			on_changed();
		}

		static void menu_add_rarrow(wMenu *w)
		{
			if (w->w_rarrow())
				return;

			if (w->parent() && w->parent()->class$.hash == cH("menubar"))
				return;
			if (!w->sub() && w->class$.hash != cH("combo"))
				return;

			w->w_title()->inner_padding$[1] += w->w_title()->size$.y * 0.6f;
			w->w_title()->set_size_auto();

			w->w_rarrow() = Widget::createT<wText>(w->instance());
			w->w_rarrow()->align$ = AlignRightNoPadding;
			w->w_rarrow()->sdf_scale() = w->w_title()->sdf_scale();
			w->w_rarrow()->text() = w->sub() ? Icon_CARET_RIGHT : Icon_ANGLE_DOWN;
			w->w_rarrow()->set_size_auto();
			w->add_child(w->w_rarrow(), 1);
		}

		FLAME_REGISTER_FUNCTION_BEG(MenuItemMouse, FLAME_GID(11216), Widget::MouseListenerParm)
			if (!(p.action() == (KeyStateDown | KeyStateUp) && p.key() == Mouse_Null))
				return;

			p.thiz()->instance()->close_popup();
		FLAME_REGISTER_FUNCTION_END(MenuItemMouse)

		void wMenuItem::init(const wchar_t *title)
		{
			wText::init();

			class$ = "menuitem";
			add_data_storages({ });
			add_string_storages(STRING_SIZE);

			inner_padding$ = Vec4(4.f, 4.f, 2.f, 2.f);
			size_policy_hori$ = SizeFitLayout;
			align$ = AlignLittleEnd;
			event_attitude$ = EventAccept;

			text() = title;
			set_size_auto();

			add_listener(ListenerMouse, MenuItemMouse::v, this, {});

			auto i = (InstancePrivate*)instance();
			add_style_background_color(this, 0, Bvec4(0), i->default_header_col_hovering, i->default_header_col_active);
		}

		FLAME_REGISTER_FUNCTION_BEG(MenuTitleMouse, FLAME_GID(10376), Widget::MouseListenerParm)
			if (!(p.action() == KeyStateNull && p.key() == Mouse_Null))
				return;

			if (p.thiz()->instance()->popup_widget())
				((wMenu*)p.thiz())->open();
		FLAME_REGISTER_FUNCTION_END(MenuTitleMouse)

		FLAME_REGISTER_FUNCTION_BEG(MenuItemsChild, FLAME_GID(21018), Widget::ChildListenerParm)
			if (p.op() != Widget::ChildAdd)
				return;

			switch (p.src()->class$.hash)
			{
			case cH("menuitem"):
				menu_add_rarrow((wMenu*)p.thiz());
				break;
			case cH("menu"):
				((wMenu*)p.src())->sub() = 1;
				((wMenu*)p.src())->size_policy_hori$ = SizeGreedy;
				((wMenu*)p.src())->w_items()->align$ = AlignRightOutside;

				menu_add_rarrow((wMenu*)p.thiz());
				break;
			}
		FLAME_REGISTER_FUNCTION_END(MenuItemsChild)

		void wMenu::init(const wchar_t *title, bool only_for_context_menu)
		{
			wLayout::init();

			class$ = "menu";
			add_data_storages({ 0, 0, nullptr, nullptr, nullptr });
			add_string_storages(STRING_SIZE);

			align$ = AlignLittleEnd;
			layout_type$ = LayoutVertical;

			w_title() = createT<wText>(instance());
			w_title()->inner_padding$ = Vec4(4.f, 4.f, 2.f, 2.f);
			w_title()->size_policy_hori$ = SizeGreedy;
			w_title()->align$ = AlignLittleEnd;
			w_title()->event_attitude$ = EventAccept;
			w_title()->text() = title;
			w_title()->set_size_auto();
			add_child(w_title());

			w_title()->add_listener(ListenerMouse, MenuTitleMouse::v, this, {});

			auto i = (InstancePrivate*)instance();
			add_style_background_color(w_title(), 0, Bvec4(0), i->default_header_col_hovering, i->default_header_col_active);

			w_rarrow() = nullptr;

			w_items() = createT<wLayout>(instance(), LayoutVertical);
			w_items()->class$ = "menu items";
			w_items()->background_col$ = i->default_window_col;
			w_items()->align$ = AlignBottomOutside;
			w_items()->visible$ = false;
			add_child(w_items(), 1);

			w_items()->add_listener(ListenerChild, MenuItemsChild::v, this, {});

			if (only_for_context_menu)
			{
				pos$ = hidden_pos;
				sub() = 1;
				w_items()->align$ = AlignFree;
				instance()->root()->add_child(this, 1);
			}
		}

		void wMenu::open()
		{
			if (opened())
				return;

			if (parent() && (parent()->class$.hash == cH("menubar") || parent()->class$.hash == cH("menu items")))
			{
				for (auto i = 0; i < parent()->children_1$.size; i++)
				{
					auto c = parent()->children_1$[i];
					if (c->class$.hash == cH("menu"))
						((wMenu*)c)->close();
				}
			}

			w_items()->set_visibility(true);
			for (auto i = 0; i < w_items()->children_1$.size; i++)
			{
				auto w = w_items()->children_1$[i];
				add_animation_fade(w, 0.2f, 0.f, w->alpha$);
			}

			opened() = 1;
		}

		void wMenu::popup(const Vec2 &pos)
		{
			if (opened() || instance()->popup_widget())
				return;

			open();

			instance()->set_popup_widget(w_items());
			w_items()->pos$ = pos - hidden_pos;
		}

		void wMenu::close()
		{
			if (!opened())
				return;

			for (auto i = 0; i < w_items()->children_1$.size; i++)
			{
				auto c = w_items()->children_1$[i];
				if (c->class$.hash == cH("menu"))
					((wMenu*)c)->close();
			}

			w_items()->set_visibility(false);

			opened() = 0;
		}

		FLAME_DATA_PACKAGE_BEGIN(MenuBarMenuTextMouseData, Widget::MouseListenerParm)
			FLAME_DATA_PACKAGE_CAPT(wMenuPtr, menu, p)
		FLAME_DATA_PACKAGE_END

		FLAME_REGISTER_FUNCTION_BEG(MenuBarMenuTextMouse, FLAME_GID(24104), MenuBarMenuTextMouseData)
			if (!(p.action() == (KeyStateDown | KeyStateUp) && p.key() == Mouse_Null))
				return;

			if (!p.menu()->opened())
			{
				if (!p.thiz()->instance()->popup_widget())
				{
					p.menu()->open();

					p.thiz()->instance()->set_popup_widget(p.thiz());
				}
			}
			else
				p.thiz()->instance()->close_popup();
		FLAME_REGISTER_FUNCTION_END(MenuBarMenuTextMouse)

		FLAME_REGISTER_FUNCTION_BEG(MenuBarChild, FLAME_GID(10208), Widget::ChildListenerParm)
			if (p.op() != Widget::ChildAdd)
				return;

			if (p.src()->class$.hash == cH("menu"))
				((wMenu*)p.src())->w_title()->add_listener(Widget::ListenerMouse, MenuBarMenuTextMouse::v, p.thiz(), { p.src() });
		FLAME_REGISTER_FUNCTION_END(MenuBarChild)

		void wMenuBar::init()
		{
			wLayout::init();

			class$ = "menubar";
			add_data_storages({ });
			add_string_storages(STRING_SIZE);

			layout_type$ = LayoutHorizontal;

			add_listener(ListenerChild, MenuBarChild::v, this, {});
		}
		
		FLAME_REGISTER_FUNCTION_BEG(ComboTextMouse, FLAME_GID(10368), Widget::MouseListenerParm)
			if (!(p.action() == (KeyStateDown | KeyStateUp) && p.key() == Mouse_Null))
				return;

			if (!((wMenu*)p.thiz())->opened())
			{
				if (!p.thiz()->instance()->popup_widget())
				{
					((wMenu*)p.thiz())->open();

					p.thiz()->instance()->set_popup_widget(p.thiz());
				}
			}
			else
				p.thiz()->instance()->close_popup();
		FLAME_REGISTER_FUNCTION_END(ComboTextMouse)

		FLAME_DATA_PACKAGE_BEGIN(ComboItemMouseData, Widget::MouseListenerParm)
			FLAME_DATA_PACKAGE_CAPT(int, idx, i1)
		FLAME_DATA_PACKAGE_END

		FLAME_REGISTER_FUNCTION_BEG(ComboItemMouse, FLAME_GID(22268), ComboItemMouseData)
			if (!(p.action() == (KeyStateDown | KeyStateUp) && p.key() == Mouse_Null))
				return;

			((wCombo*)p.thiz())->set_sel(p.idx());
		FLAME_REGISTER_FUNCTION_END(ComboItemMouse)

			FLAME_DATA_PACKAGE_BEGIN(ComboItemStyleData, Widget::StyleParm)
			FLAME_DATA_PACKAGE_CAPT(wComboPtr, list, p)
		FLAME_DATA_PACKAGE_END

		FLAME_REGISTER_FUNCTION_BEG(ComboItemStyle, FLAME_GID(3278), ComboItemStyleData)
			auto sel = p.list()->sel();
			if (sel != -1 && p.list()->w_items()->children_1$[sel] == p.thiz())
			{
				if (p.thiz()->state == StateNormal)
				{
					if (p.thiz()->style_level <= 1)
					{
						auto i = (InstancePrivate*)p.thiz()->instance();

						p.thiz()->background_col$ = i->default_header_col;
						p.thiz()->style_level = 1;
					}
				}
			}
		FLAME_REGISTER_FUNCTION_END(ComboItemStyle)

		FLAME_REGISTER_FUNCTION_BEG(ComboItemsChild, FLAME_GID(7524), Widget::ChildListenerParm)
			if (p.op() != Widget::ChildAdd)
				return;

			if (p.src()->class$.hash == cH("menuitem"))
			{
				p.thiz()->set_width(p.thiz()->inner_padding$[0] + p.thiz()->inner_padding$[1] + ((wCombo*)p.thiz())->w_title()->inner_padding$[0] + ((wCombo*)p.thiz())->w_title()->inner_padding$[1] + ((wCombo*)p.thiz())->w_items()->size$.x);
				auto idx = ((wCombo*)p.thiz())->w_items()->children_1$.size - 1;

				((wMenuItem*)p.src())->add_listener(Widget::ListenerMouse, ComboItemMouse::v, p.thiz(), { idx });

				((wMenuItem*)p.src())->add_style(0, ComboItemStyle::v, { p.thiz() });
			}
		FLAME_REGISTER_FUNCTION_END(ComboItemsChild)

		void wCombo::init(void *_enum_info, void *_target)
		{
			((wMenu*)this)->init(L"");

			class$ = "combo";
			add_data_storages({ -1, _enum_info, _target });
			add_string_storages(STRING_SIZE);

			background_frame_thickness$ = 1.f;

			size_policy_hori$ = SizeFixed;

			w_title()->size_policy_hori$ = SizeFitLayout;
			w_title()->add_listener(ListenerMouse, ComboTextMouse::v, this, {});

			w_items()->add_listener(ListenerChild, ComboItemsChild::v, this, {});

			if (enum_info())
			{
				auto e = (EnumInfo*)enum_info();

				for (auto i = 0; i < e->item_count(); i++)
					w_items()->add_child(createT<wMenuItem>(instance(), s2w(e->item(i)->name()).c_str()));
			}

			if (target())
			{
				auto p = (int*)target();
				if (enum_info())
				{
					auto e = (EnumInfo*)enum_info();
					set_sel(e->find_item(*p), true);
				}
				else
					set_sel(*p, true);
			}
		}

		void wCombo::set_sel(int idx, bool from_inner)
		{
			sel() = idx;
			auto i = (wMenuItem*)w_items()->children_1$[idx];
			w_title()->text() = i->text();

			if (from_inner)
				return;

			if (target())
			{
				auto p = (int*)target();
				if (enum_info())
				{
					auto e = (EnumInfo*)enum_info();
					*p = e->item(sel())->value();
				}
				else
					*p = sel();
			}

			on_changed();
		}

		FLAME_REGISTER_FUNCTION_BEG(EditExtraDraw, FLAME_GID(9908), Widget::ExtraDrawParm)
			if (p.thiz()->instance()->key_focus_widget() == p.thiz() && int(p.thiz()->instance()->total_time() * 2) % 2 == 0)
			{
				auto len = share_data.font_atlas->get_text_width(((wEdit*)p.thiz())->text().v, ((wEdit*)p.thiz())->text().v + ((wEdit*)p.thiz())->cursor());
				auto pos = (p.thiz()->pos$ + Vec2(p.thiz()->inner_padding$[0], p.thiz()->inner_padding$[2])) * p.scl() + p.off();
				if (((wEdit*)p.thiz())->sdf_scale() < 0.f)
					p.canvas()->add_char_stroke(pos + Vec2(len - 1.f, 0.f) * p.scl(), ((wEdit*)p.thiz())->text_col(), '|');
				else
				{
					auto sdf_scale = ((wEdit*)p.thiz())->sdf_scale() * p.scl();
					p.canvas()->add_char_sdf(pos + Vec2(len - 1.f, 0.f) * sdf_scale, ((wEdit*)p.thiz())->text_col(), '|', sdf_scale);
				}
			}
		FLAME_REGISTER_FUNCTION_END(EditExtraDraw)

		FLAME_REGISTER_FUNCTION_BEG(EditKey, FLAME_GID(27590), Widget::KeyListenerParm)
			if (p.action() == KeyStateNull)
			{
				if (((wEdit*)p.thiz())->type() != wEdit::TypeNull && p.value() != '\b' && p.value() != 22 && p.value() != 27)
				{
					switch (((wEdit*)p.thiz())->type())
					{
					case wEdit::TypeInt:
						if (p.value() == L'-')
						{
							if (((wEdit*)p.thiz())->cursor() != 0 || ((wEdit*)p.thiz())->text().v[0] == L'-')
								return;
						}
						if (p.value() < '0' || p.value() > '9')
							return;
						break;
					case wEdit::TypeUint: case wEdit::TypeUchar:
						if (p.value() < '0' || p.value() > '9')
							return;
						break;
					case wEdit::TypeFloat:
						if (p.value() == L'.')
						{
							if (((wEdit*)p.thiz())->text().find(L'.') != -1)
								return;
						}
						if (p.value() < '0' || p.value() > '9')
							return;
						break;
					}
				}

				switch (p.value())
				{
				case L'\b':
					if (((wEdit*)p.thiz())->cursor() > 0)
					{
						((wEdit*)p.thiz())->cursor()--;
						((wEdit*)p.thiz())->text().remove(((wEdit*)p.thiz())->cursor());
						p.thiz()->on_changed();
					}
					break;
				case 22:
				{
					auto str = get_clipboard();

					((wEdit*)p.thiz())->cursor() = 0;
					((wEdit*)p.thiz())->text() = str.v;
					p.thiz()->on_changed();
				}
					break;
				case 27:
					break;
				default:
					((wEdit*)p.thiz())->text().insert(((wEdit*)p.thiz())->cursor(), p.value());
					((wEdit*)p.thiz())->cursor()++;
					p.thiz()->on_changed();
				}
			}
			else if (p.action() == KeyStateDown)
			{
				switch (p.value())
				{
				case Key_Left:
					if (((wEdit*)p.thiz())->cursor() > 0)
						((wEdit*)p.thiz())->cursor()--;
					break;
				case Key_Right:
					if (((wEdit*)p.thiz())->cursor() < ((wEdit*)p.thiz())->text().size)
						((wEdit*)p.thiz())->cursor()++;
					break;
				case Key_Home:
					((wEdit*)p.thiz())->cursor() = 0;
					break;
				case Key_End:
					((wEdit*)p.thiz())->cursor() = ((wEdit*)p.thiz())->text().size;
					break;
				case Key_Del:
					if (((wEdit*)p.thiz())->cursor() < ((wEdit*)p.thiz())->text().size)
					{
						((wEdit*)p.thiz())->text().remove(((wEdit*)p.thiz())->cursor());
						p.thiz()->on_changed();
					}
					break;
				}
			}
		FLAME_REGISTER_FUNCTION_END(EditKey)

		FLAME_REGISTER_FUNCTION_BEG(ComboFocus, FLAME_GID(12998), Widget::FoucusListenerParm)
			if (p.focus_or_keyfocus() != 1)
				return;
		
			switch (p.type())
			{
			case Focus_Gain:
				if (((wEdit*)p.thiz())->target())
					((wEdit*)p.thiz())->cursor() = ((wEdit*)p.thiz())->text().size;
				break;
			case Focus_Lost:
				if (((wEdit*)p.thiz())->target())
				{
					switch (((wEdit*)p.thiz())->type())
					{
					case wEdit::TypeInt:
					{
						auto v = (int*)(((wEdit*)p.thiz())->target());
						*v = stoi1(((wEdit*)p.thiz())->text().v);
						((wEdit*)p.thiz())->text() = to_wstring(*v);
						((wEdit*)p.thiz())->cursor() = 0;
					}
						break;
					case wEdit::TypeUint:
					{
						auto v = (uint*)(((wEdit*)p.thiz())->target());
						*v = stou1(((wEdit*)p.thiz())->text().v);
						((wEdit*)p.thiz())->text() = to_wstring(*v);
						((wEdit*)p.thiz())->cursor() = 0;
					}
						break;
					case wEdit::TypeFloat:
					{
						auto v = (float*)(((wEdit*)p.thiz())->target());
						*v = stof1(((wEdit*)p.thiz())->text().v);
						((wEdit*)p.thiz())->text() = to_wstring(*v);
						((wEdit*)p.thiz())->cursor() = 0;
					}
						break;
					case wEdit::TypeUchar:
					{
						auto v = (uchar*)(((wEdit*)p.thiz())->target());
						*v = stob1(((wEdit*)p.thiz())->text().v);
						((wEdit*)p.thiz())->text() = to_wstring(*v);
						((wEdit*)p.thiz())->cursor() = 0;
					}
						break;
					}
				}
				break;
			}
		FLAME_REGISTER_FUNCTION_END(ComboFocus)

		void wEdit::init(Type _type, void *_target)
		{
			wText::init();

			class$ = "edit";
			add_data_storages({ 0, (int)_type, _target });
			add_string_storages(STRING_SIZE);

			inner_padding$ = Vec4(4.f, 4.f, 2.f, 2.f);
			auto i = (InstancePrivate*)instance();
			background_col$ = i->default_frame_col;
			event_attitude$ = EventAccept;
			want_key_focus$ = true;

			add_extra_draw(EditExtraDraw::v, {});

			add_listener(ListenerKey, EditKey::v, this, {});
			add_listener(ListenerFocus, ComboFocus::v, this, {});

			if (type() != TypeNull && target())
			{
				switch (type())
				{
				case wEdit::TypeInt:
				{
					auto p = (int*)target();
					text() = to_wstring(*p);
				}
					break;
				case wEdit::TypeUint:
				{
					auto p = (uint*)target();
					text() = to_wstring(*p);
				}
					break;
				case wEdit::TypeFloat:
				{
					auto p = (float*)target();
					text() = to_wstring(*p);
				}
					break;
				case wEdit::TypeUchar:
				{
					auto p = (uchar*)target();
					text() = to_wstring(*p);
				}
					break;
				}
			}
		}

		void wEdit::set_size_by_width(float width)
		{
			set_size(Vec2(width + inner_padding$[0] + inner_padding$[1],
				share_data.font_atlas->pixel_height * (sdf_scale() > 0.f ? sdf_scale() : 1.f) +
				inner_padding$[2] + inner_padding$[3]));
		}

		FLAME_REGISTER_FUNCTION_BEG(ImageExtraDraw, FLAME_GID(30624), Widget::ExtraDrawParm)
			auto pos = (p.thiz()->pos$ + Vec2(p.thiz()->inner_padding$[0], p.thiz()->inner_padding$[2])) * p.scl() + p.off();
			auto size = (p.thiz()->size$ - Vec2(p.thiz()->inner_padding$[0] + p.thiz()->inner_padding$[1], p.thiz()->inner_padding$[2] + p.thiz()->inner_padding$[3])) * p.scl() * p.thiz()->scale$;
			if (!((wImage*)p.thiz())->stretch())
				p.canvas()->add_image(pos, size, ((wImage*)p.thiz())->id(), ((wImage*)p.thiz())->uv0(), ((wImage*)p.thiz())->uv1());
			else
				p.canvas()->add_image_stretch(pos, size, ((wImage*)p.thiz())->id(), ((wImage*)p.thiz())->border());
		FLAME_REGISTER_FUNCTION_END(ImageExtraDraw)

		void wImage::init()
		{
			class$ = "image";
			add_data_storages({ 0, Vec2(0.f), Vec2(1.f), 0, Vec4(0.f) });
			add_string_storages(STRING_SIZE);

			add_extra_draw(ImageExtraDraw::v, {});
		}

		FLAME_DATA_PACKAGE_BEGIN(SizeDragMouseData, Widget::MouseListenerParm)
			FLAME_DATA_PACKAGE_CAPT(WidgetPtr, target, p)
		FLAME_DATA_PACKAGE_END

		FLAME_REGISTER_FUNCTION_BEG(SizeDragMouse, FLAME_GID(2863), SizeDragMouseData)
			if (!(p.action() == KeyStateNull && p.key() == Mouse_Null))
				return;

			if (p.thiz() == p.thiz()->instance()->dragging_widget())
			{
				auto changed = false;
				auto d = p.value() / p.thiz()->parent()->scale$;
				auto new_size = p.target()->size$;

				if (new_size.x + d.x > ((wSizeDrag*)p.thiz())->min_size().x)
				{
					new_size.x += d.x;
					changed = true;
				}
				if (new_size.y + d.y > ((wSizeDrag*)p.thiz())->min_size().y)
				{
					new_size.y += d.y;
					changed = true;
				}

				if (changed)
					p.target()->set_size(new_size);
			}
		FLAME_REGISTER_FUNCTION_END(SizeDragMouse)

		FLAME_REGISTER_FUNCTION_BEG(SizeDragExtraDraw, FLAME_GID(4242), Widget::ExtraDrawParm)
			p.canvas()->add_triangle_filled(
				(p.thiz()->pos$ + Vec2(p.thiz()->size$.x, 0.f)) * p.scl() + p.off(),
				(p.thiz()->pos$ + Vec2(0.f, p.thiz()->size$.y)) * p.scl() + p.off(),
				(p.thiz()->pos$ + Vec2(p.thiz()->size$)) * p.scl() + p.off(),
				p.thiz()->background_col$);
		FLAME_REGISTER_FUNCTION_END(SizeDragExtraDraw)

		void wSizeDrag::init(Widget *target)
		{
			class$ = "sizedrag";
			add_data_storages({ Vec2(0.f) });
			add_string_storages(STRING_SIZE);

			size$ = Vec2(10.f);
			background_col$ = Bvec4(140, 225, 15, 255 * 0.5f);
			align$ = AlignRightBottomNoPadding;

			draw_default$ = false;
			add_extra_draw(SizeDragExtraDraw::v, {});

			add_listener(ListenerMouse, SizeDragMouse::v, this, { target });

			auto i = (InstancePrivate*)instance();
			add_style_background_color(this, 0, i->default_button_col, i->default_button_col_hovering, i->default_button_col_active);
		}

		FLAME_REGISTER_FUNCTION_BEG(ScrollbarBtnMouse, FLAME_GID(1385), Widget::MouseListenerParm)
			if (p.action() != KeyStateNull)
				return;

			if (p.key() == Mouse_Middle)
				p.thiz()->parent()->on_mouse(KeyStateNull, Mouse_Middle, Vec2(p.value().x, 0.f));
			else
			{
				if (((wScrollbar*)p.thiz())->w_btn() == p.thiz()->instance()->dragging_widget())
				{
					((wScrollbar*)p.thiz())->w_target()->scroll_offset$ -= (p.value().y / p.thiz()->size$.y) * ((wScrollbar*)p.thiz())->w_target()->get_content_size();
					((wScrollbar*)p.thiz())->w_target()->arrange();
				}
			}
		FLAME_REGISTER_FUNCTION_END(ScrollbarBtnMouse)

		FLAME_REGISTER_FUNCTION_BEG(ScrollbarMouse, FLAME_GID(2126), Widget::MouseListenerParm)
			if (!(p.action() == KeyStateNull && p.key() == Mouse_Middle))
				return;

			((wScrollbar*)p.thiz())->scroll(p.value().x);
		FLAME_REGISTER_FUNCTION_END(ScrollbarMouse)

		FLAME_REGISTER_FUNCTION_BEG(ScrollbarStyle, FLAME_GID(18956), Widget::StyleParm)
			auto s = ((wScrollbar*)p.thiz())->w_target()->size$.y - ((wScrollbar*)p.thiz())->w_target()->inner_padding$[2] - ((wScrollbar*)p.thiz())->w_target()->inner_padding$[3];
			auto content_size = ((wScrollbar*)p.thiz())->w_target()->get_content_size();
			if (content_size > s)
			{
				((wScrollbar*)p.thiz())->w_btn()->set_visibility(true);
				((wScrollbar*)p.thiz())->w_btn()->pos$.y = p.thiz()->size$.y * (-((wScrollbar*)p.thiz())->w_target()->scroll_offset$ / content_size);
				((wScrollbar*)p.thiz())->w_btn()->size$.y = p.thiz()->size$.y * (s / content_size);
			}
			else
				((wScrollbar*)p.thiz())->w_btn()->set_visibility(false);
		FLAME_REGISTER_FUNCTION_END(ScrollbarStyle)

		void wScrollbar::init(Widget *target)
		{
			wLayout::init();

			class$ = "scrollbar";
			add_data_storages({ nullptr, nullptr });
			add_string_storages(STRING_SIZE);

			size$ = Vec2(10.f);
			size_policy_vert$ = SizeFitLayout;
			align$ = AlignRight;
			event_attitude$ = EventAccept;

			w_btn() = createT<wButton>(instance(), nullptr);
			w_btn()->size$ = size$;
			w_btn()->background_round_radius$ = 5.f;
			w_btn()->background_round_flags$ = Rect::SideNW | Rect::SideNE | Rect::SideSW | Rect::SideSE;
			add_child(w_btn());

			w_target() = target;
			w_target()->add_listener(ListenerMouse, ScrollbarMouse::v, this, {});

			w_btn()->add_listener(ListenerMouse, ScrollbarBtnMouse::v, this, {});

			add_listener(ListenerMouse, ScrollbarMouse::v, this, {});

			add_style(0, ScrollbarStyle::v, {});
		}

		void wScrollbar::scroll(int v)
		{
			w_target()->scroll_offset$ += v * 20.f;
			w_target()->arrange();
		}

		FLAME_REGISTER_FUNCTION_BEG(ListItemTextMouse, FLAME_GID(6526), Widget::MouseListenerParm)
			if (!(p.action() == KeyStateNull && p.key() == Mouse_Middle))
				return;

			if (p.thiz()->parent())
				p.thiz()->parent()->on_mouse(KeyStateNull, Mouse_Middle, Vec2(p.value().x, 0.f));
		FLAME_REGISTER_FUNCTION_END(ListItemTextMouse)

		void wListItem::init(const wchar_t *title)
		{
			wLayout::init();

			class$ = "listitem";
			add_data_storages({ nullptr });
			add_string_storages(STRING_SIZE);

			size_policy_hori$ = SizeFitLayout;
			align$ = AlignLittleEnd;
			layout_type$ = LayoutHorizontal;

			w_title() = createT<wText>(instance());
			w_title()->inner_padding$ = Vec4(4.f, 4.f, 2.f, 2.f);
			w_title()->size_policy_hori$ = SizeFitLayout;
			w_title()->align$ = AlignLittleEnd;
			w_title()->event_attitude$ = EventAccept;
			w_title()->text() = title;
			w_title()->set_size_auto();
			add_child(w_title());

			w_title()->add_listener(ListenerMouse, ListItemTextMouse::v, this, {});

			auto i = (InstancePrivate*)instance();
			add_style_background_color(w_title(), 0, Bvec4(0), i->default_header_col_hovering, i->default_header_col_active);
		}

		FLAME_DATA_PACKAGE_BEGIN(ListItemTitleMouseData, Widget::MouseListenerParm)
			FLAME_DATA_PACKAGE_CAPT(wListItemPtr, item, p)
		FLAME_DATA_PACKAGE_END

		FLAME_REGISTER_FUNCTION_BEG(ListListItemTitleMouse, FLAME_GID(8928), ListItemTitleMouseData)
			if (!(p.action() == KeyStateDown && p.key() == Mouse_Left))
				return;

			((wList*)p.thiz())->w_sel() = p.item();
			p.thiz()->on_changed();
		FLAME_REGISTER_FUNCTION_END(ListListItemTitleMouse)

		FLAME_DATA_PACKAGE_BEGIN(ListItemTitleStyleData, Widget::StyleParm)
			FLAME_DATA_PACKAGE_CAPT(wListPtr, list, p)
		FLAME_DATA_PACKAGE_END

		FLAME_REGISTER_FUNCTION_BEG(ListItemTitleStyle, FLAME_GID(408), ListItemTitleStyleData)
			if (p.list()->w_sel() && p.list()->w_sel()->w_title() == p.thiz() &&
				p.thiz()->state == StateNormal && p.thiz()->style_level <= 1)
			{
				auto i = (InstancePrivate*)p.thiz()->instance();

				p.thiz()->background_col$ = i->default_header_col;
				p.thiz()->style_level = 1;
			}
		FLAME_REGISTER_FUNCTION_END(ListItemTitleStyle)

		FLAME_REGISTER_FUNCTION_BEG(ListMouse, FLAME_GID(19124), Widget::MouseListenerParm)
			if (!(p.action() == KeyStateDown && p.key() == Mouse_Left))
				return;

			((wList*)p.thiz())->w_sel() = nullptr;
			p.thiz()->on_changed();
		FLAME_REGISTER_FUNCTION_END(ListMouse)

		FLAME_REGISTER_FUNCTION_BEG(ListChild, FLAME_GID(8288), Widget::ChildListenerParm)
			if (p.op() != Widget::ChildAdd)
				return;

			if (p.src()->class$.hash == cH("listitem"))
			{
				((wListItem*)p.src())->w_title()->add_listener(Widget::ListenerMouse, ListListItemTitleMouse::v, p.thiz(), { p.src() });

				((wListItem*)p.src())->w_title()->add_style(0, ListItemTitleStyle::v, { p.thiz() });
			}
		FLAME_REGISTER_FUNCTION_END(ListChild)

		void wList::init()
		{
			wLayout::init();

			class$ = "list";
			add_data_storages({ nullptr, nullptr });
			add_string_storages(STRING_SIZE);

			inner_padding$ = Vec4(4.f);
			auto i = (InstancePrivate*)instance();
			background_col$ = i->default_frame_col;
			size_policy_hori$ = SizeFitLayout;
			size_policy_vert$ = SizeFitLayout;
			event_attitude$ = EventAccept;
			layout_type$ = LayoutVertical;
			clip$ = true;

			add_listener(ListenerMouse, ListMouse::v, this, {});

			w_scrollbar() = createT<wScrollbar>(instance(), this);
			add_child(w_scrollbar(), 1);

			add_listener(ListenerChild, ListChild::v, this, {});
		}

		FLAME_DATA_PACKAGE_BEGIN(TreeNodeTitleMouseData, Widget::MouseListenerParm)
			FLAME_DATA_PACKAGE_CAPT(wTreeNodePtr, item, p)
		FLAME_DATA_PACKAGE_END

		FLAME_REGISTER_FUNCTION_BEG(TreeNodeTitleMouse, FLAME_GID(10825), TreeNodeTitleMouseData)
			if (p.action() == KeyStateDown && p.key() == Mouse_Left)
			{
				if (p.thiz())
				{
					((wTree*)p.thiz())->w_sel() = p.item();
					p.thiz()->on_changed();
				}
			}
			else if (p.action() == (KeyStateDown | KeyStateUp | KeyStateDouble) && p.key() == Mouse_Null)
				((wTreeNode*)p.item())->w_larrow()->on_mouse(KeyState(KeyStateDown | KeyStateUp), Mouse_Null, Vec2(0.f));
		FLAME_REGISTER_FUNCTION_END(TreeNodeTitleMouse)

		FLAME_REGISTER_FUNCTION_BEG(TreeNodeLarrowMouse, FLAME_GID(20989), Widget::MouseListenerParm)
			if (!(p.action() == (KeyStateDown | KeyStateUp) && p.key() == Mouse_Null))
				return;

			auto v = !((wTreeNode*)p.thiz())->w_items()->visible$;
			((wTreeNode*)p.thiz())->w_items()->set_visibility(v);

			((wTreeNode*)p.thiz())->w_larrow()->text() = v ? Icon_CARET_DOWN : Icon_CARET_RIGHT;
			FLAME_REGISTER_FUNCTION_END(TreeNodeLarrowMouse)

		FLAME_DATA_PACKAGE_BEGIN(TreeNodeTitleStyleData, Widget::StyleParm)
			FLAME_DATA_PACKAGE_CAPT(wTreePtr, tree, p)
		FLAME_DATA_PACKAGE_END

		FLAME_REGISTER_FUNCTION_BEG(TreeNodeTitleStyle, FLAME_GID(1879), TreeNodeTitleStyleData)
			if (p.tree()->w_sel() && p.tree()->w_sel()->w_title() == p.thiz())
			{
				auto i = (InstancePrivate*)p.thiz()->instance();

				p.thiz()->background_col$ = i->default_header_col;
				p.thiz()->style_level = 1;
			}
			else
				p.thiz()->background_col$ = Bvec4(0);
		FLAME_REGISTER_FUNCTION_END(TreeNodeTitleStyle)

		void wTreeNode::init(const wchar_t *title, wTree *tree)
		{
			wLayout::init();

			class$ = "treenode";
			add_data_storages({ nullptr, nullptr, nullptr });
			add_string_storages(STRING_SIZE);

			layout_type$ = LayoutVertical;
			align$ = AlignLittleEnd;

			w_title() = createT<wText>(instance());
			w_title()->inner_padding$[0] = share_data.font_atlas->pixel_height * 0.8f;
			w_title()->inner_padding$ += Vec4(4.f, 4.f, 2.f, 2.f);
			w_title()->align$ = AlignLittleEnd;
			w_title()->event_attitude$ = EventAccept;
			w_title()->text() = title;
			w_title()->set_size_auto();
			add_child(w_title());

			w_title()->add_listener(ListenerMouse, TreeNodeTitleMouse::v, tree, { this });

			if (tree)
				w_title()->add_style(0, TreeNodeTitleStyle::v, { tree });

			w_items() = createT<wLayout>(instance(), LayoutVertical);
			w_items()->layout_padding$ = w_title()->inner_padding$[0];
			w_items()->align$ = AlignLittleEnd;
			w_items()->visible$ = false;
			add_child(w_items());

			w_larrow() = createT<wText>(instance());
			w_larrow()->inner_padding$ = Vec4(4.f, 0.f, 4.f, 0.f);
			w_larrow()->background_col$ = Bvec4(255, 255, 255, 0);
			w_larrow()->align$ = AlignLeftTopNoPadding;
			w_larrow()->event_attitude$ = EventAccept;
			w_larrow()->set_size(Vec2(w_title()->inner_padding$[0], w_title()->size$.y));
			w_larrow()->text() = Icon_CARET_RIGHT;

			auto i = (InstancePrivate*)instance();
			add_style_text_color(w_title(), 0, i->default_text_col, i->default_text_col_hovering_or_active);
			add_style_text_color(w_larrow(), 0, i->default_text_col, i->default_text_col_hovering_or_active);

			w_larrow()->add_listener(ListenerMouse, TreeNodeLarrowMouse::v, this, {});

			add_child(w_larrow(), 1);
		}

		void wTree::init()
		{
			wLayout::init();

			class$ = "tree";
			add_data_storages({ nullptr });
			add_string_storages(STRING_SIZE);

			layout_type$ = LayoutVertical;
		}

		FLAME_REGISTER_FUNCTION_BEG(DialogMouse, FLAME_GID(9227), Widget::MouseListenerParm)
			if (p.action() == KeyStateDown && p.key() == Mouse_Left)
				p.thiz()->set_to_foreground();
			else if (p.action() == KeyStateNull && p.key() == Mouse_Null)
			{
				if (p.thiz() == p.thiz()->instance()->dragging_widget())
					p.thiz()->pos$ += p.value() / p.thiz()->parent()->scale$;
			}
		FLAME_REGISTER_FUNCTION_END(DialogMouse)

		void wDialog::init(bool resize, bool modual)
		{
			wLayout::init();

			class$ = "dialog";
			add_data_storages({ nullptr, nullptr });
			add_string_storages(STRING_SIZE);

			auto radius = 8.f;

			inner_padding$ = Vec4(radius);
			background_col$ = Colorf(0.5f, 0.5f, 0.5f, 0.9f);
			event_attitude$ = EventAccept;
			item_padding$ = radius;
			if (resize)
			{
				size_policy_hori$ = SizeFitLayout;
				size_policy_vert$ = SizeFitLayout;
				clip$ = true;
			}

			add_listener(ListenerMouse, DialogMouse::v, this, {});

			background_offset$[1] = 0.f;
			background_round_radius$ = radius;
			background_round_flags$ = Rect::SideNW | Rect::SideNE | Rect::SideSW | Rect::SideSE;
			background_shaow_thickness$ = 8.f;

			if (resize)
			{
				w_scrollbar() = createT<wScrollbar>(instance(), this);
				add_child(w_scrollbar(), 1);

				w_sizedrag() = createT<wSizeDrag>(instance(), this);
				w_sizedrag()->min_size() = Vec2(10.f);
				set_size(Vec2(100.f));
				add_child(w_sizedrag(), 1);
			}
			else
				w_sizedrag() = nullptr;

			if (modual)
			{
				pos$ = (Vec2(instance()->root()->size$) - size$) * 0.5f;

				instance()->root()->add_child(this, 0, -1, true, true);
			}
		}

		FLAME_REGISTER_FUNCTION_BEG(MessageDialogOkMouse, FLAME_GID(8291), Widget::MouseListenerParm)
			if (!(p.action() == (KeyStateDown | KeyStateUp) && p.key() == Mouse_Null))
				return;

			p.thiz()->remove_from_parent(true);
		FLAME_REGISTER_FUNCTION_END(MessageDialogOkMouse)

		void wMessageDialog::init(const wchar_t *text)
		{
			((wDialog*)this)->init(false, true);

			class$ = "message dialog";
			add_data_storages({ nullptr, nullptr });
			add_string_storages(STRING_SIZE);

			want_key_focus$ = true;

			layout_type$ = LayoutVertical;
			item_padding$ = 8.f;

			w_text() = createT<wText>(instance());
			w_text()->align$ = AlignLittleEnd;
			w_text()->text() = text;
			w_text()->set_size_auto();
			add_child(w_text());

			w_ok() = createT<wButton>(instance(), L"OK");
			w_ok()->align$ = AlignMiddle;
			add_child(w_ok());

			w_ok()->add_listener(ListenerMouse, MessageDialogOkMouse::v, this, {});
		}

		//void wYesNoDialog::init(const wchar_t *title, float sdf_scale, const wchar_t *text, const wchar_t *yes_text, const wchar_t *no_text, const std::function<void(bool)> &callback)
		//{
		//	((wDialog*)this)->init(title, sdf_scale, false);

		//	resize_data_storage(7);

		//	event_attitude = EventBlackHole;
		//	want_key_focus = true;

		//	add_listener(ListenerKeyDown, [this](int key) {
		//		switch (key)
		//		{
		//		case Key_Enter:
		//			w_yes()->on_clicked();
		//			break;
		//		case Key_Esc:
		//			w_no()->on_clicked();
		//			break;
		//		}
		//	});

		//	if (w_title())
		//		w_title()->background_col = Bvec4(200, 40, 20, 255);

		//	w_content()->layout_type = LayoutVertical;
		//	w_content()->item_padding = 8.f;
		//	if (sdf_scale > 0.f)
		//		w_content()->item_padding *= sdf_scale;

		//	if (text[0])
		//	{
		//		w_text() = wText::create(instance());
		//		w_text()->align = AlignMiddle;
		//		w_text()->sdf_scale() = sdf_scale;
		//		w_text()->set_text_and_size(text);
		//		w_content()->add_child(w_text());
		//	}
		//	else
		//		w_text() = nullptr;

		//	w_buttons() = wLayout::create(instance());
		//	w_buttons()->align = AlignMiddle;
		//	w_buttons()->layout_type = LayoutHorizontal;
		//	w_buttons()->item_padding = 4.f;
		//	if (sdf_scale > 0.f)
		//		w_buttons()->item_padding *= sdf_scale;

		//	w_yes() = wButton::create(instance());
		//	w_yes()->set_classic(yes_text, sdf_scale);
		//	w_yes()->align = AlignLittleEnd;
		//	w_buttons()->add_child(w_yes());

		//	w_no() = wButton::create(instance());
		//	w_no()->set_classic(no_text, sdf_scale);
		//	w_no()->align = AlignLittleEnd;
		//	w_buttons()->add_child(w_no());

		//	w_yes()->add_listener(ListenerClicked, [this, callback]() {
		//		callback(true);
		//		remove_from_parent(true);
		//	});

		//	w_no()->add_listener(ListenerClicked, [this, callback]() {
		//		callback(false);
		//		remove_from_parent(true);
		//	});

		//	w_content()->add_child(w_buttons());

		//	pos = (Vec2(instance()->root()->size) - size) * 0.5f;

		//	instance()->root()->add_child(this, 0, -1, true, [](CommonData *d) {
		//		auto thiz = (Widget*)d[0].p;

		//		thiz->instance()->set_focus_widget(thiz);
		//		thiz->instance()->set_dragging_widget(nullptr);
		//	}, "p", this);
		//}

		//wTextPtr &wYesNoDialog::w_text()
		//{
		//	return *((wTextPtr*)&data_storage(3).p);
		//}

		//wLayoutPtr &wYesNoDialog::w_buttons()
		//{
		//	return *((wLayoutPtr*)&data_storage(4).p);
		//}

		//wButtonPtr &wYesNoDialog::w_yes()
		//{
		//	return *((wButtonPtr*)&data_storage(5).p);
		//}

		//wButtonPtr &wYesNoDialog::w_no()
		//{
		//	return *((wButtonPtr*)&data_storage(6).p);
		//}

		//void wInputDialog::init(const wchar_t *title, float sdf_scale, const std::function<void(bool ok, const wchar_t *input)> &callback)
		//{
		//	((wDialog*)this)->init(title, sdf_scale, false);

		//	resize_data_storage(7);

		//	event_attitude = EventBlackHole;
		//	want_key_focus = true;

		//	if (w_title())
		//		w_title()->background_col = Bvec4(200, 40, 20, 255);

		//	w_content()->layout_type = LayoutVertical;
		//	w_content()->item_padding = 8.f;
		//	if (sdf_scale > 0.f)
		//		w_content()->item_padding *= sdf_scale;

		//	w_input() = wEdit::create(instance());
		//	w_input()->align = AlignLittleEnd;
		//	w_input()->sdf_scale() = sdf_scale;
		//	w_input()->set_size_by_width(100.f);
		//	w_content()->add_child(w_input());

		//	w_buttons() = wLayout::create(instance());
		//	w_buttons()->align = AlignMiddle;
		//	w_buttons()->layout_type = LayoutHorizontal;
		//	w_buttons()->item_padding = 4.f;
		//	if (sdf_scale > 0.f)
		//		w_buttons()->item_padding *= sdf_scale;

		//	w_ok() = wButton::create(instance());
		//	w_ok()->set_classic(L"OK", sdf_scale);
		//	w_ok()->align = AlignLittleEnd;
		//	w_buttons()->add_child(w_ok());

		//	w_cancel() = wButton::create(instance());
		//	w_cancel()->set_classic(L"Cancel", sdf_scale);
		//	w_cancel()->align = AlignLittleEnd;
		//	w_buttons()->add_child(w_cancel());

		//	w_content()->add_child(w_buttons());

		//	pos = (Vec2(instance()->root()->size) - size) * 0.5f;

		//	w_ok()->add_listener(ListenerClicked, [this, callback]() {
		//		callback(true, w_input()->text());
		//		remove_from_parent(true);
		//	});

		//	w_cancel()->add_listener(ListenerClicked, [this, callback]() {
		//		callback(false, nullptr);
		//		remove_from_parent(true);
		//	});

		//	instance()->root()->add_child(this, 0, -1, true, [](CommonData *d) {
		//		auto thiz = (Widget*)d[0].p;

		//		thiz->instance()->set_focus_widget(thiz);
		//		thiz->instance()->set_dragging_widget(nullptr);
		//	}, "p", this);
		//}

		//wEditPtr &wInputDialog::w_input()
		//{
		//	return *((wEditPtr*)&data_storage(3).p);
		//}

		//wLayoutPtr &wInputDialog::w_buttons()
		//{
		//	return *((wLayoutPtr*)&data_storage(4).p);
		//}

		//wButtonPtr &wInputDialog::w_ok()
		//{
		//	return *((wButtonPtr*)&data_storage(5).p);
		//}

		//wButtonPtr &wInputDialog::w_cancel()
		//{
		//	return *((wButtonPtr*)&data_storage(6).p);
		//}

		//void wFileDialog::init(const wchar_t *title, int io, const std::function<void(bool ok, const wchar_t *filename)> &callback, const wchar_t *exts)
		//{
		//	((wDialog*)this)->init(title, -1.f, true);

		//	resize_data_storage(10);
		//	resize_string_storage(2);

		//	event_attitude = EventBlackHole;
		//	want_key_focus = true;

		//	if (w_title())
		//		w_title()->background_col = Bvec4(200, 40, 20, 255);

		//	set_string_storage(0, get_curr_path());
		//	set_string_storage(1, L"");

		//	w_sizedrag()->min_size() = Vec2(300.f, 400.f);
		//	set_size(w_sizedrag()->min_size());
		//	event_attitude = EventBlackHole;

		//	w_content()->size_policy_hori = SizeFitLayout;
		//	w_content()->size_policy_vert = SizeFitLayout;
		//	w_content()->layout_type = LayoutVertical;
		//	w_content()->item_padding = 8.f;

		//	w_pathstems() = wMenuBar::create(instance());
		//	w_pathstems()->align = AlignLittleEnd;
		//	w_pathstems()->layout_type = LayoutHorizontal;
		//	w_content()->add_child(w_pathstems());

		//	w_list() = wList::create(instance());
		//	w_list()->align = AlignLittleEnd;

		//	auto upward_item = wListItem::create(instance());
		//	upward_item->w_btn()->set_text_and_size(L"..");
		//	w_list()->add_child(upward_item);

		//	upward_item->w_btn()->add_listener(ListenerDoubleClicked, [this]() {
		//		std::filesystem::path fs_path(string_storage(0));
		//		if (fs_path.root_path() != fs_path)
		//			set_path(fs_path.parent_path().generic_wstring().c_str());
		//	});

		//	w_content()->add_child(w_list());

		//	w_input() = wEdit::create(instance());
		//	w_input()->size_policy_hori = SizeFitLayout;
		//	w_input()->align = AlignLittleEnd;
		//	w_input()->set_size_by_width(100.f);
		//	w_content()->add_child(w_input());

		//	w_ext() = wCombo::create(instance());
		//	w_ext()->size_policy_hori = SizeFitLayout;
		//	w_ext()->align = AlignLittleEnd;
		//	add_style_buttoncolor(w_ext(), 0, Vec3(0.f, 0.f, 0.7f));
		//	{
		//		if (exts == nullptr)
		//			exts = L"All Files (*.*)\0";
		//		auto sp = doublenull_string_split(exts);
		//		for (auto &s : sp)
		//		{
		//			auto i = wMenuItem::create(instance(), s.c_str());
		//			w_ext()->w_items()->add_child(i);
		//		}
		//	}

		//	w_ext()->add_listener(ListenerChanged, [this]() {
		//		set_string_storage(1, w_ext()->w_btn()->text());
		//		set_path(string_storage(0));
		//	});
		//	w_content()->add_child(w_ext());
		//	w_ext()->set_sel(0);

		//	w_buttons() = wLayout::create(instance());
		//	w_buttons()->align = AlignMiddle;
		//	w_buttons()->layout_type = LayoutHorizontal;
		//	w_buttons()->item_padding = 4.f;

		//	w_ok() = wButton::create(instance());
		//	w_ok()->set_classic(io == 0 ? L"Open" : L"Save");
		//	w_ok()->align = AlignLittleEnd;
		//	w_buttons()->add_child(w_ok());

		//	w_cancel() = wButton::create(instance());
		//	w_cancel()->set_classic(L"Cancel");
		//	w_cancel()->align = AlignLittleEnd;
		//	w_buttons()->add_child(w_cancel());

		//	w_ok()->add_listener(ListenerClicked, [this, io, callback]() {
		//		auto full_filename = std::wstring(string_storage(0)) + L"/" + w_input()->text();
		//		if (io == 0)
		//		{
		//			if (std::filesystem::exists(full_filename))
		//			{
		//				callback(true, full_filename.c_str());
		//				remove_from_parent(true);
		//			}
		//			else
		//				wMessageDialog::create(instance(), L"File doesn't exist.", -1.f, L"");
		//		}
		//		else
		//		{
		//			if (std::filesystem::exists(full_filename))
		//			{
		//				wYesNoDialog::create(instance(), L"", -1.f,
		//					L"File already exists, would you like to cover it?", L"Cover", L"Cancel", [&](bool b)
		//				{
		//					if (b)
		//					{
		//						callback(true, full_filename.c_str());
		//						remove_from_parent(true);
		//					}
		//				});
		//			}
		//			else
		//			{
		//				callback(true, full_filename.c_str());
		//				remove_from_parent(true);
		//			}
		//		}
		//	});

		//	w_cancel()->add_listener(ListenerClicked, [this, callback]() {
		//		callback(false, nullptr);
		//		remove_from_parent(true);
		//	});

		//	w_content()->add_child(w_buttons());

		//	pos = (Vec2(instance()->root()->size) - size) * 0.5f;

		//	instance()->root()->add_child(this, 0, -1, true, [](CommonData *d) {
		//		auto thiz = (Widget*)d[0].p;

		//		thiz->instance()->set_focus_widget(thiz);
		//		thiz->instance()->set_dragging_widget(nullptr);
		//	}, "p", this);
		//}

		//wMenuBarPtr &wFileDialog::w_pathstems()
		//{
		//	return *((wMenuBarPtr*)&data_storage(3).p);
		//}

		//wListPtr &wFileDialog::w_list()
		//{
		//	return *((wListPtr*)&data_storage(4).p);
		//}

		//wEditPtr &wFileDialog::w_input()
		//{
		//	return *((wEditPtr*)&data_storage(5).p);
		//}

		//wComboPtr &wFileDialog::w_ext()
		//{
		//	return *((wComboPtr*)&data_storage(6).p);
		//}

		//wLayoutPtr &wFileDialog::w_buttons()
		//{
		//	return *((wLayoutPtr*)&data_storage(7).p);
		//}

		//wButtonPtr &wFileDialog::w_ok()
		//{
		//	return *((wButtonPtr*)&data_storage(8).p);
		//}

		//wButtonPtr &wFileDialog::w_cancel()
		//{
		//	return *((wButtonPtr*)&data_storage(9).p);
		//}

		//const wchar_t *wFileDialog::curr_path()
		//{
		//	return string_storage(0);
		//}

		//int wFileDialog::curr_path_len()
		//{
		//	return string_storage_len(0);
		//}

		//void wFileDialog::set_curr_path(const wchar_t *path)
		//{
		//	return set_string_storage(0, path);
		//}

		//const wchar_t *wFileDialog::curr_exts()
		//{
		//	return string_storage(1);
		//}

		//int wFileDialog::curr_exts_len()
		//{
		//	return string_storage_len(1);
		//}

		//void wFileDialog::set_curr_exts(const wchar_t *exts)
		//{
		//	return set_string_storage(1, exts);
		//}

		//void wFileDialog::set_path(const wchar_t *path)
		//{
		//	w_pathstems()->clear_children(0, 0, -1, true);
		//	w_list()->clear_children(0, 1, -1, true);
		//	w_list()->w_sel() = nullptr;

		//	set_string_storage(0, path);
		//	std::filesystem::path fs_path(path);
		//	{
		//		std::vector<std::wstring> stems;
		//		auto fs_root_path = fs_path.root_path();
		//		std::filesystem::path p(path);
		//		while (p != fs_root_path)
		//		{
		//			stems.push_back(p.filename().generic_wstring());
		//			p = p.parent_path();
		//		}
		//		auto root_path = fs_root_path.generic_wstring();
		//		if (root_path[root_path.size() - 1] != ':')
		//			root_path = string_cut(root_path, -1);
		//		stems.push_back(root_path);
		//		std::reverse(stems.begin(), stems.end());

		//		std::wstring curr_path(L"");
		//		auto build_btn_pop = [&](const std::wstring &path) {
		//			auto btn_pop = wMenu::create(instance(), Icon_CARET_RIGHT, 0.f);
		//			btn_pop->align = AlignMiddle;
		//			w_pathstems()->add_child(btn_pop, 0, -1, true);

		//			if (path == L"")
		//			{
		//				wchar_t drive_strings[256];
		//				GetLogicalDriveStringsW(FLAME_ARRAYSIZE(drive_strings), drive_strings);

		//				auto drives = doublenull_string_split(drive_strings);
		//				for (auto &d : drives)
		//				{
		//					auto i = wMenuItem::create(instance(), d.c_str());
		//					auto path = string_cut(d, -1);
		//					i->add_listener(ListenerClicked, [this, path]() {
		//						set_path(path.c_str());
		//					});
		//					btn_pop->w_items()->add_child(i, 0, -1, true);
		//				}
		//			}
		//			else
		//			{
		//				for (std::filesystem::directory_iterator end, it(path); it != end; it++)
		//				{
		//					if (std::filesystem::is_directory(it->status()))
		//					{
		//						auto str = it->path().filename().generic_wstring();
		//						auto i = wMenuItem::create(instance(), str.c_str());
		//						str = path + L"/" + str;
		//						i->add_listener(ListenerClicked, [this, str]() {
		//							set_path(str.c_str());
		//						});
		//						btn_pop->w_items()->add_child(i, 0, -1, true);
		//					}
		//				}
		//			}

		//			return btn_pop;
		//		};
		//		for (auto &s : stems)
		//		{
		//			auto btn_pop = build_btn_pop(curr_path);

		//			curr_path += s;

		//			auto btn_path = wButton::create(instance());
		//			btn_path->background_col.w = 0;
		//			btn_path->align = AlignMiddle;
		//			btn_path->set_classic(s.c_str());
		//			w_pathstems()->add_child(btn_path, 0, -1, true);

		//			btn_path->add_listener(ListenerClicked, [this, curr_path]() {
		//				set_path(curr_path.c_str());
		//			});

		//			curr_path += L"/";
		//		}
		//		build_btn_pop(curr_path);
		//	}

		//	std::vector<wListItem*> dir_list;
		//	std::vector<wListItem*> file_list;

		//	std::vector<std::wstring> exts_sp;
		//	auto sp = string_regex_split(std::wstring(string_storage(1)), std::wstring(LR"(\(*(\.\w+)\))"), 1);
		//	for (auto &e : sp)
		//		exts_sp.push_back(e);

		//	for (std::filesystem::directory_iterator end, it(fs_path); it != end; it++)
		//	{
		//		auto filename = it->path().filename().generic_wstring();
		//		auto item = wListItem::create(instance());

		//		if (std::filesystem::is_directory(it->status()))
		//		{
		//			item->w_btn()->set_text_and_size((Icon_FOLDER_O + std::wstring(L" ") + filename).c_str());
		//			dir_list.push_back(item);

		//			item->w_btn()->add_listener(ListenerDoubleClicked, [this, filename]() {
		//				set_path((std::wstring(string_storage(0)) + L"/" + filename).c_str());
		//			});
		//		}
		//		else
		//		{
		//			auto found_ext = false;
		//			for (auto &e : exts_sp)
		//			{
		//				if (e == L".*" || it->path().extension() == e)
		//				{
		//					found_ext = true;
		//					break;
		//				}
		//			}
		//			if (!found_ext)
		//				continue;

		//			item->w_btn()->set_text_and_size((Icon_FILE_O + std::wstring(L" ") + filename).c_str());
		//			file_list.push_back(item);

		//			item->w_btn()->add_listener(ListenerClicked, [this, filename]() {
		//				w_input()->set_text(filename.c_str());
		//			});

		//			item->w_btn()->add_listener(ListenerDoubleClicked, [this]() {
		//				w_ok()->on_clicked();
		//			});
		//		}
		//	}

		//	for (auto &i : dir_list)
		//		w_list()->add_child(i, 0, -1, true);
		//	for (auto &i : file_list)
		//		w_list()->add_child(i, 0, -1, true);
		//}
	}
}

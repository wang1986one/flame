#pragma once

#include <flame/universe/ui/ui.h>
#include <flame/utils/app.h>

using namespace flame;
using namespace graphics;

struct MyApp : App
{
	void create();
};

extern MyApp app;

struct _2DEditor
{
	cElement* element;
	cEventReceiver* event_receiver;

	cElement* base;
	bool moved;
	cElement* overlay;
	uint scale_level;
	uint scale_level_max;
	cText* scale_text;

	bool selecting;
	Vec2f select_begin;
	Vec2f select_end;

	_2DEditor() :
		element(nullptr),
		event_receiver(nullptr),
		base(nullptr),
		moved(false),
		scale_level(10),
		scale_level_max(10),
		scale_text(nullptr),
		selecting(false)
	{
	}

	void create(UI& ui, void(*select_callback)(Capture& c, const Vec4f& r), const Capture& _capture)
	{
		ui.e_begin_layout();
		ui.c_aligner(AlignMinMax, AlignMinMax);
		element = ui.current_entity->get_component(cElement);
		element->clip_flags = ClipFlag(ClipSelf | ClipChildren);
		element->cmds.add([](Capture& c, graphics::Canvas* canvas) {
			auto& edt = *c.thiz<_2DEditor>();

			if (edt.element->clipped)
				return true;

			{
				const auto grid_size = 50.f * edt.base->global_scale;
				auto pos = edt.base->global_pos;
				auto size = edt.element->global_size + grid_size * 2.f;
				auto grid_number = Vec2i(size / grid_size) + 2;
				auto grid_offset = edt.element->global_pos + (fract(pos / grid_size) - 1.f) * grid_size;
				for (auto x = 0; x < grid_number.x(); x++)
				{
					std::vector<Vec2f> points;
					points.push_back(grid_offset + Vec2f(x * grid_size, 0.f));
					points.push_back(grid_offset + Vec2f(x * grid_size, size.y()));
					canvas->stroke(points.size(), points.data(), Vec4c(200, 200, 200, 255), 1.f);
				}
				for (auto y = 0; y < grid_number.y(); y++)
				{
					std::vector<Vec2f> points;
					points.push_back(grid_offset + Vec2f(0.f, y * grid_size));
					points.push_back(grid_offset + Vec2f(size.x(), y * grid_size));
					canvas->stroke(points.size(), points.data(), Vec4c(200, 200, 200, 255), 1.f);
				}
			}

			return true;
		}, Capture().set_thiz(this));
		event_receiver = ui.c_event_receiver();
		event_receiver->focus_type = FocusByLeftOrRightButton;
		struct Capturing
		{
			void(*f)(Capture&, const Vec4f&);
		}capture;
		capture.f = select_callback;
		event_receiver->mouse_listeners.add([](Capture& c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
			auto& capture = c.data<Capturing>();
			auto& edt = *c.thiz<_2DEditor>();
			auto dp = c.current<cEventReceiver>()->dispatcher;
			auto mp = Vec2f(dp->mouse_pos);
			if (is_mouse_scroll(action, key))
			{
				auto v = pos.x() > 0.f ? 1 : -1;
				edt.scale_level += v;
				if (edt.scale_level < 1 || edt.scale_level > edt.scale_level_max)
					edt.scale_level -= v;
				else
				{
					auto p = (mp - edt.base->global_pos) / ((edt.scale_level - v) * 0.1f);
					edt.base->add_pos(float(v) * p * -0.1f);
					edt.base->set_scale(edt.scale_level * 0.1f);
					edt.scale_text->set_text((std::to_wstring(edt.scale_level * 10) + L"%").c_str());
				}
			}
			else if (is_mouse_down(action, key, true) && key == Mouse_Left)
			{
				edt.selecting = true;
				edt.select_begin = (Vec2f(pos) - edt.base->global_pos) / (edt.scale_level * 0.1f);
				edt.select_end = edt.select_begin;
				edt.element->mark_dirty();
				capture.f(c.release<Capturing>(), Vec4f(0.f));
			}
			else if (is_mouse_down(action, key, true) && key == Mouse_Right)
				edt.moved = false;
			else if (is_mouse_move(action, key))
			{
				if (edt.selecting)
				{
					edt.select_end = (mp - edt.base->global_pos) / (edt.scale_level * 0.1f);
					edt.element->mark_dirty();
				}

				if (dp->mouse_buttons[Mouse_Right] & KeyStateDown)
				{
					edt.base->add_pos(Vec2f(pos));
					edt.moved = true;
				}
			}
			return true;
		}, Capture().absorb(&capture, _capture).set_thiz(this));
		event_receiver->state_listeners.add([](Capture& c, EventReceiverState state) {
			auto& capture = c.data<Capturing>();
			auto& edt = *c.thiz<_2DEditor>();
			if (!(state & EventReceiverActive) && edt.selecting)
			{
				edt.selecting = false;
				edt.element->mark_dirty();

				auto p0 = edt.base->global_pos;
				auto s = edt.scale_level * 0.1f;
				capture.f(c.release<Capturing>(), rect_from_points(edt.select_begin * s + p0, edt.select_end * s + p0));
			}
			return true;
		}, Capture().absorb(&capture, _capture).set_thiz(this));
		f_free(_capture._data);

		base = ui.e_element()->get_component(cElement);
		moved = false;

		overlay = ui.e_element()->get_component(cElement);
		overlay->cmds.add([](Capture& c, graphics::Canvas* canvas) {
			auto& edt = *c.thiz<_2DEditor>();

			if (edt.element->clipped)
				return true;

			if (edt.selecting)
			{
				std::vector<Vec2f> points;
				auto p0 = edt.base->global_pos;
				auto s = edt.scale_level * 0.1f;
				auto p1 = edt.select_begin * s + p0;
				auto p2 = edt.select_end * s + p0;
				path_rect(points, p1, p2 - p1);
				points.push_back(points[0]);
				canvas->stroke(points.size(), points.data(), Vec4c(17, 193, 101, 255), 2.f);
			}

			return true;
		}, Capture().set_thiz(this));
		ui.c_event_receiver()->pass_checkers.add([](Capture& c, cEventReceiver* er, bool* pass) {
			*pass = true;
			return true;
		}, Capture());
		ui.c_aligner(AlignMinMax, AlignMinMax);

		scale_text = ui.e_text(L"100%")->get_component(cText);
		ui.c_aligner(AlignMin, AlignMax);
		ui.e_end_layout();
	}
};

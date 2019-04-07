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

#include <flame/universe/component.h>

namespace flame
{
	struct cEvent$ : Component
	{
		bool blackhole;
		bool want_key;

		ATTRIBUTE_BOOL hovering;
		ATTRIBUTE_BOOL dragging;
		ATTRIBUTE_BOOL focusing;

		FLAME_UNIVERSE_EXPORTS virtual ~cEvent$() override;

		FLAME_UNIVERSE_EXPORTS virtual const char* type_name() const override;
		FLAME_UNIVERSE_EXPORTS virtual uint type_hash() const override;

		FLAME_UNIVERSE_EXPORTS virtual void on_attach() override;

		FLAME_UNIVERSE_EXPORTS virtual void update(float delta_time) override;

		FLAME_UNIVERSE_EXPORTS bool contains(const Vec2& pos) const;

		FLAME_UNIVERSE_EXPORTS void on_key(KeyState action, int value);
		FLAME_UNIVERSE_EXPORTS void on_mouse(KeyState action, MouseKey key, const Vec2& value);
		FLAME_UNIVERSE_EXPORTS void on_drop(cEvent$* src);

		FLAME_UNIVERSE_EXPORTS static cEvent$* create$(void* data);
	};
}

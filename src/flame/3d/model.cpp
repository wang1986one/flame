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

#include "model_private.h"

namespace flame
{
	namespace _3d
	{
		inline ModelPrivate::ModelPrivate()
		{
		}

		inline void ModelPrivate::add_plane(const Vec3 &pos, const Vec3 &vx, const Vec3 &vz)
		{
			Primitive p;
			p.pt = PrimitiveTopologyPlane;
			p.p = pos;
			p.vx = vx;
			p.vz = vz;
			prims.push_back(p);
		}

		inline void ModelPrivate::add_cube(const Vec3 &pos, const Vec3 &vx, const Vec3 &vz, float height, int side)
		{
			auto normal = -cross(vx, vz);
			normal.normalize();
			auto vy = normal * height;

			if (side & AxisPositiveX)
				add_plane(pos + vx + vy + vz, -vz, -vy);
			if (side & AxisNegativeX)
				add_plane(pos + vy, vz, -vy);
			if (side & AxisPositiveY)
				add_plane(pos + vy, vx, vz);
			if (side & AxisNegativeY)
				add_plane(pos + vz, vx, -vz);
			if (side & AxisPositiveZ)
				add_plane(pos + vy + vz, vx, -vy);
			if (side & AxisNegativeZ)
				add_plane(pos + vx + vy, -vx, -vy);
		}

		void Model::add_plane(const Vec3 &pos, const Vec3 &vx, const Vec3 &vz)
		{
			((ModelPrivate*)this)->add_plane(pos, vx, vz);
		}

		void Model::add_cube(const Vec3 &pos, const Vec3 &vx, const Vec3 &vz, float height, int side)
		{
			((ModelPrivate*)this)->add_cube(pos, vx, vz, height, side);
		}

		Model *Model::create()
		{
			return new ModelPrivate;
		}

		void Model::destroy(Model *m)
		{

		}
	}
}


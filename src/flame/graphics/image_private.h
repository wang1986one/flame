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

#include "graphics_private.h"
#include "image.h"

#include <vector>

namespace flame
{
	namespace graphics
	{
		struct DevicePrivate;
		struct ImageviewPrivate;

		struct ImagePrivate : Image
		{
			int usage;
			int mem_prop;

			DevicePrivate *d;
			std::vector<ImageviewPrivate*> views;
			VkDeviceMemory m;
			VkImage v;

			ImagePrivate(Device *d, Format format, const Ivec2 &size, int level, int layer, SampleCount sample_count, int usage, int mem_prop);
			ImagePrivate(Device *d, Format format, const Ivec2 &size, int level, int layer, void *native);
			~ImagePrivate();

			void set_props();

			void init(const Bvec4 &col);
			Bvec4 get_pixel(int x, int y);
			void set_pixel(int x, int y, const Bvec4 &col);
			void set_pixel(int x, int y, const Hvec4 &col);
			void get_pixels(void *dst);
			void set_pixels(void *src);

			void save_png(const wchar_t *filename);
		};

		struct ImageviewPrivate : Imageview
		{
			ImagePrivate *i;
			VkImageView v;

			int ref_count;

			ImageviewPrivate(Image *i, ImageviewType type = Imageview2D, int base_level = 0, int level_count = 1, int base_layer = 0, int layer_count = 1, ComponentMapping *mapping = nullptr);
			~ImageviewPrivate();

			bool same(Image *i, ImageviewType type, int base_level, int level_count, int base_layer, int layer_count, ComponentMapping *mapping);
		};

		inline VkImageLayout Z(ImageLayout l, Format fmt)
		{
			switch (l)
			{
				case ImageLayoutUndefined:
					return VK_IMAGE_LAYOUT_UNDEFINED;
				case ImageLayoutAttachment:
					if (fmt >= Format_Color_Begin && fmt <= Format_Color_End)
						return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					else
						return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				case ImageLayoutShaderReadOnly:
					return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				case ImageLayoutShaderStorage:
					return VK_IMAGE_LAYOUT_GENERAL;
				case ImageLayoutTransferSrc:
					return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				case ImageLayoutTransferDst:
					return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			}
		}

		inline ImageAspect aspect_from_format(Format fmt)
		{
			if (fmt >= Format_Color_Begin && fmt <= Format_Color_End)
				return ImageAspectColor;
			if (fmt >= Format_Depth_Begin && fmt <= Format_Depth_End)
			{
				int a = ImageAspectDepth;
				if (fmt >= Format_DepthStencil_Begin && fmt <= Format_DepthStencil_End)
					a |= ImageAspectStencil;
				return (ImageAspect)a;
			}
			return ImageAspectColor;
		}
	}
}


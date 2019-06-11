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

#include <flame/graphics/commandbuffer.h>
#include "graphics_private.h"

namespace flame
{
	namespace graphics
	{
		struct DevicePrivate;
		struct ImagePrivate;
		struct RenderpassPrivate;
		struct FramebufferPrivate;
		struct PipelinePrivate;

		struct CommandpoolPrivate : Commandpool
		{
			DevicePrivate *d;
#if defined(FLAME_VULKAN)
			VkCommandPool v;
#elif defined(FLAME_D3D12)
			ID3D12CommandAllocator* v;
#endif

			CommandpoolPrivate(Device *d, int queue_family_idx);
			~CommandpoolPrivate();
		};

		struct CommandbufferPrivate : Commandbuffer
		{
			CommandpoolPrivate* p;
			RenderpassPrivate* current_renderpass;
			int current_subpass;
			FramebufferPrivate* current_framebuffer;
			PipelinePrivate* current_pipeline;
#if defined(FLAME_VULKAN)
			VkCommandBuffer v;
#elif defined(FLAME_D3D12)
			ID3D12GraphicsCommandList* v;
			bool recording;
#endif

			CommandbufferPrivate(Commandpool *p, bool sub = false);
			~CommandbufferPrivate();

			void begin(bool once = false);

			void begin_renderpass(Renderpass *r, Framebuffer *f, Clearvalues*cv);
			void end_renderpass();
			void set_viewport(const Vec4f& rect);
			void set_scissor(const Vec4f& rect);
			void bind_pipeline(Pipeline *p);
			void bind_descriptorset(Descriptorset *s, uint idx);
			void bind_vertexbuffer(Buffer *b, uint id);
			void bind_indexbuffer(Buffer *b, IndiceType t);
			void push_constant(uint offset, uint size, const void *data, Pipelinelayout *layout);
			void draw(uint count, uint instance_count, uint first_vertex, uint first_instance);
			void draw_indexed(uint count, uint first_index, int vertex_offset, uint instance_count, uint first_instance);
			void dispatch(const Vec3u& v);

			void copy_buffer(Buffer *src, Buffer *dst, uint copy_count, BufferCopy *copies);
			void copy_image(Image *src, Image *dst, uint copy_count, ImageCopy *copies);
			void copy_buffer_to_image(Buffer *src, Image *dst, uint copy_count, BufferImageCopy *copies);
			void copy_image_to_buffer(Image *src, Buffer *dst, uint copy_count, BufferImageCopy *copies);
			void change_image_layout(Image *i, ImageLayout from, ImageLayout to, uint base_level, uint level_count, uint base_layer, uint layer_count);

			void clear_image(ImagePrivate *i, const Vec4c& col);

			void end();
		};

		struct QueuePrivate : Queue
		{
			DevicePrivate *d;
#if defined(FLAME_VULKAN)
			VkQueue v;
#elif defined(FLAME_D3D12)
			ID3D12CommandQueue* v;
#endif

			QueuePrivate(Device *d, uint queue_family_idx);
			~QueuePrivate();

			void wait_idle();
			void submit(Commandbuffer *c, Semaphore *wait_semaphore, Semaphore *signal_semaphore, Fence* signal_fence);
			void present(Swapchain *s, Semaphore *wait_semaphore);
		};
	}
}

#include "device_private.h"
#include "synchronize_private.h"
#include "renderpass_private.h"
#include "swapchain_private.h"
#include "commandbuffer_private.h"
#include "buffer_private.h"
#include "image_private.h"
#include "shader_private.h"

namespace flame
{
	namespace graphics
	{
		CommandpoolPrivate::CommandpoolPrivate(DevicePrivate* d, int queue_family_idx) :
			_d(d)
		{
#if defined(FLAME_VULKAN)
			VkCommandPoolCreateInfo info;
			info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			info.pNext = nullptr;
			info.queueFamilyIndex = queue_family_idx;

			chk_res(vkCreateCommandPool(d->_v, &info, nullptr, &_v));
#elif defined(FLAME_D3D12)
			auto res = d->_v->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&v));
			assert(SUCCEEDED(res));
#endif
		}

		CommandpoolPrivate::~CommandpoolPrivate()
		{
#if defined(FLAME_VULKAN)
			vkDestroyCommandPool(_d->_v, _v, nullptr);
#elif defined(FLAME_D3D12)

#endif
		}

		Commandpool* Commandpool::create(Device* d, int queue_family_idx)
		{
			return new CommandpoolPrivate((DevicePrivate*)d, queue_family_idx);
		}

		CommandbufferPrivate::CommandbufferPrivate(CommandpoolPrivate* p, bool sub) :
			_p(p)
		{
			_current_renderpass = nullptr;
			_current_subpass = 0;
			_current_framebuffer = nullptr;
			_current_pipeline = nullptr;

#if defined(FLAME_VULKAN)
			VkCommandBufferAllocateInfo info;
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.pNext = nullptr;
			info.level = !sub ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			info.commandPool = p->_v;
			info.commandBufferCount = 1;

			chk_res(vkAllocateCommandBuffers(p->_d->_v, &info, &_v));
#elif defined(FLAME_D3D12)
			auto res = p->_d->_v->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, p->_v, nullptr, IID_PPV_ARGS(&v));
			assert(SUCCEEDED(res));
			recording = true;
			end();
#endif

			begin();
			end();
		}

		CommandbufferPrivate::~CommandbufferPrivate()
		{
#if defined(FLAME_VULKAN)
			vkFreeCommandBuffers(_p->_d->_v, _p->_v, 1, &_v);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_begin(bool once)
		{
#if defined(FLAME_VULKAN)
			VkCommandBufferBeginInfo info;
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT |
				(once ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0);
			info.pNext = nullptr;
			info.pInheritanceInfo = nullptr;

			chk_res(vkBeginCommandBuffer(_v, &info));
#elif defined(FLAME_D3D12)
			if (recording)
				return;
			v->Reset(p->_v, nullptr);
			recording = true;
#endif
			_current_renderpass = nullptr;
			_current_subpass = 0;
			_current_framebuffer = nullptr;
			_current_pipeline = nullptr;
		}

		void CommandbufferPrivate::_begin_renderpass(FramebufferPrivate* fb, std::span<const Vec4f> clearvalues)
		{
			auto rp = fb->_rp;

			_current_renderpass = rp;
			_current_subpass = 0;
			_current_framebuffer = fb;

#if defined(FLAME_VULKAN)
			VkRenderPassBeginInfo info;
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			info.pNext = nullptr;
			info.renderPass = rp->_v;
			info.framebuffer = fb->_v;
			info.renderArea.offset.x = 0;
			info.renderArea.offset.y = 0;
			auto size = fb->_views[0]->_image->_size;
			info.renderArea.extent.width = size.x();
			info.renderArea.extent.height = size.y();
			info.clearValueCount = clearvalues.size();
			info.pClearValues = (VkClearValue*)clearvalues.data();

			vkCmdBeginRenderPass(_v, &info, VK_SUBPASS_CONTENTS_INLINE);
#elif defined(FLAME_D3D12)
			auto& attachments = rp->_info.attachments;
			auto& subpass = rp->_info.subpasses[current_subpass];
			auto& views = fb->_info.views;
			for (auto& idx : subpass.color_attachments)
			{
				auto& a = attachments[idx];
				auto view = (ImageviewPrivate*)views[idx];
				auto layout_from = ImageLayoutUndefined;
				if (a.format >= Format_Swapchain_Begin && a.format <= Format_Swapchain_End)
					layout_from = ImageLayoutPresent;
				change_image_layout(view->_i, layout_from, ImageLayoutAttachment);
				auto descriptor = view->_v->GetCPUDescriptorHandleForHeapStart();
				v->OMSetRenderTargets(1, &descriptor, false, nullptr);
				if (a.clear)
					v->ClearRenderTargetView(descriptor, &cv->_v[idx].x(), 0, nullptr);
		}
#endif
		}

		void CommandbufferPrivate::_end_renderpass()
		{
#if defined(FLAME_VULKAN)
			vkCmdEndRenderPass(_v);
#elif defined(FLAME_D3D12)
			auto& attachments = current_renderpass->info.attachments;
			auto& subpass = current_renderpass->info.subpasses[current_subpass];
			auto& views = current_framebuffer->info.views;
			for (auto& idx : subpass.color_attachments)
			{
				auto& a = attachments[idx];
				auto view = (ImageviewPrivate*)views[idx];
				auto layout_to = ImageLayoutUndefined;
				if (a.format >= Format_Swapchain_Begin && a.format <= Format_Swapchain_End)
					layout_to = ImageLayoutPresent;
				change_image_layout(view->i, ImageLayoutAttachment, layout_to);
			}
#endif
		}

		void CommandbufferPrivate::_set_viewport(const Vec4f& rect)
		{
#if defined(FLAME_VULKAN)
			VkViewport vp;
			vp.minDepth = 0.f;
			vp.maxDepth = 1.f;
			vp.x = rect.x();
			vp.y = rect.y();
			vp.width = max(rect.z() - rect.x(), 1.f);
			vp.height = max(rect.w() - rect.y(), 1.f);
			vkCmdSetViewport(_v, 0, 1, &vp);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_set_scissor(const Vec4f& rect)
		{
#if defined(FLAME_VULKAN)
			VkRect2D sc;
			sc.offset.x = max(0.f, rect.x());
			sc.offset.y = max(0.f, rect.y());
			sc.extent.width = max(0.f, rect.z() - rect.x());
			sc.extent.height = max(0.f, rect.w() - rect.y());
			vkCmdSetScissor(_v, 0, 1, &sc);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_bind_pipeline(PipelinePrivate* p)
		{
			if (_current_pipeline == p)
				return;

			assert(p->_type != PipelineNone);
			_current_pipeline = p;
#if defined(FLAME_VULKAN)
			vkCmdBindPipeline(_v, to_backend(p->_type), p->_v);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_bind_descriptorset(DescriptorsetPrivate* s, uint idx, PipelinelayoutPrivate* pll)
		{
#if defined(FLAME_VULKAN)
			vkCmdBindDescriptorSets(_v, to_backend(pll ? PipelineGraphics : _current_pipeline->_type), pll ? pll->_v : _current_pipeline->_pll->_v, idx, 1, &s->_v, 0, nullptr);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_bind_vertexbuffer(BufferPrivate* b, uint id)
		{
#if defined(FLAME_VULKAN)
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(_v, id, 1, &b->_v, &offset);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_bind_indexbuffer(BufferPrivate* b, IndiceType t)
		{
#if defined(FLAME_VULKAN)
			vkCmdBindIndexBuffer(_v, b->_v, 0, t == IndiceTypeUint ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_push_constant(uint offset, uint size, const void* data, PipelinelayoutPrivate* pll)
		{
			if (!pll)
				pll = _current_pipeline->_pll;
#if defined(FLAME_VULKAN)
			vkCmdPushConstants(_v, pll ? pll->_v : _current_pipeline->_pll->_v, to_backend_flags<ShaderStage>(ShaderStageAll), offset, size, data);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_draw(uint count, uint instance_count, uint first_vertex, uint first_instance)
		{
#if defined(FLAME_VULKAN)
			vkCmdDraw(_v, count, instance_count, first_vertex, first_instance);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_draw_indexed(uint count, uint first_index, int vertex_offset, uint instance_count, uint first_instance)
		{
#if defined(FLAME_VULKAN)
			vkCmdDrawIndexed(_v, count, instance_count, first_index, vertex_offset, first_instance);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_dispatch(const Vec3u& v)
		{
#if defined(FLAME_VULKAN)
			vkCmdDispatch(_v, v.x(), v.y(), v.z());
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_copy_buffer(BufferPrivate* src, BufferPrivate* dst, std::span<BufferCopy> copies)
		{
#if defined(FLAME_VULKAN)
			std::vector<VkBufferCopy> vk_copies(copies.size());
			for (auto i = 0; i < vk_copies.size(); i++)
			{
				vk_copies[i].srcOffset = copies[i].src_off;
				vk_copies[i].dstOffset = copies[i].dst_off;
				vk_copies[i].size = copies[i].size;
			}
			vkCmdCopyBuffer(_v, src->_v, dst->_v, vk_copies.size(), vk_copies.data());
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_copy_image(ImagePrivate* src, ImagePrivate* dst, std::span<ImageCopy> copies)
		{
#if defined(FLAME_VULKAN)
			std::vector<VkImageCopy> vk_copies(copies.size());
			for (auto i = 0; i < vk_copies.size(); i++)
			{
				vk_copies[i].srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				vk_copies[i].srcSubresource.mipLevel = 0;
				vk_copies[i].srcSubresource.baseArrayLayer = 0;
				vk_copies[i].srcSubresource.layerCount = 1;
				vk_copies[i].srcOffset.x = copies[i].src_off.x();
				vk_copies[i].srcOffset.y = copies[i].src_off.y();
				vk_copies[i].srcOffset.z = 0;
				vk_copies[i].dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				vk_copies[i].dstSubresource.mipLevel = 0;
				vk_copies[i].dstSubresource.baseArrayLayer = 0;
				vk_copies[i].dstSubresource.layerCount = 1;
				vk_copies[i].dstOffset.x = copies[i].dst_off.x();
				vk_copies[i].dstOffset.y = copies[i].dst_off.y();
				vk_copies[i].dstOffset.z = 0;
				vk_copies[i].extent.width = copies[i].size.x();
				vk_copies[i].extent.height = copies[i].size.y();
				vk_copies[i].extent.depth = 1;
			}
			vkCmdCopyImage(_v, src->_v, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->_v,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vk_copies.size(), vk_copies.data());
#elif defined(FLAME_D3D12)

#endif
		}

#if defined(FLAME_VULKAN)
		VkBufferImageCopy to_backend(const BufferImageCopy& cpy, VkImageAspectFlags aspect)
		{
			VkBufferImageCopy vk_cpy = {};
			vk_cpy.bufferOffset = cpy.buffer_offset;
			vk_cpy.imageOffset.x = cpy.image_offset.x();
			vk_cpy.imageOffset.y = cpy.image_offset.y();
			vk_cpy.imageExtent.width = cpy.image_extent.x();
			vk_cpy.imageExtent.height = cpy.image_extent.y();
			vk_cpy.imageExtent.depth = 1;
			vk_cpy.imageSubresource.aspectMask = aspect;
			vk_cpy.imageSubresource.mipLevel = cpy.image_level;
			vk_cpy.imageSubresource.layerCount = 1;
			return vk_cpy;
		}
#endif

		void CommandbufferPrivate::_copy_buffer_to_image(BufferPrivate* src, ImagePrivate* dst, std::span<BufferImageCopy> copies)
		{
#if defined(FLAME_VULKAN)
			auto aspect = to_backend_flags<ImageAspect>(aspect_from_format(dst->_format));

			std::vector<VkBufferImageCopy> vk_copies(copies.size());
			for (auto i = 0; i < vk_copies.size(); i++)
				vk_copies[i] = to_backend(copies[i], aspect);
			vkCmdCopyBufferToImage(_v, src->_v, dst->_v,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vk_copies.size(), vk_copies.data());
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_copy_image_to_buffer(ImagePrivate* src, BufferPrivate* dst, std::span<BufferImageCopy> copies)
		{
#if defined(FLAME_VULKAN)
			auto aspect = to_backend_flags<ImageAspect>(aspect_from_format(src->_format));

			std::vector<VkBufferImageCopy> vk_copies(copies.size());
			for (auto i = 0; i < vk_copies.size(); i++)
				vk_copies[i] = to_backend(copies[i], aspect);
			vkCmdCopyImageToBuffer(_v, src->_v,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->_v, vk_copies.size(), vk_copies.data());
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_change_image_layout(ImagePrivate* i, ImageLayout from, ImageLayout to, uint base_level, uint level_count, uint base_layer, uint layer_count)
		{
			level_count = level_count == 0 ? i->_level : level_count;
			layer_count = layer_count == 0 ? i->_layer : layer_count;

#if defined(FLAME_VULKAN)
			VkImageMemoryBarrier barrier;
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.pNext = nullptr;
			barrier.oldLayout = to_backend(from, i->_format);
			barrier.newLayout = to_backend(to, i->_format);
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = i->_v;
			barrier.subresourceRange.aspectMask = to_backend_flags<ImageAspect>(aspect_from_format(i->_format));
			barrier.subresourceRange.baseMipLevel = base_level;
			barrier.subresourceRange.levelCount = level_count;
			barrier.subresourceRange.baseArrayLayer = base_layer;
			barrier.subresourceRange.layerCount = layer_count;

			switch (barrier.oldLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				barrier.srcAccessMask = 0;
				break;
			case VK_IMAGE_LAYOUT_GENERAL:
				barrier.srcAccessMask = 0;
				break;
			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
				barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				break;
			default:
				barrier.srcAccessMask = 0;
				break;
			}

			switch (barrier.newLayout)
			{
			case VK_IMAGE_LAYOUT_GENERAL:
				barrier.dstAccessMask = 0;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
				break;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				if (barrier.srcAccessMask == 0)
					barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
				barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				break;
			default:
				barrier.dstAccessMask = 0;
				break;
			}

			vkCmdPipelineBarrier(_v, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				0, 0, nullptr, 0, nullptr, 1, &barrier);
#elif defined(FLAME_D3D12)
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = ((ImagePrivate*)i)->v;
			barrier.Transition.StateBefore = Z(from);
			barrier.Transition.StateAfter = Z(to);
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
#endif
		}

		void CommandbufferPrivate::_clear_image(ImagePrivate* i, const Vec4c& col)
		{
#if defined(FLAME_VULKAN)
			VkClearColorValue cv;
			cv.float32[0] = col.x() / 255.f;
			cv.float32[1] = col.y() / 255.f;
			cv.float32[2] = col.z() / 255.f;
			cv.float32[3] = col.w() / 255.f;
			VkImageSubresourceRange r;
			r.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			r.baseMipLevel = 0;
			r.levelCount = 1;
			r.baseArrayLayer = 0;
			r.layerCount = 1;
			vkCmdClearColorImage(_v, i->_v, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cv, 1, &r);
#elif defined(FLAME_D3D12)

#endif
		}

		void CommandbufferPrivate::_end()
		{
#if defined(FLAME_VULKAN)
			chk_res(vkEndCommandBuffer(_v));
#elif defined(FLAME_D3D12)
			if (recording)
			{
				auto res = v->Close();
				assert(SUCCEEDED(res));
				recording = false;
			}
#endif
		}

		Commandbuffer* Commandbuffer::create(Commandpool* p, bool sub)
		{
			return new CommandbufferPrivate((CommandpoolPrivate*)p, sub);
		}

		QueuePrivate::QueuePrivate(DevicePrivate* d, uint queue_family_idx) :
			_d(d)
		{
#if defined(FLAME_VULKAN)
			vkGetDeviceQueue(d->_v, queue_family_idx, 0, &_v);
#elif defined(FLAME_D3D12)
			D3D12_COMMAND_QUEUE_DESC desc = {};
			auto res = d->v->CreateCommandQueue(&desc, IID_PPV_ARGS(&v));
			assert(SUCCEEDED(res));
#endif
		}

		void QueuePrivate::_wait_idle()
		{
#if defined(FLAME_VULKAN)
			chk_res(vkQueueWaitIdle(_v));
#elif defined(FLAME_D3D12)

#endif
		}

		void QueuePrivate::_submit(std::span<CommandbufferPrivate*> cbs, SemaphorePrivate* wait_semaphore, SemaphorePrivate* signal_semaphore, FencePrivate* signal_fence)
		{
#if defined(FLAME_VULKAN)
			VkSubmitInfo info;
			info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			info.pNext = nullptr;
			VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			info.pWaitDstStageMask = &wait_stage;
			info.waitSemaphoreCount = wait_semaphore ? 1 : 0;
			info.pWaitSemaphores = wait_semaphore ? &wait_semaphore->_v : nullptr;
			info.commandBufferCount = cbs.size();
			std::vector<VkCommandBuffer> vk_cbs;
			vk_cbs.resize(cbs.size());
			for (auto i = 0; i < vk_cbs.size(); i++)
				vk_cbs[i] = cbs[i]->_v;
			info.pCommandBuffers = vk_cbs.data();
			info.signalSemaphoreCount = signal_semaphore ? 1 : 0;
			info.pSignalSemaphores = signal_semaphore ? &signal_semaphore->_v : nullptr;

			chk_res(vkQueueSubmit(_v, 1, &info, signal_fence ? signal_fence->_v : nullptr));
			if (signal_fence)
				signal_fence->_vl = 1;
#elif defined(FLAME_D3D12)
			ID3D12CommandList* list[] = { ((CommandbufferPrivate*)c)->v };
			v->ExecuteCommandLists(1, list);

			if (signal_fence)
			{
				auto fence = (FencePrivate*)signal_fence;
				fence->vl++;
				auto res = v->Signal(fence->v, fence->vl);
				assert(SUCCEEDED(res));
			}
#endif
		}

		void QueuePrivate::_present(SwapchainPrivate* sc, SemaphorePrivate* wait_semaphore)
		{
#if defined(FLAME_VULKAN)
			VkPresentInfoKHR info;
			info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			info.pNext = nullptr;
			info.pResults = nullptr;
			info.waitSemaphoreCount = wait_semaphore ? 1 : 0;
			info.pWaitSemaphores = wait_semaphore ? &wait_semaphore->_v : nullptr;
			info.swapchainCount = 1;
			info.pSwapchains = &sc->_v;
			info.pImageIndices = &sc->_image_index;
			chk_res(vkQueuePresentKHR(_v, &info));
#elif defined(FLAME_D3D12)
			auto res = ((SwapchainPrivate*)s)->v->Present(0, 0);
			assert(SUCCEEDED(res));
#endif
		}

		Queue* Queue::create(Device* d, uint queue_family_idx)
		{
			return new QueuePrivate((DevicePrivate*)d, queue_family_idx);
		}
	}
}

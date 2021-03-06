#include "device_private.h"
#include "renderpass_private.h"
#include "buffer_private.h"
#include "image_private.h"
#include "shader_private.h"

namespace flame
{
	namespace graphics
	{
		DescriptorpoolPrivate::DescriptorpoolPrivate(DevicePrivate* d) :
			_d(d)
		{
#if defined(FLAME_VULKAN)
			VkDescriptorPoolSize descriptorPoolSizes[] = {
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128},
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128},
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 32},
			};

			VkDescriptorPoolCreateInfo descriptorPoolInfo;
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			descriptorPoolInfo.pNext = nullptr;
			descriptorPoolInfo.poolSizeCount = array_size(descriptorPoolSizes);
			descriptorPoolInfo.pPoolSizes = descriptorPoolSizes;
			descriptorPoolInfo.maxSets = 8;
			chk_res(vkCreateDescriptorPool(_d->_v, &descriptorPoolInfo, nullptr, &_v));
#elif defined(FLAME_D3D12)

#endif
		}

		DescriptorpoolPrivate::~DescriptorpoolPrivate()
		{
#if defined(FLAME_VULKAN)
			vkDestroyDescriptorPool(_d->_v, _v, nullptr);
#elif defined(FLAME_D3D12)

#endif
		}

		Descriptorpool* Descriptorpool::create(Device* d)
		{
			return new DescriptorpoolPrivate((DevicePrivate*)d);
		}

		DescriptorBindingPrivate::DescriptorBindingPrivate(uint index, const DescriptorBindingInfo& info) :
			_index(index),
			_type(info.type),
			_count(info.count),
			_name(info.name)
		{
		}

		DescriptorlayoutPrivate::DescriptorlayoutPrivate(DevicePrivate* d, std::span<const DescriptorBindingInfo> bindings, bool create_default_set) :
			_d(d)
		{
#if defined(FLAME_VULKAN)
			std::vector<VkDescriptorSetLayoutBinding> vk_bindings(bindings.size());
			_bindings.resize(bindings.size());
			for (auto i = 0; i < bindings.size(); i++)
			{
				auto& src = bindings[i];
				auto& dst = vk_bindings[i];

				dst.binding = i;
				dst.descriptorType = to_backend(src.type);
				dst.descriptorCount = src.count;
				dst.stageFlags = to_backend_flags<ShaderStage>(ShaderStageAll);
				dst.pImmutableSamplers = nullptr;

				_bindings[i].reset(new DescriptorBindingPrivate(i, src));
			}

			VkDescriptorSetLayoutCreateInfo info;
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.flags = 0;
			info.pNext = nullptr;
			info.bindingCount = vk_bindings.size();
			info.pBindings = vk_bindings.data();

			chk_res(vkCreateDescriptorSetLayout(_d->_v, &info, nullptr, &_v));
#elif defined(FLAME_D3D12)

#endif
			if (create_default_set)
				_default_set.reset(new DescriptorsetPrivate(_d->_descriptorpool.get(), this));

			_hash = 0;
			for (auto& b : bindings)
			{
				hash_update(_hash, b.type);
				hash_update(_hash, FLAME_HASH(b.name));
				hash_update(_hash, b.count);
			}
		}

		DescriptorlayoutPrivate::~DescriptorlayoutPrivate()
		{
#if defined(FLAME_VULKAN)
			vkDestroyDescriptorSetLayout(_d->_v, _v, nullptr);
#elif defined(FLAME_D3D12)

#endif
		}

		Descriptorlayout* Descriptorlayout::create(Device* d, uint binding_count, const DescriptorBindingInfo* bindings, bool create_default_set)
		{
			return new DescriptorlayoutPrivate((DevicePrivate*)d, { bindings, binding_count }, create_default_set);
		}

		DescriptorsetPrivate::DescriptorsetPrivate(DescriptorpoolPrivate* p, DescriptorlayoutPrivate* l) :
			_p(p),
			_l(l)
		{
#if defined(FLAME_VULKAN)
			VkDescriptorSetAllocateInfo info;
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			info.pNext = nullptr;
			info.descriptorPool = p->_v;
			info.descriptorSetCount = 1;
			info.pSetLayouts = &l->_v;

			chk_res(vkAllocateDescriptorSets(p->_d->_v, &info, &_v));
#elif defined(FLAME_D3D12)

#endif
		}

		DescriptorsetPrivate::~DescriptorsetPrivate()
		{
#if defined(FLAME_VULKAN)
			chk_res(vkFreeDescriptorSets(_p->_d->_v, _p->_v, 1, &_v));
#elif defined(FLAME_D3D12)

#endif
		}

		void DescriptorsetPrivate::_set_buffer(uint binding, uint index, BufferPrivate* b, uint offset, uint range)
		{
#if defined(FLAME_VULKAN)
			VkDescriptorBufferInfo i;
			i.buffer = b->_v;
			i.offset = offset;
			i.range = range == 0 ? b->_size : range;

			VkWriteDescriptorSet write;
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.pNext = nullptr;
			write.dstSet = _v;
			write.dstBinding = binding;
			write.dstArrayElement = index;
			write.descriptorType = to_backend(_l->_bindings[binding]->_type);
			write.descriptorCount = 1;
			write.pBufferInfo = &i;
			write.pImageInfo = nullptr;
			write.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(_p->_d->_v, 1, &write, 0, nullptr);
#elif defined(FLAME_D3D12)

#endif
		}

		void DescriptorsetPrivate::_set_image(uint binding, uint index, ImageviewPrivate* iv, SamplerPrivate* sampler)
		{
#if defined(FLAME_VULKAN)
			VkDescriptorImageInfo i;
			i.imageView = iv->_v;
			i.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			i.sampler = sampler->_v;

			VkWriteDescriptorSet write;
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.pNext = nullptr;
			write.dstSet = _v;
			write.dstBinding = binding;
			write.dstArrayElement = index;
			write.descriptorType = to_backend(_l->_bindings[binding]->_type);
			write.descriptorCount = 1;
			write.pBufferInfo = nullptr;
			write.pImageInfo = &i;
			write.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(_p->_d->_v, 1, &write, 0, nullptr);
#elif defined(FLAME_D3D12)

#endif
		}

		Descriptorset* Descriptorset::create(Descriptorpool* p, Descriptorlayout* l)
		{
			return new DescriptorsetPrivate((DescriptorpoolPrivate*)p, (DescriptorlayoutPrivate*)l);
		}

		PipelinelayoutPrivate::PipelinelayoutPrivate(DevicePrivate* d, std::span<DescriptorlayoutPrivate*> descriptorlayouts, uint push_constant_size) :
			_d(d),
			_pc_size(push_constant_size)
		{
#if defined(FLAME_VULKAN)
			std::vector<VkDescriptorSetLayout> vk_descriptorsetlayouts;
			vk_descriptorsetlayouts.resize(descriptorlayouts.size());
			for (auto i = 0; i < descriptorlayouts.size(); i++)
				vk_descriptorsetlayouts[i] = descriptorlayouts[i]->_v;

			VkPushConstantRange vk_pushconstant;
			vk_pushconstant.offset = 0;
			vk_pushconstant.size = _pc_size;
			vk_pushconstant.stageFlags = to_backend_flags<ShaderStage>(ShaderStageAll);

			VkPipelineLayoutCreateInfo info;
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.flags = 0;
			info.pNext = nullptr;
			info.setLayoutCount = vk_descriptorsetlayouts.size();
			info.pSetLayouts = vk_descriptorsetlayouts.data();
			info.pushConstantRangeCount = _pc_size > 0 ? 1 : 0;
			info.pPushConstantRanges = _pc_size > 0 ? &vk_pushconstant : nullptr;

			chk_res(vkCreatePipelineLayout(_d->_v, &info, nullptr, &_v));
#elif defined(FLAME_D3D12)

#endif

			_dsls.resize(descriptorlayouts.size());
			for (auto i = 0; i < _dsls.size(); i++)
				_dsls[i] = (DescriptorlayoutPrivate*)descriptorlayouts[i];

			_hash = 0;
			for (auto d : _dsls)
				hash_update(_hash, d->_hash);
			hash_update(_hash, _pc_size);
		}

		PipelinelayoutPrivate::~PipelinelayoutPrivate()
		{
#if defined(FLAME_VULKAN)
			vkDestroyPipelineLayout(_d->_v, _v, nullptr);
#elif defined(FLAME_D3D12)

#endif
		}

		Pipelinelayout* Pipelinelayout::create(Device* d, uint descriptorlayout_count, Descriptorlayout* const* descriptorlayouts, uint push_constant_size)
		{
			return new PipelinelayoutPrivate((DevicePrivate*)d, { (DescriptorlayoutPrivate**)descriptorlayouts, descriptorlayout_count }, push_constant_size);
		}

		bool compile_shaders(DevicePrivate* d, const std::filesystem::path& dir, std::span<CompiledShader> shaders, PipelinelayoutPrivate* pll, const VertexInfo* vi)
		{
			static std::regex regex_in(R"(\s*in\s+([\w]+)\s+i_([\w]+)\s*;)");
			static std::regex regex_out(R"(\s*out\s+([\w]+)\s+o_([\w]+)(\{([\w:\s]+)\})?\s*;)");
			static std::regex regex_pc(R"(\s*pushconstant)");
			static std::regex regex_ubo(R"(\s*uniform\s+([\w]+))");
			static std::regex regex_tex(R"(\s*sampler2D\s+([\w]+)([\[\]0-9\s]+)?;)");

			auto hash = 0U;
			for (auto& s : shaders)
			{
				hash = hash_update(hash, FLAME_HASH(s.s->_filename.c_str()));
				hash = hash_update(hash, FLAME_HASH(s.s->_prefix.c_str()));
			}
			hash = hash_update(hash, pll->_hash);
			if (vi)
			{
				for (auto i = 0; i < vi->buffers_count; i++)
				{
					auto& b = vi->buffers[i];
					for (auto j = 0; j < b.attributes_count; j++)
					{
						auto& a = b.attributes[j];
						hash = hash_update(hash, a.format);
						hash = hash_update(hash, FLAME_HASH(a.name));
					}
					hash = hash_update(hash, b.rate);
				}
				hash = hash_update(hash, vi->primitive_topology);
				hash = hash_update(hash, vi->patch_control_points);
			}
			auto str_hash = std::to_wstring(hash);

			for (auto i = 0; i < shaders.size(); i++)
			{
				auto& s = shaders[i].s;
				auto& r = shaders[i].r;

				auto shader_path = dir / s->_filename;
				auto spv_path = shader_path;
				spv_path += L".";
				spv_path += str_hash;
				auto res_path = spv_path;
				spv_path += L".spv";
				res_path += L".res";

				auto ok = true;
				if (std::filesystem::exists(shader_path) && (!std::filesystem::exists(spv_path) || std::filesystem::last_write_time(spv_path) < std::filesystem::last_write_time(shader_path)))
				{
					wprintf(L"begin compiling shader:%s\n", shader_path.c_str());

					std::ifstream src(shader_path);

					std::string glsl_header = "#version 450 core\n"
						"#extension GL_ARB_shading_language_420pack : enable\n";
					if (s->_type != ShaderStageComp)
						glsl_header += "#extension GL_ARB_separate_shader_objects : enable\n";
					glsl_header += s->_prefix + "\n";

					auto type_id = 0;

					printf("generating glsl file");
					std::ofstream glsl_file(L"out.glsl");
					glsl_file << glsl_header;
					{
						std::string line;
						std::smatch res;
						while (!src.eof())
						{
							std::getline(src, line);

							auto get_formated_type = [](const std::string& type) {
								auto ret = type;
								if (ret[0] == 'i' || ret[0] == 'u')
									ret = "flat " + ret;
								return ret;
							};

							if (std::regex_search(line, res, regex_in))
							{
								ShaderInOut in;
								in.type = res[1].str();
								in.name = res[2].str();
								if (s->_type == ShaderStageVert)
								{
									auto location = 0;
									if (vi)
									{
										for (auto i = 0; i < vi->buffers_count; i++)
										{
											auto& b = vi->buffers[i];
											for (auto j = 0; j < b.attributes_count; j++)
											{
												auto& a = b.attributes[j];
												if (a.name == in.name)
												{
													glsl_file << "layout (location = " + std::to_string(location) + +") in " + get_formated_type(in.type) + " i_" + in.name + ";\n";
													location = -1;
													break;
												}
												location++;
											}
											if (location == -1)
												break;
										}
									}
								}
								else
								{
									auto& ps = shaders[i - 1].r;
									for (auto j = 0; j < ps._outputs.size(); j++)
									{
										if (ps._outputs[j].name == in.name)
										{
											glsl_file << "layout (location = " + std::to_string(j) + +") in " + get_formated_type(in.type) + " i_" + in.name + ";\n";
											break;
										}
									}
								}
								r._inputs.push_back(in);
							}
							else if (std::regex_search(line, res, regex_out))
							{
								ShaderInOut out;
								out.type = res[1].str();
								out.name = res[2].str();
								auto dual = false;
								if (s->_type == ShaderStageFrag)
								{
									BlendOptions bo;
									if (res[3].matched)
									{
										bo.enable = true;
										auto sp = SUS::split(res[4].str());
										for (auto& p : sp)
										{
											auto sp = SUS::split(p, ':');
											BlendFactor f;
											if (sp[1] == "0")
												f = BlendFactorZero;
											else if (sp[1] == "1")
												f = BlendFactorOne;
											else if (sp[1] == "sa")
												f = BlendFactorSrcAlpha;
											else if (sp[1] == "1msa")
												f = BlendFactorOneMinusSrcAlpha;
											else if (sp[1] == "s1c")
											{
												dual = true;
												f = BlendFactorSrc1Color;
											}
											else if (sp[1] == "1ms1c")
											{
												dual = true;
												f = BlendFactorOneMinusSrc1Color;
											}
											else
												continue;
											if (sp[0] == "sc")
												bo.src_color = f;
											else if (sp[0] == "dc")
												bo.dst_color = f;
											else if (sp[0] == "sa")
												bo.src_alpha = f;
											else if (sp[0] == "da")
												bo.dst_alpha = f;
										}
									}
									r._blend_options.push_back(bo);
								}
								if (dual)
								{
									glsl_file << "layout (location = " + std::to_string((int)r._outputs.size()) + +", index = 0) out " + get_formated_type(out.type) + " o_" + out.name + "0;\n";
									glsl_file << "layout (location = " + std::to_string((int)r._outputs.size()) + +", index = 1) out " + get_formated_type(out.type) + " o_" + out.name + "1;\n";
								}
								else
									glsl_file << "layout (location = " + std::to_string((int)r._outputs.size()) + +") out " + get_formated_type(out.type) + " o_" + out.name + ";\n";
								r._outputs.push_back(out);
							}
							else if (std::regex_search(line, res, regex_pc))
							{
								if (pll && pll->_pc_size > 0)
									glsl_file << "layout (push_constant) uniform pc_t\n";
								else
									glsl_file << "struct type_" + std::to_string(type_id++) + "\n";
							}
							else if (std::regex_search(line, res, regex_ubo))
							{
								auto set = 0;
								auto binding = -1;
								if (pll)
								{
									auto name = res[1].str();
									for (auto j = 0; j < pll->_dsls.size(); j++)
									{
										auto dsl = pll->_dsls[j];
										for (auto k = 0; k < dsl->_bindings.size(); k++)
										{
											if (dsl->_bindings[k]->_name == name)
											{
												set = j;
												binding = k;
												glsl_file << "layout (set = " + std::to_string(set) + ", binding = " + std::to_string(binding) + ") uniform type_" + std::to_string(type_id) + "\n";
												break;
											}
										}
										if (binding != -1)
											break;
									}
								}
								if (binding == -1)
									glsl_file << "struct eliminate_" + std::to_string(type_id) + "\n";
								type_id++;
							}
							else if (std::regex_search(line, res, regex_tex))
							{
								auto set = 0;
								auto binding = -1;
								if (pll)
								{
									auto name = res[1].str();
									for (auto j = 0; j < pll->_dsls.size(); j++)
									{
										auto dsl = pll->_dsls[j];
										for (auto k = 0; k < dsl->_bindings.size(); k++)
										{
											if (dsl->_bindings[k]->_name == name)
											{
												set = j;
												binding = k;
												glsl_file << "layout (set = " + std::to_string(set) + ", binding = " + std::to_string(binding) + ") uniform sampler2D " + name + (res[2].matched ? res[2].str() : "") + ";\n";
												break;
											}
										}
										if (binding != -1)
											break;
									}
								}
							}
							else
								glsl_file << line + "\n";
						}

					}
					glsl_file.close();
					printf(" - done\n");

					if (std::filesystem::exists(spv_path))
						std::filesystem::remove(spv_path);

					auto vk_sdk_path = getenv("VK_SDK_PATH");
					if (vk_sdk_path)
					{
						std::filesystem::path glslc_path = vk_sdk_path;
						glslc_path /= L"Bin/glslc.exe";

						std::wstring command_line(L" -fshader-stage=" + shader_stage_name(s->_type) + L" out.glsl -o" + spv_path.wstring());

						std::string output;
						exec(glslc_path.c_str(), (wchar_t*)command_line.c_str(), [](Capture& c, uint size) {
							auto& output = *c.thiz<std::string>();
							output.resize(size);
							return output.data();
						}, Capture().set_thiz(&output));
						std::filesystem::remove(L"out.glsl");
						if (!std::filesystem::exists(spv_path))
						{
							printf("compile error:\n%s\n", output.c_str());
							printf("trying to use fallback");

							std::ofstream glsl_file(L"out.glsl");
							glsl_file << glsl_header;
							switch (s->_type)
							{
							case ShaderStageVert:
								glsl_file << "void main()\n"
									"{\n"
									"\tgl_Position = vec4(0, 0, 0, 1);"
									"}\n";
								break;
							case ShaderStageFrag:
								glsl_file <<
									"void main()\n"
									"{\n"
									"}\n";
								break;
							default:
								assert(0); // WIP
							}
							glsl_file.close();

							exec(glslc_path.c_str(), (wchar_t*)command_line.c_str(), [](Capture& c, uint size) {
								auto& output = *c.thiz<std::string>();
								output.resize(size);
								return output.data();
							}, Capture().set_thiz(&output));
							std::filesystem::remove(L"out.glsl");
							if (!std::filesystem::exists(spv_path))
							{
								printf(" - failed\n error:\n%s", output.c_str());
								assert(0);
								ok = false;
							}
							else
								printf(" - ok\n");
						}

						wprintf(L"end compiling shader:%s\n", shader_path.c_str());
					}
					else
					{
						printf("cannot find vk sdk\n");
						assert(0);
					}

					{
						nlohmann::json json;
						if (s->_type == ShaderStageFrag)
						{
							auto& bos = json["blend_options"];
							for (auto i = 0; i < r._blend_options.size(); i++)
							{
								auto& src = r._blend_options[i];
								auto& dst = bos[i];
								dst["enable"] = src.enable;
								if (src.enable)
								{
									dst["sc"] = (int)src.src_color;
									dst["dc"] = (int)src.dst_color;
									dst["sa"] = (int)src.src_alpha;
									dst["da"] = (int)src.dst_alpha;
								}
							}
						}
						std::ofstream res(res_path);
						res << json.dump();
						res.close();
					}
				}
				else
				{
					auto res = get_file_content(res_path);
					if (!res.empty())
					{
						auto json = nlohmann::json::parse(res);
						if (s->_type == ShaderStageFrag)
						{
							auto& bos = json["blend_options"];
							for (auto j = 0; j < bos.size(); j++)
							{
								auto& src = bos[j];
								BlendOptions dst;
								dst.enable = src["enable"].get<bool>();
								if (dst.enable)
								{
									dst.src_color = (BlendFactor)src["sc"].get<int>();
									dst.dst_color = (BlendFactor)src["dc"].get<int>();
									dst.src_alpha = (BlendFactor)src["sa"].get<int>();
									dst.dst_alpha = (BlendFactor)src["da"].get<int>();
								}
								r._blend_options.push_back(dst);
							}
						}
					}
				}

				if (!ok)
					return false;

				auto spv_file = get_file_content(spv_path);
				if (spv_file.empty())
				{
					assert(0);
					return false;
				}

				report_used_file(spv_path.c_str());

#if defined(FLAME_VULKAN)
				VkShaderModuleCreateInfo shader_info;
				shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				shader_info.flags = 0;
				shader_info.pNext = nullptr;
				shader_info.codeSize = spv_file.size();
				shader_info.pCode = (uint*)spv_file.data();
				chk_res(vkCreateShaderModule(d->_v, &shader_info, nullptr, &shaders[i].m));
#elif defined(FLAME_D3D12)

#endif
			}

			return true;
		}

		ShaderPrivate::ShaderPrivate(const std::filesystem::path& filename, const std::string& prefix) :
			_filename(filename),
			_prefix(prefix)
		{
			_filename.make_preferred();
			_type = shader_stage_from_ext(_filename.extension());
		}

		PipelinePrivate::PipelinePrivate(DevicePrivate* d, std::span<CompiledShader> shaders, PipelinelayoutPrivate* pll, 
			RenderpassPrivate* rp, uint subpass_idx, VertexInfo* vi, const Vec2u& vp, RasterInfo* raster, SampleCount sc, 
			DepthInfo* depth, std::span<const uint> dynamic_states) :
			_d(d),
			_pll(pll)
		{
			_type = PipelineGraphics;

#if defined(FLAME_VULKAN)
			std::vector<VkPipelineShaderStageCreateInfo> vk_stage_infos;
			std::vector<VkVertexInputAttributeDescription> vk_vi_attributes;
			std::vector<VkVertexInputBindingDescription> vk_vi_bindings;
			std::vector<VkPipelineColorBlendAttachmentState> vk_blend_attachment_states;
			std::vector<VkDynamicState> vk_dynamic_states;

			vk_stage_infos.resize(shaders.size());
			for (auto i = 0; i < shaders.size(); i++)
			{
				auto& src = shaders[i];
				auto& dst = vk_stage_infos[i];
				dst.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				dst.flags = 0;
				dst.pNext = nullptr;
				dst.pSpecializationInfo = nullptr;
				dst.pName = "main";
				dst.stage = to_backend(src.s->_type);
				dst.module = src.m;
			}

			_shaders.resize(shaders.size());
			for (auto i = 0; i < shaders.size(); i++)
				_shaders[i] = std::move(shaders[i]);

			if (vi)
			{
				auto attribute_location = 0;
				vk_vi_bindings.resize(vi->buffers_count);
				for (auto i = 0; i < vk_vi_bindings.size(); i++)
				{
					auto& src = vi->buffers[i];
					auto& dst = vk_vi_bindings[i];
					dst.binding = i;
					for (auto j = 0; j < src.attributes_count; j++)
					{
						auto& _src = src.attributes[j];
						VkVertexInputAttributeDescription _dst;
						_dst.location = attribute_location++;
						_dst.binding = i;
						_dst.offset = dst.stride;
						dst.stride += format_size(_src.format);
						_dst.format = to_backend(_src.format);
						vk_vi_attributes.push_back(_dst);
					}
					dst.inputRate = to_backend(src.rate);
				}
			}

			VkPipelineVertexInputStateCreateInfo vertex_input_state;
			vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertex_input_state.pNext = nullptr;
			vertex_input_state.flags = 0;
			vertex_input_state.vertexBindingDescriptionCount = vk_vi_bindings.size();
			vertex_input_state.pVertexBindingDescriptions = vk_vi_bindings.empty() ? nullptr : vk_vi_bindings.data();
			vertex_input_state.vertexAttributeDescriptionCount = vk_vi_attributes.size();
			vertex_input_state.pVertexAttributeDescriptions = vk_vi_attributes.empty() ? nullptr : vk_vi_attributes.data();

			VkPipelineInputAssemblyStateCreateInfo assembly_state;
			assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			assembly_state.flags = 0;
			assembly_state.pNext = nullptr;
			assembly_state.topology = to_backend(vi ? vi->primitive_topology : PrimitiveTopologyTriangleList);
			assembly_state.primitiveRestartEnable = VK_FALSE;

			VkPipelineTessellationStateCreateInfo tess_state;
			tess_state.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			tess_state.pNext = nullptr;
			tess_state.flags = 0;
			tess_state.patchControlPoints = vi ? vi->patch_control_points : 0;

			VkViewport viewport;
			viewport.width = (float)vp.x();
			viewport.height = (float)vp.y();
			viewport.minDepth = (float)0.0f;
			viewport.maxDepth = (float)1.0f;
			viewport.x = 0;
			viewport.y = 0;

			VkRect2D scissor;
			scissor.extent.width = vp.x();
			scissor.extent.height = vp.y();
			scissor.offset.x = 0;
			scissor.offset.y = 0;

			VkPipelineViewportStateCreateInfo viewport_state;
			viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewport_state.pNext = nullptr;
			viewport_state.flags = 0;
			viewport_state.viewportCount = 1;
			viewport_state.scissorCount = 1;
			viewport_state.pScissors = &scissor;
			viewport_state.pViewports = &viewport;

			VkPipelineRasterizationStateCreateInfo raster_state;
			raster_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			raster_state.pNext = nullptr;
			raster_state.flags = 0;
			raster_state.depthClampEnable = raster ? raster->depth_clamp : false;
			raster_state.rasterizerDiscardEnable = VK_FALSE;
			raster_state.polygonMode = to_backend(raster ? raster->polygon_mode : PolygonModeFill);
			raster_state.cullMode = to_backend(raster ? raster->cull_mode : CullModeNone);
			raster_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			raster_state.depthBiasEnable = VK_FALSE;
			raster_state.depthBiasConstantFactor = 0.f;
			raster_state.depthBiasClamp = 0.f;
			raster_state.depthBiasSlopeFactor = 0.f;
			raster_state.lineWidth = 1.f;

			VkPipelineMultisampleStateCreateInfo multisample_state;
			multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisample_state.flags = 0;
			multisample_state.pNext = nullptr;
			multisample_state.rasterizationSamples = to_backend(sc);
			multisample_state.sampleShadingEnable = VK_FALSE;
			multisample_state.minSampleShading = 0.f;
			multisample_state.pSampleMask = nullptr;
			multisample_state.alphaToCoverageEnable = VK_FALSE;
			multisample_state.alphaToOneEnable = VK_FALSE;

			VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
			depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depth_stencil_state.flags = 0;
			depth_stencil_state.pNext = nullptr;
			depth_stencil_state.depthTestEnable = depth ? depth->test : false;
			depth_stencil_state.depthWriteEnable = depth ? depth->write : false;
			depth_stencil_state.depthCompareOp = to_backend(depth ? depth->compare_op : CompareOpLess);
			depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
			depth_stencil_state.minDepthBounds = 0;
			depth_stencil_state.maxDepthBounds = 0;
			depth_stencil_state.stencilTestEnable = VK_FALSE;
			depth_stencil_state.front = {};
			depth_stencil_state.back = {};

			vk_blend_attachment_states.resize(rp->_subpasses[subpass_idx]->_color_attachments.size());
			for (auto& a : vk_blend_attachment_states)
			{
				memset(&a, 0, sizeof(a));
				a.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			}
			if (_shaders.back().s->_type == ShaderStageFrag)
			{
				auto& bos = _shaders.back().r._blend_options;
				for (auto i = 0; i < bos.size(); i++)
				{
					const auto& src = bos[i];
					auto& dst = vk_blend_attachment_states[i];
					dst.blendEnable = src.enable;
					dst.srcColorBlendFactor = to_backend(src.src_color);
					dst.dstColorBlendFactor = to_backend(src.dst_color);
					dst.colorBlendOp = VK_BLEND_OP_ADD;
					dst.srcAlphaBlendFactor = to_backend(src.src_alpha);
					dst.dstAlphaBlendFactor = to_backend(src.dst_alpha);
					dst.alphaBlendOp = VK_BLEND_OP_ADD;
					dst.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				}
			}

			VkPipelineColorBlendStateCreateInfo blend_state;
			blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blend_state.flags = 0;
			blend_state.pNext = nullptr;
			blend_state.blendConstants[0] = 0.f;
			blend_state.blendConstants[1] = 0.f;
			blend_state.blendConstants[2] = 0.f;
			blend_state.blendConstants[3] = 0.f;
			blend_state.logicOpEnable = VK_FALSE;
			blend_state.logicOp = VK_LOGIC_OP_COPY;
			blend_state.attachmentCount = vk_blend_attachment_states.size();
			blend_state.pAttachments = vk_blend_attachment_states.data();

			for (auto i = 0; i < dynamic_states.size(); i++)
				vk_dynamic_states.push_back(to_backend((DynamicState)dynamic_states[i]));
			if (vp.x() == 0 && vp.y() == 0)
			{
				if (std::find(vk_dynamic_states.begin(), vk_dynamic_states.end(), VK_DYNAMIC_STATE_VIEWPORT) == vk_dynamic_states.end())
					vk_dynamic_states.push_back(VK_DYNAMIC_STATE_VIEWPORT);
				if (std::find(vk_dynamic_states.begin(), vk_dynamic_states.end(), VK_DYNAMIC_STATE_SCISSOR) == vk_dynamic_states.end())
					vk_dynamic_states.push_back(VK_DYNAMIC_STATE_SCISSOR);
			}

			VkPipelineDynamicStateCreateInfo dynamic_state;
			dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamic_state.pNext = nullptr;
			dynamic_state.flags = 0;
			dynamic_state.dynamicStateCount = vk_dynamic_states.size();
			dynamic_state.pDynamicStates = vk_dynamic_states.data();

			VkGraphicsPipelineCreateInfo pipeline_info;
			pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipeline_info.pNext = nullptr;
			pipeline_info.flags = 0;
			pipeline_info.stageCount = vk_stage_infos.size();
			pipeline_info.pStages = vk_stage_infos.data();
			pipeline_info.pVertexInputState = &vertex_input_state;
			pipeline_info.pInputAssemblyState = &assembly_state;
			pipeline_info.pTessellationState = tess_state.patchControlPoints > 0 ? &tess_state : nullptr;
			pipeline_info.pViewportState = &viewport_state;
			pipeline_info.pRasterizationState = &raster_state;
			pipeline_info.pMultisampleState = &multisample_state;
			pipeline_info.pDepthStencilState = &depth_stencil_state;
			pipeline_info.pColorBlendState = &blend_state;
			pipeline_info.pDynamicState = vk_dynamic_states.size() ? &dynamic_state : nullptr;
			pipeline_info.layout = pll->_v;
			pipeline_info.renderPass = rp ? rp->_v : nullptr;
			pipeline_info.subpass = subpass_idx;
			pipeline_info.basePipelineHandle = 0;
			pipeline_info.basePipelineIndex = 0;

			chk_res(vkCreateGraphicsPipelines(d->_v, 0, 1, &pipeline_info, nullptr, &_v));
#elif defined(FLAME_D3D12)

#endif
		}

		PipelinePrivate::PipelinePrivate(DevicePrivate* d, CompiledShader& compute_shader, PipelinelayoutPrivate* pll) :
			_d(d),
			_pll(pll)
		{
			_type = PipelineCompute;

#if defined(FLAME_VULKAN)
			_shaders.resize(1);
			_shaders[0] = std::move(compute_shader);

			VkComputePipelineCreateInfo pipeline_info;
			pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipeline_info.pNext = nullptr;
			pipeline_info.flags = 0;

			pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			pipeline_info.stage.flags = 0;
			pipeline_info.stage.pNext = nullptr;
			pipeline_info.stage.pSpecializationInfo = nullptr;
			pipeline_info.stage.pName = "main";
			pipeline_info.stage.stage = to_backend(ShaderStageComp);
			pipeline_info.stage.module = compute_shader.m;

			pipeline_info.basePipelineHandle = 0;
			pipeline_info.basePipelineIndex = 0;
			pipeline_info.layout = pll->_v;

			chk_res(vkCreateComputePipelines(_d->_v, 0, 1, &pipeline_info, nullptr, &_v));
#elif defined(FLAME_D3D12)

#endif
		}

		PipelinePrivate::~PipelinePrivate()
		{
#if defined(FLAME_VULKAN)
			for (auto& s : _shaders)
				vkDestroyShaderModule(_d->_v, s.m, nullptr);

			vkDestroyPipeline(_d->_v, _v, nullptr);
#elif defined(FLAME_D3D12)

#endif
		}

		PipelinePrivate* PipelinePrivate::_create(DevicePrivate* d, const std::filesystem::path& shader_dir, std::span<ShaderPrivate*> shaders,
			PipelinelayoutPrivate* pll, Renderpass* rp, uint subpass_idx, VertexInfo* vi, const Vec2u& vp, RasterInfo* raster, SampleCount sc, 
			DepthInfo* depth, std::span<const uint> dynamic_states)
		{
			auto has_vert_stage = false;
			auto tess_stage_count = 0;
			for (auto& s : shaders)
			{
				for (auto& ss : shaders)
				{
					if (ss != s && (ss->_filename == s->_filename || ss->_type == s->_type))
						return nullptr;
				}
				if (s->_type == ShaderStageComp)
					return nullptr;
				if (s->_type == ShaderStageVert)
					has_vert_stage = true;
				if (s->_type == ShaderStageTesc || s->_type == ShaderStageTese)
					tess_stage_count++;
			}
			if (!has_vert_stage || (tess_stage_count != 0 && tess_stage_count != 2))
				return nullptr;

			std::vector<CompiledShader> ss;
			ss.resize(shaders.size());
			for (auto i = 0; i < shaders.size(); i++)
				ss[i].s = shaders[i];
			std::sort(ss.begin(), ss.end(), [](const auto& a, const auto& b) {
				return (int)a.s->_type < (int)b.s->_type;
			});
			if (!compile_shaders(d, shader_dir, ss, pll, vi))
				return nullptr;

			return new PipelinePrivate(d, ss, pll, (RenderpassPrivate*)rp, subpass_idx, vi, vp, raster, sc, depth, dynamic_states);
		}

		PipelinePrivate* PipelinePrivate::_create(DevicePrivate* d, const std::filesystem::path& shader_dir, ShaderPrivate* compute_shader, PipelinelayoutPrivate* pll)
		{
			if (compute_shader->_type != ShaderStageComp)
				return nullptr;

			CompiledShader s;
			s.s = compute_shader;
			if (!compile_shaders(d, shader_dir, { &s, 1 }, pll, nullptr))
				return nullptr;

			return new PipelinePrivate(d, s, pll);
		}

		Pipeline* create(Device* d, const wchar_t* shader_dir, uint shaders_count,
			Shader* const* shaders, Pipelinelayout* pll, Renderpass* rp, uint subpass_idx,
			VertexInfo* vi, const Vec2u& vp, RasterInfo* raster, SampleCount sc, DepthInfo* depth,
			uint dynamic_states_count, const uint* dynamic_states)
		{
			return PipelinePrivate::_create((DevicePrivate*)d, shader_dir, { (ShaderPrivate**)shaders, shaders_count }, (PipelinelayoutPrivate*)pll, rp, subpass_idx, vi, vp, raster, sc, depth, { dynamic_states , dynamic_states_count });
		}
		Pipeline* create(Device* d, const wchar_t* shader_dir, Shader* compute_shader, Pipelinelayout* pll)
		{
			return PipelinePrivate::_create((DevicePrivate*)d, shader_dir, (ShaderPrivate*)compute_shader, (PipelinelayoutPrivate*)pll);
		}
	}
}

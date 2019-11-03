#include <flame/foundation/bitmap.h>
#include <flame/foundation/blueprint.h>
#include <flame/graphics/image.h>
#include <flame/graphics/font.h>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftlcdfil.h>
#include FT_OUTLINE_H

#include <msdfgen.h>
#include <msdfgen-ext.h>

namespace flame
{
	namespace graphics
	{
		FT_Library ft_library = 0;

		void get_latin_code_range(wchar_t& out_begin, wchar_t& out_end)
		{
			out_begin = 0x20;
			out_end = 0xff;
		}

		struct FontPrivate : Font
		{
			std::pair<std::unique_ptr<char[]>, long long> font_file;
			FT_Face ft_face;

			FontPrivate(const std::wstring& filename)
			{
				if (!ft_library)
				{
					FT_Init_FreeType(&ft_library);
					FT_Library_SetLcdFilter(ft_library, FT_LCD_FILTER_DEFAULT);
				}

				font_file = get_file_content(filename);
				FT_New_Memory_Face(ft_library, (uchar*)font_file.first.get(), font_file.second, 0, &ft_face);
			}

			~FontPrivate()
			{
				FT_Done_Face(ft_face);
			}
		};

		Font* Font::create(const std::wstring& filename)
		{
			return new FontPrivate(filename);
		}

		void Font::destroy(Font* f)
		{
			delete (FontPrivate*)f;
		}

		struct Font$
		{
			AttributeV<std::wstring> filename$i;

			AttributeP<void> out$o;

			FLAME_GRAPHICS_EXPORTS Font$()
			{
			}

			FLAME_GRAPHICS_EXPORTS void update$()
			{
				if (filename$i.frame > out$o.frame)
				{
					if (out$o.v)
						Font::destroy((Font*)out$o.v);
					if (std::filesystem::exists(filename$i.v))
						out$o.v = Font::create(filename$i.v);
					else
						printf("cannot create font\n");
					out$o.frame = filename$i.frame;
				}
			}

			FLAME_GRAPHICS_EXPORTS ~Font$()
			{
				if (out$o.v)
					Font::destroy((Font*)out$o.v);
			}
		};

		struct FontAtlasPrivate : FontAtlas
		{
			std::vector<FontPrivate*> fonts;

			std::unordered_map<uint, std::unique_ptr<Glyph>> map;
			std::unique_ptr<BinPackNode> bin_pack_root;

			Image* image;
			Imageview* imageview;

			FontAtlasPrivate(Device* d, FontDrawType$ _draw_type, const std::vector<void*>& _fonts)
			{
				for (auto f : _fonts)
					fonts.push_back((FontPrivate*)f);

				draw_type = _draw_type;

				bin_pack_root.reset(new BinPackNode(font_atlas_size));

				image = Image::create(d, draw_type == FontDrawPixel ? Format_R8_UNORM : Format_R8G8B8A8_UNORM, font_atlas_size, 1, 1, SampleCount_1, ImageUsage$(ImageUsageSampled | ImageUsageTransferDst));
				image->init(Vec4c(0, 0, 0, 255));
				if (draw_type == FontDrawPixel)
					imageview = Imageview::create(image, Imageview2D, 0, 1, 0, 1, SwizzleOne, SwizzleOne, SwizzleOne, SwizzleR);
				else
					imageview = Imageview::create(image);
			}

			~FontAtlasPrivate()
			{
				Imageview::destroy(imageview);
				Image::destroy(image);
			}

			Glyph* get_glyph(wchar_t unicode, uint font_size)
			{
				if (draw_type == FontDrawSdf)
					font_size = sdf_font_size;
				auto hash = hash_update(unicode, font_size);

				if (!map[hash])
				{
					auto g = new Glyph;
					g->unicode = unicode;
					g->size = 0;
					g->off = 0;
					g->advance = 0;
					g->uv0 = Vec2f(0.f);
					g->uv1 = Vec2f(0.f);
					map[hash].reset(g);

					for (auto font : fonts)
					{
						auto ft_face = font->ft_face;
						auto glyph_index = FT_Get_Char_Index(ft_face, unicode);
						if (glyph_index == 0)
							continue;

						FT_Size_RequestRec ft_req = {};
						ft_req.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
						ft_req.height = font_size * 64;
						FT_Request_Size(ft_face, &ft_req);
						auto ascender = ft_face->size->metrics.ascender / 64;

						FT_Load_Glyph(ft_face, glyph_index, draw_type == FontDrawLcd ? FT_LOAD_TARGET_LCD : FT_LOAD_DEFAULT);

						auto ft_glyph = ft_face->glyph;
						auto size = Vec2u(ft_glyph->bitmap.width, ft_glyph->bitmap.rows);
						if (draw_type == FontDrawLcd)
							size.x() /= 3;
						g->size = size;
						g->off = Vec2u(ft_glyph->bitmap_left, ascender + g->size.y() - ft_glyph->metrics.horiBearingY / 64.f);
						g->advance = ft_glyph->advance.x / 64;

						if (size > 0U)
						{
							auto n = bin_pack_root->find(size + (draw_type == FontDrawSdf ? sdf_range * 2 : 0));
							if (n)
							{
								auto& atlas_pos = n->pos;

								switch (draw_type)
								{
								case FontDrawPixel:
								{
									FT_Render_Glyph(ft_glyph, FT_RENDER_MODE_NORMAL);

									auto pitch_ft = ft_glyph->bitmap.pitch;
									auto pitch_temp = size.x();
									auto temp = new uchar[pitch_temp * size.y()];
									for (auto y = 0; y < size.y(); y++)
									{
										for (auto x = 0; x < size.x(); x++)
											temp[y * pitch_temp + x] = ft_glyph->bitmap.buffer[y * pitch_ft + x];
									}

									image->set_pixels(atlas_pos, size, temp);

									delete[] temp;
								}
									break;
								case FontDrawLcd:
								{
									FT_Render_Glyph(ft_glyph, FT_RENDER_MODE_LCD);

									auto pitch_ft = ft_glyph->bitmap.pitch;
									auto pitch_temp = size.x() * 4;
									auto temp = new uchar[pitch_temp * size.y()];
									for (auto y = 0; y < size.y(); y++)
									{
										for (auto x = 0; x < size.x(); x++)
										{
											temp[y * pitch_temp + x * 4 + 0] = ft_glyph->bitmap.buffer[y * pitch_ft + x * 3 + 0];
											temp[y * pitch_temp + x * 4 + 1] = ft_glyph->bitmap.buffer[y * pitch_ft + x * 3 + 1];
											temp[y * pitch_temp + x * 4 + 2] = ft_glyph->bitmap.buffer[y * pitch_ft + x * 3 + 2];
											temp[y * pitch_temp + x * 4 + 3] = 255;
										}
									}

									image->set_pixels(atlas_pos, size, temp);

									delete[] temp;
								}
									break;
								case FontDrawSdf:
								{
									void* ptr = ft_face;

									msdfgen::Shape shape;
									msdfgen::loadGlyph(shape, (msdfgen::FontHandle*) & ptr, unicode);

									size += sdf_range * 2;

									shape.normalize();
									msdfgen::edgeColoringSimple(shape, 3.f);
									msdfgen::Bitmap<float, 3> bmp(size.x(), size.y());
									msdfgen::generateMSDF(bmp, shape, sdf_range, 1.f, msdfgen::Vector2(-g->off.x(), g->off.y() - ascender) + sdf_range);

									auto pitch_temp = size.x() * 4;
									auto temp = new uchar[pitch_temp * size.y()];
									for (auto y = 0; y < size.y(); y++)
									{
										for (auto x = 0; x < size.x(); x++)
										{
											auto src = bmp(x, y);
											temp[y * pitch_temp + x * 4 + 0] = clamp(src[0] * 255.f, 0.f, 255.f);
											temp[y * pitch_temp + x * 4 + 1] = clamp(src[1] * 255.f, 0.f, 255.f);
											temp[y * pitch_temp + x * 4 + 2] = clamp(src[2] * 255.f, 0.f, 255.f);
											temp[y * pitch_temp + x * 4 + 3] = 255.f;
										}
									}

									image->set_pixels(atlas_pos, size, temp);

									delete[] temp;
								}
									break;
								}

								if (draw_type == FontDrawSdf)
								{
									g->uv0 = (Vec2f(atlas_pos.x(), atlas_pos.y()) + (float)sdf_range) / image->size;
									g->uv1 = (Vec2f(atlas_pos.x() + size.x(), atlas_pos.y() + size.y()) - (float)sdf_range) / image->size;
								}
								else
								{
									g->uv0 = Vec2f(atlas_pos.x(), atlas_pos.y() + size.y()) / image->size;
									g->uv1 = Vec2f(atlas_pos.x() + size.x(), atlas_pos.y()) / image->size;
								}
							}
						}

						break;
					}
				}

				return map[hash].get();
			}

			Vec2u get_text_offset(const std::wstring_view& text, uint font_size)
			{
				auto w = 0U;
				auto line_space = draw_type == FontDrawSdf ? sdf_font_size : font_size;
				auto h = 0U;
				for (auto ch : text)
				{
					if (!ch)
						break;
					if (ch == '\n')
					{
						w = 0.f;
						h += font_size;
					}
					else if (ch != '\r' && ch != '\t')
						w += get_glyph(ch, font_size)->advance;
				}
				return Vec2u(w, h);
			}

			Vec2u get_text_size(const std::wstring_view& text, uint font_size)
			{
				auto w = 0U;
				auto line_space = draw_type == FontDrawSdf ? sdf_font_size : font_size;
				auto h = line_space;
				auto lw = 0U;
				for (auto ch : text)
				{
					if (ch == '\n')
					{
						h += line_space;
						lw = 0.f;
					}
					else if (ch != '\r')
					{
						if (ch == '\t')
							ch = ' ';
						lw += get_glyph(ch, font_size)->advance;
						if (lw > w)
							w = lw;
					}
				}
				return Vec2u(w, h);
			}

			Mail<std::wstring> slice_text_by_width(const std::wstring_view& text, uint font_size, uint width)
			{
				assert(width > font_size);

				auto ret = new_mail<std::wstring>();
				auto w = 0;
				for (auto ch : text)
				{
					if (!ch)
						break;
					switch (ch)
					{
					case '\n':
						w = 0;
						*ret.p += '\n';
						break;
					case '\r':
						break;
					case '\t':
						ch = ' ';
					default:
						auto adv = get_glyph(ch, font_size)->advance;
						if (w + adv >= width)
						{
							w = adv;
							*ret.p += '\n';
						}
						else
							w += adv;
						*ret.p += ch;
					}
				}
				return ret;
			}
		};

		FontAtlas* FontAtlas::create(Device* d, FontDrawType$ draw_type, const std::vector<void*>& fonts)
		{
			return new FontAtlasPrivate(d, draw_type, fonts);
		}

		void FontAtlas::destroy(FontAtlas* f)
		{
			delete (FontAtlasPrivate*)f;
		}

		Glyph* FontAtlas::get_glyph(wchar_t unicode, uint font_size)
		{
			return ((FontAtlasPrivate*)this)->get_glyph(unicode, font_size);
		}

		Vec2u FontAtlas::get_text_offset(const std::wstring_view& text, uint font_size)
		{
			return ((FontAtlasPrivate*)this)->get_text_offset(text, font_size);
		}

		Vec2u FontAtlas::get_text_size(const std::wstring_view& text, uint font_size)
		{
			return ((FontAtlasPrivate*)this)->get_text_size(text, font_size);
		}

		Mail<std::wstring> FontAtlas::slice_text_by_width(const std::wstring_view& text, uint font_size, uint width)
		{
			return ((FontAtlasPrivate*)this)->slice_text_by_width(text, font_size, width);
		}

		Image* FontAtlas::image() const
		{
			return ((FontAtlasPrivate*)this)->image;
		}

		Imageview* FontAtlas::imageview() const
		{
			return ((FontAtlasPrivate*)this)->imageview;
		}

		struct FontAtlas$
		{
			AttributeE<FontDrawType$> draw_type$i;
			AttributeP<std::vector<void*>> fonts$i;

			AttributeP<void> out$o;

			FLAME_GRAPHICS_EXPORTS void update$()
			{
				if (draw_type$i.frame > out$o.frame || fonts$i.frame > out$o.frame)
				{
					if (out$o.v)
						FontAtlas::destroy((FontAtlas*)out$o.v);
					auto d = (Device*)bp_env().graphics_device;
					auto fonts = get_attribute_vec(fonts$i);
					if (d && !fonts.empty())
						out$o.v = FontAtlas::create(d, draw_type$i.v, fonts);
					else
						printf("cannot create fontatlas\n");
					out$o.frame = max(draw_type$i.frame, fonts$i.frame);
				}
			}

			FLAME_GRAPHICS_EXPORTS ~FontAtlas$()
			{
				if (out$o.v)
					FontAtlas::destroy((FontAtlas*)out$o.v);
			}
		};
	}
}


#include <flame/foundation/serialize.h>
#include <flame/foundation/window.h>
#include <flame/graphics/device.h>
#include <flame/graphics/synchronize.h>
#include <flame/graphics/renderpass.h>
#include <flame/graphics/swapchain.h>
#include <flame/graphics/commandbuffer.h>
#include <flame/graphics/image.h>
#include <flame/graphics/shader.h>
#include <flame/graphics/font.h>
#include <flame/graphics/canvas.h>
#include <flame/universe/entity.h>
#include <flame/universe/default_style.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/text.h>
#include <flame/universe/components/event_dispatcher.h>
#include <flame/universe/components/event_receiver.h>
#include <flame/universe/components/aligner.h>
#include <flame/universe/components/layout.h>
#include <flame/universe/components/style.h>
#include <flame/universe/components/checkbox.h>
#include <flame/universe/components/toggle.h>
#include <flame/universe/components/image.h>
#include <flame/universe/components/edit.h>

using namespace flame;
using namespace graphics;

const auto img_id = 9;

struct App
{
	Window* w;
	Device* d;
	Semaphore* render_finished;
	SwapchainResizable* scr;
	Fence* fence;
	std::vector<Commandbuffer*> cbs;

	FontAtlas* font_atlas_pixel;
	FontAtlas* font_atlas_lcd;
	FontAtlas* font_atlas_sdf;
	Canvas* canvas;
	int rt_frame;

	Entity* root;
	cElement* c_element_root;
	cText* c_text_fps;

	void run()
	{
		auto sc = scr->sc();
		auto sc_frame = scr->sc_frame();

		if (sc_frame > rt_frame)
		{
			canvas->set_render_target(TargetImages, sc ? &sc->images() : nullptr);
			rt_frame = sc_frame;
		}

		if (sc)
		{
			sc->acquire_image();
			fence->wait();

			c_element_root->width = w->size.x();
			c_element_root->height = w->size.y();
			c_text_fps->set_text(std::to_wstring(app_fps()));
			root->update();

			auto img_idx = sc->image_index();
			auto cb = cbs[img_idx];
			canvas->record(cb, img_idx);

			d->gq->submit(cb, sc->image_avalible(), render_finished, fence);
			d->gq->present(sc, render_finished);
		}
	}
}app;

int main(int argc, char** args)
{
	typeinfo_load(L"flame_graphics.typeinfo");
	
	app.w = Window::create("UI Test", Vec2u(1280, 720), WindowFrame | WindowResizable);
	app.d = Device::create(true);
	app.render_finished = Semaphore::create(app.d);
	app.scr = SwapchainResizable::create(app.d, app.w);
	app.fence = Fence::create(app.d);
	auto sc = app.scr->sc();
	app.canvas = Canvas::create(app.d, TargetImages, &sc->images());
	app.cbs.resize(sc->images().size());
	for (auto i = 0; i < app.cbs.size(); i++)
		app.cbs[i] = Commandbuffer::create(app.d->gcp);

	auto font1 = Font::create(L"c:/windows/fonts/msyh.ttc", 14);
	auto font2 = Font::create(L"c:/windows/fonts/msyh.ttc", 32);
	app.font_atlas_pixel = FontAtlas::create(app.d, FontDrawPixel, { font1 });
	app.font_atlas_lcd = FontAtlas::create(app.d, FontDrawLcd, { font1 });
	app.font_atlas_sdf = FontAtlas::create(app.d, FontDrawSdf, { font2 });
	app.font_atlas_pixel->index = 1;
	app.font_atlas_lcd->index = 2;
	app.font_atlas_sdf->index = 3;
	app.canvas->set_image(app.font_atlas_pixel->index, Imageview::create(app.font_atlas_pixel->image(), Imageview2D, 0, 1, 0, 1, SwizzleOne, SwizzleOne, SwizzleOne, SwizzleR));
	app.canvas->set_image(app.font_atlas_lcd->index, Imageview::create(app.font_atlas_lcd->image()));
	app.canvas->set_image(app.font_atlas_sdf->index, Imageview::create(app.font_atlas_sdf->image()));

	app.root = Entity::create();
	{
		app.c_element_root = cElement::create(app.root, app.canvas);

		cEventDispatcher::create(app.root, app.w);

		cLayout::create(app.root);
	}

	auto e_fps = Entity::create();
	{
		cElement::create(e_fps);

		auto c_text = cText::create(e_fps, app.font_atlas_pixel);
		app.c_text_fps = c_text;

		auto c_aligner = cAligner::create(e_fps);
		c_aligner->x_align = AlignxLeft;
		c_aligner->y_align = AlignyBottom;
	}
	app.root->add_child(e_fps);

	app.canvas->set_image(img_id, Imageview::create(Image::create_from_file(app.d, L"../asset/ui/imgs/9.png")));

	auto e_layout_left = Entity::create();
	{
		auto c_element = cElement::create(e_layout_left);
		c_element->x = 16.f;
		c_element->y = 8.f;

		auto c_layout = cLayout::create(e_layout_left);
		c_layout->type = LayoutVertical;
		c_layout->item_padding = 8.f;

		cAligner::create(e_layout_left);
	}
	app.root->add_child(e_layout_left);

	auto e_text_pixel = Entity::create();
	{
		cElement::create(e_text_pixel);

		auto c_text = cText::create(e_text_pixel, app.font_atlas_pixel);
		c_text->set_text(L"Text Pixel");

		cAligner::create(e_text_pixel);
	}
	e_layout_left->add_child(e_text_pixel);

	auto e_text_lcd = Entity::create();
	{
		cElement::create(e_text_lcd);

		auto c_text = cText::create(e_text_lcd, app.font_atlas_lcd);
		c_text->set_text(L"Text Lcd");

		cAligner::create(e_text_lcd);
	}
	e_layout_left->add_child(e_text_lcd);

	auto e_text_sdf = Entity::create();
	{
		cElement::create(e_text_sdf);

		auto c_text = cText::create(e_text_sdf, app.font_atlas_sdf);
		c_text->set_text(L"Text Sdf");
		c_text->sdf_scale = 14.f / 32.f;

		cAligner::create(e_text_sdf);
	}
	e_layout_left->add_child(e_text_sdf);

	auto e_button = Entity::create();
	{
		auto c_element = cElement::create(e_button);
		c_element->inner_padding = Vec4f(4.f, 2.f, 4.f, 2.f);

		auto c_text = cText::create(e_button, app.font_atlas_pixel);
		c_text->set_text(L"Click Me!");

		auto c_event_receiver = cEventReceiver::create(e_button);
		c_event_receiver->add_mouse_listener([](void* c, KeyState action, MouseKey key, const Vec2f& pos) {
			if (is_mouse_clicked(action, key))
			{
				auto thiz = *(App * *)c;
				printf("thank you for clicking me\n");
			}
		}, new_mail_p(&app));

		cStyleBgCol::create(e_button, default_style.button_color_normal, default_style.button_color_hovering, default_style.button_color_active);

		cAligner::create(e_button);
	}
	e_layout_left->add_child(e_button);

	auto e_checkbox = Entity::create();
	{
		auto c_element = cElement::create(e_checkbox);
		c_element->width = 16.f;
		c_element->height = 16.f;
		c_element->inner_padding = Vec4f(20.f, 1.f, 1.f, 1.f);
		c_element->draw = false;

		auto c_text = cText::create(e_checkbox, app.font_atlas_pixel);
		c_text->set_text(L"Checkbox");

		auto c_event_receiver = cEventReceiver::create(e_checkbox);

		cStyleBgCol::create(e_checkbox, default_style.frame_color_normal, default_style.frame_color_hovering, default_style.frame_color_active);

		cCheckbox::create(e_checkbox);

		cAligner::create(e_checkbox);
	}
	e_layout_left->add_child(e_checkbox);

	auto e_toggle = Entity::create();
	{
		auto c_element = cElement::create(e_toggle);
		c_element->background_round_flags = SideNW | SideNE | SideSW | SideSE;
		c_element->background_round_radius = app.font_atlas_pixel->pixel_height * 0.5f;
		c_element->background_offset = Vec4f(c_element->background_round_radius, 2.f, c_element->background_round_radius, 2.f);

		auto c_text = cText::create(e_toggle, app.font_atlas_pixel);
		c_text->set_text(L"Toggle");

		cEventReceiver::create(e_toggle);

		cStyleBgCol::create(e_toggle, Vec4c(0), Vec4c(0), Vec4c(0));

		cToggle::create(e_toggle);

		cAligner::create(e_toggle);
	}
	e_layout_left->add_child(e_toggle);

	auto e_image = Entity::create();
	{
		auto c_element = cElement::create(e_image);
		c_element->width = 258.f;
		c_element->height = 258.f;
		c_element->inner_padding = Vec4f(4.f);
		c_element->background_frame_color = Vec4c(10, 200, 10, 255);
		c_element->background_frame_thickness = 2.f;

		auto c_image = cImage::create(e_image);
		c_image->id = img_id;

		cAligner::create(e_image);
	}
	e_layout_left->add_child(e_image);

	auto e_edit = Entity::create();
	{
		auto c_element = cElement::create(e_edit);
		c_element->width = 108.f;
		c_element->height = app.font_atlas_pixel->pixel_height + 4;
		c_element->inner_padding = Vec4f(4.f, 2.f, 4.f, 2.f);
		c_element->background_color = default_style.frame_color_normal;

		auto c_text = cText::create(e_edit, app.font_atlas_pixel);
		c_text->set_text(L"���ֱ༭");
		c_text->auto_size = false;

		cEventReceiver::create(e_edit);

		auto c_edit = cEdit::create(e_edit);
		c_edit->cursor = 2;

		cAligner::create(e_edit);
	}
	e_layout_left->add_child(e_edit);

	auto e_layout_right = Entity::create();
	{
		auto c_element = cElement::create(e_layout_right);
		c_element->x = 416.f;
		c_element->y = 8.f;

		auto c_layout = cLayout::create(e_layout_right);
		c_layout->type = LayoutVertical;
		c_layout->item_padding = 8.f;

		cAligner::create(e_layout_right);
	}
	app.root->add_child(e_layout_right);

	auto e_list = Entity::create();
	{
		auto c_element = cElement::create(e_list);
		c_element->width = 108.f;
		c_element->inner_padding = Vec4f(4.f);
		c_element->background_frame_color = Vec4c(1255);
		c_element->background_frame_thickness = 2.f;

		auto c_layout = cLayout::create(e_list);
		c_layout->type = LayoutVertical;
		c_layout->item_padding = 4.f;
		c_layout->width_fit_children = false;

		cAligner::create(e_list);
	}
	e_layout_right->add_child(e_list);

	for (auto i = 0; i < 3; i++)
	{
		auto e_item = Entity::create();
		{
			cElement::create(e_item);

			auto c_text = cText::create(e_item, app.font_atlas_pixel);
			c_text->set_text(L"item" + std::to_wstring(i));

			cEventReceiver::create(e_item);

			cStyleBgCol::create(e_item, default_style.header_color_normal, default_style.header_color_hovering, default_style.header_color_active);

			auto c_aligner = cAligner::create(e_item);
			c_aligner->width_policy = SizeGreedy;
		}
		e_list->add_child(e_item);
	}

	//auto w_list = Element::createT<wList>(ui);
	//w_list->pos$ = Vec2f(800.f, 8.f);
	//w_list->size$ = Vec2f(300.f);

	//auto w_sizedrag = Element::createT<wSizeDrag>(ui, w_list);
	//w_sizedrag->min_size() = Vec2f(100.f);

	//w_list->add_child(w_sizedrag, 1);

	//for (auto i = 0; i < 20; i++)
	//{
	//	auto item = Element::createT<wListItem>(ui, font_atlas_index, (L"item " + to_stdwstring(i)).c_str());
	//	w_list->add_child(item);
	//}

	auto e_layout_menus = Entity::create();
	{
		cElement::create(e_layout_menus);

		auto c_layout = cLayout::create(e_layout_menus);
		c_layout->type = LayoutVertical;

		cAligner::create(e_layout_menus);
	}
	e_layout_right->add_child(e_layout_menus);

	//layout->add_child(w_list, 1);

	//auto w_menubar = Element::createT<wMenuBar>(ui);
	//w_menubar->align$ = AlignLittleEnd;

	//auto w_menu = Element::createT<wMenu>(ui, font_atlas_index, L"menu");

	//auto w_menuitem1 = Element::createT<wMenuItem>(ui, font_atlas_index, L"item 1");
	//auto w_menuitem2 = Element::createT<wMenuItem>(ui, font_atlas_index, L"item 2");
	//auto w_menuitem3 = Element::createT<wMenuItem>(ui, font_atlas_index, L"item 3");

	//w_menu->w_items()->add_child(w_menuitem1);
	//w_menu->w_items()->add_child(w_menuitem2);
	//w_menu->w_items()->add_child(w_menuitem3);

	//w_menubar->add_child(w_menu);

	//layout1->add_child(w_menubar);

	//auto w_combo = Element::createT<wCombo>(ui, font_atlas_index);
	//w_combo->align$ = AlignLittleEnd;

	//auto w_comboitem1 = Element::createT<wMenuItem>(ui, font_atlas_index, L"item 1");
	//auto w_comboitem2 = Element::createT<wMenuItem>(ui, font_atlas_index, L"item 2");
	//auto w_comboitem3 = Element::createT<wMenuItem>(ui, font_atlas_index, L"item 3");

	//w_combo->w_items()->add_child(w_comboitem1);
	//w_combo->w_items()->add_child(w_comboitem2);
	//w_combo->w_items()->add_child(w_comboitem3);

	//layout1->add_child(w_combo);

	//layout->add_child(layout1, 1);

	//auto w_treenode1 = Element::createT<wTreeNode>(ui, font_atlas_index, L"A");
	//w_treenode1->pos$ = Vec2f(800.f, 400.f);

	//auto w_treenode2 = Element::createT<wTreeNode>(ui, font_atlas_index, L"B");
	//auto w_treenode3 = Element::createT<wTreeNode>(ui, font_atlas_index, L"C");
	//auto w_treenode4 = Element::createT<wTreeNode>(ui, font_atlas_index, L"D");

	//w_treenode1->w_items()->add_child(w_treenode2);
	//w_treenode1->w_items()->add_child(w_treenode3);
	//w_treenode3->w_items()->add_child(w_treenode4);

	//layout->add_child(w_treenode1, 1);

	//layout = Element::createT<wLayout>(ui);
	//ui->root()->add_child(layout, 1);

	app_run([](void* c) {
		auto app = (*(App * *)c);
		app->run();
	}, new_mail_p(&app));

	return 0;
}

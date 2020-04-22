#include <flame/serialize.h>
#include <flame/graphics/image.h>
#include <flame/utils/app.h>
#include <flame/utils/fps.h>
#include <flame/universe/utils/reflector.h>

#include <flame/universe/utils/entity_impl.h>
#include <flame/universe/utils/ui_impl.h>
using namespace flame;
using namespace graphics;

const auto img_id = 9;

struct MyApp : App
{
	void create_widgets()
	{
		utils::push_parent(root);

			utils::e_begin_layout(LayoutVertical, 0.f, false, false);
			utils::c_aligner(AlignMinMax, AlignMinMax);
				utils::e_begin_menu_bar();
					utils::e_begin_menubar_menu(L"Style");
						utils::e_menu_item(L"Dark", [](void* c) {
							looper().add_event([](void*, bool*) {
								app.root->remove_children(0, -1);
								utils::style_set_to_dark();
								app.canvas->clear_color = Vec4f(utils::style_4c(BackgroundColor)) / 255.f;
								app.create_widgets();
							}, Mail());
						}, Mail());
						utils::e_menu_item(L"Light", [](void* c) {
							looper().add_event([](void*, bool*) {
								app.root->remove_children(0, -1);
								utils::style_set_to_light();
								app.canvas->clear_color = Vec4f(utils::style_4c(BackgroundColor)) / 255.f;
								app.create_widgets();
							}, Mail());
						}, Mail());
					utils::e_end_menubar_menu();
					utils::e_begin_menubar_menu(L"Window");
						utils::e_menu_item(L"Reflector", [](void* c) {
							utils::next_element_pos = Vec2f(100.f);
							utils::e_reflector_window();
						}, Mail());
					utils::e_end_menubar_menu();
				utils::e_end_menu_bar();

				utils::e_begin_layout();
				utils::c_aligner(AlignMinMax, AlignMinMax);
					utils::next_element_pos = Vec2f(16.f, 10.f);
					utils::e_begin_layout(LayoutVertical, 16.f);
						utils::e_text(L"Text");
						utils::next_entity = Entity::create();
						utils::e_button(L"Click Me!", [](void* c) {
							(*(Entity**)c)->get_component(cText)->set_text(L"Click Me! :)");
							printf("thank you for clicking me\n");
						}, Mail::from_p(utils::next_entity));
						utils::e_checkbox(L"Checkbox");
						utils::next_element_size = 258.f;
						utils::next_element_padding = 4.f;
						utils::next_element_frame_thickness = 2.f;
						utils::next_element_frame_color = utils::style_4c(ForegroundColor);
						utils::e_image(img_id << 16);
						utils::e_edit(100.f);
					utils::e_end_layout();

					utils::next_element_pos = Vec2f(416.f, 10.f);
					utils::e_begin_layout(LayoutVertical, 16.f);
						utils::next_element_size = Vec2f(200.f, 100.f);
						utils::next_element_padding = 4.f;
						utils::next_element_frame_thickness = 2.f;
						utils::next_element_frame_color = utils::style_4c(ForegroundColor);
						utils::e_begin_scrollbar(ScrollbarVertical, false);
							utils::e_begin_list(true);
							for (auto i = 0; i < 10; i++)
								utils::e_list_item((L"item" + std::to_wstring(i)).c_str());
							utils::e_end_list();
						utils::e_end_scrollbar();
						
						utils::next_element_padding = 4.f;
						utils::next_element_frame_thickness = 2.f;
						utils::next_element_frame_color = utils::style_4c(ForegroundColor);
						utils::e_begin_tree(false);
							utils::e_begin_tree_node(L"A");
								utils::e_tree_leaf(L"C");
								utils::e_tree_leaf(L"D");
							utils::e_end_tree_node();
							utils::e_tree_leaf(L"B");
						utils::e_end_tree();

						utils::e_begin_combobox();
							utils::e_combobox_item(L"Apple");
							utils::e_combobox_item(L"Boy");
							utils::e_combobox_item(L"Cat");
						utils::e_end_combobox();
					utils::e_end_layout();

				utils::e_end_layout();

			utils::e_end_layout();

			utils::next_element_pos = Vec2f(416.f, 300.f);
			utils::next_element_size = Vec2f(200.f, 200.f);
			utils::e_begin_docker_floating_container();
				utils::e_begin_docker();
					utils::e_begin_docker_page(L"ResourceExplorer").second->get_component(cElement)->color = utils::style_4c(FrameColorNormal);
						utils::e_text(L"flower.png  main.cpp");
					utils::e_end_docker_page();
				utils::e_end_docker();
			utils::e_end_docker_floating_container();

			utils::next_element_pos = Vec2f(640.f, 300.f);
			utils::next_element_size = Vec2f(200.f, 200.f);
			utils::e_begin_docker_floating_container();
				utils::e_begin_docker_layout(LayoutHorizontal);
					utils::e_begin_docker();
						utils::e_begin_docker_page(L"TextEditor").second->get_component(cElement)->color = utils::style_4c(FrameColorNormal);
							utils::e_text(L"printf(\"Hello World!\\n\");");
						utils::e_end_docker_page();
					utils::e_end_docker();
					utils::e_begin_docker_layout(LayoutVertical);
						utils::e_begin_docker();
							utils::e_begin_docker_page(L"Hierarchy").second->get_component(cElement)->color = utils::style_4c(FrameColorNormal);
								utils::e_text(L"Node A\n--Node B");
							utils::e_end_docker_page();
						utils::e_end_docker();
						utils::e_begin_docker();
							utils::e_begin_docker_page(L"Inspector").second->get_component(cElement)->color = utils::style_4c(FrameColorNormal);
								utils::e_text(L"Name: James Bond\nID: 007");
							utils::e_end_docker_page();
						utils::e_end_docker();
					utils::e_end_docker_layout();
				utils::e_end_docker_layout();
			utils::e_end_docker_floating_container();
			
			{
				auto menu = root->get_component(cMenu);
				if (menu)
					root->remove_component(menu);
			}
			utils::e_begin_popup_menu();
				utils::e_menu_item(L"Refresh", [](void*) {
					wprintf(L"%s!\n", cEventReceiver::current()->entity->get_component(cText)->text.v);
				}, Mail());
				utils::e_menu_item(L"Save", [](void*) {
					wprintf(L"%s!\n", cEventReceiver::current()->entity->get_component(cText)->text.v);
				}, Mail());
				utils::e_menu_item(L"Help", [](void*) {
					wprintf(L"%s!\n", cEventReceiver::current()->entity->get_component(cText)->text.v);
				}, Mail());
				utils::e_separator();
				utils::e_begin_sub_menu(L"Add");
					utils::e_menu_item(L"Tree", [](void*) {
						wprintf(L"Add %s!\n", cEventReceiver::current()->entity->get_component(cText)->text.v);
					}, Mail());
					utils::e_menu_item(L"Car", [](void*) {
						wprintf(L"Add %s!\n", cEventReceiver::current()->entity->get_component(cText)->text.v);
					}, Mail());
					utils::e_menu_item(L"House", [](void*) {
						wprintf(L"Add %s!\n", cEventReceiver::current()->entity->get_component(cText)->text.v);
					}, Mail());
				utils::e_end_sub_menu();
				utils::e_begin_sub_menu(L"Remove");
					utils::e_menu_item(L"Tree", [](void*) {
						wprintf(L"Remove %s!\n", cEventReceiver::current()->entity->get_component(cText)->text.v);
					}, Mail());
					utils::e_menu_item(L"Car", [](void*) {
						wprintf(L"Remove %s!\n", cEventReceiver::current()->entity->get_component(cText)->text.v);
					}, Mail());
					utils::e_menu_item(L"House", [](void*) {
						wprintf(L"Remove %s!\n", cEventReceiver::current()->entity->get_component(cText)->text.v);
					}, Mail());
				utils::e_end_sub_menu();
			utils::e_end_popup_menu();

		utils::pop_parent();
	}
}app;

int main(int argc, char** args)
{
	app.create("UI Test", Vec2u(1280, 720), WindowFrame | WindowResizable, true);

	app.canvas->set_resource(img_id, Imageview::create(Image::create_from_file(app.graphics_device, (app.engine_path / L"art/9.png").c_str())));

	app.create_widgets();

	looper().loop([](void*) {
		app.run();
	}, Mail());

	return 0;
}

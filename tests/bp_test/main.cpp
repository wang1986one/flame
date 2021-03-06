#include <flame/foundation/blueprint.h>
#include <flame/universe/ui/ui.h>
#include <flame/utils/app.h>
#include <flame/utils/fps.h>

using namespace flame;
using namespace graphics;

struct MyApp : App
{
	bpScene* bp;
}app;

struct MainForm : GraphicsWindow
{
	UI ui;

	MainForm();
};

MainForm* main_window = nullptr;

MainForm::MainForm() :
	GraphicsWindow(&app, true, true, "BP Test", Vec2u(1280, 720), WindowFrame | WindowResizable)
{
	main_window = this;

	setup_as_main_window();

	ui.init(world);

	ui.parents.push(main_window->root);

	ui.e_text(L"");
	ui.c_aligner(AlignMin, AlignMax);
	add_fps_listener([](Capture& c, uint fps) {
		c.thiz<cText>()->set_text(std::to_wstring(fps).c_str());
	}, Capture().set_thiz(ui.current_entity->get_component(cText)));

	ui.next_element_pos = 100.f;
	ui.next_element_size = 10.f;
	auto patch = ui.e_element()->get_component(cElement);
	patch->pivot = 0.5f;
	patch->color = Vec4c(255);
	//cElement::set_linked_object(patch);

	ui.parents.pop();
}

int main(int argc, char** args)
{
	app.create();

	new MainForm;

	app.bp = bpScene::create_from_file((app.resource_path / L"test.bp").c_str());
	auto r = app.bp->root;
	auto nr = r->add_node("", "flame::cElement", bpNodeRefRead);
	auto nw = r->add_node("", "flame::cElement", bpNodeRefWrite);
	auto nt = r->add_node("", "flame::R_Time");
	auto na = r->add_node("", "flame::R_Add");
	na->find_input("a")->link_to(nr->find_output("scale"));
	na->find_input("b")->link_to(nt->find_output("total"));
	nw->find_input("scale")->link_to(na->find_output("out"));

	get_looper()->loop([](Capture&) {
		app.bp->update();
		app.run();
	}, Capture());

	return 0;
}

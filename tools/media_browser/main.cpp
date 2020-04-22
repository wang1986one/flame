#include <flame/serialize.h>
#include <flame/graphics/image.h>
#include <flame/utils/app.h>

#include <flame/universe/utils/entity_impl.h>
#include <flame/universe/utils/ui_impl.h>

using namespace flame;
using namespace graphics;

struct MyApp : App
{
}app;

int main(int argc, char** args)
{
	app.create("Media Browser", Vec2u(1280, 720), WindowFrame | WindowResizable, true);

	utils::push_parent(app.root);
		utils::e_begin_splitter(SplitterHorizontal);
		utils::e_element()->get_component(cElement)->color = Vec4c(255, 0, 0, 255);
		utils::c_aligner(AlignMinMax, AlignMinMax);
		utils::e_element()->get_component(cElement)->color = Vec4c(0, 255, 0, 255);
		utils::c_aligner(AlignMinMax, AlignMinMax);
		utils::e_end_splitter();
	utils::pop_parent();

	looper().loop([](void*) {
		app.run();
	}, Mail());

	return 0;
}

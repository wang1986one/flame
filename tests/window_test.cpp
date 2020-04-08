#include <flame/foundation/foundation.h>

using namespace flame;

int main(int argc, char** args)
{
	auto w = SysWindow::create("Window Test", Vec2u(1280, 720), WindowFrame);
	w->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
		if (is_mouse_down(action, key))
			(*(SysWindow**)c)->close();
		return true;
	}, Mail::from_p(w));

	looper().loop([](void*) {
	}, Mail());

	return 0;
}
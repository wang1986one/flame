#include <flame/serialize.h>
#include <flame/foundation/foundation.h>
#include <flame/graphics/device.h>
#include <flame/graphics/swapchain.h>
#include <flame/graphics/synchronize.h>
#include <flame/graphics/commandbuffer.h>
#include <flame/graphics/renderpass.h>
#include <flame/graphics/image.h>
#include <flame/graphics/shader.h>

using namespace flame;
using namespace graphics;

struct App
{
	SysWindow* w;
	Device* d;
	Swapchain* sc;
	Image* img;

	void on_resize()
	{
		if (sc->image_count() > 0)
		{
			sc->acquire_image();

			auto dst = sc->image(sc->image_index());
			auto cb = Commandbuffer::create(d->gcp);
			cb->begin();
			cb->change_image_layout(dst, ImageLayoutUndefined, ImageLayoutTransferDst);
			ImageCopy cpy;
			cpy.size = min(dst->size, img->size);
			cb->copy_image(img, dst, 1, &cpy);
			cb->change_image_layout(dst, ImageLayoutTransferDst, ImageLayoutPresent);
			cb->end();
			auto finished = Semaphore::create(d);
			d->gq->submit(1, &cb, sc->image_avalible(), finished, nullptr);

			d->gq->present(sc, finished);
		}
	}
}app;

int main(int argc, char** args)
{
	std::filesystem::path engine_path = getenv("FLAME_PATH");

	app.w = SysWindow::create("Graphics Test", Vec2u(800, 600), WindowFrame | WindowResizable);
	app.d = Device::create(true);
	app.sc = Swapchain::create(app.d, app.w, true);
	app.img = Image::create_from_file(app.d, (engine_path / L"art/9.png").c_str(), ImageUsageTransferSrc, false);
	app.img->change_layout(ImageLayoutShaderReadOnly, ImageLayoutTransferSrc);
	app.on_resize();
	app.w->resize_listeners.add([](void*, const Vec2u&) {
		app.on_resize();
		return true;
	}, Mail());

	looper().loop([](void*) {
	}, Mail());
}

#pragma comment(lib, "flame_foundation.lib")
#pragma comment(lib, "flame_graphics.lib")

#include <flame/foundation/foundation.h>
#include <flame/graphics/all.h>

namespace flame
{
	struct MakeCmd$
	{
		Array<void*> cmdbufs$i;
		void* renderpass$i;
		void* clearvalues$i;
		Array<void*> framebuffers$i;
		
		__declspec(dllexport) void initialize$()
		{
		}
		
		__declspec(dllexport) void finish$()
		{
		}
		
		__declspec(dllexport) void update$()
		{
			for (auto i = 0; i < cmdbufs$i.size; i++)
			{
				auto cb = (graphics::Commandbuffer*)cmdbufs$i.v[i];
				cb->begin();
				cb->begin_renderpass((graphics::Renderpass*)renderpass$i, (graphics::Framebuffer*)framebuffers$i.v[i], (graphics::Clearvalues*)clearvalues$i);
				cb->end_renderpass();
				cb->end();
			}
		}
	};

	static MakeCmd$ bp_makecmd_unused;
}
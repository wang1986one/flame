#pragma once

#include <flame/universe/universe.h>
#include "world_private.h"

namespace flame
{
	struct ListenerHub
	{
		std::vector<std::unique_ptr<Closure<void(void* c)>>> listeners;
	};

	struct UniversePrivate : Universe
	{
		std::vector<std::unique_ptr<WorldPrivate>> worlds;

		std::map<std::string, void*> bank;

		void update();
	};
}

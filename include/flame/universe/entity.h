#pragma once

#include <flame/foundation/foundation.h>
#include <flame/universe/universe.h>

namespace flame
{
	struct TypeinfoDatabase;

	struct World;
	struct Component;

	struct Entity
	{
		ListenerHub<bool(void* c)> on_removed_listeners;
		ListenerHub<bool(void* c)> on_destroyed_listeners;

		void* gene;

		uint depth_;
		uint index_;
		int created_frame_;
		bool dying_;

		bool visible_;
		bool global_visible_;

		FLAME_UNIVERSE_EXPORTS const char* name() const;
		FLAME_UNIVERSE_EXPORTS uint name_hash() const;
		FLAME_UNIVERSE_EXPORTS void set_name(const char* name) const;

		FLAME_UNIVERSE_EXPORTS void set_visible(bool v);

		FLAME_UNIVERSE_EXPORTS Component* get_component_plain(uint hash) const;

		template <class T>
		T* get_component_t(uint hash) const
		{
			return (T*)get_component_plain(hash);
		}

#define get_component(T) get_component_t<T>(FLAME_CHASH(#T))
#define get_id_component(T, id) get_component_t<T>(hash_update(FLAME_CHASH(#T), id))

		FLAME_UNIVERSE_EXPORTS Array<Component*> get_components() const;
		FLAME_UNIVERSE_EXPORTS void add_component(Component* c);
		FLAME_UNIVERSE_EXPORTS void remove_component(Component* c);

		FLAME_UNIVERSE_EXPORTS World* world() const;
		FLAME_UNIVERSE_EXPORTS Entity* parent() const;
		FLAME_UNIVERSE_EXPORTS uint child_count() const;
		FLAME_UNIVERSE_EXPORTS Entity* child(uint index) const;
		FLAME_UNIVERSE_EXPORTS Entity* find_child(const char* name) const;
		FLAME_UNIVERSE_EXPORTS int find_child(Entity* e) const;
		FLAME_UNIVERSE_EXPORTS void add_child(Entity* e, int position = -1); /* -1 is end */
		FLAME_UNIVERSE_EXPORTS void reposition_child(Entity* e, int position); /* -1 is last */
		FLAME_UNIVERSE_EXPORTS void remove_child(Entity* e, bool destroy = true);
		FLAME_UNIVERSE_EXPORTS void remove_children(int from, int to /* -1 is end */, bool destroy = true);

		FLAME_UNIVERSE_EXPORTS Entity* copy();

		FLAME_UNIVERSE_EXPORTS static Entity* create();
		FLAME_UNIVERSE_EXPORTS static Entity* create_from_file(World* w, const wchar_t* filename);
		FLAME_UNIVERSE_EXPORTS static void save_to_file(Entity* e, const wchar_t* filename);
		FLAME_UNIVERSE_EXPORTS static void destroy(Entity* w);
	};
}

#include <flame/universe/world.h>
#include "../systems/event_dispatcher_private.h"
#include <flame/universe/components/element.h>
#include "event_receiver_private.h"

namespace flame
{
	cEventReceiverPrivate::cEventReceiverPrivate()
	{
		dispatcher = nullptr;
		element = nullptr;

		focus_type = FocusByLeftButton;
		drag_hash = 0;
		state = EventReceiverNormal;

		pass_checkers.impl = ListenerHubImpl::create();
		key_listeners.impl = ListenerHubImpl::create();
		mouse_listeners.impl = ListenerHubImpl::create();
		drag_and_drop_listeners.impl = ListenerHubImpl::create();
		hover_listeners.impl = ListenerHubImpl::create();
		focus_listeners.impl = ListenerHubImpl::create();
		state_listeners.impl = ListenerHubImpl::create();

		frame = -1;
	}

	cEventReceiverPrivate::~cEventReceiverPrivate()
	{
		ListenerHubImpl::destroy(pass_checkers.impl);
		ListenerHubImpl::destroy(key_listeners.impl);
		ListenerHubImpl::destroy(mouse_listeners.impl);
		ListenerHubImpl::destroy(drag_and_drop_listeners.impl);
		ListenerHubImpl::destroy(hover_listeners.impl);
		ListenerHubImpl::destroy(focus_listeners.impl);
		ListenerHubImpl::destroy(state_listeners.impl);
	}

	void cEventReceiverPrivate::on_key(KeyStateFlags action, uint value)
	{
		key_listeners.call(action, value);
	}

	void cEventReceiverPrivate::on_mouse(KeyStateFlags action, MouseKey key, const Vec2i& value)
	{
		mouse_listeners.call(action, key, value);
	}

	void cEventReceiverPrivate::on_drag_and_drop(DragAndDrop action, cEventReceiver* er, const Vec2i& pos)
	{
		drag_and_drop_listeners.call(action, er, pos);
	}

	void cEventReceiverPrivate::on_entered_world()
	{
		dispatcher = entity->world()->get_system(sEventDispatcher);
		dispatcher->pending_update = true;
	}

	void cEventReceiverPrivate::on_left_world()
	{
		((sEventDispatcherPrivate*)dispatcher)->on_receiver_removed(this);
		dispatcher->pending_update = true;
		dispatcher = nullptr;
	}

	void cEventReceiverPrivate::on_component_added(Component* c)
	{
		if (c->name_hash == FLAME_CHASH("cElement"))
			element = (cElement*)c;
	}

	void cEventReceiverPrivate::on_visibility_changed()
	{
		if (dispatcher)
			dispatcher->pending_update = true;
	}

	Component* cEventReceiverPrivate::copy()
	{
		auto copy = new cEventReceiverPrivate();

		copy->drag_hash = drag_hash;

		return copy;
	}

	void cEventReceiver::set_acceptable_drops(uint drop_count, const uint* _drops)
	{
		auto& drops = ((cEventReceiverPrivate*)this)->acceptable_drops;
		drops.resize(drop_count);
		for (auto i = 0; i < drop_count; i++)
			drops[i] = _drops[i];
	}

	void cEventReceiver::on_key(KeyStateFlags action, uint value)
	{
		((cEventReceiverPrivate*)this)->on_key(action, value);
	}

	void cEventReceiver::on_mouse(KeyStateFlags action, MouseKey key, const Vec2i& value)
	{
		((cEventReceiverPrivate*)this)->on_mouse(action, key, value);
	}

	void cEventReceiver::on_drag_and_drop(DragAndDrop action, cEventReceiver* er, const Vec2i& pos)
	{
		((cEventReceiverPrivate*)this)->on_drag_and_drop(action, er, pos);
	}

	cEventReceiver* cEventReceiver::create()
	{
		return new cEventReceiverPrivate();
	}
}

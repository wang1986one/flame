#pragma once

#ifdef FLAME_NETWORK_MODULE
#define FLAME_NETWORK_EXPORTS __declspec(dllexport)
#else
#define FLAME_NETWORK_EXPORTS __declspec(dllimport)
#endif

#include <flame/foundation/foundation.h>

namespace flame
{
	enum SocketType
	{
		SocketNormal,
		SocketWeb
	};

	struct Client
	{
		FLAME_NETWORK_EXPORTS void send(void* data, uint size);

		FLAME_NETWORK_EXPORTS static Client* create(SocketType type, const char* ip, uint port, void on_message(Capture& c, const char* msg, uint size), void on_close(Capture& c), const Capture& capture);
		FLAME_NETWORK_EXPORTS static void destroy(Client* c);
	};

	struct Server
	{
		FLAME_NETWORK_EXPORTS void set_client(void* id, void on_message(Capture& c, const char* msg, uint size), void on_close(Capture& c), const Capture& capture);
		FLAME_NETWORK_EXPORTS void send(void* id, void* data, uint size, bool is_dgram);

		FLAME_NETWORK_EXPORTS static Server* create(SocketType type, uint port, void on_dgram(Capture& c, void* id, const char* msg, uint size), void on_connect(Capture& c, void* id), const Capture& capture);
		FLAME_NETWORK_EXPORTS static void destroy(Server* s);
	};

	struct FrameSyncServer
	{
		FLAME_NETWORK_EXPORTS bool send(uint client_idx, void* data, uint size);

		FLAME_NETWORK_EXPORTS static FrameSyncServer* create(SocketType type, uint port, uint clients_count);
		FLAME_NETWORK_EXPORTS static void destroy(FrameSyncServer* s);
	};

	FLAME_NETWORK_EXPORTS void board_cast(uint port, void* data, uint size, uint timeout/* second */, void on_message(Capture& c, const char* ip, const char* msg, uint size), const Capture& capture);
}

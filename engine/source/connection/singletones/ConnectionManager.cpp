#include "ConnectionManager.h"

namespace engine::connection
{
	WSADATA s_wsaData;
	void init()
	{
		WORD version = MAKEWORD(2, 2);
		int result = WSAStartup(version, &s_wsaData);
		ALWAYS_ASSERT(result == 0 && "WSAStartup");
	}

	void deinit()
	{
		int result = WSACleanup();
		ALWAYS_ASSERT(result == 0 && "WSACleanup");
	}
}
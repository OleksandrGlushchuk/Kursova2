#pragma once
#include "../../include/win_def.hpp"
#include <iostream>
#include <WS2tcpip.h>
#pragma comment (lib, "ws2_32.lib")

#include "../../include/Assert.h"

namespace engine::connection
{
	extern WSADATA s_wsaData;
	void init();
	void deinit();
}

#include "../../include/win_undef.hpp"
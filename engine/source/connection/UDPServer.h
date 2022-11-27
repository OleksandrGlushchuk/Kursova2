#pragma once
#include "singletones/ConnectionManager.h"

namespace engine::connection
{
	class UDPServer
	{
		sockaddr_in m_clientInfo;
		int m_clientLength = sizeof(m_clientInfo);

		sockaddr_in m_serverInfo;
		SOCKET m_socket;
	public:
		UDPServer(){}
		void Init(USHORT server_port)
		{
			m_socket = socket(AF_INET, SOCK_DGRAM, 0);

			m_serverInfo.sin_addr.S_un.S_addr = ADDR_ANY;
			m_serverInfo.sin_family = AF_INET;
			m_serverInfo.sin_port = htons(server_port);

			int result = bind(m_socket, (sockaddr*)&m_serverInfo, sizeof(m_serverInfo));

			if(result == SOCKET_ERROR)
			{
				result = WSAGetLastError();
				__debugbreak();
				std::abort();
			}
		}

		template<typename DATA>
		void RecvData(DATA& outData, int dataLength);

		void Deinit()
		{
			int result = closesocket(m_socket);
			if (result == SOCKET_ERROR)
			{
				result = WSAGetLastError();
				__debugbreak();
				std::abort();
			}
		}
	};

	template<typename DATA>
	inline void UDPServer::RecvData(DATA& outData, int dataLength)
	{
		int recvBytes = recvfrom(m_socket, (char*)&outData, dataLength, 0, (sockaddr*)&m_clientInfo, &m_clientLength);

		if (recvBytes == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			__debugbreak();
			std::abort();
		}
	}
}
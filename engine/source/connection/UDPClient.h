#pragma once
#include "singletones/ConnectionManager.h"
#include <string>
#include <vector>

namespace engine::connection
{
	class UDPClient
	{
		SOCKET m_socket;
		std::vector<sockaddr_in> m_serverInfo;
	public:

		void Init(USHORT server_port, const std::string& serverAddress)
		{
			m_socket = socket(AF_INET, SOCK_DGRAM, 0);
			auto& serverInfo = m_serverInfo.emplace_back();
			serverInfo.sin_family = AF_INET;
			serverInfo.sin_port = htons(server_port);
			inet_pton(AF_INET, serverAddress.c_str(), &serverInfo.sin_addr);
		}

		void Init(uint32_t num_dst_servers, std::vector<USHORT> dst_server_ports, std::vector<std::string> dst_server_addresses)
		{
			m_socket = socket(AF_INET, SOCK_DGRAM, 0);
			m_serverInfo.resize(num_dst_servers);

			for (uint32_t i = 0; i < num_dst_servers; ++i)
			{
				m_serverInfo[i].sin_family = AF_INET;
				m_serverInfo[i].sin_port = htons(dst_server_ports[i]);
				inet_pton(AF_INET, dst_server_addresses[i].c_str(), &m_serverInfo[i].sin_addr);
			}
		}

		template<typename DATA>
		void SendData(const DATA &data, int dataLength);

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
	inline void UDPClient::SendData(const DATA& data, int dataLength)
	{
		int sendOk;

		for (uint32_t i = 0; i < m_serverInfo.size(); ++i)
		{
			sendOk = sendto(m_socket, (const char*)&data, dataLength, 0, (sockaddr*)&m_serverInfo[i], sizeof(m_serverInfo[i]));

			if (sendOk == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				__debugbreak();
				std::abort();
			}
		}
	}
}
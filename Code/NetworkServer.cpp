#include "NetworkServer.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <CrySystem/ISystem.h>

#include "Rollback/GameState.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BUFLEN 1216	//Max length of buffer
#define PORT 20100	//The port on which to listen for incoming data

void CNetworkServer::ThreadEntry()
{
	sockaddr_in server, si_other;
	int slen, recv_len;
	ClientToServerUpdateBytesUnion clientToServerUpdate;
	char* recvBuffer = clientToServerUpdate.buff;
	ServerToClientUpdateBytesUnion serverToClientUpdate;
	WSADATA wsa;

	sockaddr_in clientSockets[NUM_PLAYERS];

	slen = sizeof(si_other);

	char clientConnectionCounter = 0;

	//Initialise winsock
	CryLog("NetworkServer: Initialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		const auto e = WSAGetLastError();
		CryFatalError("NetworkServer: Failed. Error Code : %d", e);
		return;
		//exit(EXIT_FAILURE);
	}
	CryLog("NetworkServer: Initialised.");

	m_ListenSocket = socket(AF_INET, SOCK_DGRAM, 0);
	//Create a socket
	if (m_ListenSocket == INVALID_SOCKET)
	{
		const auto e = WSAGetLastError();
		CryFatalError("NetworkServer: Could not create socket : %d", e);
	}
	CryLog("NetworkServer: Socket created.");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT);

	//Bind
	if (bind(m_ListenSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		const auto e = WSAGetLastError();
		CryFatalError("NetworkServer: Bind failed with error code : %d", e);
		return;
		// exit(EXIT_FAILURE);
	}
	CryLog("NetworkServer: Bind done");

	int packetCounter = 0;
	//keep listening for data
	while (!m_bStop)
	{
		CryLog("NetworkServer: Waiting for data...");

		//clear the buffer by filling null, it might have previously received data
		memset(recvBuffer, '\0', BUFLEN);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(m_ListenSocket, recvBuffer, BUFLEN, 0, reinterpret_cast<sockaddr*>(&si_other),
		                         &slen)) ==
			SOCKET_ERROR)
		{
			const auto e = WSAGetLastError();
			CryFatalError("NetworkServer: recvfrom() failed with error code : %d", e);
			return;
			// exit(EXIT_FAILURE);
		}

		if (recvBuffer[0] == 'c')
		{
			const char c[] = {'p', clientConnectionCounter, '\0'};
			if (sendto(m_ListenSocket, c, sizeof(c), 0, reinterpret_cast<sockaddr*>(&si_other), slen) == SOCKET_ERROR)
			{
				const auto e = WSAGetLastError();
				CryFatalError("RollbackNetClient: sendto() failed with error code : %d", e);
				return;
				// exit(EXIT_FAILURE);
			}
			clientSockets[clientConnectionCounter] = si_other;

			clientConnectionCounter++;
			if (clientConnectionCounter == NUM_PLAYERS)
			{
				StartBytesUnion start;
				start.start.packetTypeCode = 's';
				QueryPerformanceCounter(&start.start.gameStartTimestamp);
				//QueryPerformanceFrequency()
				constexpr long long frequency = 10000000;
				start.start.gameStartTimestamp.QuadPart += (frequency * 2); // start after 2 seconds to give clients time to settle.

				// send start game signal to clients
				for (auto& clientSocket : clientSockets)
				{
					if (sendto(m_ListenSocket, start.buff, sizeof(start.buff), 0, reinterpret_cast<sockaddr*>(&clientSocket), slen) ==
						SOCKET_ERROR)
					{
						const auto e = WSAGetLastError();
						CryFatalError("RollbackNetClient: sendto() failed with error code : %d", e);
						return;
						// exit(EXIT_FAILURE);
					}
				}
			}
		}
		else if (recvBuffer[0] == 't')
		{
			const char playerNum = clientToServerUpdate.ticks.playerNum;
			const int updateLastTickNum = clientToServerUpdate.ticks.tickNum;

			const int serverLatestTickReceived = m_latestTickNumber[playerNum];

			if (updateLastTickNum > serverLatestTickReceived)
			{
				const char updateTickCount = clientToServerUpdate.ticks.tickCount;

				const int ticksToAdd = updateLastTickNum - serverLatestTickReceived;

				const int skip = updateTickCount - ticksToAdd;

				for (int i = 0; i < ticksToAdd; ++i)
				{
					const int tickToUpdate = serverLatestTickReceived + i + 1;
					(*m_clientInputsBuffer.GetAt(tickToUpdate))[playerNum] = clientToServerUpdate.ticks.playerInputs[skip + i];
				}

				m_latestTickNumber[playerNum] = updateLastTickNum;
			}
			

			const int ackedServerUpdateNumber = clientToServerUpdate.ticks.ackServerUpdateNumber;
			RingBuffer<int[NUM_PLAYERS-1]>* playerUpdatesTickNumbersBuffer = &m_clientUpdatesTickNumbersBuffers[playerNum];
			int* ackedClientTickNumbers = *playerUpdatesTickNumbersBuffer->GetAt(ackedServerUpdateNumber);

			int thisUpdateNumber = m_clientUpdateNumber[playerNum] += 1;

			int* thisUpdateTickNumbers = *playerUpdatesTickNumbersBuffer->GetAt(thisUpdateNumber);

			serverToClientUpdate.ticks.packetTypeCode = 'r';
			serverToClientUpdate.ticks.ackClientTickNum = updateLastTickNum;
			serverToClientUpdate.ticks.updateNumber = thisUpdateNumber;
			int nInputs = 0;
			for (int i = 0, p = 0; i < NUM_PLAYERS; ++i)
			{
				if (i == playerNum)
					continue;

				int firstTickToSend = ackedClientTickNumbers[p]+1;
				int lastTickToSend = m_latestTickNumber[i];
				int count = lastTickToSend - firstTickToSend;

				serverToClientUpdate.ticks.playerInputsTickNums[p] = firstTickToSend;
				serverToClientUpdate.ticks.playerInputsTickCounts[p] = count;

				for (int k = 0; k < count; ++k)
				{
					serverToClientUpdate.ticks.playerInputs[nInputs] = (*m_clientInputsBuffer.GetAt(firstTickToSend + k))[i];
					nInputs++;
				}
				thisUpdateTickNumbers[p] = lastTickToSend;
				p++;
			}


			const size_t len = sizeof(ServerToClientUpdate) - (sizeof(serverToClientUpdate.ticks.playerInputs) - sizeof(CPlayerInput) * nInputs);
			if (sendto(m_ListenSocket, serverToClientUpdate.buff, len, 0, reinterpret_cast<sockaddr*>(&si_other),
				slen) == SOCKET_ERROR)
			{
				const auto e = WSAGetLastError();
				CryFatalError("RollbackNetClient: sendto() failed with error code : %d", e);
				return;
				// exit(EXIT_FAILURE);
			}			
		// 	int tickCount = 0;
		// 	for (int p = 0; p < NUM_PLAYERS; ++p)
		// 	{
		// 		if(p == playerNum)
		// 			continue;
		//
		// 		int latestAckedTickForOtherPlayer = m_playerLatestAckedUpdatesTickNumbers[playerNum][p];
		// 		int latestTickForOtherPlayer = m_latestTickNumber[p];
		// 		char ticksToSendForPlayer = (char)(latestTickForOtherPlayer - latestAckedTickForOtherPlayer);
		// 		int j = 1;
		// 		for (; j < ticksToSendForPlayer; ++j)
		// 		{
		//
		// 			const CPlayerInput* tickPlayerInputs = *m_clientInputsBuffer.GetAt(latestAckedTickForOtherPlayer + j);
		// 			
		// 			serverToClientUpdate.ticks.playerInputs[tickCount++] = tickPlayerInputs[p];
		// 		}
		// 		serverToClientUpdate.ticks.playerInputsTickCounts[p] = j - 1;
		// 	}
		// 	
		//
		//
		//
		// 	if (playerTickNum > m_latestTickNumber)
		// 		m_latestTickNumber = playerTickNum;
		//
		// 	m_playerLatestTicks[playerNum] = playerTickNum;
		//
		// 	const int ticksToSend = m_latestTickNumber - playerOldestTickNum;
		//
		// 	if (ticksToSend > 0)
		// 	{
		// 		int minTickNum = MAXINT32;
		// 		for (const int playersLatestTick : m_playerLatestTicks)
		// 		{
		// 			if (playersLatestTick < minTickNum)
		// 				minTickNum = playerLatestTick;
		// 		}
		//
		// 		serverToClientUpdate.ticks.packetTypeCode = 'r';
		// 		serverToClientUpdate.ticks.tickNum = m_latestTickNumber;
		// 		serverToClientUpdate.ticks.newOldestTickNum = minTickNum;
		// 		serverToClientUpdate.ticks.count = ticksToSend;
		//
		// 		for (int i = 0; i < ticksToSend; ++i)
		// 		{
		// 			const CPlayerInput* tickPlayerInputs = *m_clientInputsBuffer.GetAt(playerOldestTickNum + i);
		// 			for (int pn = 0; pn < NUM_PLAYERS; ++pn)
		// 			{
		// 				serverToClientUpdate.ticks.playerInputs[pn][i] = tickPlayerInputs[pn];
		// 			}
		// 		}
		//
		// 		const size_t len = sizeof(ServerToClientUpdate) - (sizeof(CPlayerInput[NUM_PLAYERS]) * (MAX_TICKS_TO_SEND - ticksToSend));
		// 		if (sendto(m_ListenSocket, serverToClientUpdate.buff, len, 0, reinterpret_cast<sockaddr*>(&si_other),
		// 			slen) == SOCKET_ERROR)
		// 		{
		// 			const auto e = WSAGetLastError();
		// 			CryFatalError("RollbackNetClient: sendto() failed with error code : %d", e);
		// 			return;
		// 			// exit(EXIT_FAILURE);
		// 		}
		// 	}
		// 	CryLog("NetworkServer: Got a tick %i from %i count %i", clientToServerUpdate.ticks.tickNum, playerNum,
		// 	       clientToServerUpdate.ticks.tickCount);
		}

		packetCounter++;

		//print details of the client/peer and the data received
		// CryLog("NetworkServer: Received packet from %s:%d\n", si_other.sin_addr, ntohs(si_other.sin_port));
		CryLog("NetworkServer: Packet #%i Data:  %s", packetCounter, recvBuffer);
	}

	closesocket(m_ListenSocket);

	WSACleanup();
}


void CNetworkServer::SignalStopWork()
{
	if (m_bStop)
		return;

	m_bStop = true;

	shutdown(m_ListenSocket, SD_BOTH);

	CryLog("NetworkServer: Stopped");
}

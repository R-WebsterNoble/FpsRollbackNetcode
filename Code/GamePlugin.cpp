// Copyright 2016-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"

#include "GamePlugin.h"

#include "Components/Player.h"

#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/EnvPackage.h>
#include <CrySchematyc/Utils/SharedString.h>

#include <IGameObjectSystem.h>
#include <IGameObject.h>

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

#include <IActionMapManager.h>

//#define test

#ifdef test

#include "Net/PlayerInputsSynchronizer.h"
// #include <flatbuffers/idl.h>


class CTestNetUdp : public CNetUdpClientInterface, public CNetUdpServerInterface
{
private:
	std::function<void(char*, int)> m_clientSendCallback = nullptr;
	std::function<int(char* buff, int len)> m_clientReceiveCallback = nullptr;
	std::function<void(char* buff, int len, sockaddr_in* to)> m_serverSendCallback = nullptr;
	std::function<int(char* buff, int len, sockaddr_in* si_other)> m_serverReceiveCallback = nullptr;
	int m_expectedNextCallback = 0;
	int m_expectedCallbackSequenceNumber = 0;
	int m_actualCallbackSequenceNumber = 1;

public:

	void Send(char* buff, int len) override
	{
		if (m_expectedNextCallback != 1 || m_expectedCallbackSequenceNumber != m_actualCallbackSequenceNumber)
			CryFatalError("Test Callback called in incorrect order");

		m_actualCallbackSequenceNumber++;

		m_clientSendCallback(buff, len);
	}

	int Receive(char* buff, int len) override
	{
		if (m_expectedNextCallback != 2 || m_expectedCallbackSequenceNumber != m_actualCallbackSequenceNumber)
			CryFatalError("Test Callback called in incorrect order");

		m_actualCallbackSequenceNumber++;

		return m_clientReceiveCallback(buff, len);
	}

	void Send(char* buff, int len, sockaddr_in* to) override
	{
		if (m_expectedNextCallback != 3 || m_expectedCallbackSequenceNumber != m_actualCallbackSequenceNumber)
			CryFatalError("Test Callback called in incorrect order");

		m_actualCallbackSequenceNumber++;

		m_serverSendCallback(buff, len, to);
	}

	int Receive(char* buff, int len, sockaddr_in* si_other) override
	{
		if (m_expectedNextCallback != 4 || m_expectedCallbackSequenceNumber != m_actualCallbackSequenceNumber)
			CryFatalError("Test Callback called in incorrect order");

		m_actualCallbackSequenceNumber++;
		m_expectedCallbackSequenceNumber++;

		m_actualCallbackSequenceNumber = m_expectedCallbackSequenceNumber;
		m_expectedNextCallback = 3;

		return m_serverReceiveCallback(buff, len, si_other);
	}


	void SetClientSendCallback(int n, const std::function<void(char*, int)>& callback)
	{
		m_expectedCallbackSequenceNumber = n;
		m_expectedNextCallback = 1;
		m_clientSendCallback = callback;
	}

	void SetClientReceiveCallback(int n, const std::function<int(char* buff, int len)>& callback)
	{
		m_expectedCallbackSequenceNumber = n;
		m_expectedNextCallback = 2;
		m_clientReceiveCallback = callback;
	}

	void SetServerCallbacks(int n, const std::function<int(char* buff, int len, sockaddr_in* si_other)>& receiveCallback, const std::function<void(char* buff, int len, sockaddr_in* to)>& sendCallback)
	{
		m_expectedCallbackSequenceNumber = n;
		m_expectedNextCallback = 4;
		m_serverSendCallback = sendCallback;
		m_serverReceiveCallback = receiveCallback;
	}

	// void SetServerReceiveCallback(int n, int (*callback)(char* buff, int len, sockaddr_in* si_other))
	// {
	// 	m_expectedCallbackSequenceNumber = n;
	// 	m_expectedNextCallback = 4;
	// 	m_serverReceiveCallback = callback;
	// }
};
/*
void Test1()
{
	CGameState gs;

	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkClient networkClient = CNetworkClient(&testNetUdp);

	CGameStateManager gameStateManager = CGameStateManager(1.0f);

	testNetUdp.SetClientSendCallback(1, [](char* buff, int len) -> void
	{
		constexpr char c[2] = { 'c', '\0' };
		if (len != sizeof c || memcmp(buff, c, sizeof c) != 0)
			CryFatalError("Client connect packet not sent as expected");

	});
	networkClient.Start();


	testNetUdp.SetClientReceiveCallback(2, [](char* buff, int len) -> int
	{
		constexpr char p[] = { 'p', 0, '\0' };
		memcpy(buff, p, sizeof p);

		return len;
	});
	networkClient.DoWork();


	testNetUdp.SetClientReceiveCallback(3, [](char* buff, int len) -> int
	{
		StartBytesUnion start;
		start.start.packetTypeCode = 's';
		start.start.gameStartTimestamp.QuadPart = 10000000;

		memcpy(buff, start.buff, sizeof(start.buff));

		return len;
	});
	networkClient.DoWork();


	testNetUdp.SetClientSendCallback(4, [](char* buff, int len) -> void
	{
		ClientToServerUpdateBytesUnion packet;
		packet.ticks.packetTypeCode = 't';
		packet.ticks.playerNum = 0;
		packet.ticks.tickNum = 0;
		packet.ticks.tickCount = 1;
		packet.ticks.ackServerUpdateNumber = OptInt{};

		packet.ticks.playerInputs[0].mouseDelta = Vec2(0.0f, 0.0f);
		packet.ticks.playerInputs[0].playerActions = EInputFlag::None;

		constexpr size_t pLen = sizeof(ClientToServerUpdateBytesUnion) - (sizeof(CPlayerInput) * (MAX_TICKS_TO_TRANSMIT - 1));

		if (len != pLen || memcmp(buff, packet.buff, pLen) != 0)
			CryFatalError("ClientToServerUpdateBytesUnion packet not sent as expected");

	});
	constexpr float TICKS_PER_SECOND = 128.0f;
	constexpr float t = 1.0f / TICKS_PER_SECOND;
	CPlayerInput blah;
	blah.mouseDelta = Vec2(1.0f, 0.0f);
	blah.playerActions = EInputFlag::None;
	gameStateManager.Update(0, t, blah, &networkClient, gs);


	testNetUdp.SetClientReceiveCallback(5, [](char* buff, int len) -> int
	{
		ServerToClientUpdateBytesUnion packet;
		packet.ticks.packetTypeCode = 'r';
		packet.ticks.ackClientTickNum = 1;
		packet.ticks.updateNumber = 0;
		packet.ticks.playerInputsTickCounts[0] = 1;
		packet.ticks.playerInputsTickNums[0] = OptInt{ 0 };
		packet.ticks.playerInputs[0].mouseDelta = Vec2(0.0f, 0.0f);
		packet.ticks.playerInputs[0].playerActions = EInputFlag::None;


		const size_t pLen = sizeof(ServerToClientUpdate) - (sizeof(packet.ticks.playerInputs) - sizeof(CPlayerInput) * 1);

		memcpy(buff, packet.buff, pLen);

		return pLen;
	});
	networkClient.DoWork();
}

void Test2()
{
	CGameState gs;
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkClient networkClient = CNetworkClient(&testNetUdp);

	CGameStateManager gameStateManager = CGameStateManager(1.0f);

	testNetUdp.SetClientSendCallback(1, [](char* buff, int len) -> void
	{
		constexpr char c[2] = { 'c', '\0' };
		if (len != sizeof c || memcmp(buff, c, sizeof c) != 0)
			CryFatalError("Client connect packet not sent as expected");
	});
	networkClient.Start();


	testNetUdp.SetClientReceiveCallback(2, [](char* buff, int len) -> int
	{
		constexpr char p[] = { 'p', 0, '\0' };
		memcpy(buff, p, sizeof p);

		return len;
	});
	networkClient.DoWork();


	testNetUdp.SetClientReceiveCallback(3, [](char* buff, int len) -> int
	{
		StartBytesUnion start;
		start.start.packetTypeCode = 's';
		start.start.gameStartTimestamp.QuadPart = 10000000;

		memcpy(buff, start.buff, sizeof(start.buff));

		return len;
	});
	networkClient.DoWork();


	testNetUdp.SetClientSendCallback(4, [](char* buff, int len) -> void
	{
		ClientToServerUpdateBytesUnion packet;
		packet.ticks.packetTypeCode = 't';
		packet.ticks.playerNum = 0;
		packet.ticks.tickNum = 0;
		packet.ticks.tickCount = 1;
		packet.ticks.ackServerUpdateNumber = OptInt{};

		packet.ticks.playerInputs[0].mouseDelta = Vec2(0.992248058f, 0.0f);
		packet.ticks.playerInputs[0].playerActions = EInputFlag::None;

		constexpr size_t pLen = sizeof(ClientToServerUpdateBytesUnion) - (sizeof(CPlayerInput) * (MAX_TICKS_TO_TRANSMIT - 1));

		if (len != pLen || memcmp(buff, packet.buff, pLen) != 0)
			CryFatalError("ClientToServerUpdateBytesUnion packet not sent as expected");

	});
	constexpr float TICKS_PER_SECOND = 128.0f;
	constexpr float t = 1.0f / TICKS_PER_SECOND;
	CPlayerInput playerInput;
	playerInput.mouseDelta = Vec2(1.0f, 0.0f);
	playerInput.playerActions = EInputFlag::None;
	gameStateManager.Update(0, t, playerInput, &networkClient, gs);


	testNetUdp.SetClientSendCallback(5, [](char* buff, int len) -> void
	{
		ClientToServerUpdateBytesUnion packet;
		packet.ticks.packetTypeCode = 't';
		packet.ticks.playerNum = 0;
		packet.ticks.tickNum = 1;
		packet.ticks.tickCount = 2;
		packet.ticks.ackServerUpdateNumber = OptInt{};

		packet.ticks.playerInputs[0].mouseDelta = Vec2(0.0f, 0.0f);
		packet.ticks.playerInputs[0].playerActions = EInputFlag::None;

		packet.ticks.playerInputs[1].mouseDelta = Vec2(1.0f, 0.0f);
		packet.ticks.playerInputs[1].playerActions = EInputFlag::None;

		constexpr size_t pLen = sizeof(ClientToServerUpdateBytesUnion) - (sizeof(CPlayerInput) * (MAX_TICKS_TO_TRANSMIT - 2));

		// if (len != pLen || memcmp(buff, packet.buff, pLen) != 0)
		// 	CryFatalError("ClientToServerUpdateBytesUnion packet not sent as expected");

	});
	playerInput.mouseDelta = Vec2(1.0f, 0.0f);
	playerInput.playerActions = EInputFlag::None;
	gameStateManager.Update(0, t, playerInput, &networkClient, gs);


	testNetUdp.SetClientReceiveCallback(6, [](char* buff, int len) -> int
	{
		ServerToClientUpdateBytesUnion packet;
		packet.ticks.packetTypeCode = 'r';
		packet.ticks.ackClientTickNum = 1;
		packet.ticks.updateNumber = 0;
		packet.ticks.playerInputsTickCounts[0] = 1;
		packet.ticks.playerInputsTickNums[0] = OptInt{};
		packet.ticks.playerInputs[0].mouseDelta = Vec2(0.0f, 0.0f);
		packet.ticks.playerInputs[0].playerActions = EInputFlag::None;

		constexpr size_t pLen = sizeof(ServerToClientUpdate) - (sizeof(packet.ticks.playerInputs) - sizeof(CPlayerInput) * 1);
		memcpy(buff, packet.buff, pLen);

		return pLen;
	});
	networkClient.DoWork();


	testNetUdp.SetClientReceiveCallback(7, [](char* buff, int len) -> int
	{
		ServerToClientUpdateBytesUnion packet;
		packet.ticks.packetTypeCode = 'r';
		packet.ticks.ackClientTickNum = 1;
		packet.ticks.updateNumber = 1;
		packet.ticks.playerInputsTickCounts[0] = 1;
		packet.ticks.playerInputsTickNums[0] = OptInt{};
		packet.ticks.playerInputs[0].mouseDelta = Vec2(0.0f, 0.0f);
		packet.ticks.playerInputs[0].playerActions = EInputFlag::None;

		const size_t pLen = sizeof(ServerToClientUpdate) - (sizeof packet.ticks.playerInputs - sizeof(CPlayerInput) * 1);

		memcpy(buff, packet.buff, pLen);

		return pLen;
	});
	networkClient.DoWork();


	testNetUdp.SetClientSendCallback(8, [](char* buff, int len) -> void
	{
		ClientToServerUpdateBytesUnion packet;
		packet.ticks.packetTypeCode = 't';
		packet.ticks.playerNum = 0;
		packet.ticks.tickNum = 2;
		packet.ticks.tickCount = 2;
		packet.ticks.ackServerUpdateNumber = OptInt{ 1 };

		packet.ticks.playerInputs[0].mouseDelta = Vec2(0.0f, 0.0f);
		packet.ticks.playerInputs[0].playerActions = EInputFlag::None;

		packet.ticks.playerInputs[1].mouseDelta = Vec2(1.0f, 0.0f);
		packet.ticks.playerInputs[1].playerActions = EInputFlag::None;

		constexpr size_t pLen = sizeof(ClientToServerUpdateBytesUnion) - (sizeof(CPlayerInput) * (MAX_TICKS_TO_TRANSMIT - 2));

		// if (len != pLen || memcmp(buff, packet.buff, pLen) != 0)
		// 	CryFatalError("ClientToServerUpdateBytesUnion packet not sent as expected");

	});
	playerInput.mouseDelta = Vec2(1.0f, 0.0f);
	playerInput.playerActions = EInputFlag::None;
	gameStateManager.Update(0, t, playerInput, &networkClient, gs);
}
*/

void Test3()
{
	constexpr float TICKS_PER_SECOND = 1.0f;//128.0f;
	constexpr float t = 1.0f / TICKS_PER_SECOND;

	CPlayerInput pi;
	CGameState gs;

	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkServer networkServer = CNetworkServer(&testNetUdp);
	CNetworkClient networkClient1 = CNetworkClient(&testNetUdp);
	CNetworkClient networkClient2 = CNetworkClient(&testNetUdp);


	CGameStateManager Client1gameStateManager = CGameStateManager(1.0f);
	CGameStateManager Client2gameStateManager = CGameStateManager(1.0f);
	ClientToServerUpdateBytesUnion u;
	char* client1TestPacketBuffer = u.buff;
	int client1TestPacketBufferLength = 0;

	char* client2TestPacketBuffer = u.buff;
	int client2TestPacketBufferLength = 0;


	int sn = 1;


	testNetUdp.SetClientReceiveCallback(sn++, [](char* buff, int len) -> int
	{
		constexpr char p[] = { 'p', 0, '\0' };
		memcpy(buff, p, sizeof p);

		return len;
	});
	networkClient1.DoWork();

	testNetUdp.SetClientReceiveCallback(sn++, [](char* buff, int len) -> int
	{
		constexpr char p[] = { 'p', 1, '\0' };
		memcpy(buff, p, sizeof p);

		return len;
	});
	networkClient2.DoWork();

	gEnv->pLog->SetFileName("Test.log");

	testNetUdp.SetClientReceiveCallback(sn++, [](char* buff, int len) -> int
	{
		StartBytesUnion start;
		start.start.packetTypeCode = 's';
		start.start.gameStartTimestamp.QuadPart = 10000000;

		memcpy(buff, start.buff, sizeof(start.buff));

		return len;
	});
	networkClient1.DoWork();


	testNetUdp.SetClientSendCallback(
		sn++, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len) -> void
		{
			client1TestPacketBufferLength = len;
			memcpy(client1TestPacketBuffer, buff, len);
		});
	pi.mouseDelta = Vec2(0.0f, 0.0f);
	pi.playerActions = EInputFlag::None;
	Client1gameStateManager.Update(0, t, pi, &networkClient1, gs);


	testNetUdp.SetServerCallbacks(
		sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len, sockaddr_in* si_other) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	}, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len, sockaddr_in* to) -> void
	{
		client1TestPacketBufferLength = len;
		memcpy(client1TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();
	sn++;

	testNetUdp.SetClientSendCallback(
		sn++, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len) -> void
		{
			client1TestPacketBufferLength = len;
			memcpy(client1TestPacketBuffer, buff, len);
		});
	pi.mouseDelta = Vec2(1.0f, 0.0f);
	pi.playerActions = EInputFlag::None;
	Client1gameStateManager.Update(0, t, pi, &networkClient1, gs);


	testNetUdp.SetServerCallbacks(
		sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len, sockaddr_in* si_other) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	}, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len, sockaddr_in* to) -> void
	{
		client1TestPacketBufferLength = len;
		memcpy(client1TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();
	sn++;

	testNetUdp.SetClientReceiveCallback(sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	});
	networkClient1.DoWork();

	testNetUdp.SetClientSendCallback(
		sn++, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len) -> void
		{
			client1TestPacketBufferLength = len;
			memcpy(client1TestPacketBuffer, buff, len);
		});
	pi.mouseDelta = Vec2(1.0f, 0.0f);
	pi.playerActions = EInputFlag::None;
	Client1gameStateManager.Update(0, t, pi, &networkClient1, gs);

	testNetUdp.SetServerCallbacks(sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len, sockaddr_in* si_other) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	}, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len, sockaddr_in* to) -> void
	{
		client1TestPacketBufferLength = len;
		memcpy(client1TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();
	sn++;

	testNetUdp.SetClientReceiveCallback(sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	});
	networkClient1.DoWork();

	testNetUdp.SetClientSendCallback(sn++, [&client2TestPacketBufferLength, &client2TestPacketBuffer](char* buff, int len) -> void
	{
		client2TestPacketBufferLength = len;
		memcpy(client2TestPacketBuffer, buff, len);
	});
	pi.mouseDelta = Vec2(100.0f, 0.0f);
	pi.playerActions = EInputFlag::None;
	Client2gameStateManager.Update(0, t, pi, &networkClient2, gs);

	testNetUdp.SetServerCallbacks(sn++, [&client2TestPacketBuffer, &client2TestPacketBufferLength](char* buff, int len, sockaddr_in* si_other) -> int
	{
		memcpy(buff, client2TestPacketBuffer, client2TestPacketBufferLength);
		return client2TestPacketBufferLength;
	}, [&client2TestPacketBuffer, &client2TestPacketBufferLength](char* buff, int len, sockaddr_in* to) -> void
	{
		client2TestPacketBufferLength = len;
		memcpy(client2TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();

}

void Test4()
{
	constexpr float TICKS_PER_SECOND = 1.0f;//128.0f;
	constexpr float t = 1.0f / TICKS_PER_SECOND;

	CPlayerInput pi;
	CGameState gs;

	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkServer networkServer = CNetworkServer(&testNetUdp);
	CNetworkClient networkClient1 = CNetworkClient(&testNetUdp);
	CNetworkClient networkClient2 = CNetworkClient(&testNetUdp);


	CGameStateManager Client1gameStateManager = CGameStateManager(1.0f);
	CGameStateManager Client2gameStateManager = CGameStateManager(1.0f);
	ClientToServerUpdateBytesUnion u;
	char* client1TestPacketBuffer = u.buff;
	int client1TestPacketBufferLength = 0;

	char* client2TestPacketBuffer = u.buff;
	int client2TestPacketBufferLength = 0;


	int sn = 1;


	testNetUdp.SetClientReceiveCallback(sn++, [](char* buff, int len) -> int
	{
		constexpr char p[] = { 'p', 0, '\0' };
		memcpy(buff, p, sizeof p);

		return len;
	});
	networkClient1.DoWork();

	testNetUdp.SetClientReceiveCallback(sn++, [](char* buff, int len) -> int
	{
		constexpr char p[] = { 'p', 1, '\0' };
		memcpy(buff, p, sizeof p);

		return len;
	});
	networkClient2.DoWork();

	gEnv->pLog->SetFileName("Test.log");

	testNetUdp.SetClientReceiveCallback(sn++, [](char* buff, int len) -> int
	{
		StartBytesUnion start;
		start.start.packetTypeCode = 's';
		start.start.gameStartTimestamp.QuadPart = 10000000;

		memcpy(buff, start.buff, sizeof(start.buff));

		return len;
	});
	networkClient1.DoWork();


	testNetUdp.SetClientSendCallback(
		sn++, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len) -> void
		{
			client1TestPacketBufferLength = len;
			memcpy(client1TestPacketBuffer, buff, len);
		});
	pi.mouseDelta = Vec2(0.0f, 0.0f);
	pi.playerActions = EInputFlag::None;
	Client1gameStateManager.Update(0, t, pi, &networkClient1, gs);


	testNetUdp.SetServerCallbacks(
		sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len, sockaddr_in* si_other) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	}, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len, sockaddr_in* to) -> void
	{
		client1TestPacketBufferLength = len;
		memcpy(client1TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();
	sn++;

	testNetUdp.SetClientSendCallback(
		sn++, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len) -> void
		{
			client1TestPacketBufferLength = len;
			memcpy(client1TestPacketBuffer, buff, len);
		});
	pi.mouseDelta = Vec2(1.0f, 0.0f);
	pi.playerActions = EInputFlag::None;
	Client1gameStateManager.Update(0, t, pi, &networkClient1, gs);


	testNetUdp.SetServerCallbacks(
		sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len, sockaddr_in* si_other) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	}, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len, sockaddr_in* to) -> void
	{
		client1TestPacketBufferLength = len;
		memcpy(client1TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();
	sn++;

	testNetUdp.SetClientReceiveCallback(sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	});
	networkClient1.DoWork();

	testNetUdp.SetClientSendCallback(
		sn++, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len) -> void
		{
			client1TestPacketBufferLength = len;
			memcpy(client1TestPacketBuffer, buff, len);
		});
	pi.mouseDelta = Vec2(1.0f, 0.0f);
	pi.playerActions = EInputFlag::None;
	Client1gameStateManager.Update(0, t, pi, &networkClient1, gs);

	testNetUdp.SetServerCallbacks(sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len, sockaddr_in* si_other) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	}, [&client1TestPacketBufferLength, &client1TestPacketBuffer](char* buff, int len, sockaddr_in* to) -> void
	{
		client1TestPacketBufferLength = len;
		memcpy(client1TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();
	sn++;

	testNetUdp.SetClientReceiveCallback(sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	});
	networkClient1.DoWork();

	testNetUdp.SetClientSendCallback(sn++, [&client2TestPacketBufferLength, &client2TestPacketBuffer](char* buff, int len) -> void
	{
		client2TestPacketBufferLength = len;
		memcpy(client2TestPacketBuffer, buff, len);
	});
	pi.mouseDelta = Vec2(100.0f, 0.0f);
	pi.playerActions = EInputFlag::None;
	Client2gameStateManager.Update(0, t, pi, &networkClient2, gs);

	testNetUdp.SetServerCallbacks(sn++, [&client2TestPacketBuffer, &client2TestPacketBufferLength](char* buff, int len, sockaddr_in* si_other) -> int
	{
		memcpy(buff, client2TestPacketBuffer, client2TestPacketBufferLength);
		return client2TestPacketBufferLength;
	}, [&client2TestPacketBuffer, &client2TestPacketBufferLength](char* buff, int len, sockaddr_in* to) -> void
	{
		client2TestPacketBufferLength = len;
		memcpy(client2TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();

}

const FlatBuffPacket::PlayerInputsSynchronizer* SerializeAndDeserialize(flatbuffers::FlatBufferBuilder& builder, const flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>& synchronizer)
{
	const flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizers[1] = { synchronizer };
	const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>> synchronizersVector = builder.CreateVector(synchronizers, 1);
	const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> clientToServerUpdate = CreateClientToServerUpdate(builder, 0, synchronizersVector);
	builder.Finish(clientToServerUpdate);
	const uint8_t* bufferPointer = builder.GetBufferPointer();
	flatbuffers::Verifier verifier(bufferPointer, builder.GetSize(), 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
	CRY_TEST_ASSERT(FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier));

	const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(bufferPointer);
	return update->player_synchronizers()->Get(0);
}


void TestPlayerInputsSynchronizer_HandlesEmptySendAndReceive()
{
	CPlayerInputsSynchronizer s = CPlayerInputsSynchronizer();

	flatbuffers::FlatBufferBuilder builder;
	flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizer;


	CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0));


	const FlatBuffPacket::PlayerInputsSynchronizer* const sync = SerializeAndDeserialize(builder, synchronizer);
	OptInt optInt{};


	auto [tickNum, count, inputs] = CPlayerInputsSynchronizer::ParsePaket(sync, optInt, 0);


	CRY_TEST_ASSERT(tickNum == 0);
	CRY_TEST_ASSERT(count == 0);
	CRY_TEST_ASSERT(inputs.empty());
}

void TestPlayerInputsSynchronizer_HandlesSingleSendAndReceive()
{
	CPlayerInputsSynchronizer s = CPlayerInputsSynchronizer();

	flatbuffers::FlatBufferBuilder builder;
	flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizer;


	s.Enqueue(0, CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward });
	CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0));


	const FlatBuffPacket::PlayerInputsSynchronizer* const sync = SerializeAndDeserialize(builder, synchronizer);
	OptInt optInt{};


	auto [tickNum, count, inputs] = CPlayerInputsSynchronizer::ParsePaket(sync, optInt, 0);



	CRY_TEST_ASSERT(tickNum == 0);
	CRY_TEST_ASSERT(count == 1);
	CRY_TEST_ASSERT(inputs[0].mouseDelta.x == 0.1f);
}



void TestPlayerInputsSynchronizer_HandlesSingleSendWithMultipleInputsAndSingleReceive()
{
	CPlayerInputsSynchronizer s = CPlayerInputsSynchronizer();

	flatbuffers::FlatBufferBuilder builder;
	flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizer;


	s.Enqueue(0, CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward });
	s.Enqueue(1, CPlayerInput{ {0.2f, 0.0f}, EInputFlag::MoveForward });
	CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0));


	const FlatBuffPacket::PlayerInputsSynchronizer* const sync = SerializeAndDeserialize(builder, synchronizer);
	OptInt optInt{};


	auto [tickNum, count, inputs] = CPlayerInputsSynchronizer::ParsePaket(sync, optInt, 0);

	CRY_TEST_ASSERT(tickNum == 0);
	CRY_TEST_ASSERT(count == 2);
	CRY_TEST_ASSERT(inputs[0].mouseDelta.x == 0.1f);
	CRY_TEST_ASSERT(inputs[1].mouseDelta.x == 0.2f);
}


void TestPlayerInputsSynchronizer_HandlesSingleSendAndAlreadyAckedReceive()
{
	CPlayerInputsSynchronizer s = CPlayerInputsSynchronizer();

	flatbuffers::FlatBufferBuilder builder;
	flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizer;

	s.Enqueue(0, CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward });
	CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0));


	const FlatBuffPacket::PlayerInputsSynchronizer* const sync = SerializeAndDeserialize(builder, synchronizer);
	OptInt optInt{0};

	auto [tickNum, count, inputs] = CPlayerInputsSynchronizer::ParsePaket(sync, optInt, 0);
	
	CRY_TEST_ASSERT(count == 0);
}

void TestPlayerInputsSynchronizer_HandlesMultipleSendAndSingleReceive()
{
	CPlayerInputsSynchronizer s = CPlayerInputsSynchronizer();

	flatbuffers::FlatBufferBuilder builder;
	flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizer;


	s.Enqueue(0, CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward });
	CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0));


	s.Enqueue(1, CPlayerInput{ {0.2f, 0.0f}, EInputFlag::MoveForward });
	CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0));


	const FlatBuffPacket::PlayerInputsSynchronizer* const sync = SerializeAndDeserialize(builder, synchronizer);
	OptInt optInt{};


	auto [tickNum, count, inputs] = CPlayerInputsSynchronizer::ParsePaket(sync, optInt, 0);
	CRY_TEST_ASSERT(tickNum == 0);
	CRY_TEST_ASSERT(count == 2);
	CRY_TEST_ASSERT(inputs[0].mouseDelta.x == 0.1f);
	CRY_TEST_ASSERT(inputs[1].mouseDelta.x == 0.2f);
}

void TestPlayerInputsSynchronizer_HandlesMaxInputs()
{
	CPlayerInputsSynchronizer s = CPlayerInputsSynchronizer();

	flatbuffers::FlatBufferBuilder builder;
	flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizer;
	

	for (int i = 0; i < MAX_TICKS_TO_TRANSMIT; ++i)
	{
		const CPlayerInput pi = CPlayerInput{ {1.0f * (1 + i), 0.0f}, EInputFlag::MoveForward };
		s.Enqueue(i, pi);
	}

	CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0));


	const FlatBuffPacket::PlayerInputsSynchronizer* const sync = SerializeAndDeserialize(builder, synchronizer);
	OptInt optInt{ };


	auto [tickNum, count, inputs] = CPlayerInputsSynchronizer::ParsePaket(sync, optInt, 0);


	CRY_TEST_ASSERT(tickNum == 0);
	CRY_TEST_ASSERT(count == MAX_TICKS_TO_TRANSMIT);
	CRY_TEST_ASSERT(inputs[0].mouseDelta.x == 1.0f * 1);
	CRY_TEST_ASSERT(inputs[MAX_TICKS_TO_TRANSMIT - 1].mouseDelta.x == 1.0f * (MAX_TICKS_TO_TRANSMIT));
}

void TestPlayerInputsSynchronizer_SenderHandlesTooManyInputs()
{
	CPlayerInputsSynchronizer s = CPlayerInputsSynchronizer();

	flatbuffers::FlatBufferBuilder builder;
	flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizer;
	

	for (int i = 0; i <= MAX_TICKS_TO_TRANSMIT; ++i)
	{
		const CPlayerInput pi = CPlayerInput{ {1.0f * (1 + i), 0.0f}, EInputFlag::MoveForward };
		s.Enqueue(i, pi);
	}

	CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0) == false);
}


void TestPlayerInputsSynchronizer_ReceiverHandlesTooManyTicks()
{
	CPlayerInputsSynchronizer s = CPlayerInputsSynchronizer();

	flatbuffers::FlatBufferBuilder builder;
	const FlatBuffPacket::OInt optIntFb{};

	const flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizer = CreatePlayerInputsSynchronizer(builder, &optIntFb);
	const FlatBuffPacket::PlayerInputsSynchronizer* const sync = SerializeAndDeserialize(builder, synchronizer);

	OptInt optInt{ };
	auto [tickNum, count, inputs] = CPlayerInputsSynchronizer::ParsePaket(sync, optInt, 0);

	CRY_TEST_ASSERT(tickNum == 0);
	CRY_TEST_ASSERT(count == 0);
}

void TestPlayerInputsSynchronizer_ReceiverHandlesSendThenReceiveThenSend()
{
	// CPlayerInputsSynchronizer s = CPlayerInputsSynchronizer();
	//
	// flatbuffers::FlatBufferBuilder builder;
	// flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizer;
	//
	//
	// const CPlayerInput i = CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward };
	// s.Enqueue(0, i);
	// CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0));
	//
	//
	// const FlatBuffPacket::PlayerInputsSynchronizer* sync = SerializeAndDeserialize(builder, synchronizer);
	//
	//
	// s.UpdateFromPacket(sync);
	//
	//
	// s.Enqueue(1, CPlayerInput{ {0.2f, 0.0f}, EInputFlag::MoveForward });
	// CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0));
	//
	// sync = SerializeAndDeserialize(builder, synchronizer);
	// OptInt optInt{1};
	//
	//
	// auto [tickNum, count, inputs] = CPlayerInputsSynchronizer::ParsePaket(sync, optInt, 0);
	//
	// CRY_TEST_ASSERT(tickNum == 0);
	// CRY_TEST_ASSERT(count == 1);
	// CRY_TEST_ASSERT(inputs[0].mouseDelta.x == 0.1f);
	// CRY_TEST_ASSERT(inputs[1].mouseDelta.x == 0.2f);
}

void TestPlayerInputsSynchronizer_HandlesAlreadyReceivedInputs()
{
	CPlayerInputsSynchronizer s = CPlayerInputsSynchronizer();

	flatbuffers::FlatBufferBuilder builder;
	flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> synchronizer;

	
	s.Enqueue(0, CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward });
	s.Enqueue(1, CPlayerInput{ {0.2f, 0.0f}, EInputFlag::MoveForward });
	CRY_TEST_ASSERT(s.GetPaket(builder, synchronizer, 0));


	const FlatBuffPacket::PlayerInputsSynchronizer* const sync = SerializeAndDeserialize(builder, synchronizer);
	OptInt optInt{0};


	auto [tickNum, count, inputs] = CPlayerInputsSynchronizer::ParsePaket(sync, optInt, 0);



	CRY_TEST_ASSERT(tickNum == 1);
	CRY_TEST_ASSERT(count == 1);
	CRY_TEST_ASSERT(inputs[0].mouseDelta.x == 0.2f);
}

void TestPlayerInputsSynchronizer()
{
	TestPlayerInputsSynchronizer_HandlesEmptySendAndReceive();
	TestPlayerInputsSynchronizer_HandlesSingleSendAndReceive();
	TestPlayerInputsSynchronizer_HandlesSingleSendWithMultipleInputsAndSingleReceive();
	TestPlayerInputsSynchronizer_HandlesSingleSendAndAlreadyAckedReceive();
	TestPlayerInputsSynchronizer_HandlesMultipleSendAndSingleReceive();
	TestPlayerInputsSynchronizer_HandlesMaxInputs();
	TestPlayerInputsSynchronizer_SenderHandlesTooManyInputs();
	TestPlayerInputsSynchronizer_ReceiverHandlesTooManyTicks();
	TestPlayerInputsSynchronizer_ReceiverHandlesSendThenReceiveThenSend();
	TestPlayerInputsSynchronizer_HandlesAlreadyReceivedInputs();
	
}

void TestGameStateManagerCreatesNoInputsForShortFrame()
{
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkClient networkClient = CNetworkClient(&testNetUdp);

	CGameState gs;
	CGameStateManager gsm = CGameStateManager(1.0f);

	CPlayerInput pi = CPlayerInput{ Vec2{1.0f, 0.0f}, EInputFlag::MoveForward };

	gsm.Update(0, 0.5f, pi, &networkClient, gs);
}

void TestGameStateManagerSendsSingleInput()
{
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkClient networkClient = CNetworkClient(&testNetUdp);

	CGameState gs;
	CGameStateManager gsm = CGameStateManager(1.0f);

	testNetUdp.SetClientSendCallback(1, [](char* buff, int len) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 0);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 1);
			const FlatBuffPacket::PlayerInput* playerInput = playerInputs->Get(0);
			CRY_TEST_CHECK_CLOSE(playerInput->mouse_delta().x(), 1.0f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
		}
	});

	CPlayerInput pi = CPlayerInput{ Vec2{1.0f, 0.0f}, EInputFlag::MoveForward };


	gsm.Update(0, 1.0f, pi, &networkClient, gs);
}

void TestGameStateManagerCreatesMultipleInputs()
{
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkClient networkClient = CNetworkClient(&testNetUdp);

	CGameState gs;
	CGameStateManager gsm = CGameStateManager(1.0f);

	testNetUdp.SetClientSendCallback(1, [](char* buff, int len) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 1);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 2);
			const FlatBuffPacket::PlayerInput* playerInput1 = playerInputs->Get(0);
			CRY_TEST_CHECK_CLOSE(playerInput1->mouse_delta().x(), 0.5f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput1->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
			const FlatBuffPacket::PlayerInput* playerInput2 = playerInputs->Get(1);
			CRY_TEST_CHECK_CLOSE(playerInput2->mouse_delta().x(), 0.5f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput2->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
		}
	});

	CPlayerInput pi = CPlayerInput{ Vec2{1.0f, 0.0f}, EInputFlag::MoveForward };


	gsm.Update(0, 2.0f, pi, &networkClient, gs);
}

void TestGameStateManagerCreatesMultipleInputsWithMisalignedTick()
{
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkClient networkClient = CNetworkClient(&testNetUdp);

	CGameState gs;
	CGameStateManager gsm = CGameStateManager(1.0f);

	testNetUdp.SetClientSendCallback(1, [](char* buff, int len) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 1);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 2);
			const FlatBuffPacket::PlayerInput* playerInput1 = playerInputs->Get(0);
			CRY_TEST_CHECK_CLOSE(playerInput1->mouse_delta().x(), 0.4f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput1->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
			const FlatBuffPacket::PlayerInput* playerInput2 = playerInputs->Get(1);
			CRY_TEST_CHECK_CLOSE(playerInput2->mouse_delta().x(), 0.4f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput2->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
		}
	});

	CPlayerInput pi = CPlayerInput{ Vec2{1.0f, 0.0f}, EInputFlag::MoveForward };


	gsm.Update(0, 2.5f, pi, &networkClient, gs);


	testNetUdp.SetClientSendCallback(2, [](char* buff, int len) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 2);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 3);
			const FlatBuffPacket::PlayerInput* playerInput1 = playerInputs->Get(0);
			CRY_TEST_CHECK_CLOSE(playerInput1->mouse_delta().x(), 0.4f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput1->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
			const FlatBuffPacket::PlayerInput* playerInput2 = playerInputs->Get(1);
			CRY_TEST_CHECK_CLOSE(playerInput2->mouse_delta().x(), 0.4f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput2->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
			const FlatBuffPacket::PlayerInput* playerInput3 = playerInputs->Get(2);
			CRY_TEST_CHECK_CLOSE(playerInput3->mouse_delta().x(), 2.2f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput3->player_actions(), FlatBuffPacket::InputFlags_MoveBackward);
		}
	});


	CPlayerInput pi2{ Vec2{2.0f, 0.0f}, EInputFlag::MoveBackward };
	gsm.Update(0, 0.5f, pi2, &networkClient, gs);


	gsm.Update(0, 0.01f, pi2, &networkClient, gs);
}

void TestGameStateManagerCreatesHandlesAck()
{
	CTestNetUdp testNetUdp = CTestNetUdp();
	
	CNetworkClient networkClient = CNetworkClient(&testNetUdp);
	
	CGameState gs;
	CGameStateManager gsm = CGameStateManager(1.0f);
	
	testNetUdp.SetClientSendCallback(1, [](char* buff, int len) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 0);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 1);
			const FlatBuffPacket::PlayerInput* playerInput = playerInputs->Get(0);
			CRY_TEST_CHECK_CLOSE(playerInput->mouse_delta().x(), 1.0f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
		}
	});
	
	CPlayerInput pi = CPlayerInput{ Vec2{1.0f, 0.0f}, EInputFlag::MoveForward };
	
	gsm.Update(0, 1.0f, pi, &networkClient, gs);

	testNetUdp.SetClientReceiveCallback(2, [](char* buff, int len) -> int
	{
		flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];
		

		for (int i = 0; i < NUM_PLAYERS; ++i)
		{
			const FlatBuffPacket::OInt noTicks = FlatBuffPacket::OInt(i == 0 ? 0 : OptInt().I);		
			
			s[i] = CreatePlayerInputsSynchronizer(builder, &noTicks, 0);
		}
		

		const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>>	playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
		const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

		builder.Finish(serverToClientUpdate);
		const uint8_t* bufferPointer = builder.GetBufferPointer();

		len = builder.GetSize();
		memcpy(buff, bufferPointer, len);
		return len;
	});
	networkClient.DoWork();

	
	testNetUdp.SetClientSendCallback(3, [](char* buff, int len) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 1);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 1);
			const FlatBuffPacket::PlayerInput* playerInput = playerInputs->Get(0);
			CRY_TEST_CHECK_CLOSE(playerInput->mouse_delta().x(), 2.0f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
		}
	});
	
	const CPlayerInput pi2 = CPlayerInput{ Vec2{2.0f, 0.0f}, EInputFlag::MoveForward };
	
	
	gsm.Update(0, 1.0f, pi2, &networkClient, gs);
}

void TestGameStateManagerHandlesOtherPlayerInputs()
{
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkClient networkClient = CNetworkClient(&testNetUdp);

	CGameState gs;
	CGameStateManager gsm = CGameStateManager(1.0f);

	testNetUdp.SetClientSendCallback(1, [](char* buff, int len) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 0);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 1);
			const FlatBuffPacket::PlayerInput* playerInput = playerInputs->Get(0);
			CRY_TEST_CHECK_CLOSE(playerInput->mouse_delta().x(), 1.0f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
		}
	});

	CPlayerInput pi = CPlayerInput{ Vec2{1.0f, 0.0f}, EInputFlag::MoveForward };

	gsm.Update(0, 1.0f, pi, &networkClient, gs);

	testNetUdp.SetClientReceiveCallback(2, [](char* buff, int len) -> int
	{
		flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];


		const FlatBuffPacket::OInt tickNum = FlatBuffPacket::OInt(0);
		s[0] = FlatBuffPacket::CreatePlayerInputsSynchronizer(builder, &tickNum, 0);

	
		for (int i = 1; i < NUM_PLAYERS; ++i)
		{
			const FlatBuffPacket::PlayerInput playerInputs[] = { FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(2.0f, 2.0f), FlatBuffPacket::InputFlags_MoveForward} };
			const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, 1);
			s[i] = CreatePlayerInputsSynchronizer(builder, &tickNum, inputsFlatBuff);
		}

		const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>>	playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
		const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

		builder.Finish(serverToClientUpdate);
		const uint8_t* bufferPointer = builder.GetBufferPointer();

		len = builder.GetSize();
		memcpy(buff, bufferPointer, len);
		return len;
	});
	networkClient.DoWork();


	STickInput tickInput;
	for (int i = 1; i < NUM_PLAYERS; ++i)
	{
		networkClient.GetInputUpdates(tickInput);

		CRY_TEST_CHECK_EQUAL(tickInput.tickNum, 0);
		CRY_TEST_CHECK_EQUAL(tickInput.playerNum, 1);
		CRY_TEST_CHECK_CLOSE(tickInput.inputs.at(0).mouseDelta.x, 2.0f, 0.001f);
	}

	CRY_TEST_CHECK_EQUAL(networkClient.GetInputUpdates(tickInput), false);
}


void TestGameStateManagerHandlesOtherPlayerInputsAcked()
{
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkClient networkClient = CNetworkClient(&testNetUdp);

	CGameState gs;
	CGameStateManager gsm = CGameStateManager(1.0f);

	testNetUdp.SetClientSendCallback(1, [](char* buff, int len) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 0);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 1);
			const FlatBuffPacket::PlayerInput* playerInput = playerInputs->Get(0);
			CRY_TEST_CHECK_CLOSE(playerInput->mouse_delta().x(), 1.0f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
		}
	});

	CPlayerInput pi = CPlayerInput{ Vec2{1.0f, 0.0f}, EInputFlag::MoveForward };

	gsm.Update(0, 1.0f, pi, &networkClient, gs);

	testNetUdp.SetClientReceiveCallback(2, [](char* buff, int len) -> int
	{
		flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];


		const FlatBuffPacket::OInt tickNum = FlatBuffPacket::OInt(0);
		s[0] = FlatBuffPacket::CreatePlayerInputsSynchronizer(builder, &tickNum, 0);


		for (int i = 1; i < NUM_PLAYERS; ++i)
		{
			const FlatBuffPacket::PlayerInput playerInputs[] = { FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(2.0f, 2.0f), FlatBuffPacket::InputFlags_MoveForward} };
			const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, 1);
			s[i] = CreatePlayerInputsSynchronizer(builder, &tickNum, inputsFlatBuff);
		}

		const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>>	playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
		const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

		builder.Finish(serverToClientUpdate);
		const uint8_t* bufferPointer = builder.GetBufferPointer();

		len = builder.GetSize();
		memcpy(buff, bufferPointer, len);
		return len;
	});
	networkClient.DoWork();

	testNetUdp.SetClientReceiveCallback(3, [](char* buff, int len) -> int
	{
		flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];


		const FlatBuffPacket::OInt tickNum = FlatBuffPacket::OInt(0);
		s[0] = FlatBuffPacket::CreatePlayerInputsSynchronizer(builder, &tickNum, 0);


		for (int i = 1; i < NUM_PLAYERS; ++i)
		{
			const FlatBuffPacket::PlayerInput playerInputs[] = { FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(2.0f, 2.0f), FlatBuffPacket::InputFlags_MoveForward} };
			const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, 1);
			s[i] = CreatePlayerInputsSynchronizer(builder, &tickNum, inputsFlatBuff);
		}

		const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>>	playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
		const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

		builder.Finish(serverToClientUpdate);
		const uint8_t* bufferPointer = builder.GetBufferPointer();

		len = builder.GetSize();
		memcpy(buff, bufferPointer, len);
		return len;
	});
	networkClient.DoWork();


	STickInput tickInput;
	for (int i = 1; i < NUM_PLAYERS; ++i)
	{
		networkClient.GetInputUpdates(tickInput);

		CRY_TEST_CHECK_EQUAL(tickInput.tickNum, 0);
		CRY_TEST_CHECK_EQUAL(tickInput.playerNum, 1);
		CRY_TEST_CHECK_CLOSE(tickInput.inputs.at(0).mouseDelta.x, 2.0f, 0.001f);
	}

	CRY_TEST_CHECK_EQUAL(networkClient.GetInputUpdates(tickInput), false);
}

void TestGameStateManagerHandlesMoreOtherPlayerInputsAfterAck()
{
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkClient networkClient = CNetworkClient(&testNetUdp);

	CGameState gs;
	CGameStateManager gsm = CGameStateManager(1.0f);

	testNetUdp.SetClientSendCallback(1, [](char* buff, int len) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 0);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 1);
			const FlatBuffPacket::PlayerInput* playerInput = playerInputs->Get(0);
			CRY_TEST_CHECK_CLOSE(playerInput->mouse_delta().x(), 1.0f, 0.001f);
			CRY_TEST_CHECK_EQUAL(playerInput->player_actions(), FlatBuffPacket::InputFlags_MoveForward);
		}
	});

	CPlayerInput pi = CPlayerInput{ Vec2{1.0f, 0.0f}, EInputFlag::MoveForward };

	gsm.Update(0, 1.0f, pi, &networkClient, gs);

	testNetUdp.SetClientReceiveCallback(2, [](char* buff, int len) -> int
	{
		flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];


		const FlatBuffPacket::OInt tickNum = FlatBuffPacket::OInt(0);
		s[0] = FlatBuffPacket::CreatePlayerInputsSynchronizer(builder, &tickNum, 0);


		for (int i = 1; i < NUM_PLAYERS; ++i)
		{
			const FlatBuffPacket::PlayerInput playerInputs[] = { FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(102.0f, 2.0f), FlatBuffPacket::InputFlags_MoveForward} };
			const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, 1);
			s[i] = CreatePlayerInputsSynchronizer(builder, &tickNum, inputsFlatBuff);
		}

		const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>>	playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
		const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

		builder.Finish(serverToClientUpdate);
		const uint8_t* bufferPointer = builder.GetBufferPointer();

		len = builder.GetSize();
		memcpy(buff, bufferPointer, len);
		return len;
	});
	networkClient.DoWork();

	STickInput tickInput;
	for (int i = 1; i < NUM_PLAYERS; ++i)
	{
		networkClient.GetInputUpdates(tickInput);

		CRY_TEST_CHECK_EQUAL(tickInput.tickNum, 0);
		CRY_TEST_CHECK_EQUAL(tickInput.playerNum, i);
		CRY_TEST_CHECK_CLOSE(tickInput.inputs.at(0).mouseDelta.x, 102.0f, 0.001f);
	}

	CRY_TEST_CHECK_EQUAL(networkClient.GetInputUpdates(tickInput), false);

	testNetUdp.SetClientReceiveCallback(3, [](char* buff, int len) -> int
	{
		flatbuffers::FlatBufferBuilder builder;

		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];


		const FlatBuffPacket::OInt tickNum = FlatBuffPacket::OInt(1);
		s[0] = FlatBuffPacket::CreatePlayerInputsSynchronizer(builder, &tickNum, 0);


		for (int i = 1; i < NUM_PLAYERS; ++i)
		{
			const FlatBuffPacket::PlayerInput playerInputs[] = { FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(3.0f, 0.0f), FlatBuffPacket::InputFlags_MoveBackward} };
			const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, 1);
			s[i] = CreatePlayerInputsSynchronizer(builder, &tickNum, inputsFlatBuff);
		}

		const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>>	playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
		const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

		builder.Finish(serverToClientUpdate);
		const uint8_t* bufferPointer = builder.GetBufferPointer();

		// std::string jsongen;
		// flatbuffers::Parser parser;
		// std::string schemafile;
		// flatbuffers::LoadFile(R"(D:\Documents\Projects\FpsRollbackNetcode-Cryengine\Code\Net\ClientToServerUpdate.fbs)", false, &schemafile);
		// parser.Parse(schemafile.c_str());
		// GenerateText(parser, builder.GetBufferPointer(), &jsongen);

		len = builder.GetSize();
		memcpy(buff, bufferPointer, len);
		return len;
	});
	networkClient.DoWork();
	
	for (int i = 1; i < NUM_PLAYERS; ++i)
	{
		networkClient.GetInputUpdates(tickInput);

		CRY_TEST_CHECK_EQUAL(tickInput.tickNum, 1);
		CRY_TEST_CHECK_EQUAL(tickInput.playerNum, i);
		CRY_TEST_CHECK_CLOSE(tickInput.inputs.at(0).mouseDelta.x, 3.0f, 0.001f);
	}

	CRY_TEST_CHECK_EQUAL(networkClient.GetInputUpdates(tickInput), false);

	
	testNetUdp.SetClientSendCallback(4, [](char* buff, int len) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 1);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 0);


			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type player1InputsSynchronizer = update->player_synchronizers()->Get(1);
			CRY_TEST_CHECK_EQUAL(player1InputsSynchronizer->tick_num()->i(), 1);
			CRY_TEST_CHECK_EQUAL(player1InputsSynchronizer->inputs()->size(), 0);
		}
	});

	CPlayerInput pi2 = CPlayerInput{ Vec2{2.0f, 0.0f}, EInputFlag::MoveBackward };

	gsm.Update(0, 1.0f, pi2, &networkClient, gs);
}

void TestGameStateManager()
{
	TestGameStateManagerCreatesNoInputsForShortFrame();
	TestGameStateManagerSendsSingleInput();
	TestGameStateManagerCreatesMultipleInputs();
	TestGameStateManagerCreatesMultipleInputsWithMisalignedTick();
	TestGameStateManagerCreatesHandlesAck();
	TestGameStateManagerHandlesOtherPlayerInputs();
	TestGameStateManagerHandlesOtherPlayerInputsAcked();
	TestGameStateManagerHandlesMoreOtherPlayerInputsAfterAck();
}


void TestNetworkServerHandlesFirstPacket()
{	
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkServer networkServer = CNetworkServer(&testNetUdp);

	int sn = 1;

	testNetUdp.SetServerCallbacks(
		sn++, [](char* buff, int len, sockaddr_in* si_other) -> int
		{
			flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
			flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];
			
			const FlatBuffPacket::OInt tickNum = FlatBuffPacket::OInt(0);

			const FlatBuffPacket::PlayerInput playerInputs[] = { FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(1.0f, 0.0f), FlatBuffPacket::InputFlags_MoveBackward} };
			const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, 1);
			s[0] = CreatePlayerInputsSynchronizer(builder, &tickNum, inputsFlatBuff);

			for (int i = 1; i < NUM_PLAYERS; ++i)
			{
				const FlatBuffPacket::OInt noTickNum = FlatBuffPacket::OInt(OptInt().I);
				s[i] = FlatBuffPacket::CreatePlayerInputsSynchronizer(builder, &noTickNum, 0);
			}

			const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>> playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
			const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

			builder.Finish(serverToClientUpdate);
			const uint8_t* bufferPointer = builder.GetBufferPointer();

			len = builder.GetSize();
			memcpy(buff, bufferPointer, len);
			return len;
		}, [](char* buff, int len,
		      sockaddr_in* to) -> void
		{
			const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
			flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
			if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
			{
				const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
				const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
				CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 0);
				const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
				CRY_TEST_CHECK_EQUAL(playerInputs->size(), 0);
			}
		});
	networkServer.DoWork();
}

void TestNetworkServerHandlesFirstTwoPackets()
{
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkServer networkServer = CNetworkServer(&testNetUdp);

	int sn = 1;

	testNetUdp.SetServerCallbacks(
		sn++, [](char* buff, int len, sockaddr_in* si_other) -> int
	{
		flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];

		const FlatBuffPacket::OInt tickNum = FlatBuffPacket::OInt(0);

		const FlatBuffPacket::PlayerInput playerInputs[] = { FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(1.0f, 0.0f), FlatBuffPacket::InputFlags_MoveBackward} };
		const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, 1);
		s[0] = CreatePlayerInputsSynchronizer(builder, &tickNum, inputsFlatBuff);

		for (int i = 1; i < NUM_PLAYERS; ++i)
		{
			const FlatBuffPacket::OInt noTickNum = FlatBuffPacket::OInt(OptInt().I);
			s[i] = FlatBuffPacket::CreatePlayerInputsSynchronizer(builder, &noTickNum, 0);
		}

		const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>> playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
		const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

		builder.Finish(serverToClientUpdate);
		const uint8_t* bufferPointer = builder.GetBufferPointer();

		len = builder.GetSize();
		memcpy(buff, bufferPointer, len);
		return len;
	}, [](char* buff, int len,
		sockaddr_in* to) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 0);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 0);
		}
	});
	networkServer.DoWork();

	testNetUdp.SetServerCallbacks(
		(++sn)++, [](char* buff, int len, sockaddr_in* si_other) -> int
	{
		flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];

		const FlatBuffPacket::OInt tickNum = FlatBuffPacket::OInt(1);

		const FlatBuffPacket::PlayerInput playerInputs[] = {
			FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(1.0f, 0.0f), FlatBuffPacket::InputFlags_MoveBackward},
			FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(2.0f, 0.0f), FlatBuffPacket::InputFlags_MoveBackward}
		};
		const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, 1);
		s[0] = CreatePlayerInputsSynchronizer(builder, &tickNum, inputsFlatBuff);

		for (int i = 1; i < NUM_PLAYERS; ++i)
		{
			const FlatBuffPacket::OInt noTickNum = FlatBuffPacket::OInt(OptInt().I);
			s[i] = FlatBuffPacket::CreatePlayerInputsSynchronizer(builder, &noTickNum, 0);
		}

		const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>> playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
		const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

		builder.Finish(serverToClientUpdate);
		const uint8_t* bufferPointer = builder.GetBufferPointer();

		len = builder.GetSize();
		memcpy(buff, bufferPointer, len);
		return len;
	}, [](char* buff, int len,
		sockaddr_in* to) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 1);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 0);
		}
	});
	networkServer.DoWork();
}

void TestNetworkServerHandlesPacketsFromTwoPlayers()
{
	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkServer networkServer = CNetworkServer(&testNetUdp);

	int sn = 1;

	testNetUdp.SetServerCallbacks(
		sn++, [](char* buff, int len, sockaddr_in* si_other) -> int
	{
		flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];

		const FlatBuffPacket::OInt tickNum = FlatBuffPacket::OInt(0);

		const FlatBuffPacket::PlayerInput playerInputs[] = { FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(1.0f, 0.0f), FlatBuffPacket::InputFlags_MoveBackward} };
		const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, 1);
		s[0] = CreatePlayerInputsSynchronizer(builder, &tickNum, inputsFlatBuff);

		for (int i = 1; i < NUM_PLAYERS; ++i)
		{
			const FlatBuffPacket::OInt noTickNum = FlatBuffPacket::OInt(OptInt().I);
			s[i] = FlatBuffPacket::CreatePlayerInputsSynchronizer(builder, &noTickNum, 0);
		}

		const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>> playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
		const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

		builder.Finish(serverToClientUpdate);
		const uint8_t* bufferPointer = builder.GetBufferPointer();

		len = builder.GetSize();
		memcpy(buff, bufferPointer, len);
		return len;
	}, [](char* buff, int len,
		sockaddr_in* to) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type playerInputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(playerInputsSynchronizer->tick_num()->i(), 0);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* playerInputs = playerInputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(playerInputs->size(), 0);
		}
	});
	networkServer.DoWork();

	testNetUdp.SetServerCallbacks(
		(++sn)++, [](char* buff, int len, sockaddr_in* si_other) -> int
	{
		flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];

		const FlatBuffPacket::OInt tickNum = FlatBuffPacket::OInt(0);

		const FlatBuffPacket::PlayerInput playerInputs[] = {FlatBuffPacket::PlayerInput{FlatBuffPacket::V2(101.0f, 0.0f), FlatBuffPacket::InputFlags_MoveBackward}};
		const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, 1);
		s[1] = CreatePlayerInputsSynchronizer(builder, &tickNum, inputsFlatBuff);

		for (int i = 0; i < NUM_PLAYERS; ++i)
		{
			if(i == 1)
				continue;

			const FlatBuffPacket::OInt noTickNum = FlatBuffPacket::OInt(OptInt().I);
			s[i] = FlatBuffPacket::CreatePlayerInputsSynchronizer(builder, &noTickNum, 0);
		}

		const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>> playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
		const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate = CreateClientToServerUpdate(builder, 0, playerSynchronizers);

		builder.Finish(serverToClientUpdate);
		const uint8_t* bufferPointer = builder.GetBufferPointer();

		len = builder.GetSize();
		memcpy(buff, bufferPointer, len);
		return len;
	}, [](char* buff, int len,
		sockaddr_in* to) -> void
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);

			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type player0InputsSynchronizer = update->player_synchronizers()->Get(0);
			CRY_TEST_CHECK_EQUAL(player0InputsSynchronizer->tick_num()->i(), 0);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* player0Inputs = player0InputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(player0Inputs->size(), 1);
			CRY_TEST_CHECK_CLOSE(player0Inputs->Get(0)->mouse_delta().x(), 1.0f, 0.001f);

			const flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>::value_type player1InputsSynchronizer = update->player_synchronizers()->Get(1);
			CRY_TEST_CHECK_EQUAL(player1InputsSynchronizer->tick_num()->i(), 0);
			const flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>* player1Inputs = player1InputsSynchronizer->inputs();
			CRY_TEST_CHECK_EQUAL(player1Inputs->size(), 0);
		}
	});
	networkServer.DoWork();
}

void TestNetworkServer()
{
	TestNetworkServerHandlesFirstPacket();
	TestNetworkServerHandlesFirstTwoPackets();
	TestNetworkServerHandlesPacketsFromTwoPlayers();
}

#endif

CGamePlugin::~CGamePlugin()
{
	// Remove any registered listeners before 'this' becomes invalid
	if (gEnv->pGameFramework != nullptr)
	{
		gEnv->pGameFramework->RemoveNetworkedClientListener(*this);
	}

	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	if (gEnv->pSchematyc)
	{
		gEnv->pSchematyc->GetEnvRegistry().DeregisterPackage(GetCID());
	}
}

bool CGamePlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
#ifdef test
	//
	// Test1();
	// Test2();
	Test3();
	TestPlayerInputsSynchronizer();
	TestGameStateManager();
	TestNetworkServer();

	gEnv->pLog->FlushAndClose();
	gEnv->pSystem->Quit();
#endif

	EnableUpdate(EUpdateStep::MainUpdate, true);

	// Register for engine system events, in our case we need ESYSTEM_EVENT_GAME_POST_INIT to load the map
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this, "CGamePlugin");



	return true;
}

void CGamePlugin::TryInitialiseRollback()
{

	//CPlayerComponent* pLocalPlayerComponent;

	if (!m_pLocalPlayerComponent)
	{
		const IEntity* m_pPlayerEntity = gEnv->pEntitySystem->GetEntity(LOCAL_PLAYER_ENTITY_ID);

		if (!m_pPlayerEntity)
			return;

		CPlayerComponent* pLocalPlayerComponent = m_pPlayerEntity->GetComponent<CPlayerComponent>();
		m_pLocalPlayerComponent = pLocalPlayerComponent;

		if (!pLocalPlayerComponent)
			return;
	}

	if (!m_pLocalPlayerComponent->IsAlive() || m_lastUpdateTime.QuadPart == 0)
	{
		const LARGE_INTEGER startTime = m_pCNetworkClient->StartTime();
		if (startTime.QuadPart > 0)
		{
			m_lastUpdateTime = startTime;
		}

		return;
	}

	const char localPlayerNumber = m_pCNetworkClient->LocalPlayerNumber();

	if (localPlayerNumber == -1)
	{
		m_pLocalPlayerComponent->SetIsRollbackControlled(false);
		return;
	}
	
	m_pLocalPlayerComponent->SetIsRollbackControlled(true);

	for (int i = 0; i < NUM_PLAYERS; ++i)
	{
		if (i == localPlayerNumber)
		{
			m_rollbackPlayers[i] = m_pLocalPlayerComponent;

			continue;
		}

		// Connection received from a client, create a player entity and component
		SEntitySpawnParams spawnParams;
		spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

		// Set a unique name for the player entity
		const string playerName = string().Format("Player%" PRISIZE_T, m_players.size());
		spawnParams.sName = playerName;

		// // Set local player details
		// if (m_players.empty() && !gEnv->IsDedicated())
		// {
		// 	spawnParams.id = LOCAL_PLAYER_ENTITY_ID;
		// 	spawnParams.nFlags |= ENTITY_FLAG_LOCAL_PLAYER;
		// }

		// Spawn the player entity
		if (IEntity* pPlayerEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
		{
			// Create the player component instance

			m_rollbackPlayers[i] = pPlayerEntity->GetOrCreateComponentClass<CPlayerComponent>();
		}
	}

	m_rollbackInitialised = true;
}

void CGamePlugin::MainUpdate(float frameTime)
{
	if (!gEnv->IsGameOrSimulation() || gEnv->IsDedicated())
		return;

	if (!m_rollbackInitialised)
	{
		TryInitialiseRollback();
		return;
	}

	const char localPlayerNumber = m_pCNetworkClient->LocalPlayerNumber();
	m_gameStateManager.DoRollback(m_pCNetworkClient, localPlayerNumber);

	LARGE_INTEGER updateTime;
	QueryPerformanceCounter(&updateTime);

	if (m_lastUpdateTime.QuadPart > updateTime.QuadPart)
		return;

	//LARGE_INTEGER Frequency;
	//QueryPerformanceFrequency(&Frequency);

	constexpr long long frequency = 10000000;

	const float t = static_cast<float>((static_cast<double>(updateTime.QuadPart - m_lastUpdateTime.QuadPart) / static_cast<double>(frequency)));
	if (t < 0)
	{
		gEnv->pLog->LogToFile("Negative time step");
		return;
	}

	m_lastUpdateTime = updateTime;


	// CryLog("CGameStateManager.Update: t %f, ", t);

	const CPlayerInput playerInput = m_pLocalPlayerComponent->GetInput();

	CGameState gameState;
	if (!m_gameStateManager.Update(localPlayerNumber, t, playerInput, m_pCNetworkClient, gameState, m_delay))
		return;

	// m_pLocalPlayerComponent->SetState(gameState.players[localPlayerNumber]);
	for (int i = 0; i < NUM_PLAYERS; i++)
	{
		m_rollbackPlayers[i]->SetState(gameState.players[i]);
	}

	// IPersistantDebug* pPD = gEnv->pGameFramework->GetIPersistantDebug();	
	// pPD->Begin("p:localPlayerNumber" + (int)localPlayerNumber, true);
	//
	// pPD->AddText(10.0f, 10.0f, 1.2f, ColorF(1.f, 0.f, 0.f, 1.f), 60.f, 
	// 	"p:%i p[0]x:%f p[0]y:%f p[0]z:%f - p[1]x:%f p[1]y:%f p[1]z:%f", localPlayerNumber,
	// 	gameState.players[0].position.x, gameState.players[0].position.y, gameState.players[0].position.z,
	// 	gameState.players[1].position.x, gameState.players[1].position.y, gameState.players[1].position.z);
}

void CGamePlugin::OnAction(const ActionId& actionId, int activationMode, float value)
{
	const bool isInputPressed = (activationMode & eIS_Pressed) != 0;

	if (!isInputPressed)
		return;

	// Check if the triggered action
	if (actionId == m_increaseDelayActionId)
	{
		m_delay += 5;
	}
	else if (actionId == m_decreaseDelayActionId)
	{
		m_delay -= 5;
	}
	m_delay = clamp_tpl(m_delay, 0, MAX_TICKS_TO_TRANSMIT-1);
	IPersistantDebug* pPD = gEnv->pGameFramework->GetIPersistantDebug();
	pPD->Begin("delay", true);

	pPD->AddText(10.0f, 30.0f, 1.2f, ColorF(1.f, 1.f, 1.f, 1.f), 5.0f,
	             "Delay: %i (%fms)", m_delay, TICKS_DURATION * m_delay * 1000.0f);
}

void CGamePlugin::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
		// Called when the game framework has initialized and we are ready for game logic to start
	case ESYSTEM_EVENT_GAME_POST_INIT:
	{
		// Listen for client connection events, in order to create the local player
		gEnv->pGameFramework->AddNetworkedClientListener(*this);

		// Don't need to load the map in editor
		if (!gEnv->IsEditor())
		{
			IActionMapManager* pActionMapManager = gEnv->pGameFramework->GetIActionMapManager();
			IActionMap* pActionMap = pActionMapManager->CreateActionMap("system");
			pActionMapManager->AddExtraActionListener(this, "system");

			pActionMap->CreateAction(m_increaseDelayActionId);
			SActionInput increaseDelayInput;
			increaseDelayInput.inputDevice = eAID_KeyboardMouse;
			increaseDelayInput.input = increaseDelayInput.defaultInput = "up";
			increaseDelayInput.activationMode = eIS_Pressed;
			pActionMap->AddAndBindActionInput(m_increaseDelayActionId, increaseDelayInput);

			const ActionId decreaseDelayActionId("decreaseDelay");
			pActionMap->CreateAction(m_decreaseDelayActionId);
			pActionMap->CreateAction(decreaseDelayActionId);
			SActionInput decreaseDelayInput;
			decreaseDelayInput.inputDevice = eAID_KeyboardMouse;
			decreaseDelayInput.input = decreaseDelayInput.defaultInput = "down";
			decreaseDelayInput.activationMode = eIS_Pressed;
			pActionMap->AddAndBindActionInput(m_decreaseDelayActionId, decreaseDelayInput);

			pActionMap->Enable(true);

			// Load the example map in client server mode
			gEnv->pConsole->ExecuteString("map example s", false, true);

			// CNetworkServer* m_pCNetworkServer = new CNetworkServer();
			if (gEnv->IsDedicated())
			{

				gEnv->pLog->SetFileName("RollbackServerLog.log");

				CNetUdpServer* netUdpServer = new CNetUdpServer();
				m_pCNetworkServer = new CNetworkServer(netUdpServer);
				CThreadRunner* pThread = new CThreadRunner(m_pCNetworkServer);
				if (!gEnv->pThreadManager->SpawnThread(pThread, "CNetworkServerThread"))
				{
					// your failure handle code
					delete m_pCNetworkServer;
					// CryFatalError(...);
				}
			}
			else
			{
				CNetUdpClient* netUdpClient = new CNetUdpClient();
				m_pCNetworkClient = new CNetworkClient(netUdpClient);
				CThreadRunner* pThread = new CThreadRunner(m_pCNetworkClient);
				if (!gEnv->pThreadManager->SpawnThread(pThread, "CNetworkClientThread"))
				{
					// your failure handle code
					delete m_pCNetworkClient;
					// CryFatalError(...);
				}
			}
		}
	}
	break;

	case ESYSTEM_EVENT_REGISTER_SCHEMATYC_ENV:
	{
		// Register all components that belong to this plug-in
		auto staticAutoRegisterLambda = [](Schematyc::IEnvRegistrar& registrar)
		{
			// Call all static callback registered with the CRY_STATIC_AUTO_REGISTER_WITH_PARAM
			Detail::CStaticAutoRegistrar<Schematyc::IEnvRegistrar&>::InvokeStaticCallbacks(registrar);
		};

		if (gEnv->pSchematyc)
		{
			gEnv->pSchematyc->GetEnvRegistry().RegisterPackage(
				stl::make_unique<Schematyc::CEnvPackage>(
					GetCID(),
					"EntityComponents",
					"Crytek GmbH",
					"Components",
					staticAutoRegisterLambda
				)
			);
		}
	}
	break;

	case ESYSTEM_EVENT_LEVEL_UNLOAD:
	{
		m_players.clear();
	}
	break;
	}
}

bool CGamePlugin::OnClientConnectionReceived(int channelId, bool bIsReset)
{
	// Connection received from a client, create a player entity and component
	SEntitySpawnParams spawnParams;
	spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

	// Set a unique name for the player entity
	const string playerName = string().Format("Player%" PRISIZE_T, m_players.size());
	spawnParams.sName = playerName;

	// Set local player details
	if (m_players.empty() && !gEnv->IsDedicated())
	{
		spawnParams.id = LOCAL_PLAYER_ENTITY_ID;
		spawnParams.nFlags |= ENTITY_FLAG_LOCAL_PLAYER;
	}

	// Spawn the player entity
	if (IEntity* pPlayerEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
	{
		// Set the local player entity channel id, and bind it to the network so that it can support Multiplayer contexts
		pPlayerEntity->GetNetEntity()->SetChannelId(channelId);

		// Create the player component instance
		CPlayerComponent* pPlayer = pPlayerEntity->GetOrCreateComponentClass<CPlayerComponent>();

		if (spawnParams.id != LOCAL_PLAYER_ENTITY_ID)
			pPlayer->SetIsRollbackControlled(false);

		if (pPlayer != nullptr)
		{
			// Push the component into our map, with the channel id as the key
			m_players.emplace(std::make_pair(channelId, pPlayerEntity->GetId()));
		}
	}

	return true;
}

bool CGamePlugin::OnClientReadyForGameplay(int channelId, bool bIsReset)
{
	// Revive players when the network reports that the client is connected and ready for gameplay
	auto it = m_players.find(channelId);
	if (it != m_players.end())
	{
		if (IEntity* pPlayerEntity = gEnv->pEntitySystem->GetEntity(it->second))
		{
			if (CPlayerComponent* pPlayer = pPlayerEntity->GetComponent<CPlayerComponent>())
			{
				pPlayer->OnReadyForGameplayOnServer();
			}
		}
	}

	return true;
}

void CGamePlugin::OnClientDisconnected(int channelId, EDisconnectionCause cause, const char* description, bool bKeepClient)
{
	// Client disconnected, remove the entity and from map
	auto it = m_players.find(channelId);
	if (it != m_players.end())
	{
		gEnv->pEntitySystem->RemoveEntity(it->second);

		m_players.erase(it);
	}
}

void CGamePlugin::IterateOverPlayers(std::function<void(CPlayerComponent& player)> func) const
{
	for (const std::pair<int, EntityId>& playerPair : m_players)
	{
		if (IEntity* pPlayerEntity = gEnv->pEntitySystem->GetEntity(playerPair.second))
		{
			if (CPlayerComponent* pPlayer = pPlayerEntity->GetComponent<CPlayerComponent>())
			{
				func(*pPlayer);
			}
		}
	}
}

CRYREGISTER_SINGLETON_CLASS(CGamePlugin)
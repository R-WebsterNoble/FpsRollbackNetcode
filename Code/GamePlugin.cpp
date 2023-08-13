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

#include "Net/PlayerInputsSynchronizer.h"

#define test

#ifdef test

class CTestNetUdp : public CNetUdpClientInterface, public CNetUdpServerInterface
{
private:
	std::function<void(const char*, int)> m_clientSendCallback = nullptr;
	std::function<int(char* buff, int len)> m_clientReceiveCallback = nullptr;
	std::function<void(const char* buff, int len, sockaddr_in* to)> m_serverSendCallback = nullptr;
	std::function<int(char* buff, int len, sockaddr_in* si_other)> m_serverReceiveCallback = nullptr;
	int m_expectedNextCallback = 0;
	int m_expectedCallbackSequenceNumber = 0;
	int m_actualCallbackSequenceNumber = 1;

public:

	void Send(const char* buff, int len) override
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

	void Send(const char* buff, int len, sockaddr_in* to) override
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


	void SetClientSendCallback(int n, const std::function<void(const char*, int)>& callback)
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

	void SetServerCallbacks(int n, const std::function<int(char* buff, int len, sockaddr_in* si_other)>& receiveCallback, const std::function<void(const char* buff, int len, sockaddr_in* to)>& sendCallback)
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



void Test1()
{
	CGameState gs;

	CTestNetUdp testNetUdp = CTestNetUdp();

	CNetworkClient networkClient = CNetworkClient(&testNetUdp);

	CGameStateManager gameStateManager = CGameStateManager();

	testNetUdp.SetClientSendCallback(1, [](const char* buff, int len) -> void
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


	testNetUdp.SetClientSendCallback(4, [](const char* buff, int len) -> void
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

	CGameStateManager gameStateManager = CGameStateManager();

	testNetUdp.SetClientSendCallback(1, [](const char* buff, int len) -> void
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


	testNetUdp.SetClientSendCallback(4, [](const char* buff, int len) -> void
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


	testNetUdp.SetClientSendCallback(5, [](const char* buff, int len) -> void
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


	testNetUdp.SetClientSendCallback(8, [](const char* buff, int len) -> void
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


	CGameStateManager Client1gameStateManager = CGameStateManager();
	CGameStateManager Client2gameStateManager = CGameStateManager();
	ClientToServerUpdateBytesUnion u;
	char* client1TestPacketBuffer = u.buff;
	int client1TestPacketBufferLength = 0;

	char* client2TestPacketBuffer = u.buff;
	int client2TestPacketBufferLength = 0;


	int sn = 1;

	// NetworkServer: Receive: ClientToServerUpdate{ 't', 1, 1, 0, 0, { CPlayerInput{Vec2{100, 0}, EInputFlag::MoveForward } } };; int expectedLen = 24
	// NetworkServer: Sending: ServerToClientUpdate{ 'r', 0, 0, { 0  }, { 0  }, { } };; int expectedLen = 20

	// NetworkServer: Receive: ClientToServerUpdate{ 't', 0, 1, 0, 0, { CPlayerInput{Vec2{0, 0}, EInputFlag::None } } };; int expectedLen = 24
	// NetworkServer: Sending: ServerToClientUpdate{ 'r', 0, 0, { 0  }, { 0  }, { } };; int expectedLen = 20

	// NetworkServer: Receive: ClientToServerUpdate{ 't', 0, 1, 1, 0, { CPlayerInput{Vec2{0, 0}, EInputFlag::None } } };; int expectedLen = 24
	// NetworkServer: Sending: ServerToClientUpdate{ 'r', 1, 1, { -1  }, { 1  }, { } };; int expectedLen = 20

	// NetworkServer: Receive: ClientToServerUpdate{ 't', 1, 1, 1, 0, { CPlayerInput{Vec2{100, 0}, EInputFlag::MoveForward } } };; int expectedLen = 24
	// NetworkServer: Sending: ServerToClientUpdate{ 'r', 1, 1, { 0  }, { 1  }, { } };; int expectedLen = 20


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
		sn++, [&client1TestPacketBufferLength, &client1TestPacketBuffer](const char* buff, int len) -> void
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
	}, [&client1TestPacketBufferLength, &client1TestPacketBuffer](const char* buff, int len, sockaddr_in* to) -> void
	{
		client1TestPacketBufferLength = len;
		memcpy(client1TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();
	sn++;

	testNetUdp.SetClientSendCallback(
		sn++, [&client1TestPacketBufferLength, &client1TestPacketBuffer](const char* buff, int len) -> void
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
	}, [&client1TestPacketBufferLength, &client1TestPacketBuffer](const char* buff, int len, sockaddr_in* to) -> void
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
		sn++, [&client1TestPacketBufferLength, &client1TestPacketBuffer](const char* buff, int len) -> void
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
	}, [&client1TestPacketBufferLength, &client1TestPacketBuffer](const char* buff, int len, sockaddr_in* to) -> void
	{
		client1TestPacketBufferLength = len;
		memcpy(client1TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();

	testNetUdp.SetClientReceiveCallback(sn++, [&client1TestPacketBuffer, &client1TestPacketBufferLength](char* buff, int len) -> int
	{
		memcpy(buff, client1TestPacketBuffer, client1TestPacketBufferLength);
		return client1TestPacketBufferLength;
	});
	networkClient1.DoWork();

	testNetUdp.SetClientSendCallback(
		sn++, [&client2TestPacketBufferLength, &client2TestPacketBuffer](const char* buff, int len) -> void
	{
		client2TestPacketBufferLength = len;
		memcpy(client2TestPacketBuffer, buff, len);
	});
	pi.mouseDelta = Vec2(100.0f, 0.0f);
	pi.playerActions = EInputFlag::None;
	Client2gameStateManager.Update(0, t, pi, &networkClient2, gs);

	testNetUdp.SetServerCallbacks(
		sn++, [&client2TestPacketBuffer, &client2TestPacketBufferLength](char* buff, int len, sockaddr_in* si_other) -> int
	{
		memcpy(buff, client2TestPacketBuffer, client2TestPacketBufferLength);
		return client2TestPacketBufferLength;
	}, [&client2TestPacketBuffer, &client2TestPacketBufferLength](const char* buff, int len, sockaddr_in* to) -> void
	{
		client2TestPacketBufferLength = len;
		memcpy(client2TestPacketBuffer, buff, len);
	});
	networkServer.DoWork();



}

void TestPlayerInputsSynchronizer_HandlesEmptySendAndReceive()
{
	CPlayerInputsSynchronizerSender s = CPlayerInputsSynchronizerSender();
	CPlayerInputsSynchronizerReceiver r = CPlayerInputsSynchronizerReceiver();

	char buff[sizeof PlayerInputsSynchronizerClientSendPacket];
	size_t size;

	RingBuffer<CPlayerInput> resultsBuffer = RingBuffer<CPlayerInput>();

	CRY_TEST_ASSERT(s.GetPaket(buff, size));

	CRY_TEST_ASSERT(size == sizeof PlayerInputsSynchronizerClientSendPacket - (sizeof CPlayerInput * MAX_TICKS_TO_TRANSMIT));

	const OptInt result = r.LoadPaket(buff, size, resultsBuffer);

	CRY_TEST_ASSERT(result.I == OptInt().I);
}

void TestPlayerInputsSynchronizer_HandlesSingleSendAndReceive()
{
	CPlayerInputsSynchronizerSender s = CPlayerInputsSynchronizerSender();
	CPlayerInputsSynchronizerReceiver r = CPlayerInputsSynchronizerReceiver();

	char buff[sizeof PlayerInputsSynchronizerClientSendPacket];
	size_t size;

	RingBuffer<CPlayerInput> resultsBuffer = RingBuffer<CPlayerInput>();


	const CPlayerInput i = CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward };
	s.Enqueue(0, i);
	CRY_TEST_ASSERT(s.GetPaket(buff, size));

	CRY_TEST_ASSERT(size == sizeof PlayerInputsSynchronizerClientSendPacket - (sizeof CPlayerInput * (MAX_TICKS_TO_TRANSMIT - 1)));

	const OptInt result = r.LoadPaket(buff, size, resultsBuffer);

	CRY_TEST_ASSERT(result.I == 0);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(0)->mouseDelta.x == 0.1f);
}

void TestPlayerInputsSynchronizer_HandlesSingleSendWithMultipleInputsAndSingleReceive()
{
	CPlayerInputsSynchronizerSender s = CPlayerInputsSynchronizerSender();
	CPlayerInputsSynchronizerReceiver r = CPlayerInputsSynchronizerReceiver();

	char buff[sizeof PlayerInputsSynchronizerClientSendPacket];
	size_t size;

	RingBuffer<CPlayerInput> resultsBuffer = RingBuffer<CPlayerInput>();


	const CPlayerInput i = CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward };
	s.Enqueue(0, i);
	const CPlayerInput i2 = CPlayerInput{ {0.2f, 0.0f}, EInputFlag::MoveForward };
	s.Enqueue(1, i2);
	CRY_TEST_ASSERT(s.GetPaket(buff, size));

	CRY_TEST_ASSERT(size == sizeof PlayerInputsSynchronizerClientSendPacket - (sizeof CPlayerInput * (MAX_TICKS_TO_TRANSMIT - 2)));

	const OptInt result = r.LoadPaket(buff, size, resultsBuffer);

	CRY_TEST_ASSERT(result.I == 1);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(0)->mouseDelta.x == 0.1f);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(1)->mouseDelta.x == 0.2f);
}


void TestPlayerInputsSynchronizer_HandlesSingleSendAndMultipleReceive()
{
	CPlayerInputsSynchronizerSender s = CPlayerInputsSynchronizerSender();
	CPlayerInputsSynchronizerReceiver r = CPlayerInputsSynchronizerReceiver();

	char buff[sizeof PlayerInputsSynchronizerClientSendPacket];
	size_t size;

	RingBuffer<CPlayerInput> resultsBuffer = RingBuffer<CPlayerInput>();


	const CPlayerInput i = CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward };
	s.Enqueue(0, i);
	CRY_TEST_ASSERT(s.GetPaket(buff, size));

	CRY_TEST_ASSERT(size == sizeof PlayerInputsSynchronizerClientSendPacket - (sizeof CPlayerInput * (MAX_TICKS_TO_TRANSMIT - 1)));

	r.LoadPaket(buff, size, resultsBuffer);

	const OptInt result = r.LoadPaket(buff, size, resultsBuffer);

	CRY_TEST_ASSERT(result.I == 0);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(0)->mouseDelta.x == 0.1f);
}

void TestPlayerInputsSynchronizer_HandlesMultipleSendAndSingleReceive()
{
	CPlayerInputsSynchronizerSender s = CPlayerInputsSynchronizerSender();
	CPlayerInputsSynchronizerReceiver r = CPlayerInputsSynchronizerReceiver();

	char buff[sizeof PlayerInputsSynchronizerClientSendPacket];
	size_t size;

	RingBuffer<CPlayerInput> resultsBuffer = RingBuffer<CPlayerInput>();


	const CPlayerInput i = CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward };
	s.Enqueue(0, i);
	CRY_TEST_ASSERT(s.GetPaket(buff, size));

	CRY_TEST_ASSERT(size == sizeof PlayerInputsSynchronizerClientSendPacket - (sizeof CPlayerInput * (MAX_TICKS_TO_TRANSMIT - 1)));

	const CPlayerInput i2 = CPlayerInput{ {0.2f, 0.0f}, EInputFlag::MoveForward };
	s.Enqueue(1, i2);
	CRY_TEST_ASSERT(s.GetPaket(buff, size));

	CRY_TEST_ASSERT(size == sizeof PlayerInputsSynchronizerClientSendPacket - (sizeof CPlayerInput * (MAX_TICKS_TO_TRANSMIT - 2)));

	const OptInt result = r.LoadPaket(buff, size, resultsBuffer);
	CRY_TEST_ASSERT(result.I == 1);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(0)->mouseDelta.x == i.mouseDelta.x);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(2)->mouseDelta.x == i2.mouseDelta.x);
}

void TestPlayerInputsSynchronizer_HandlesMaxInputs()
{
	CPlayerInputsSynchronizerSender s = CPlayerInputsSynchronizerSender();
	CPlayerInputsSynchronizerReceiver r = CPlayerInputsSynchronizerReceiver();

	char buff[sizeof PlayerInputsSynchronizerClientSendPacket];
	size_t size;

	RingBuffer<CPlayerInput> resultsBuffer = RingBuffer<CPlayerInput>();

	for (int i = 0; i < MAX_TICKS_TO_TRANSMIT; ++i)
	{
		const CPlayerInput pi = CPlayerInput{ {1.0f * (1 + i), 0.0f}, EInputFlag::MoveForward };
		s.Enqueue(i, pi);
	}

	CRY_TEST_ASSERT(s.GetPaket(buff, size));

	CRY_TEST_ASSERT(size == sizeof PlayerInputsSynchronizerClientSendPacket);

	const OptInt result = r.LoadPaket(buff, size, resultsBuffer);
	CRY_TEST_ASSERT(result.I == MAX_TICKS_TO_TRANSMIT - 1);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(0)->mouseDelta.x == 1.0f * 1);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(MAX_TICKS_TO_TRANSMIT - 1)->mouseDelta.x == 1.0f * (MAX_TICKS_TO_TRANSMIT));
}

void TestPlayerInputsSynchronizer_SenderHandlesTooManyInputs()
{
	CPlayerInputsSynchronizerSender s = CPlayerInputsSynchronizerSender();
	CPlayerInputsSynchronizerReceiver r = CPlayerInputsSynchronizerReceiver();

	char buff[sizeof PlayerInputsSynchronizerClientSendPacket];
	size_t size;


	for (int i = 0; i <= MAX_TICKS_TO_TRANSMIT; ++i)
	{
		const CPlayerInput pi = CPlayerInput{ {1.0f * (1 + i), 0.0f}, EInputFlag::MoveForward };
		s.Enqueue(i, pi);
	}

	CRY_TEST_ASSERT(s.GetPaket(buff, size) == false);
}


void TestPlayerInputsSynchronizer_ReceiverHandlesTooLargePacket()
{
	CPlayerInputsSynchronizerReceiver r = CPlayerInputsSynchronizerReceiver();

	RingBuffer<CPlayerInput> resultsBuffer = RingBuffer<CPlayerInput>();

	PlayerInputsSynchronizerClientSendPacket p;
	PlayerInputsSynchronizerClientSendPacketBytesUnion* packet = reinterpret_cast<PlayerInputsSynchronizerClientSendPacketBytesUnion*>(&p);

	const OptInt result = r.LoadPaket(packet->buff, (sizeof PlayerInputsSynchronizerClientSendPacket)+1, resultsBuffer);

	CRY_TEST_ASSERT(result.has_value() == false);
}

void TestPlayerInputsSynchronizer_ReceiverHandlesTooManyTicks()
{
	CPlayerInputsSynchronizerReceiver r = CPlayerInputsSynchronizerReceiver();

	RingBuffer<CPlayerInput> resultsBuffer = RingBuffer<CPlayerInput>();

	PlayerInputsSynchronizerClientSendPacket p;
	p.TickNum.I = MAX_TICKS_TO_TRANSMIT + 1;
	p.TickCount = MAX_TICKS_TO_TRANSMIT + 1;	

	PlayerInputsSynchronizerClientSendPacketBytesUnion* packet = reinterpret_cast<PlayerInputsSynchronizerClientSendPacketBytesUnion*>(&p);

	const OptInt result = r.LoadPaket(packet->buff, sizeof PlayerInputsSynchronizerClientSendPacketBytesUnion, resultsBuffer);

	CRY_TEST_ASSERT(result.has_value() == false);
}

void TestPlayerInputsSynchronizer_ReceiverHandlesSendThenReceiveThenSend()
{
	CPlayerInputsSynchronizerSender s = CPlayerInputsSynchronizerSender();
	CPlayerInputsSynchronizerReceiver r = CPlayerInputsSynchronizerReceiver();

	char buff[sizeof PlayerInputsSynchronizerClientSendPacket];
	size_t size;

	RingBuffer<CPlayerInput> resultsBuffer = RingBuffer<CPlayerInput>();


	const CPlayerInput i = CPlayerInput{ {0.1f, 0.0f}, EInputFlag::MoveForward };
	s.Enqueue(0, i);
	CRY_TEST_ASSERT(s.GetPaket(buff, size));

	CRY_TEST_ASSERT(size == sizeof PlayerInputsSynchronizerClientSendPacket - (sizeof CPlayerInput * (MAX_TICKS_TO_TRANSMIT - 1)));

	const OptInt result = r.LoadPaket(buff, size, resultsBuffer);

	CRY_TEST_ASSERT(result.I == 0);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(0)->mouseDelta.x == 0.1f);

	s.Ack(result);

	const CPlayerInput i2 = CPlayerInput{ {0.2f, 0.0f}, EInputFlag::MoveForward };
	s.Enqueue(1, i2);
	CRY_TEST_ASSERT(s.GetPaket(buff, size));

	const OptInt result2 = r.LoadPaket(buff, size, resultsBuffer);

	CRY_TEST_ASSERT(result2.I == 1);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(0)->mouseDelta.x == 0.1f);
	CRY_TEST_ASSERT(resultsBuffer.GetAt(1)->mouseDelta.x == 0.2f);
}

void TestPlayerInputsSynchronizer()
{
	TestPlayerInputsSynchronizer_HandlesEmptySendAndReceive();
	TestPlayerInputsSynchronizer_HandlesSingleSendAndReceive();
	TestPlayerInputsSynchronizer_HandlesSingleSendWithMultipleInputsAndSingleReceive();
	TestPlayerInputsSynchronizer_HandlesSingleSendAndMultipleReceive();
	TestPlayerInputsSynchronizer_HandlesMultipleSendAndSingleReceive();
	TestPlayerInputsSynchronizer_HandlesMaxInputs();
	TestPlayerInputsSynchronizer_SenderHandlesTooManyInputs();
	TestPlayerInputsSynchronizer_ReceiverHandlesTooLargePacket();
	TestPlayerInputsSynchronizer_ReceiverHandlesTooManyTicks();
	TestPlayerInputsSynchronizer_ReceiverHandlesSendThenReceiveThenSend();
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
		gEnv->pSchematyc->GetEnvRegistry().DeregisterPackage(CGamePlugin::GetCID());
	}
}

bool CGamePlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
#ifdef test

	//Test1();
	//Test2();
	// Test3();
	TestPlayerInputsSynchronizer();

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

		// Set local player details
		if (m_players.empty() && !gEnv->IsDedicated())
		{
			spawnParams.id = LOCAL_PLAYER_ENTITY_ID;
			spawnParams.nFlags |= ENTITY_FLAG_LOCAL_PLAYER;
		}

		// Spawn the player entity
		if (IEntity* pPlayerEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
		{
			// Create the player component instance
			CPlayerComponent* pPlayer = pPlayerEntity->GetOrCreateComponentClass<CPlayerComponent>();

			m_rollbackPlayers[i] = pPlayer;
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

	m_gameStateManager.DoRollback(m_pCNetworkClient);

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
	const char localPlayerNumber = m_pCNetworkClient->LocalPlayerNumber();

	CGameState gameState;
	if (!m_gameStateManager.Update(localPlayerNumber, t, playerInput, m_pCNetworkClient, gameState))
		return;

	// m_pLocalPlayerComponent->SetState(gameState.players[localPlayerNumber]);
	for (int i = 0; i < NUM_PLAYERS; i++)
	{
		m_rollbackPlayers[i]->SetState(gameState.players[i]);
	}

	// CryLog("p:%i p[0]x:%f p[0]y:%f p[0]z:%f - p[1]x:%f p[1]y:%f p[1]z:%f", localPlayerNumber,
	//        gameState.players[0].position.x, gameState.players[0].position.y, gameState.players[0].position.z,
	//        gameState.players[1].position.x, gameState.players[1].position.y, gameState.players[1].position.z);
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
					CGamePlugin::GetCID(),
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
#pragma once

#include <ostream>

#include "InputFlag.h"
#include "StdAfx.h"

constexpr int NUM_PLAYERS = 2;

constexpr static int MAX_TICKS_TO_SEND = 128;//1280 * 10;

constexpr static int MAX_GAME_DURATION_TICKS = 127;//MAX_TICKS_TO_SEND * 60 * 60;

// enum EPlayerActionFlag : uint32
// {
// 	None = 0,
// 	MoveForward = 1 << 1,
// 	MoveBackward = 1 << 2,
// 	MoveLeft = 1 << 3,
// 	MoveRight = 1 << 4,
// };
//
// DEFINE_ENUM_FLAG_OPERATORS(EPlayerActionFlag)



struct CPlayerInput
{
	Vec2 mouseDelta;
	EInputFlag playerActions;
};

inline std::ostream& operator<<(std::ostream& out, CPlayerInput const& a)
{
	//CPlayerInput{ Vec2{0.1,0.1}, EInputFlag::MoveForward };
	out << "CPlayerInput{Vec2{" << a.mouseDelta.x << ", " << a.mouseDelta.y << "}, ";
	switch (a.playerActions)
	{
	case EInputFlag::None: out << "EInputFlag::None";  break;
	case EInputFlag::MoveLeft: out << "EInputFlag::MoveLeft";  break;
	case EInputFlag::MoveRight: out << "EInputFlag::MoveRight";  break;
	case EInputFlag::MoveForward: out << "EInputFlag::MoveForward";  break;
	case EInputFlag::MoveBackward: out << "EInputFlag::MoveBackward";  break;
	}
	out << " }";
	return out;
}

struct CPlayerState
{
	Vec3 position;
	Vec3 velocity;
	Vec2 rotation;
};


struct CGameState
{
	CPlayerState players[NUM_PLAYERS];
};


struct CTick
{
	CGameState gameState;
	CPlayerInput playerInputs[NUM_PLAYERS];
	int tickNum;
};


struct Start
{
    char packetTypeCode = 's';
	LARGE_INTEGER gameStartTimestamp;	
};

union StartBytesUnion
{
	StartBytesUnion() {  }
	char buff[sizeof(struct Start)];
	struct Start start;
};

struct ClientToServerUpdate
{
    char packetTypeCode;
	char playerNum;
	char tickCount;
    int tickNum;
	int ackServerUpdateNumber;
    CPlayerInput playerInputs[MAX_TICKS_TO_SEND];

};


inline std::ostream& operator<<(std::ostream& out, ClientToServerUpdate const& a) {
	//ClientToServerUpdate{ packetTypeCode = 'r', playerNum = 1, tickCount = 1, tickNum = 1, ackServerUpdateNumber = 1, playerInputs = {CPlayerInput{Vec2{0.1, 0.1}, EInputFlag::MoveForward}} };
	/*packetTypeCode*/
	out << "ClientToServerUpdate{ '"
		<< "/*packetTypeCode*/ " << a.packetTypeCode << "', "
		<< "/*playerNum*/ " << (int)a.playerNum << ", "
		<< "/*tickCount*/ " << (int)a.tickCount << ", "
		<< "/*tickNum*/ " << a.tickNum << ", "
		<< "/*ackServerUpdateNumber*/ " << a.ackServerUpdateNumber << ", "
		<< "{ ";
	for (int i = 0; i < a.tickCount; ++i)
	{
		out << a.playerInputs[i];// "CPlayerInput{Vec2{" << playerInputs[i].mouseDelta.x << ", " << playerInputs[i].mouseDelta.y << "}, " << playerInputs[i].playerActions;
		if (i < a.tickNum - 1)
			out << ", ";
		else
			out << " ";
	}
	out << "}};";
	// precise formatting depends on your use case
	return out;
}

union ClientToServerUpdateBytesUnion
{
	ClientToServerUpdateBytesUnion() {  }
	char buff[sizeof(ClientToServerUpdate)];
	ClientToServerUpdate ticks;
};

struct ServerToClientUpdate
{
	char packetTypeCode;
	int updateNumber;
	int ackClientTickNum;
	int playerInputsTickCounts[NUM_PLAYERS - 1];
	int playerInputsTickNums[NUM_PLAYERS - 1];
	CPlayerInput playerInputs[MAX_TICKS_TO_SEND * NUM_PLAYERS - 1];

};

inline std::ostream& operator<<(std::ostream& out, ServerToClientUpdate const& a) {
	//ServerToClientUpdate{packetTypeCode = 'r', updateNumber = 1, ackClientTickNum = 1, playerInputsTickCounts = {1},playerInputsTickNums = {1}, playerInputs = {CPlayerInput{Vec2{0.1, 0.1}, EInputFlag::MoveForward}}};

	out << "ServerToClientUpdate{ '"
		<< "/*packetTypeCode*/ " << a.packetTypeCode << "', "
		<< "/*updateNumber*/ " << a.updateNumber << ", "
		<< "/*ackClientTickNum*/ " << a.ackClientTickNum << ", "
		<< "{ ";
	int totalTicks = 0;
	for (int i = 0; i < NUM_PLAYERS - 1; ++i)
	{
		int playerInputsTickCount = a.playerInputsTickCounts[i];
		totalTicks += playerInputsTickCount;
		out << playerInputsTickCount << " ";
		if (i < NUM_PLAYERS - 2)
			out << ", ";
		else
			out << " ";
	}
	out << "}, ";
	out << "{ ";
	for (int i = 0; i < NUM_PLAYERS - 1; ++i)
	{
		out << a.playerInputsTickNums[i] << " ";
		if (i < NUM_PLAYERS - 2)
			out << ", ";
		else
			out << " ";
	}
	out << "}, ";
	out << "{ ";
	for (int i = 0; i < totalTicks; ++i)
	{
		out << a.playerInputs[i];// "CPlayerInput{Vec2{" << playerInputs[i].mouseDelta.x << ", " << playerInputs[i].mouseDelta.y << "}, " << playerInputs[i].playerActions;
		if (i < totalTicks - 1)
			out << ", ";
		else
			out << " ";
	}
	out << "}};";
	// precise formatting depends on your use case
	return out;
}



union ServerToClientUpdateBytesUnion
{
	ServerToClientUpdateBytesUnion() {  }
    char buff[sizeof(ServerToClientUpdate)];
	ServerToClientUpdate ticks;
};


template <typename T> class RingBuffer
{
	static constexpr int buffer_capacity = 64000;
	T m_buffer[MAX_TICKS_TO_SEND];
	int m_bufferHead = 0;

public:

	T* GetAt(int index)
	{
		return &m_buffer[index % buffer_capacity];
	}

	T& Insert(T* elemets, int count)
	{
		for (int i = 0; i < count; ++i)
		{
			Rotate();
			m_buffer[m_bufferHead] = elemets[i];			
		}
	}

	T* PeakHead()
	{
		return &m_buffer[m_bufferHead];
	}

	T* PeakNext()
	{
		int next = m_bufferHead + 1;

		if (next == buffer_capacity)
			next -= buffer_capacity;

		return &m_buffer[next];
	}

	void Rotate()
	{
		m_bufferHead++;

		if (m_bufferHead == buffer_capacity)
			m_bufferHead = 0;
	}
};
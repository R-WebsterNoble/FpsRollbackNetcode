#pragma once

#include "InputFlag.h"
#include "StdAfx.h"

constexpr int NUM_PLAYERS = 2;

constexpr static int MAX_TICKS_TO_SEND = 127;

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

struct ClientToServerUpdate
{
    char packetTypeCode;
	char playerNum;
	char tickCount;
    int lastTickNum;
	int ackServerUpdateNumber;
    CPlayerInput playerInputs[MAX_TICKS_TO_SEND];
};

struct ServerToClientUpdate
{
	char packetTypeCode;
	int updateNumber;
	int ackClientTickNum;
	int playerInputsCounts[NUM_PLAYERS - 1];
	int playerInputsTickNums[NUM_PLAYERS - 1];
	CPlayerInput playerInputs[MAX_TICKS_TO_SEND * NUM_PLAYERS - 1];
};

union ClientToServerUpdateBytesUnion
{
	ClientToServerUpdateBytesUnion() {  }
	char buff[sizeof(ClientToServerUpdate)];
	ClientToServerUpdate ticks;
};

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
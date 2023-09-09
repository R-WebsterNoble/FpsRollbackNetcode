#pragma once

#include <ostream>

#include "InputFlag.h"
#include "StdAfx.h"
#include "lib/optional.h"

constexpr int NUM_PLAYERS = 2;

constexpr static int MAX_TICKS_TO_TRANSMIT = 128*2;// 1280 * 10;

constexpr static int MAX_GAME_DURATION_TICKS = MAX_TICKS_TO_TRANSMIT * 60 * 60;


constexpr static int TICKS_PER_SECOND = 128;
constexpr static float TICKS_DURATION = 1.0f/TICKS_PER_SECOND;

constexpr static int BUFFER_SIZE = 10000;

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

struct OptInt {

	int I;

	OptInt()
	{
		I = INT_MIN;
	}

	explicit OptInt(int i) : I(i){}


	int value_or(int v) const{
		if (has_value())			
			return I;
		return v;
	}

	bool has_value() const{
		return I != INT_MIN;
	}

	int value() const{
		if(!has_value())
			CryFatalError("OptInt uninitialized");
		return I;
	}
};

struct AtomicOptInt {

	std::atomic<int> I;

	AtomicOptInt()
	{
		I = INT_MIN;
	}

	explicit AtomicOptInt(int i) : I(0) {}


	int value_or(int v) const {
		if (has_value())
			return I;
		return v;
	}

	bool has_value() const {
		return I != INT_MIN;
	}

	int value() const {
		if (!has_value())
			CryFatalError("OptInt uninitialized");
		return I;
	}
};


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

inline std::ostream& operator<<(std::ostream& out, OptInt const& rhs)
{
	out << rhs.value_or(-99);
	return out;
};


template <typename T> class RingBuffer
{
	static constexpr int buffer_capacity = MAX_TICKS_TO_TRANSMIT;
	T m_buffer[MAX_TICKS_TO_TRANSMIT];
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

struct STickInput
{
	int playerNum;
	int tickNum;
	std::vector<CPlayerInput> inputs;
};

// namespace FlatBuffPacket
// {
// 	struct OInt;
// }
//
// namespace flatbuffers
// {
// 	inline FlatBuffPacket::OInt Pack(OptInt optInt)
// 	{
// 		return FlatBuffPacket::OInt(optInt.I);
// 	}
//
// 	inline OptInt UnPack(FlatBuffPacket::OInt oInt)
// 	{
// 		return OptInt(oInt.i());
// 	}
// }
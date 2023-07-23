#pragma once

#include "InputFlag.h"
#include "StdAfx.h"

constexpr int NUM_PLAYERS = 10;

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

constexpr static int MAX_TICKS_TO_SEND = 100;

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

struct TicksToSend
{
    char packetTypeCode;
    int playerNum;
    int tickNum;
    int tickCount;
    CPlayerInput playerInputs[MAX_TICKS_TO_SEND];
};

union TickBytesUnion
{
    TickBytesUnion() {  }
    char buff[sizeof(TicksToSend)];
    TicksToSend ticks;
};

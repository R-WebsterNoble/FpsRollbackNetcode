#pragma once

#include "StdAfx.h"

constexpr int NUM_PLAYERS = 10;

enum EPlayerActionFlag : uint32
{
	None = 0,
	MoveForward = 1 << 1,
	MoveBackward = 1 << 2,
	MoveLeft = 1 << 3,
	MoveRight = 1 << 4,
};

DEFINE_ENUM_FLAG_OPERATORS(EPlayerActionFlag)


struct CPlayerInput
{
	Vec2 mouseDelta;
	EPlayerActionFlag playerActions;
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

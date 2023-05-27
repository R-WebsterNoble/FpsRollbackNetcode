#pragma once
#include "GameState.h"


class CSimulation final
{
public:
	static void Next(const float dt, const CGameState& pPrevious, const CPlayerInput playerInputs[], CGameState& pNext);
	static void Next(const float dt, const CPlayerState& pPrevious, const CPlayerInput& playerInput, CPlayerState& pNext);

};
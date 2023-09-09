#pragma once

#include "GameState.h"
#include "NetworkClient.h"
#include "Components/Player.h"

class CGameStateManagerInterface
{
	virtual bool Update(char playerNumber, const float frameTime, CPlayerInput playerInput,
	                    CNetworkClient* pNetworkClient, OUT CGameState& outGameState, int delay) = 0;
};

class CGameStateManager : CGameStateManagerInterface
{
public:

	CGameStateManager(float ticksPerSecond)
	{
		constexpr float MAX_ROLLBACK_TIME = 500.0f;

		m_tickDuration = 1.0f / ticksPerSecond;
		m_maxRollbackTime = MAX_ROLLBACK_TIME;

		m_maxRollbackTicks = static_cast<int>(floor(MAX_ROLLBACK_TIME / m_tickDuration));

		CTick* latest = m_gamesStates.PeakHead();


		for (int i = 0; i < NUM_PLAYERS; ++i)
		{
			CPlayerState& pPlayerState = latest->gameState.players[i];
			pPlayerState.position = Vec3(66.0f + (i * 3.0f), 49.0f , 32.0161781f);
			// pPlayerState.position = Vec3(66.1380920f, 49.5188217f, 32.0161781f);
			pPlayerState.rotation = ZERO;//Vec3(gf_PI * 0.5f, 0.0f, 0.0f);
			pPlayerState.velocity = ZERO;

			CPlayerInput& pPlayerInput = latest->playerInputs[i];
			pPlayerInput.mouseDelta = ZERO;
			pPlayerInput.playerActions = EInputFlag::None;
		}

		latest->tickNum = 0;

		m_inputAccumulator.mouseDelta = ZERO;
		m_inputAccumulator.playerActions = EInputFlag::None;
	}

	bool Update(char playerNumber, const float frameTime, CPlayerInput playerInput,
	            CNetworkClient* pNetworkClient, OUT CGameState& outGameState, int delay) override;

	void DoRollback(CNetworkClient* pNetworkClient, int clientPlayerNum);
	

private:	

	float m_tickDuration;
	float m_maxRollbackTime;
	int m_maxRollbackTicks;
	int m_tickNum = 0;

	float m_timeRemainingAfterProcessingFixedTicks = 0;	

	CPlayerInput m_inputAccumulator;

	RingBuffer<CTick> m_gamesStates;
};

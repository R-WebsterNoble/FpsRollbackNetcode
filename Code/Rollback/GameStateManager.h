#pragma once

#include "GameState.h"
#include "NetworkClient.h"
#include "Components/Player.h"

class CGameStateManager
{
public:

	CGameStateManager()
	{
		constexpr float TICKS_PER_SECOND = 128.0f;
		constexpr float MAX_ROLLBACK_TIME = 500.0f;

		m_tickDuration = 1.0f / TICKS_PER_SECOND;
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
	
	void Update(char playerNumber, const float frameTime, CPlayerComponent* pLocalPlayer, CNetworkClient* pNetworkClient);
	

private:

	struct CGameStateRingBuffer
	{
		static constexpr int buffer_capacity = 64000;
		CTick m_buffer[buffer_capacity]; //28416000 bytes = 28.42 megabytes
		int m_bufferHead = 0;

		CTick* PeakHead()
		{
			return &m_buffer[m_bufferHead];
		}

		CTick* PeakNext()
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
				m_bufferHead -= buffer_capacity;
		}
	};
	

	float m_tickDuration;
	float m_maxRollbackTime;
	int m_maxRollbackTicks;
	int m_tickNum = 0;

	float m_timeRemainingAfterProcessingFixedTicks = 0;

	CPlayerInput m_inputAccumulator;

	CGameStateRingBuffer m_gamesStates;
};

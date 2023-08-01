#include "GameStateManager.h"


#include "Simulation.h"


CPlayerState CGameStateManager::Update(char playerNumber, const float frameTime, CPlayerInput localPayerInput,
                                       CNetworkClient* pNetworkClient)
{
	if (m_tickNum > MAX_GAME_DURATION_TICKS)
		return Null();


if (frameTime > 1.0f)
{
	CryLog("CGameStateManager.Update: skipped slow frame! t = %f, ", frameTime);
	return Null();
}

	const CTick* last = m_gamesStates.PeakHead();
	CTick* next = m_gamesStates.PeakNext();

	CPlayerInput& playerInput = next->playerInputs[playerNumber];

	playerInput = localPayerInput;

playerInput.mouseDelta.x = (float)(playerNumber * 100 + m_tickNum);

	m_timeRemainingAfterProcessingFixedTicks += frameTime;
	m_inputAccumulator.mouseDelta += playerInput.mouseDelta;
	playerInput.mouseDelta = m_inputAccumulator.mouseDelta;
	
	m_inputAccumulator.playerActions |= playerInput.playerActions;
	playerInput.playerActions = m_inputAccumulator.playerActions;

	const float fTicksToProcess = floor(m_timeRemainingAfterProcessingFixedTicks / m_tickDuration);
	const int ticksToProcess = static_cast<int>(fTicksToProcess);

	// if(ticksToProcess == 0)
	// 	CryLog("CGameStateManager.Update: No NewTicks: Tick %i, t %d, ", m_tickNum, frameTime);

	if (ticksToProcess >= 1)
	{
		const float ticksRemaining = remainderf(m_timeRemainingAfterProcessingFixedTicks, fTicksToProcess);
		const float remainderFraction = ticksRemaining / (fTicksToProcess + ticksRemaining);

		const Vec2 mouseDeltaReminder = playerInput.mouseDelta * remainderFraction;
		const Vec2 mouseDeltaPerTick = (playerInput.mouseDelta - mouseDeltaReminder) / fTicksToProcess;

		playerInput.mouseDelta = mouseDeltaPerTick;

		for (int i = 0; i < ticksToProcess; i++)
		{
playerInput.mouseDelta.x = (float)(playerNumber * 100 + m_tickNum);

			// CryLog("CGameStateManager.Update: SendTicks Tick %i, t %d, ", m_tickNum, frameTime);
			//if(m_tickNum % 10 == 0)
				// CryLog("CGameStateManager.Update: SendTicks Tick %i, t %f, ", m_tickNum, frameTime);
			pNetworkClient->EnqueueTick(m_tickNum, playerInput);

			CSimulation::Next(m_tickDuration, last->gameState, next->playerInputs, next->gameState);

			next->tickNum = m_tickNum++;			

			m_timeRemainingAfterProcessingFixedTicks -= m_tickDuration;
			const CTick* previous = last;
			last = next;

			m_gamesStates.Rotate();

			next = m_gamesStates.PeakNext();

			if (i < ticksToProcess - 1)
				std::memcpy(next->playerInputs, previous->playerInputs, sizeof(previous->playerInputs));

		}

		m_inputAccumulator.mouseDelta = ZERO;
		m_inputAccumulator.playerActions = EInputFlag::None;

		next->playerInputs[playerNumber].mouseDelta = mouseDeltaReminder;
	}

	if(ticksToProcess > 0)
		pNetworkClient->SendTicks(m_tickNum-1);

	CGameState gameState;
	CSimulation::Next(m_timeRemainingAfterProcessingFixedTicks, last->gameState, next->playerInputs, gameState);

	return gameState.players[playerNumber];
}


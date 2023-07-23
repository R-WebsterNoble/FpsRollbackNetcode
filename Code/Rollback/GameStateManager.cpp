#include "GameStateManager.h"


#include "Simulation.h"


void CGameStateManager::Update(char playerNumber, const float frameTime, CPlayerComponent* pLocalPlayer, CNetworkClient* pNetworkClient)
{
	const CTick* last = m_gamesStates.PeakHead();
	CTick* next = m_gamesStates.PeakNext();

	CPlayerInput& playerInput = next->playerInputs[playerNumber];

	pLocalPlayer->GetInput(playerInput);

	playerInput.mouseDelta.x = (float)m_tickNum;

	m_timeRemainingAfterProcessingFixedTicks += frameTime;
	m_inputAccumulator.mouseDelta += playerInput.mouseDelta;
	playerInput.mouseDelta = m_inputAccumulator.mouseDelta;
	
	m_inputAccumulator.playerActions |= playerInput.playerActions;
	playerInput.playerActions = m_inputAccumulator.playerActions;

	const float fTicksToProcess = floor(m_timeRemainingAfterProcessingFixedTicks / m_tickDuration);
	const int ticksToProcess = static_cast<int>(fTicksToProcess);

	if (ticksToProcess >= 1)
	{
		const float ticksRemaining = remainderf(m_timeRemainingAfterProcessingFixedTicks, fTicksToProcess);
		const float remainderFraction = ticksRemaining / (fTicksToProcess + ticksRemaining);

		const Vec2 mouseDeltaReminder = playerInput.mouseDelta * remainderFraction;
		const Vec2 mouseDeltaPerTick = (playerInput.mouseDelta - mouseDeltaReminder) / fTicksToProcess;

		playerInput.mouseDelta = mouseDeltaPerTick;

		for (int i = 0; i < ticksToProcess; i++)
		{
			playerInput.mouseDelta.x = (float)m_tickNum;
			pNetworkClient->SendTick(m_tickNum, playerInput);

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

	CGameState gameState;
	CSimulation::Next(m_timeRemainingAfterProcessingFixedTicks, last->gameState, next->playerInputs, gameState);

	pLocalPlayer->SetState(gameState.players[playerNumber]);
}


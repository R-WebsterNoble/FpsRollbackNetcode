#include "GameStateManager.h"


#include "Simulation.h"


bool CGameStateManager::Update(char playerNumber, const float frameTime, CPlayerInput localPayerInput,
                               CNetworkClient* pNetworkClient, OUT CGameState& outGameState)
{
	if (m_tickNum > MAX_GAME_DURATION_TICKS)
		return false;

if (frameTime > 1.0f)
{
	CryLog("CGameStateManager.Update: skipped slow frame! t = %f, ", frameTime);

	return false;
}

	const CTick* last = m_gamesStates.PeakHead();
	CTick* next = m_gamesStates.PeakNext();

	// CPlayerInput& playerInput = next->playerInputs[playerNumber];

if (playerNumber == 1)
{
	localPayerInput.mouseDelta.x = (float)(playerNumber * 100 + (m_tickNum * 0.0001f));
	localPayerInput.playerActions = EInputFlag::MoveForward;
}

	m_timeRemainingAfterProcessingFixedTicks += frameTime;
	m_inputAccumulator.mouseDelta += localPayerInput.mouseDelta;
	localPayerInput.mouseDelta = m_inputAccumulator.mouseDelta;
	
	m_inputAccumulator.playerActions |= localPayerInput.playerActions;
	localPayerInput.playerActions = m_inputAccumulator.playerActions;

	const float fTicksToProcess = floor(m_timeRemainingAfterProcessingFixedTicks / m_tickDuration);
	const int ticksToProcess = static_cast<int>(fTicksToProcess);

	// if(ticksToProcess == 0)
	// 	CryLog("CGameStateManager.Update: No NewTicks: Tick %i, t %d, ", m_tickNum, frameTime);

	if (ticksToProcess >= 1)
	{
		const float ticksRemaining = remainderf(m_timeRemainingAfterProcessingFixedTicks, fTicksToProcess);
		const float remainderFraction = ticksRemaining / (fTicksToProcess + ticksRemaining);

		const Vec2 mouseDeltaReminder = localPayerInput.mouseDelta * remainderFraction;
		const Vec2 mouseDeltaPerTick = (localPayerInput.mouseDelta - mouseDeltaReminder) / fTicksToProcess;

		localPayerInput.mouseDelta = mouseDeltaPerTick;

		for (int i = 0; i < ticksToProcess; i++)
		{
if (playerNumber == 1)
{
	localPayerInput.mouseDelta.x = (float)(playerNumber * 100 + (m_tickNum * 0.0001f));
	localPayerInput.playerActions = EInputFlag::MoveForward;
}

			// CryLog("CGameStateManager.Update: SendTicks Tick %i, t %d, ", m_tickNum, frameTime);
			//if(m_tickNum % 10 == 0)
				// CryLog("CGameStateManager.Update: SendTicks Tick %i, t %f, ", m_tickNum, frameTime);
			pNetworkClient->EnqueueTick(m_tickNum, localPayerInput);


			std::memcpy(next->playerInputs, last->playerInputs, sizeof(next->playerInputs));
			next->playerInputs[playerNumber] = localPayerInput;

			CSimulation::Next(m_tickDuration, last->gameState, next->playerInputs, next->gameState);

			next->tickNum = m_tickNum++;			

			m_timeRemainingAfterProcessingFixedTicks -= m_tickDuration;

			last = next;

			m_gamesStates.Rotate();

			next = m_gamesStates.PeakNext();
		}

		m_inputAccumulator.mouseDelta = ZERO;
		m_inputAccumulator.playerActions = EInputFlag::None;

		std::memcpy(next->playerInputs, last->playerInputs, sizeof(next->playerInputs));
			next->playerInputs[playerNumber].mouseDelta = mouseDeltaReminder;
	}

	if(ticksToProcess > 0)
		pNetworkClient->SendTicks(m_tickNum-1);
	else	
		next->playerInputs[playerNumber] = localPayerInput;
		
	CSimulation::Next(m_timeRemainingAfterProcessingFixedTicks, last->gameState, next->playerInputs, outGameState);

	return true;
}

void CGameStateManager::DoRollback(CNetworkClient* pNetworkClient)
{
	STickInput update;
	int earliestUpdatedTick = INT_MAX;

	while (pNetworkClient->GetInputUpdates(update))
	{
		const int p = update.playerNum;

		if (update.tickNum < earliestUpdatedTick)
			earliestUpdatedTick = update.tickNum;

		if (m_latestPlayerTickNumbers[p] < update.tickNum)
		{
			m_gamesStates.GetAt(update.tickNum)->playerInputs[p] = update.inputs;
		}
	}

	if (earliestUpdatedTick == INT_MAX)
		return;

	for (int i = earliestUpdatedTick; i <= m_tickNum; ++i)
	{
		const CTick* previousTick = m_gamesStates.GetAt(i);
		CTick* nextTick = m_gamesStates.GetAt(i + 1);
		CSimulation::Next(m_tickDuration, previousTick->gameState, previousTick->playerInputs, nextTick->gameState);
	}
}


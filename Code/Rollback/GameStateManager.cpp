#include "GameStateManager.h"


#include "Simulation.h"


bool CGameStateManager::Update(char playerNumber, const float frameTime, CPlayerInput localPayerInput,
                               CNetworkClient* pNetworkClient, OUT CGameState& outGameState, int delay)
{
	if (m_tickNum > MAX_GAME_DURATION_TICKS)
		return false;

	if (frameTime > m_tickDuration * MAX_TICKS_TO_TRANSMIT || frameTime <= 0.0f)
	{
		CryLog("CGameStateManager.Update: skipped frame! t = %f, ", frameTime);

		return false;
	}

	const CTick* last = m_gamesStates.PeakHead();
	CTick* next = m_gamesStates.PeakNext();

	// CPlayerInput& playerInput = next->playerInputs[playerNumber];

 // if (playerNumber == 1)
 // {
 // 	localPayerInput.mouseDelta.x = (((float)(m_tickNum + 1))/100.0f) * 0.001f;
 // 	localPayerInput.playerActions = EInputFlag::MoveForward;
 // }
// else 
//if (playerNumber == 0)
//{
//	localPayerInput.mouseDelta.x = (((float)(m_tickNum + 1)) / 100.0f) * -0.001f;
//	localPayerInput.playerActions = EInputFlag::MoveBackward;
//}

	float fTotalTime = m_timeRemainingAfterProcessingFixedTicks + frameTime;
	m_inputAccumulator.mouseDelta += localPayerInput.mouseDelta;
	localPayerInput.mouseDelta = m_inputAccumulator.mouseDelta;
	
	m_inputAccumulator.playerActions |= localPayerInput.playerActions;
	localPayerInput.playerActions = m_inputAccumulator.playerActions;

	const float fTicksToProcess = floor(fTotalTime / m_tickDuration);
	const int ticksToProcess = static_cast<int>(fTicksToProcess);

	// if(ticksToProcess == 0)
	// 	CryLog("CGameStateManager.Update: No NewTicks: Tick %i, t %d, ", m_tickNum, frameTime);

	if (ticksToProcess > 0)
	{
		const float ticksRemaining = fTotalTime - (fTicksToProcess * m_tickDuration);
		const float remainderFraction = ticksRemaining / fTotalTime;

		const Vec2 mouseDeltaRemainder = localPayerInput.mouseDelta * remainderFraction;
		const Vec2 mouseDeltaPerTick = (localPayerInput.mouseDelta - mouseDeltaRemainder) / fTicksToProcess;

		localPayerInput.mouseDelta = mouseDeltaPerTick;

		for (int i = 0; i < ticksToProcess; i++)
		{
 // if (playerNumber == 1)
 // {
 // 	localPayerInput.mouseDelta.x = (((float)(m_tickNum + 1)) / 100.0f) * 0.001f;
 // 	localPayerInput.playerActions = EInputFlag::MoveForward;
 // }
// else 
//if (playerNumber == 0)
//{
//	localPayerInput.mouseDelta.x = (((float)(m_tickNum + 1)) / 100.0f) * -0.001f;
//	localPayerInput.playerActions = EInputFlag::MoveBackward;
//}

			// CryLog("CGameStateManager.Update: SendTicks Tick %i, t %d, ", m_tickNum, frameTime);
			//if(m_tickNum % 10 == 0)
				// CryLog("CGameStateManager.Update: SendTicks Tick %i, t %f, ", m_tickNum, frameTime);
			pNetworkClient->EnqueueTick(m_tickNum, localPayerInput);


			std::memcpy(next->playerInputs, last->playerInputs, sizeof(next->playerInputs));
			next->playerInputs[playerNumber] = localPayerInput;

			CSimulation::Next(m_tickDuration, last->gameState, next->playerInputs, next->gameState);

			next->tickNum = m_tickNum++;			

			fTotalTime -= m_tickDuration;

			last = next;

			m_gamesStates.Rotate();

			next = m_gamesStates.PeakNext();
		}

		m_inputAccumulator.mouseDelta = mouseDeltaRemainder;
		m_inputAccumulator.playerActions = EInputFlag::None;

		m_timeRemainingAfterProcessingFixedTicks = ticksRemaining;

		std::memcpy(next->playerInputs, last->playerInputs, sizeof(next->playerInputs));

		next->playerInputs[playerNumber].mouseDelta = mouseDeltaRemainder;
	}
	else
		m_timeRemainingAfterProcessingFixedTicks = fTotalTime;

	IGameFramework* pGameFramework = gEnv->pGameFramework;
	if (pGameFramework != nullptr)
	{
		IPersistantDebug* pPD = pGameFramework->GetIPersistantDebug();
		pPD->Begin("p:localPlayerNumber" + (int)playerNumber, true);

		pPD->AddText(10.0f, 10.0f, 1.2f, ColorF(1.f, 0.f, 0.f, 1.f), 60.f,
			"[0]: %s [1]: %s", ToString(next->playerInputs[0].playerActions).c_str(), ToString(next->playerInputs[1].playerActions).c_str());
	}

	if(ticksToProcess > 0)
		pNetworkClient->SendTicks();
	else	
		next->playerInputs[playerNumber] = localPayerInput;
		

	const int delayTickNum = max(m_tickNum - delay, 0);
	CTick delayTick = *m_gamesStates.GetAt(delayTickNum);
	
	delayTick.gameState.players[playerNumber] = last->gameState.players[playerNumber];
	delayTick.playerInputs[playerNumber] = last->playerInputs[playerNumber];

	CSimulation::Next(fTotalTime, delayTick.gameState, delayTick.playerInputs, outGameState);

	return true;
}

void CGameStateManager::DoRollback(CNetworkClient* pNetworkClient, int clientPlayerNum)
{
	STickInput update;
	int earliestUpdatedTick = INT_MAX;
	
	while (pNetworkClient->GetInputUpdates(update))
	{
		const int playerNum = update.playerNum;

		if(playerNum == clientPlayerNum)
			CryFatalError("CGameStateManager DoRollback() Attempted to overwrite local player's (#%i) inputs", clientPlayerNum);
	
		if (update.tickNum < earliestUpdatedTick)
			earliestUpdatedTick = update.tickNum;

		const int size = update.inputs.size();
		for (int i = 0; i < size; ++i)
		{
			const CPlayerInput playerInput = update.inputs[i];

			m_gamesStates.GetAt(update.tickNum + i)->playerInputs[playerNum] = playerInput;
		}
		const CPlayerInput lastInput = update.inputs[size - 1];
		for (int t = (update.tickNum + size); t <= m_tickNum; t++)
		{
			m_gamesStates.GetAt(t)->playerInputs[playerNum] = lastInput;
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


// Copyright 2016-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "GamePlugin.h"

#include "Components/Player.h"

#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/EnvPackage.h>
#include <CrySchematyc/Utils/SharedString.h>

#include <IGameObjectSystem.h>
#include <IGameObject.h>

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>


CGamePlugin::~CGamePlugin()
{
	// Remove any registered listeners before 'this' becomes invalid
	if (gEnv->pGameFramework != nullptr)
	{
		gEnv->pGameFramework->RemoveNetworkedClientListener(*this);
	}

	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	if (gEnv->pSchematyc)
	{
		gEnv->pSchematyc->GetEnvRegistry().DeregisterPackage(CGamePlugin::GetCID());
	}
}

bool CGamePlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	EnableUpdate(EUpdateStep::MainUpdate, true);

	// Register for engine system events, in our case we need ESYSTEM_EVENT_GAME_POST_INIT to load the map
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this, "CGamePlugin");

	return true;
}

void CGamePlugin::MainUpdate(float frameTime)
{
	if (!gEnv->IsGameOrSimulation() || gEnv->IsDedicated())
		return;	

	if (!m_pPlayerComponent)
	{
		const IEntity* m_pPlayerEntity = gEnv->pEntitySystem->GetEntity(LOCAL_PLAYER_ENTITY_ID);

		if (!m_pPlayerEntity)
			return;

		m_pPlayerComponent = m_pPlayerEntity->GetComponent<CPlayerComponent>();

		if (!m_pPlayerComponent)
			return;
	}	

	if (!m_pPlayerComponent->IsAlive() || m_lastUpdateTime.QuadPart == 0)
	{
		const LARGE_INTEGER startTime = m_pCNetworkClient->StartTime();
		if(startTime.QuadPart > 0)
		{
			m_lastUpdateTime = startTime;
		}

		return;
	}

	LARGE_INTEGER updateTime;
	QueryPerformanceCounter(&updateTime);

	if (m_lastUpdateTime.QuadPart > updateTime.QuadPart)
		return;

	//LARGE_INTEGER Frequency;
	//QueryPerformanceFrequency(&Frequency);

	constexpr long long frequency = 10000000;

	float t = static_cast<float>((static_cast<double>(updateTime.QuadPart - m_lastUpdateTime.QuadPart) / static_cast<double>(frequency)));

	m_lastUpdateTime = updateTime;


	// CryLog("CGameStateManager.Update: t %f, ", t);

	
	m_gameStateManager.Update(m_pCNetworkClient->PlayerNumber(), t, m_pPlayerComponent, m_pCNetworkClient);
}

void CGamePlugin::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
		// Called when the game framework has initialized and we are ready for game logic to start
	case ESYSTEM_EVENT_GAME_POST_INIT:
	{
		// Listen for client connection events, in order to create the local player
		gEnv->pGameFramework->AddNetworkedClientListener(*this);

		// Don't need to load the map in editor
		if (!gEnv->IsEditor())
		{
			// Load the example map in client server mode
			gEnv->pConsole->ExecuteString("map example s", false, true);

			// CNetworkServer* m_pCNetworkServer = new CNetworkServer();
			if (gEnv->IsDedicated())
			{
				if (!gEnv->pThreadManager->SpawnThread(m_pCNetworkServer, "CNetworkServerThread"))
				{
					// your failure handle code
					delete m_pCNetworkServer;
					// CryFatalError(...);
				}
			}
			else
			{
				if (!gEnv->pThreadManager->SpawnThread(m_pCNetworkClient, "CNetworkClientThread"))
				{
					// your failure handle code
					delete m_pCNetworkClient;
					// CryFatalError(...);
				}
			}
		}
	}
	break;

	case ESYSTEM_EVENT_REGISTER_SCHEMATYC_ENV:
	{
		// Register all components that belong to this plug-in
		auto staticAutoRegisterLambda = [](Schematyc::IEnvRegistrar& registrar)
		{
			// Call all static callback registered with the CRY_STATIC_AUTO_REGISTER_WITH_PARAM
			Detail::CStaticAutoRegistrar<Schematyc::IEnvRegistrar&>::InvokeStaticCallbacks(registrar);
		};

		if (gEnv->pSchematyc)
		{
			gEnv->pSchematyc->GetEnvRegistry().RegisterPackage(
				stl::make_unique<Schematyc::CEnvPackage>(
					CGamePlugin::GetCID(),
					"EntityComponents",
					"Crytek GmbH",
					"Components",
					staticAutoRegisterLambda
				)
			);
		}
	}
	break;

	case ESYSTEM_EVENT_LEVEL_UNLOAD:
	{
		m_players.clear();
	}
	break;
	}
}

bool CGamePlugin::OnClientConnectionReceived(int channelId, bool bIsReset)
{
	// Connection received from a client, create a player entity and component
	SEntitySpawnParams spawnParams;
	spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

	// Set a unique name for the player entity
	const string playerName = string().Format("Player%" PRISIZE_T, m_players.size());
	spawnParams.sName = playerName;

	// Set local player details
	if (m_players.empty() && !gEnv->IsDedicated())
	{
		spawnParams.id = LOCAL_PLAYER_ENTITY_ID;
		spawnParams.nFlags |= ENTITY_FLAG_LOCAL_PLAYER;
	}

	// Spawn the player entity
	if (IEntity* pPlayerEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
	{
		// Set the local player entity channel id, and bind it to the network so that it can support Multiplayer contexts
		pPlayerEntity->GetNetEntity()->SetChannelId(channelId);

		// Create the player component instance
		CPlayerComponent* pPlayer = pPlayerEntity->GetOrCreateComponentClass<CPlayerComponent>();

		if (pPlayer != nullptr)
		{
			// Push the component into our map, with the channel id as the key
			m_players.emplace(std::make_pair(channelId, pPlayerEntity->GetId()));
		}
	}

	return true;
}

bool CGamePlugin::OnClientReadyForGameplay(int channelId, bool bIsReset)
{
	// Revive players when the network reports that the client is connected and ready for gameplay
	auto it = m_players.find(channelId);
	if (it != m_players.end())
	{
		if (IEntity* pPlayerEntity = gEnv->pEntitySystem->GetEntity(it->second))
		{
			if (CPlayerComponent* pPlayer = pPlayerEntity->GetComponent<CPlayerComponent>())
			{
				pPlayer->OnReadyForGameplayOnServer();
			}
		}
	}

	return true;
}

void CGamePlugin::OnClientDisconnected(int channelId, EDisconnectionCause cause, const char* description, bool bKeepClient)
{
	// Client disconnected, remove the entity and from map
	auto it = m_players.find(channelId);
	if (it != m_players.end())
	{
		gEnv->pEntitySystem->RemoveEntity(it->second);

		m_players.erase(it);
	}
}

void CGamePlugin::IterateOverPlayers(std::function<void(CPlayerComponent& player)> func) const
{
	for (const std::pair<int, EntityId>& playerPair : m_players)
	{
		if (IEntity* pPlayerEntity = gEnv->pEntitySystem->GetEntity(playerPair.second))
		{
			if (CPlayerComponent* pPlayer = pPlayerEntity->GetComponent<CPlayerComponent>())
			{
				func(*pPlayer);
			}
		}
	}
}

CRYREGISTER_SINGLETON_CLASS(CGamePlugin)
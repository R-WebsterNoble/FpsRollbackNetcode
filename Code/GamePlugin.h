// Copyright 2016-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <CrySystem/ICryPlugin.h>
#include <CryEntitySystem/IEntityClass.h>
#include <CryNetwork/INetwork.h>

#include "Rollback/GameStateManager.h"

#include "NetworkServer.h"
#include "NetworkClient.h"

class CPlayerComponent;

// The entry-point of the application
// An instance of CGamePlugin is automatically created when the library is loaded
// We then construct the local player entity and CPlayerComponent instance when OnClientConnectionReceived is first called.
class CGamePlugin 
	: public Cry::IEnginePlugin
	, public ISystemEventListener
	, public INetworkedClientListener
	, public IActionListener
{
public:
	CRYINTERFACE_SIMPLE(Cry::IEnginePlugin)
	CRYGENERATE_SINGLETONCLASS_GUID(CGamePlugin, "FirstPersonShooter", "{FC9BD884-49DE-4494-9D64-191734BBB7E3}"_cry_guid)

	virtual ~CGamePlugin() override;
	
	// Cry::IEnginePlugin
	virtual const char* GetCategory() const override { return "Game"; }
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
	void TryInitialiseRollback();
	// ~Cry::IEnginePlugin
		
	virtual void MainUpdate(float frameTime) override;

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

	// INetworkedClientListener
	// Sent to the local client on disconnect
	virtual void OnLocalClientDisconnected(EDisconnectionCause cause, const char* description) override {}

	// Sent to the server when a new client has started connecting
	// Return false to disallow the connection
	virtual bool OnClientConnectionReceived(int channelId, bool bIsReset) override;
	// Sent to the server when a new client has finished connecting and is ready for gameplay
	// Return false to disallow the connection and kick the player
	virtual bool OnClientReadyForGameplay(int channelId, bool bIsReset) override;
	// Sent to the server when a client is disconnected
	virtual void OnClientDisconnected(int channelId, EDisconnectionCause cause, const char* description, bool bKeepClient) override;
	// Sent to the server when a client is timing out (no packets for X seconds)
	// Return true to allow disconnection, otherwise false to keep client.
	virtual bool OnClientTimingOut(int channelId, EDisconnectionCause cause, const char* description) override { return true; }
	// ~INetworkedClientListener

	// Helper function to call the specified callback for every player in the game
	void IterateOverPlayers(std::function<void(CPlayerComponent& player)> func) const;

	virtual void OnAction(const ActionId& actionId, int activationMode, float value) override;

	// Helper function to get the CGamePlugin instance
	// Note that CGamePlugin is declared as a singleton, so the CreateClassInstance will always return the same pointer
	static CGamePlugin* GetInstance()
	{
		return cryinterface_cast<CGamePlugin>(CGamePlugin::s_factory.CreateClassInstance().get());
	}	

protected:
	// Map containing player components, key is the channel id received in OnClientConnectionReceived
	std::unordered_map<int, EntityId> m_players;

	CPlayerComponent* m_pLocalPlayerComponent;

	LARGE_INTEGER m_lastUpdateTime;	

	CGameStateManager m_gameStateManager = CGameStateManager(128.0f);
	CNetworkServer* m_pCNetworkServer;
	CNetworkClient* m_pCNetworkClient;

	CPlayerComponent* m_rollbackPlayers[NUM_PLAYERS];
	CPlayerComponent* m_resycSmoothedPlayers[NUM_PLAYERS];
	CPlayerState m_resycSmoothedPlayerStates[NUM_PLAYERS];

	bool m_rollbackInitialised = false;

	int m_delay = 0;

	const ActionId m_increaseDelayActionId = ActionId("increaseDelay");
	const ActionId m_decreaseDelayActionId = ActionId("decreaseDelay");
	const ActionId m_showSmoothedActionId = ActionId("toggle_show_smoothed");
	const ActionId m_showRollbackActionId = ActionId("toggle_show_rollback");
};

#pragma once
#include "stdafx.h"
#include "IExamPlugin.h"

//My includes
#include <map>
#include "Behaviors.h"
#include "StatesAndTransitions.h"

using namespace Elite;
class IBaseInterface;
class IExamInterface;
class ISteeringBehavior;
class Plugin :public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin() {};

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void Update(float dt) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;
	
	// ---------- Only in debug ---------- //
	bool m_CanRun = false; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose
	// ----------------------------------- //

	//My Functions
	void CheckInventory();

	//My Vars
	Blackboard* m_pBlackBoard = nullptr;
	BehaviorTree* m_pBehaviorTree = nullptr;
	vector<Elite::FSMState*> m_States = {};
	vector<Elite::FSMTransition*> m_Transitions = {};

	//Blackboard
		//Lists
	map<string, bool> m_StateChecks = {};
	vector<ItemInfo> m_CheckedItems = {};
	vector<HouseInfo> m_CheckedHouses = {};
	map<string, ISteeringBehavior*> m_SteeringBehaviors = {};
	pair<ISteeringBehavior*, ISteeringBehavior*>* m_pCurrentBehavior = new pair<ISteeringBehavior*, ISteeringBehavior*>(make_pair<>(nullptr, nullptr));
	map<string, FiniteStateMachine*> m_FSM = {};

		//Variables
	Vector2 m_MovementTarget = {};
	Vector2 m_RotationTarget = {};
	pair<EntityInfo, ItemInfo> m_ItemToCheck = {}; // <- due to GrabItem(), this needs to be an EntityInfo
	HouseInfo m_HouseToCheck = {};
	float m_HouseTimer = 0.f;
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}
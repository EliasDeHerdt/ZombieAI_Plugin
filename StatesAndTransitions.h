/*=============================================================================*/
// Copyright 2020-2021 Elite Engine
/*=============================================================================*/
// StatesAndTransitions.h: Implementation of the state/transition classes
/*=============================================================================*/
#ifndef ELITE_APPLICATION_FSM_STATES_TRANSITIONS
#define ELITE_APPLICATION_FSM_STATES_TRANSITIONS

#include "SteeringBehaviors.h"
#include "EFiniteStateMachine.h"
using namespace Elite;

//STATES
//-----------------------

	//Wander State
class WanderState : public Elite::FSMState
{
public:
	WanderState() : FSMState() {};
	virtual void OnEnter(Blackboard* pB) {
	}

	virtual void Update(Blackboard* pB, float deltaTime) override {
		Elite::FSMState::Update(pB, deltaTime);

		std::pair<ISteeringBehavior*, ISteeringBehavior*>* pCurrentBehavior = nullptr;
		std::map<std::string, ISteeringBehavior*>* steeringBehaviors = nullptr;

		auto dataAvailable = pB->GetData("Steering", pCurrentBehavior);
		dataAvailable = pB->GetData("SteeringBehaviors", steeringBehaviors);

		if (!dataAvailable)
			return;

		pCurrentBehavior->first = steeringBehaviors->at("Wander");
		pCurrentBehavior->second = nullptr;
		std::cout << "Wandering\n";
	}
private:
	bool m_DataAvailabe;
};

	//Seek With Pathfinding State
class SeekWithPathfindingState : public Elite::FSMState
{
public:
	SeekWithPathfindingState() : FSMState() {};
	virtual void OnEnter(Blackboard* pB) {
	}

	virtual void Update(Blackboard* pB, float deltaTime) override {
		Elite::FSMState::Update(pB, deltaTime);

		IExamInterface* pInterface = nullptr;
		std::pair<ISteeringBehavior*, ISteeringBehavior*>* pCurrentBehavior = nullptr;
		std::map<std::string, ISteeringBehavior*>* steeringBehaviors = nullptr;
		HouseInfo* pHouseToCheck = nullptr;
		Vector2* pMovementTarget = nullptr;


		auto dataAvailable = pB->GetData("Interface", pInterface);
		dataAvailable = pB->GetData("Steering", pCurrentBehavior);
		dataAvailable = pB->GetData("SteeringBehaviors", steeringBehaviors);
		dataAvailable = pB->GetData("HouseToCheck", pHouseToCheck);
		dataAvailable = pB->GetData("MovementTarget", pMovementTarget);

		if (!dataAvailable)
			return;

		*pMovementTarget = pInterface->NavMesh_GetClosestPathPoint(pHouseToCheck->Center);

		pCurrentBehavior->first = steeringBehaviors->at("Seek");
		pCurrentBehavior->second = nullptr;
		std::cout << "Wandering\n";
	}
private:
	bool m_DataAvailabe;
};

	//Flee State
class FleeState : public Elite::FSMState
{
public:
	FleeState() : FSMState() {};
	virtual void OnEnter(Blackboard* pB) {
	}

	virtual void Update(Blackboard* pB, float deltaTime) override {
		Elite::FSMState::Update(pB, deltaTime);

		std::pair<ISteeringBehavior*, ISteeringBehavior*>* pCurrentBehavior = nullptr;
		std::map<std::string, ISteeringBehavior*>* steeringBehaviors = nullptr;
		bool* pCanRun = nullptr;

		auto dataAvailable = pB->GetData("Steering", pCurrentBehavior);
		dataAvailable = pB->GetData("SteeringBehaviors", steeringBehaviors);
		dataAvailable = pB->GetData("CanRun", pCanRun);

		if (!dataAvailable)
			return;

		*pCanRun = true;

		pCurrentBehavior->first = steeringBehaviors->at("Flee");
		pCurrentBehavior->second = nullptr;
		std::cout << "Fleeing\n";
	}
private:
	bool m_DataAvailabe;
};

	//Flee With Face
class FightAndFleeState : public Elite::FSMState
{
public:
	FightAndFleeState() : FSMState() {};
	virtual void OnEnter(Blackboard* pB) {
	}

	virtual void Update(Blackboard* pB, float deltaTime) override {
		Elite::FSMState::Update(pB, deltaTime);

		IExamInterface* pInterface = nullptr;
		std::pair<ISteeringBehavior*, ISteeringBehavior*>* pCurrentBehavior = nullptr;
		std::map<std::string, ISteeringBehavior*>* steeringBehaviors = nullptr;
		Vector2* pRotationTarget = nullptr;

		auto dataAvailable = pB->GetData("Interface", pInterface);
		dataAvailable = pB->GetData("Steering", pCurrentBehavior);
		dataAvailable = pB->GetData("SteeringBehaviors", steeringBehaviors);
		dataAvailable = pB->GetData("RotationTarget", pRotationTarget);

		if (!dataAvailable)
			return;

		//Check if we have a weapon
		ItemInfo item;
		for (int i = 0; i < 5; ++i){

			if (pInterface->Inventory_GetItem(i, item)) {

				if (pInterface->Weapon_GetAmmo(item) >= 1) {

					//Calculate angle towards the target
					//AgentOrientation should be corrected by -90°
					float agentOrientation = fmodf(pInterface->Agent_GetInfo().Orientation - (float)M_PI / 2, 2 * (float)M_PI);
					Vector2 toTarget = GetNormalized(*pRotationTarget - pInterface->Agent_GetInfo().Position);
					Vector2 agentForward = GetNormalized(Vector2{ cosf(agentOrientation), sinf(agentOrientation) });
					float angle = Dot(toTarget, agentForward);

					//Check if we are looking in the targets general direction
					float buffer = 0.999f;
					if (angle >= buffer)
						pInterface->Inventory_UseItem(i);

					break;
				}
			}
		}
		
		pCurrentBehavior->first = steeringBehaviors->at("Flee");
		pCurrentBehavior->second = steeringBehaviors->at("Face");
		std::cout << "Engaging Target\n";
	}
private:
	bool m_DataAvailabe;
};

	//Flee from your current house sprinting
class FleeFromHouseState : public Elite::FSMState
{
public:
	FleeFromHouseState() : FSMState() {};
	virtual void OnEnter(Blackboard* pB) {
	}

	virtual void Update(Blackboard* pB, float deltaTime) override {
		Elite::FSMState::Update(pB, deltaTime);

		IExamInterface* pInterface = nullptr;
		std::pair<ISteeringBehavior*, ISteeringBehavior*>* pCurrentBehavior = nullptr;
		std::map<std::string, ISteeringBehavior*>* steeringBehaviors = nullptr;
		Vector2* pMovementTarget = nullptr;
		bool* pCanRun = nullptr;

		auto dataAvailable = pB->GetData("Interface", pInterface);
		dataAvailable = pB->GetData("Steering", pCurrentBehavior);
		dataAvailable = pB->GetData("SteeringBehaviors", steeringBehaviors);
		dataAvailable = pB->GetData("MovementTarget", pMovementTarget);
		dataAvailable = pB->GetData("CanRun", pCanRun);

		if (!dataAvailable)
			return;

		*pMovementTarget = pInterface->NavMesh_GetClosestPathPoint({ 0, 0 });
		*pCanRun = true;

		pCurrentBehavior->first = steeringBehaviors->at("Seek");
		pCurrentBehavior->second = nullptr;
		std::cout << "Fleeing house\n";
	}
private:
	bool m_DataAvailabe;
};

	//Flee from your current house walking
class ExitHouseState : public Elite::FSMState
{
public:
	ExitHouseState() : FSMState() {};
	virtual void OnEnter(Blackboard* pB) {
	}

	virtual void Update(Blackboard* pB, float deltaTime) override {
		Elite::FSMState::Update(pB, deltaTime);

		IExamInterface* pInterface = nullptr;
		std::pair<ISteeringBehavior*, ISteeringBehavior*>* pCurrentBehavior = nullptr;
		std::map<std::string, ISteeringBehavior*>* steeringBehaviors = nullptr;
		Vector2* pMovementTarget = nullptr;

		auto dataAvailable = pB->GetData("Interface", pInterface);
		dataAvailable = pB->GetData("Steering", pCurrentBehavior);
		dataAvailable = pB->GetData("SteeringBehaviors", steeringBehaviors);
		dataAvailable = pB->GetData("MovementTarget", pMovementTarget);

		if (!dataAvailable)
			return;

		*pMovementTarget = pInterface->NavMesh_GetClosestPathPoint({ 0, 0 });

		pCurrentBehavior->first = steeringBehaviors->at("Seek");
		pCurrentBehavior->second = nullptr;
		std::cout << "Exiting house\n";
	}
private:
	bool m_DataAvailabe;
};

	//Remember the house target
class RememberHouseState : public Elite::FSMState
{
public:
	RememberHouseState() : FSMState() {};
	virtual void OnEnter(Blackboard* pB) {
	}

	virtual void Update(Blackboard* pB, float deltaTime) override {
		Elite::FSMState::Update(pB, deltaTime);

		IExamInterface* pInterface = nullptr;
		vector<HouseInfo>* pCheckedHouses = nullptr;
		HouseInfo* pHouseToCheck = nullptr;
		float* pHouseTimer = nullptr;

		auto dataAvailable = pB->GetData("Interface", pInterface);
		dataAvailable = pB->GetData("CheckedHouses", pCheckedHouses);
		dataAvailable = pB->GetData("HouseToCheck", pHouseToCheck);
		dataAvailable = pB->GetData("HouseTimer", pHouseTimer);

		if (!dataAvailable)
			return;

		if (pHouseToCheck->Center != Vector2{ 0, 0 }
			&& pHouseToCheck->Size != Vector2{ 0, 0 }) {
			pCheckedHouses->push_back(*pHouseToCheck);
			*pHouseToCheck = HouseInfo{};
			*pHouseTimer = 0.f;
		}
		std::cout << "Remember house\n";
	}
private:
	bool m_DataAvailabe;
};

//TRANSITIONS
//------------------------

	//In House Check
class InHouseCheck : public Elite::FSMTransition
{
public:
	InHouseCheck() : FSMTransition() {};
	virtual bool ToTransition(Blackboard* pB) const override {
		
		map<string, bool>* pStateChecks = nullptr;
		
		bool dataAvailable = pB->GetData("StateChecks", pStateChecks);
		
		if (!dataAvailable)
			return false;
		
		if (pStateChecks->at("InsideHouse"))
			return true;
		
		return false;
	}
};

	//Not In House Check
class NotInHouseCheck : public Elite::FSMTransition
{
public:
	NotInHouseCheck() : FSMTransition() {};
	virtual bool ToTransition(Blackboard* pB) const override {

		map<string, bool>* pStateChecks = nullptr;
		float* pHouseTimer = nullptr;

		bool dataAvailable = pB->GetData("StateChecks", pStateChecks);
		dataAvailable = pB->GetData("HouseTimer", pHouseTimer);

		if (!dataAvailable)
			return false;

		if (!pStateChecks->at("InsideHouse")) {

			*pHouseTimer = false;
			return true;
		}

		return false;
	}
};

	//Reached Destination Check
class ReachedDestinationCheck : public Elite::FSMTransition
{
public:
	ReachedDestinationCheck() : FSMTransition() {};
	virtual bool ToTransition(Blackboard* pB) const override {

		IExamInterface* pInterface = nullptr;
		Vector2* pMovementTarget = nullptr;

		auto dataAvailable = pB->GetData("Interface", pInterface);
		dataAvailable = pB->GetData("MovementTarget", pMovementTarget);

		if (!dataAvailable)
			return false;

		float bufferSquared = .5f;
		if (Distance(*pMovementTarget, pInterface->Agent_GetInfo().Position) <= bufferSquared)
			return true;

		return false;
	}
};

	//House Timer Check
class HouseTimerCheck : public Elite::FSMTransition
{
public:
	HouseTimerCheck() : FSMTransition() {};
	virtual bool ToTransition(Blackboard* pB) const override {

		float* pHouseTimer = nullptr;

		auto dataAvailable = pB->GetData("HouseTimer", pHouseTimer);

		if (!dataAvailable)
			return false;

		if (*pHouseTimer > 10.f)
			return true;

		return false;
	}
};

	//True Check (instantly go to the next check)
class TrueCheck : public Elite::FSMTransition
{
public:
	TrueCheck() : FSMTransition() {};
	virtual bool ToTransition(Blackboard* pB) const override {

		return true;
	}
};

#endif
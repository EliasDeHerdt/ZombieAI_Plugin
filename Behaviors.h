/*=============================================================================*/
// Copyright 2020-2021 Elite Engine
/*=============================================================================*/
// Behaviors.h: Implementation of certain reusable behaviors for the BT version of the Agario Game
/*=============================================================================*/
#ifndef ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
#define ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
//-----------------------------------------------------------------
// Includes & Forward Declarations
//-----------------------------------------------------------------
#include "EBehaviorTree.h"
#include "EFiniteStateMachine.h"
#include "SteeringBehaviors.h"
#include "IExamInterface.h"
#include <map>

//-----------------------------------------------------------------
// Behaviors
//-----------------------------------------------------------------

//Check for change of behaviour
//-----------------------------------------------------------------
	//Spot Enemy
bool SpotEnemy(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	map<string, bool>* pStateChecks = nullptr;
	Vector2* pMovementTarget = nullptr;
	Vector2* pRotationTarget = nullptr;
	std::vector<EntityInfo> entitiesInFov;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("StateChecks", pStateChecks);
	dataAvailable = pBlackboard->GetData("MovementTarget", pMovementTarget);
	dataAvailable = pBlackboard->GetData("RotationTarget", pRotationTarget);
	dataAvailable = pBlackboard->GetData("EntitiesInFov", entitiesInFov);

	if (!dataAvailable)
		return false;

	float closestEnemy = FLT_MAX;
	for (const auto& currentEntity : entitiesInFov) {

		if (currentEntity.Type == eEntityType::ENEMY) {
			
			EnemyInfo foundEnemy;
			if (pInterface->Enemy_GetInfo(currentEntity, foundEnemy)) {

				float distance = Elite::Distance(pInterface->Agent_GetInfo().Position, foundEnemy.Location);
				if (distance < closestEnemy) {

					closestEnemy = distance;
					*pMovementTarget = foundEnemy.Location;
					*pRotationTarget = foundEnemy.Location;
				}
			}
		}
	}

	if (closestEnemy != FLT_MAX
		&& pStateChecks->at("HasWeapon"))
		return true;
	
	return false;
}

	//Spot House
bool SpotHouse(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	map<string, bool>* pStateChecks = nullptr;
	std::vector<HouseInfo>* CheckedHouses = nullptr;
	HouseInfo* pHouseToCheck = nullptr;
	std::vector<HouseInfo> HousesInFov;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("StateChecks", pStateChecks);
	dataAvailable = pBlackboard->GetData("CheckedHouses", CheckedHouses);
	dataAvailable = pBlackboard->GetData("HouseToCheck", pHouseToCheck);
	dataAvailable = pBlackboard->GetData("HousesInFov", HousesInFov);

	if (!dataAvailable)
		return false;

	if (pHouseToCheck->Center != Vector2{ 0, 0 }
		&& pHouseToCheck->Size != Vector2{ 0, 0 })
		return true;

	float closestHouse = FLT_MAX;
	for (const auto& currentHouse : HousesInFov) {

		bool newHouse = true;
		for (const auto& check : *CheckedHouses) {

			if (currentHouse.Center == check.Center)
				newHouse = false;
		}

		if (newHouse){

			float distance = Elite::Distance(pInterface->Agent_GetInfo().Position, currentHouse.Center);
			if (distance < closestHouse) {

				closestHouse = distance;
				*pHouseToCheck = currentHouse;
			}
		}
	}

	if (closestHouse != FLT_MAX) {

		pStateChecks->at("StartHouseFound") = true;
		return true;
	}

	return false;
}

	//Spot Item
bool SpotItem(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	map<string, bool>* pStateChecks = nullptr;
	Vector2* pMovementTarget = nullptr;
	Vector2* pRotationTarget = nullptr;
	vector<ItemInfo>* pCheckedItems = nullptr;
	pair<EntityInfo, ItemInfo>* pItemToCheck = nullptr;
	vector<EntityInfo> entitiesInFov;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("StateChecks", pStateChecks);
	dataAvailable = pBlackboard->GetData("MovementTarget", pMovementTarget);
	dataAvailable = pBlackboard->GetData("RotationTarget", pRotationTarget);
	dataAvailable = pBlackboard->GetData("CheckedItems", pCheckedItems);
	dataAvailable = pBlackboard->GetData("ItemToCheck", pItemToCheck);
	dataAvailable = pBlackboard->GetData("EntitiesInFov", entitiesInFov);

	if (!dataAvailable)
		return false;

	//if we already have an item target, this check should always pass
	bool itemSpotted = false;
	if (pItemToCheck->first.EntityHash != 0)
		itemSpotted = true;

	bool& inventoryFull = pStateChecks->at("InventoryFull");

	float closestItem = FLT_MAX;
	for (const auto& currentEntity : entitiesInFov) {

		ItemInfo foundItem;
		if (pInterface->Item_GetInfo(currentEntity, foundItem)) {

			//If our inventory is full, check if we have seen the item before
			bool newItem = true;
			if (inventoryFull) {

				for (const auto& check : *pCheckedItems) {

					if (foundItem.Type == check.Type
						&& foundItem.Location == check.Location)
						newItem = false;
				}
			}

			//Check which item is closest
			if (newItem) {

				//If we already have an item target, skip everything that is not this target
				if (pItemToCheck->first.EntityHash != 0
					&& pItemToCheck->second.Type != foundItem.Type
					&& pItemToCheck->second.Location != foundItem.Location)
					continue;

				itemSpotted = true;
				float distance = Elite::Distance(pInterface->Agent_GetInfo().Position, foundItem.Location);
				if (distance < closestItem) {

					closestItem = distance;
					pItemToCheck->first = currentEntity;
					pItemToCheck->second = foundItem;
				}
			}
		}
	}

	return itemSpotted;
}

	//Spot Out Of World
bool SpotOutOfWorld(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	Vector2* pMovementTarget = nullptr;
	Vector2* pRotationTarget = nullptr;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("MovementTarget", pMovementTarget);
	dataAvailable = pBlackboard->GetData("RotationTarget", pRotationTarget);

	if (!dataAvailable)
		return false;

	WorldInfo world = pInterface->World_GetInfo();
	AgentInfo agent = pInterface->Agent_GetInfo();
	
	if (agent.Position.x < world.Center.x - world.Dimensions.x / 2
		|| agent.Position.x > world.Center.x + world.Dimensions.x / 2
		|| agent.Position.y < world.Center.y - world.Dimensions.y / 2
		|| agent.Position.y > world.Center.y + world.Dimensions.y / 2)
		return true;

	return false;
}

	//Spot Damage
bool SpotDamage(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	map<string, bool>* pStateChecks = nullptr;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("StateChecks", pStateChecks);

	if (!dataAvailable)
		return false;

	float damage = 10 - pInterface->Agent_GetInfo().Health;

	if (damage > 0 && pStateChecks->at("HasMedkit")) {

		ItemInfo item;
		for (int i = 0; i < 5; ++i)
			if (pInterface->Inventory_GetItem(i, item))
				if (pInterface->Medkit_GetHealth(item) <= damage
					&& pInterface->Medkit_GetHealth(item) != -1)
					return true;
	}

	return false;
}

	//Spot Hunger
bool SpotHunger(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	map<string, bool>* pStateChecks = nullptr;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("StateChecks", pStateChecks);

	if (!dataAvailable)
		return false;

	float hunger = 10 - pInterface->Agent_GetInfo().Energy;

	if (hunger > 0 && pStateChecks->at("HasFood")) {

		ItemInfo item;
		for (int i = 0; i < 5; ++i)
			if (pInterface->Inventory_GetItem(i, item))
				if (pInterface->Food_GetEnergy(item) <= hunger
					&& pInterface->Food_GetEnergy(item) != -1)
					return true;
	}

	return false;
}

	//Spot Purge Zone
bool SpotPurgeZone(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	Vector2* pMovementTarget = nullptr;
	std::vector<EntityInfo> entitiesInFov;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("MovementTarget", pMovementTarget);
	dataAvailable = pBlackboard->GetData("EntitiesInFov", entitiesInFov);

	if (!dataAvailable)
		return false;

	for (const auto& currentEntity : entitiesInFov) {

		PurgeZoneInfo zoneInfo;
		if (pInterface->PurgeZone_GetInfo(currentEntity, zoneInfo)) {

			*pMovementTarget = zoneInfo.Center;
			return true;
		}
	}

	return false;
}

	//Force the bot to move somewhere untill a house is found
bool SpotStartHouse(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	map<string, bool>* pStateChecks = nullptr;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("StateChecks", pStateChecks);

	if (!dataAvailable)
		return false;

	bool& startHouseFound = pStateChecks->at("StartHouseFound");

	if (!startHouseFound)
		return true;

	return false;
}

	//Spot Needed Item
bool SpotNeedItem(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	map<string, bool>* pStateChecks = nullptr;
	std::vector<ItemInfo>* pCheckedItems = nullptr;
	pair<EntityInfo, ItemInfo>* pItemToCheck = nullptr;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("StateChecks", pStateChecks);
	dataAvailable = pBlackboard->GetData("CheckedItems", pCheckedItems);
	dataAvailable = pBlackboard->GetData("ItemToCheck", pItemToCheck);

	if (!dataAvailable)
		return false;

	bool& inventoryFull = pStateChecks->at("InventoryFull");
	
	if (!inventoryFull) {

		bool& hasFood = pStateChecks->at("HasFood");
		bool& hasMedkit = pStateChecks->at("HasMedkit");
		bool& hasWeapon = pStateChecks->at("HasWeapon");

		for (const auto& check : *pCheckedItems) {

			EntityInfo item = { eEntityType::ITEM, check.Location, check.ItemHash };
			if (check.Type == eItemType::FOOD
				&& !hasFood) {

				pItemToCheck->first = item;
				pItemToCheck->second = check;
				return true;
			}

			if (check.Type == eItemType::PISTOL
				&& !hasWeapon) {

				pItemToCheck->first = item;
				pItemToCheck->second = check;
				return true;
			}

			if (check.Type == eItemType::MEDKIT
				&& !hasMedkit) {

				pItemToCheck->first = item;
				pItemToCheck->second = check;
				return true;
			}
		}
	}

	return false;
}

//Behaviour States
//-----------------------------------------------------------------
	//Wander
BehaviorState GoWander(Elite::Blackboard* pBlackboard)
{
	std::map<std::string, FiniteStateMachine*>* pFSM = nullptr;

	auto dataAvailable = pBlackboard->GetData("FSM", pFSM);

	if (!dataAvailable)
		return Failure;
	
	pFSM->at("WanderStateMachine")->Update(0.f);

	return Success;
}

	//Fight Zombies
BehaviorState GoCombat(Elite::Blackboard* pBlackboard)
{
	std::map<std::string, FiniteStateMachine*>* pFSM = nullptr;

	auto dataAvailable = pBlackboard->GetData("FSM", pFSM);

	if (!dataAvailable)
		return Failure;

	pFSM->at("FleeStateMachine")->Update(0.f);

	return Success;
}

	//Enter a House
BehaviorState GoEnterHouse(Elite::Blackboard* pBlackboard)
{
	map<string, FiniteStateMachine*>* pFSM = nullptr;

	auto dataAvailable = pBlackboard->GetData("FSM", pFSM);

	if (!dataAvailable)
		return Failure;

	pFSM->at("HouseCheckStateMachine")->Update(0.f);

	return Success;
}

	//Check an Item
BehaviorState GoCheckItem(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	std::pair<ISteeringBehavior*, ISteeringBehavior*>* pCurrentBehavior = nullptr;
	std::map<std::string, ISteeringBehavior*>* steeringBehaviors = nullptr;
	Vector2* pMovementTarget = nullptr;
	Vector2* pRotationTarget = nullptr;
	std::vector<ItemInfo>* pCheckedItems = nullptr;
	pair<EntityInfo, ItemInfo>* pItemToCheck = nullptr;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("Steering", pCurrentBehavior);
	dataAvailable = pBlackboard->GetData("SteeringBehaviors", steeringBehaviors);
	dataAvailable = pBlackboard->GetData("MovementTarget", pMovementTarget);
	dataAvailable = pBlackboard->GetData("RotationTarget", pRotationTarget);
	dataAvailable = pBlackboard->GetData("CheckedItems", pCheckedItems);
	dataAvailable = pBlackboard->GetData("ItemToCheck", pItemToCheck);
	
	if (!dataAvailable)
		return Failure;
	
	*pMovementTarget = pInterface->NavMesh_GetClosestPathPoint(pItemToCheck->first.Location);
	*pRotationTarget = *pMovementTarget;

	bool canPickUp = true;
	ItemInfo grabbedItem;
	if (pInterface->Item_Grab(pItemToCheck->first, grabbedItem)) {

		for (int i = 0; i < 5; ++i) {

			if (canPickUp = pInterface->Inventory_AddItem(i, grabbedItem)) {

				pItemToCheck->first.EntityHash = 0;
				pCheckedItems->erase(std::remove_if(pCheckedItems->begin(), pCheckedItems->end(),
					[grabbedItem](ItemInfo val) {
						return (grabbedItem.Location == val.Location && grabbedItem.Type == val.Type);
					}), pCheckedItems->end());
				break;
			}
		}
	}

	if (!canPickUp) {

		pItemToCheck->first.EntityHash = 0;
		pCheckedItems->push_back(grabbedItem);
	}

	pCurrentBehavior->first = steeringBehaviors->at("Seek");
	pCurrentBehavior->second = steeringBehaviors->at("Face");
	std::cout << "Spotted Item\n";

	return Success;
}

	//Move back to the World
BehaviorState GoMoveToWorld(Elite::Blackboard* pBlackboard)
{
	std::pair<ISteeringBehavior*, ISteeringBehavior*>* pCurrentBehavior = nullptr;
	std::map<std::string, ISteeringBehavior*>* steeringBehaviors = nullptr;
	Vector2* pMovementTarget = nullptr;
	Vector2* pRotationTarget = nullptr;

	auto dataAvailable = pBlackboard->GetData("Steering", pCurrentBehavior);
	dataAvailable = pBlackboard->GetData("SteeringBehaviors", steeringBehaviors);
	dataAvailable = pBlackboard->GetData("MovementTarget", pMovementTarget);
	dataAvailable = pBlackboard->GetData("RotationTarget", pRotationTarget);

	if (!dataAvailable)
		return Failure;

	*pMovementTarget = { 0, 0 };

	pCurrentBehavior->first = steeringBehaviors->at("Seek");
	pCurrentBehavior->second = nullptr;
	std::cout << "Went out-of-bounds\n";

	return Success;
}

	//Heal
BehaviorState GoHeal(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface); 

	if (!dataAvailable)
		return Failure;

	float damage = 10 - pInterface->Agent_GetInfo().Health;

	ItemInfo item;
	for (int i = 0; i < 5; ++i)
		if (pInterface->Inventory_GetItem(i, item))
			if (pInterface->Medkit_GetHealth(item) <= damage
				&& pInterface->Medkit_GetHealth(item) != -1) {

				pInterface->Inventory_UseItem(i);
				damage = 10 - pInterface->Agent_GetInfo().Health;
			}

	std::cout << "Heal Wounds\n";

	return Success;
}

	//Eat
BehaviorState GoEat(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;

	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);

	if (!dataAvailable)
		return Failure;

	float hunger = 10 - pInterface->Agent_GetInfo().Energy;

	ItemInfo item;
	for (int i = 0; i < 5; ++i)
		if (pInterface->Inventory_GetItem(i, item))
			if (pInterface->Food_GetEnergy(item) <= hunger
				&& pInterface->Food_GetEnergy(item) != -1) {

				pInterface->Inventory_UseItem(i);
				hunger = 10 - pInterface->Agent_GetInfo().Energy;
			}

	std::cout << "Eat Food\n";

	return Success;
}

	//Escape a Pure Zone
BehaviorState GoEscapePurgeZone(Elite::Blackboard* pBlackboard)
{
	map<string, FiniteStateMachine*>* pFSM = nullptr;

	auto dataAvailable = pBlackboard->GetData("FSM", pFSM);

	if (!dataAvailable)
		return Failure;

	pFSM->at("PurgeZoneStateMachine")->Update(0.f);

	std::cout << "Escaping Purge Zone\n";

	return Success;
}

	//Force the bot to move somewhere untill a house is found
BehaviorState GoStartHouse(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	std::pair<ISteeringBehavior*, ISteeringBehavior*>* pCurrentBehavior = nullptr;
	std::map<std::string, ISteeringBehavior*>* steeringBehaviors = nullptr;
	Vector2* pMovementTarget = nullptr;
	
	auto dataAvailable = pBlackboard->GetData("Interface", pInterface);
	dataAvailable = pBlackboard->GetData("Steering", pCurrentBehavior);
	dataAvailable = pBlackboard->GetData("SteeringBehaviors", steeringBehaviors);
	dataAvailable = pBlackboard->GetData("MovementTarget", pMovementTarget);

	if (!dataAvailable)
		return Failure;
	
	*pMovementTarget = pInterface->Agent_GetInfo().Position + Vector2{ 0, 1 };

	pCurrentBehavior->first = steeringBehaviors->at("Seek");
	pCurrentBehavior->second = nullptr;
	std::cout << "Looking For Start House\n";

	return Success;
}
#endif
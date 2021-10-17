#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"

//My includes 
#include "Exam_HelperStructs.h"
#include "SteeringBehaviors.h"

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "Jackie-Bot";
	info.Student_FirstName = "Elias";
	info.Student_LastName = "De Herdt";
	info.Student_Class = "2DAE07";

	//Creating steering Behaviours
	m_SteeringBehaviors.emplace("Seek", new Seek{});
	m_SteeringBehaviors.emplace("Wander", new Wander{});
	m_SteeringBehaviors.emplace("Face", new Face{});
	m_SteeringBehaviors.emplace("Flee", new Flee{});

	//Creating State boolians
	m_StateChecks.emplace("StartHouseFound", false);
	m_StateChecks.emplace("InsideHouse", false);
	m_StateChecks.emplace("InventoryFull", false);
	m_StateChecks.emplace("HasFood", false);
	m_StateChecks.emplace("HasMedkit", false);
	m_StateChecks.emplace("HasWeapon", false);

	//Creating Blackboard
	m_pBlackBoard = new Blackboard{};
	m_pBlackBoard->AddData("Interface", m_pInterface);
	m_pBlackBoard->AddData("DeltaTime", float{});
		//Lists
	m_pBlackBoard->AddData("StateChecks", &m_StateChecks);
	m_pBlackBoard->AddData("CheckedItems", &m_CheckedItems);
	m_pBlackBoard->AddData("CheckedHouses", &m_CheckedHouses);
	m_pBlackBoard->AddData("HousesInFov", std::vector<HouseInfo>{});
	m_pBlackBoard->AddData("EntitiesInFov", std::vector<EntityInfo>{});
	m_pBlackBoard->AddData("SteeringBehaviors", &m_SteeringBehaviors);
	m_pBlackBoard->AddData("Steering", m_pCurrentBehavior);
		//Variables
	m_pBlackBoard->AddData("MovementTarget", &m_MovementTarget);
	m_pBlackBoard->AddData("RotationTarget", &m_RotationTarget);
	m_pBlackBoard->AddData("ItemToCheck", &m_ItemToCheck);
	m_pBlackBoard->AddData("HouseToCheck", &m_HouseToCheck);
	m_pBlackBoard->AddData("HouseTimer", &m_HouseTimer);
	m_pBlackBoard->AddData("CanRun", &m_CanRun);

	//Creating FSM
		//States
	WanderState* pWanderState = new WanderState();
	m_States.push_back(pWanderState);

	SeekWithPathfindingState* pSeekWithPathfindingState = new SeekWithPathfindingState();
	m_States.push_back(pSeekWithPathfindingState);

	FleeState* pFleeState = new FleeState();
	m_States.push_back(pFleeState);
	
	FightAndFleeState* pFleeWithFaceState = new FightAndFleeState();
	m_States.push_back(pFleeWithFaceState);

	FleeFromHouseState* pFleeFromHouseState = new FleeFromHouseState();
	m_States.push_back(pFleeFromHouseState);

	ExitHouseState* pExitHouseState = new ExitHouseState();
	m_States.push_back(pExitHouseState);

	RememberHouseState* pRememberHouseState = new RememberHouseState();
	m_States.push_back(pRememberHouseState);

		//Transitions
	InHouseCheck* pInHousCheck = new InHouseCheck();
	m_Transitions.push_back(pInHousCheck);

	NotInHouseCheck* pNotInHousCheck = new NotInHouseCheck();
	m_Transitions.push_back(pNotInHousCheck);

	ReachedDestinationCheck* pReachedDestinationCheck = new ReachedDestinationCheck();
	m_Transitions.push_back(pReachedDestinationCheck);

	HouseTimerCheck* pHouseTimerCheck = new HouseTimerCheck();
	m_Transitions.push_back(pHouseTimerCheck);

	TrueCheck* pTrueCheck = new TrueCheck();
	m_Transitions.push_back(pTrueCheck);

		//FSMs
	FiniteStateMachine* pWanderStateMachine = new FiniteStateMachine{ pWanderState, m_pBlackBoard };
	pWanderStateMachine->AddTransition(pWanderState, pFleeFromHouseState, pInHousCheck);
	pWanderStateMachine->AddTransition(pFleeFromHouseState, pWanderState, pNotInHousCheck);
	m_FSM.emplace("WanderStateMachine", pWanderStateMachine);

	//If the the "RememberHosueCheck" isn't instantly changed,
	//it's possible to mark houses that haven't been visited before.
	//This uses the fact that any current state must trigger at least once before transitioning kicks is.
	FiniteStateMachine* pHouseCheckMachine = new FiniteStateMachine{ pSeekWithPathfindingState, m_pBlackBoard };
	pHouseCheckMachine->AddTransition(pSeekWithPathfindingState, pWanderState, pReachedDestinationCheck);
	pHouseCheckMachine->AddTransition(pWanderState, pSeekWithPathfindingState, pNotInHousCheck);
	pHouseCheckMachine->AddTransition(pWanderState, pExitHouseState, pHouseTimerCheck);
	pHouseCheckMachine->AddTransition(pExitHouseState, pRememberHouseState, pNotInHousCheck);
	pHouseCheckMachine->AddTransition(pRememberHouseState, pSeekWithPathfindingState, pTrueCheck);
	m_FSM.emplace("HouseCheckStateMachine", pHouseCheckMachine);

	FiniteStateMachine* pFleeStateMachine = new FiniteStateMachine{ pFleeWithFaceState, m_pBlackBoard };
	m_FSM.emplace("FleeStateMachine", pFleeStateMachine);

	FiniteStateMachine* pPurgeZoneMachine = new FiniteStateMachine{ pFleeState, m_pBlackBoard };
	pPurgeZoneMachine->AddTransition(pFleeState, pFleeFromHouseState, pInHousCheck);
	pPurgeZoneMachine->AddTransition(pFleeFromHouseState, pFleeState, pNotInHousCheck);
	m_FSM.emplace("PurgeZoneStateMachine", pPurgeZoneMachine);

	//Adding FSMs to the Blackboard
	m_pBlackBoard->AddData("FSM", &m_FSM);

	//Creating BehaviorTree
	m_pBehaviorTree = new BehaviorTree(m_pBlackBoard,
		new BehaviorSelector({

				new BehaviorSequence({
						new BehaviorConditional(SpotPurgeZone),
						new BehaviorAction(GoEscapePurgeZone)
					}),

				new BehaviorSequence({
						new BehaviorConditional(SpotDamage),
						new BehaviorAction(GoHeal)
					}),

				new BehaviorSequence({
						new BehaviorConditional(SpotHunger),
						new BehaviorAction(GoEat)
					}),

				new BehaviorSequence({
						new BehaviorConditional(SpotOutOfWorld),
						new BehaviorAction(GoMoveToWorld)
					}),

				new BehaviorSequence({
					new BehaviorConditional(SpotEnemy),
					new BehaviorAction(GoCombat)
					}),

				new BehaviorSequence({
					new BehaviorConditional(SpotItem),
					new BehaviorAction(GoCheckItem)
					}),

			new BehaviorSequence({
					new BehaviorConditional(SpotNeedItem),
					new BehaviorAction(GoCheckItem)
					}),

				new BehaviorSequence({
					new BehaviorConditional(SpotHouse),
					new BehaviorAction(GoEnterHouse)
					}),

				new BehaviorSequence({
						new BehaviorConditional(SpotStartHouse),
						new BehaviorAction(GoStartHouse)
					}),
				
				new BehaviorAction(GoWander)
			})
	);
	
}

//Called only once
void Plugin::DllInit()
{
	//Called when the plugin is loaded
}

//Called only once
void Plugin::DllShutdown()
{
	//Called wheb the plugin gets unloaded
	m_pCurrentBehavior = nullptr;
	SAFE_DELETE(m_pBehaviorTree);

	for (auto& item : m_SteeringBehaviors)
		SAFE_DELETE(item.second);

	for (auto& s : m_States)
	{
		SAFE_DELETE(s);
	}

	for (auto& t : m_Transitions)
	{
		SAFE_DELETE(t);
	}
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be usefull to inspect certain behaviours (Default = false)
	params.AutoGrabClosestItem = true; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
	params.SpawnPurgeZonesOnMiddleClick = true;
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	//if (m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	//{
	//	//Update target based on input
	//	Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
	//	const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
	//	m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	//}
	//else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	//{
	//	m_CanRun = true;
	//}
	//else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	//{
	//	m_AngSpeed -= Elite::ToRadians(10);
	//}
	//else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	//{
	//	m_AngSpeed += Elite::ToRadians(10);
	//}
	//else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	//{
	//	m_GrabItem = true;
	//}
	//else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	//{
	//	m_UseItem = true;
	//}
	//else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	//{
	//	m_RemoveItem = true;
	//}
	//else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	//{
	//	m_CanRun = false;
	//}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	//SET-UP
	m_CanRun = false;
	m_pBlackBoard->ChangeData("DeltaTime", dt);
	m_pBlackBoard->ChangeData("HousesInFov", GetHousesInFOV());
	m_pBlackBoard->ChangeData("EntitiesInFov", GetEntitiesInFOV());

	CheckInventory();

	m_pBehaviorTree->Update(dt);

	if (m_StateChecks.at("InsideHouse") = m_pInterface->Agent_GetInfo().IsInHouse)
		m_HouseTimer += dt;

	auto steering = SteeringPlugin_Output();

	//Use the Interface (IAssignmentInterface) to 'interface' with the AI_Framework
	auto agentInfo = m_pInterface->Agent_GetInfo();

	auto nextTargetPos = m_MovementTarget; //To start you can use the mouse position as guidance
	auto nextLookAtPos = m_RotationTarget; //To start you can use the mouse position as guidance

	auto vHousesInFOV = GetHousesInFOV();//uses m_pInterface->Fov_GetHouseByIndex(...)
	auto vEntitiesInFOV = GetEntitiesInFOV(); //uses m_pInterface->Fov_GetEntityByIndex(...)
	
	//INVENTORY
	if (m_GrabItem)
	{
		ItemInfo item;
		//Item_Grab > When DebugParams.AutoGrabClosestItem is TRUE, the Item_Grab function returns the closest item in range
		//Keep in mind that DebugParams are only used for debugging purposes, by default this flag is FALSE
		//Otherwise, use GetEntitiesInFOV() to retrieve a vector of all entities in the FOV (EntityInfo)
		//Item_Grab gives you the ItemInfo back, based on the passed EntityHash (retrieved by GetEntitiesInFOV)
		if (m_pInterface->Item_Grab({}, item))
		{
			//Once grabbed, you can add it to a specific inventory slot
			//Slot must be empty
			m_pInterface->Inventory_AddItem(0, item);
		}
	}

	if (m_UseItem)
	{
		//Use an item (make sure there is an item at the given inventory slot)
		m_pInterface->Inventory_UseItem(0);
	}

	if (m_RemoveItem)
	{
		//Remove an item from a inventory slot
		m_pInterface->Inventory_RemoveItem(0);
	}

	//steering.AngularVelocity = m_AngSpeed; //Rotate your character to inspect the world while walking
	steering.AutoOrient = false; //Setting AutoOrientate to true overrides the AngularVelocity

	//MOVEMENT
	if (m_pCurrentBehavior->first != nullptr) {

		m_pCurrentBehavior->first->SetTarget(nextTargetPos);
		SteeringOutput MovementOutput = m_pCurrentBehavior->first->CalculateSteering(dt, agentInfo);

		m_AngSpeed += m_pInterface->Agent_GetInfo().MaxAngularSpeed;
		steering.LinearVelocity = MovementOutput.LinearVelocity;
		steering.AngularVelocity = m_AngSpeed;
	}

	//ROTATION
	if (m_pCurrentBehavior->second != nullptr) {

		m_pCurrentBehavior->second->SetTarget(nextLookAtPos);
		SteeringOutput RotationOutput = m_pCurrentBehavior->second->CalculateSteering(dt, agentInfo);

		steering.AngularVelocity = RotationOutput.AngularVelocity;
	}

	steering.RunMode = m_CanRun; //If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)

								 //SteeringPlugin_Output is works the exact same way a SteeringBehaviour output

								 //@End (Demo Purposes)
	m_GrabItem = false; //Reset State
	m_UseItem = false;
	m_RemoveItem = false;

	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_MovementTarget, .7f, { 0,0 }, { 1, 0, 0 });

	for (const auto& item : m_CheckedItems)
		m_pInterface->Draw_Circle(item.Location, 3.f, {0.f, 0.f, 1.f});

	for (const auto& house : m_CheckedHouses) {

		Vector2 points[4];
		points[0] = {house.Center.x - house.Size.x/2, house.Center.y - house.Size.y / 2};
		points[1] = {house.Center.x + house.Size.x/2, house.Center.y - house.Size.y / 2};
		points[2] = {house.Center.x + house.Size.x/2, house.Center.y + house.Size.y / 2};
		points[3] = {house.Center.x - house.Size.x/2, house.Center.y + house.Size.y / 2};
		m_pInterface->Draw_Polygon(points, 4, {0.f, 0.f, 1.f});
	}

	Vector2 worldCenter = m_pInterface->World_GetInfo().Center;
	Vector2 worldDimensions = m_pInterface->World_GetInfo().Dimensions;
	Vector2 points[4];
	points[0] = { worldCenter.x - worldDimensions.x / 2, worldCenter.y - worldDimensions.y / 2 };
	points[1] = { worldCenter.x + worldDimensions.x / 2, worldCenter.y - worldDimensions.y / 2 };
	points[2] = { worldCenter.x + worldDimensions.x / 2, worldCenter.y + worldDimensions.y / 2 };
	points[3] = { worldCenter.x - worldDimensions.x / 2, worldCenter.y + worldDimensions.y / 2 };
	m_pInterface->Draw_Polygon(points, 4, { 0.f, 0.f, 1.f });
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}

void Plugin::CheckInventory()
{
	m_StateChecks.at("InventoryFull") = false;
	m_StateChecks.at("HasFood") = false;
	m_StateChecks.at("HasMedkit") = false;
	m_StateChecks.at("HasWeapon") = false;

	ItemInfo item;
	float filledSlots = 0;
	for (int i = 0; i < 5; ++i) {

		if (m_pInterface->Inventory_GetItem(i, item)) {

			++filledSlots;
			//If items are empty, drop them
			//If we have atleast 1 of an ItemType, set the corresponding bool to true
			switch (item.Type)
			{
			case eItemType::FOOD:
				if (m_pInterface->Food_GetEnergy(item) == 0) {

					--filledSlots;
					m_pInterface->Inventory_RemoveItem(i);
				}
				else
					m_StateChecks.at("HasFood") = true;
				break;
			case eItemType::MEDKIT:
				if (m_pInterface->Medkit_GetHealth(item) == 0) {

					--filledSlots;
					m_pInterface->Inventory_RemoveItem(i);
				}
				else
					m_StateChecks.at("HasMedkit") = true;
				break;
			case eItemType::PISTOL:
				if (m_pInterface->Weapon_GetAmmo(item) == 0) {

					--filledSlots;
					m_pInterface->Inventory_RemoveItem(i);
				}
				else
					m_StateChecks.at("HasWeapon") = true;
				break;
			case eItemType::GARBAGE:
				--filledSlots;
				m_pInterface->Inventory_RemoveItem(i);
				break;
			default:
				break;
			}
		}
	}

	if (filledSlots == 5)
		m_StateChecks.at("InventoryFull") = true;
}

//Precompiled Header [ALWAYS ON TOP IN CPP]
#include "stdafx.h"

//Includes
#include "SteeringBehaviors.h"
#include "Exam_HelperStructs.h"

//SEEK
//****
SteeringOutput Seek::CalculateSteering(float deltaT, const AgentInfo& pAgent)
{
	SteeringOutput steering{};

	steering.LinearVelocity = m_Target.Position - pAgent.Position;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= pAgent.MaxLinearSpeed;

	return steering;
}

//WANDER (base> SEEK)
//******
SteeringOutput Wander::CalculateSteering(float deltaT, const AgentInfo& pAgent)
{
	SteeringOutput steering{};
	Elite::Vector2 circleCenter{};
	const Elite::Vector2 circleOffset{ pAgent.LinearVelocity.GetNormalized() * m_OffsetDistance };
	circleCenter = pAgent.Position + circleOffset;

	m_WanderAngle += randomFloat() * m_MaxAngleChange - m_MaxAngleChange * 0.5f;
	const Elite::Vector2 randomPointOnCircle = { cos(m_WanderAngle) * m_CircleRadius, sin(m_WanderAngle) * m_CircleRadius };

	m_Target = TargetData(randomPointOnCircle + circleCenter);

	steering.LinearVelocity = m_Target.Position - pAgent.Position;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= pAgent.MaxLinearSpeed;

	return steering;
}

//Flee
//****
SteeringOutput Flee::CalculateSteering(float deltaT, const AgentInfo& pAgent)
{
	SteeringOutput steering{};

	steering.LinearVelocity = pAgent.Position - m_Target.Position;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= pAgent.MaxLinearSpeed;

	return steering;
}

//Arrive
//****
SteeringOutput Arrive::CalculateSteering(float deltaT, const AgentInfo& pAgent)
{
	SteeringOutput steering{};

	steering.LinearVelocity = m_Target.Position - pAgent.Position;
	float VectorLength = steering.LinearVelocity.Magnitude();
	if (VectorLength > m_SlowRadius && VectorLength > m_ArrivalRadius) {
		steering.LinearVelocity.Normalize();
		steering.LinearVelocity *= pAgent.MaxLinearSpeed;
	}
	else if (VectorLength > m_ArrivalRadius){
		steering.LinearVelocity.Normalize();
		steering.LinearVelocity *= ((VectorLength / m_SlowRadius) * pAgent.MaxLinearSpeed);
	}
	else {
		steering.LinearVelocity *= 0;
	}

	return steering;
}

//FACE
//****
SteeringOutput Face::CalculateSteering(float deltaT, const AgentInfo& pAgent)
{
	SteeringOutput steering{};
	//pAgent->SetAutoOrient(false);

	//Create a vector between the target and the agent position 
	Elite::Vector2 targetDirection = m_Target.Position - pAgent.Position;
	//Normalize the vector to use it as a direction instead of a velocity
	targetDirection.Normalize();
	
	//Find the angle of the target using atan2 to compensate if target is in 4th quadrant
	float AngleofTarget = atan2(targetDirection.y, targetDirection.x);
	AngleofTarget += float(M_PI / 2.f);
	
	//Calculate the needed change of angle
	float currentAngle = pAgent.Orientation;
	float rotation = AngleofTarget - currentAngle;
	//Subtract 360° if the angle is greater than 180° to stop agent from turning the wrong direction
	while (rotation > float(M_PI))
		rotation -= float(2.f * M_PI);
	while (rotation < -float(M_PI))
		rotation += float(2.f * M_PI);

	//Limit the turning speed tot he maximing speed of the agent
	float maxSpeed = pAgent.MaxAngularSpeed;
	steering.AngularVelocity = Clamp(ToDegrees(rotation), -maxSpeed, maxSpeed);

	return steering;
}

//Evade
//****
SteeringOutput Evade::CalculateSteering(float deltaT, const AgentInfo& pAgent)
{
	SteeringOutput steering{};
	auto distanceToTarget = Distance(pAgent.Position, m_Target.Position);

	if (distanceToTarget > m_EvadeRadius) {
		steering.IsValid = false;
		return steering;
	}

	steering.LinearVelocity = pAgent.Position - m_Target.Position;
	steering.LinearVelocity += m_Target.LinearVelocity;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= pAgent.MaxLinearSpeed;

	return steering;
}

//Pursuit
//****
SteeringOutput Pursuit::CalculateSteering(float deltaT, const AgentInfo& pAgent)
{
	SteeringOutput steering{};

	steering.LinearVelocity = m_Target.Position - pAgent.Position;
	steering.LinearVelocity += m_Target.LinearVelocity;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= pAgent.MaxLinearSpeed;

	return steering;
}
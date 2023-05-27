#include "RollbackPlayer.h"


void CRollbackPlayer::Update(CEnumFlags<EInputFlag> inputFlags, Vec2 mouseDeltaRotation, float frameTime)
{

	//m_rotation += m_rotation.CreateRotationXYZ(Ang3(mouseDeltaRotation.x, mouseDeltaRotation.y, 0.0f));

	const float rotationSpeed = 0.002f;
	const float rotationLimitsMinPitch = -0.84f;
	const float rotationLimitsMaxPitch = 1.5f;

	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_rotation));
	// Yaw
	ypr.x += mouseDeltaRotation.x * rotationSpeed;

	// Pitch
	// TODO: Perform soft clamp here instead of hard wall, should reduce rot speed in this direction when close to limit.
	ypr.y = CLAMP(ypr.y + mouseDeltaRotation.y * rotationSpeed, rotationLimitsMinPitch, rotationLimitsMaxPitch);

	// Roll (skip)
	ypr.z = 0;

	m_rotation = Quat(CCamera::CreateOrientationYPR(ypr));


	const float moveSpeed = 0.205f;

	Vec3 velocity = ZERO;
	if (inputFlags != EInputFlag::None)
	{
		if (inputFlags & EInputFlag::MoveLeft)
		{
			velocity.x -= moveSpeed * frameTime;
		}
		if (inputFlags & EInputFlag::MoveRight)
		{
			velocity.x += moveSpeed * frameTime;
		}
		if (inputFlags & EInputFlag::MoveForward)
		{
			velocity.y += moveSpeed * frameTime;
		}
		if (inputFlags & EInputFlag::MoveBackward)
		{
			velocity.y -= moveSpeed * frameTime;
		}

		if (velocity != ZERO)
			velocity.NormalizeFast();
	}
		


	velocity = velocity.GetRotated(Vec3(0.0f, 0.0f, 1.0f), ypr.x);

	//velocity = direction.GetRotated(Vec3(0.0f, 0.0f, 1.0f), pNext->rotation.x);

	velocity = Quat(CCamera::CreateOrientationYPR(Vec3(ypr.x, 0.0f, 0.0f))) * velocity;

	// m_velocity += velocity;

	m_position += velocity;

}

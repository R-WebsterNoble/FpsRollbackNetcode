#pragma once

#include "InputFlag.h"

#include <CrySchematyc/Utils/EnumFlags.h>
#include <CryMath/Cry_Math.h>

class CRollbackPlayer
{
public:
	Vec3 m_position = ZERO;
	Vec3 m_velocity = ZERO;
	Quat m_rotation = ZERO;

	void Update(CEnumFlags<EInputFlag> inputFlags, Vec2 mouseDeltaRotation, float frameTime);
};

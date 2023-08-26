#pragma once

enum class EInputFlag : uint8
{
	None = 0,
	MoveLeft = 1 << 0,
	MoveRight = 1 << 1,
	MoveForward = 1 << 2,
	MoveBackward = 1 << 3
};

DEFINE_ENUM_FLAG_OPERATORS(EInputFlag)

inline std::string ToString(EInputFlag i)
{
	switch (i)
	{
	case EInputFlag::None: return "None";
	case EInputFlag::MoveLeft: return "MoveLeft";
	case EInputFlag::MoveRight: return "MoveRight";
	case EInputFlag::MoveForward: return "MoveForward";
	case EInputFlag::MoveBackward: return "MoveBackward";
	default: return "";
	}
};

#pragma once

enum class EInputFlag : uint32
{
	None = 0,
	MoveLeft = 1 << 0,
	MoveRight = 1 << 1,
	MoveForward = 1 << 2,
	MoveBackward = 1 << 3

};

DEFINE_ENUM_FLAG_OPERATORS(EInputFlag)
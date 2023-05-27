#pragma once

enum class EInputFlag : uint8
{
	None = 0,
	MoveLeft = 1 << 0,
	MoveRight = 1 << 1,
	MoveForward = 1 << 2,
	MoveBack = 1 << 3
};
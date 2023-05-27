#include "Simulation.h"
#include "GameState.h"

#include "StdAfx.h"


void CSimulation::Next(const float dt, const CGameState& pPrevious, const CPlayerInput playerInputs[], OUT CGameState& pNext)
{
    // for (int i = 0; i < NUM_PLAYERS; i++)
    for (int i = 0; i < 1; i++)
    {
        Next(dt, pPrevious.players[i], playerInputs[i], pNext.players[i]);
    }
}

float WrapRadians(const float value)
{
	const auto times = floor((value + gf_PI) / (gf_PI * 2.0f));

    return value - (times * (gf_PI * 2.0f));
}

constexpr float ACCELERATION = 50.0f;// 0.01f / 1000.0f;

constexpr float MAX_VELOCITY = 5.0f;// 0.002f;

void CSimulation::Next(const float dt, const CPlayerState& pPrevious, const CPlayerInput& playerInput, OUT CPlayerState& pNext)
{    
    constexpr float rotationSpeed = 0.002f;

    pNext.rotation.x = WrapRadians(pPrevious.rotation.x + playerInput.mouseDelta.x * rotationSpeed);
    pNext.rotation.y = crymath::clamp(pPrevious.rotation.y + playerInput.mouseDelta.y * rotationSpeed, -gf_PI * 0.5f,gf_PI * 0.5f);

	Vec3 direction = Vec3(0.0f, 0.0f, 0.0f);
    
    const EInputFlag playerActions = playerInput.playerActions;
    if (playerActions != EInputFlag::None)
    {
	    if ((playerActions & EInputFlag::MoveForward) == EInputFlag::MoveForward)
            direction += Vec3(0.0f, 1.0f, 0.0f);

        if ((playerActions & EInputFlag::MoveBackward) == EInputFlag::MoveBackward)
            direction += Vec3(0.0f, -1.0f, 0.0f);
        
        if ((playerActions & EInputFlag::MoveRight) == EInputFlag::MoveRight)
            direction += Vec3(1.0f, 0.0f, 0.0f);
        
        if ((playerActions & EInputFlag::MoveLeft) == EInputFlag::MoveLeft)
            direction += Vec3(-1.0f, 0.0f, 0.0f);
        
        if (direction != Vec3(0.0f, 0.0f, 0.0f))
            direction.NormalizeFast();

        direction = direction.GetRotated(Vec3(0.0f, 0.0f, 1.0f), pNext.rotation.x);
    }
    else
    {
        // the player is not inputting any movement actions 
        if (pPrevious.velocity.GetLengthSquared2D() <= pow(ACCELERATION * dt, 2.0f)) // if applying one ticks worth of deceleration would result in the player accelerating in the opposite direction
        {
            pNext.velocity.zero(); // zero the velocity
            pNext.position = pPrevious.position;
            return;
        }
        else // if the player is moving
        {
            direction = pPrevious.velocity.GetNormalized().Flip(); // act as if they are trying to move in the opposite direction of their current direction
        }
    }

    pNext.velocity = pPrevious.velocity + direction * ACCELERATION * dt;

    if (pNext.velocity.GetLength2D() > MAX_VELOCITY)
    {
        pNext.velocity.Normalize();
        pNext.velocity *= MAX_VELOCITY;
    }

    pNext.position = pPrevious.position + pNext.velocity * dt;
}

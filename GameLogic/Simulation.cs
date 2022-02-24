using Microsoft.Xna.Framework;

namespace GameLogic;

public static class Simulation
{
    public static GameState Next(float delta, GameState previous, PlayerInput[] playerInputs)
    {
        var next = new GameState { Players = new PlayerState[previous.Players.Length] };

        for (var i = 0; i < previous.Players.Length; i++)
            next.Players[i] = PlayerSimulation.Next(delta, previous.Players[i], playerInputs[i]);

        return next;
    }
}

public static class PlayerSimulation
{
    public static float Acceleration = 0.01f / 1000f;

    public static float MaxVelocity = 0.002f;
    //const float DRAG_COEFFICIENT = 0.005f;

    public static PlayerState Next(float delta, PlayerState previous, PlayerInput playerInput)
    {
        var deltaSeconds = delta; // / 1000f;

        var position = previous.Position;
        var velocity = previous.Velocity;

        var playerActions = playerInput.PlayerActions;

        var direction = Vector3.Zero;

        if (playerActions != PlayerAction.None)
        {
            if (playerActions.HasFlag(PlayerAction.MoveForward))
                direction.Y += 1f;

            if (playerActions.HasFlag(PlayerAction.MoveBackward))
                direction.Y -= 1f;

            if (playerActions.HasFlag(PlayerAction.MoveRight))
                direction.X += 1f;

            if (playerActions.HasFlag(PlayerAction.MoveLeft))
                direction.X -= 1f;

            if (direction != Vector3.Zero)
                direction = Vector3.Normalize(direction);
        }
        else
        {
            // the player is not inputting any morevment actions 
            if (velocity.LengthSquared() <=
                MathF.Pow(Acceleration * deltaSeconds,
                    2f)) // if applying one ticks worth of deceleration would result in the player accelerating in the opposite direction
                velocity = Vector3.Zero; // zero the velocity
            else // if the player is moving
                direction = -Vector3
                    .Normalize(velocity); // act as if they are trying to move in the opposite direction of their current direction
        }

        velocity += direction * Acceleration * deltaSeconds;

        //velocity *= 1f - DRAG_COEFFICIENT * deltaSeconds;

        if (velocity.Length() > MaxVelocity)
        {
            velocity.Normalize();
            velocity *= MaxVelocity;
        }

        position += velocity * deltaSeconds;


        return new PlayerState
        {
            Position = position,
            Velocity = velocity
        };
    }
}
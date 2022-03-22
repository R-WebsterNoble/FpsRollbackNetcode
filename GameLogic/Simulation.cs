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
        var position = previous.Position;
        var velocity = previous.Velocity;
        // var rotation = previous.Rotation + new Vector2(playerInput.MouseDelta.X * 0.001f, playerInput.MouseDelta.Y * 0.001f);
        var rotation = new Vector2(WrapRadians(previous.Rotation.X + playerInput.MouseDelta.X * 0.001f), Math.Clamp(previous.Rotation.Y + playerInput.MouseDelta.Y * 0.001f, -MathF.PI * 0.5f, + MathF.PI * 0.5f));

        float WrapRadians(float value)
        {
            var times = (float)Math.Floor((value + MathF.PI) / (MathF.PI *2f));

            return value - (times * (MathF.PI *2f));
        }

        var playerActions = playerInput.PlayerActions;

        var direction = Vector3.Zero;

        if (playerActions != PlayerAction.None)
        {
            if (playerActions.HasFlag(PlayerAction.MoveForward))
                direction += Vector3.Forward;

            if (playerActions.HasFlag(PlayerAction.MoveBackward))
                direction += Vector3.Backward;

            if (playerActions.HasFlag(PlayerAction.MoveRight))
                direction += Vector3.Right;

            if (playerActions.HasFlag(PlayerAction.MoveLeft))
                direction += Vector3.Left;

            if (direction != Vector3.Zero)
                direction = Vector3.Normalize(direction);

            direction = Vector3.Transform(direction, Matrix.CreateRotationY(rotation.X));
        }
        else
        {
            // the player is not inputting any movement actions 
            if (velocity.LengthSquared() <= MathF.Pow(Acceleration * delta, 2f)) // if applying one ticks worth of deceleration would result in the player accelerating in the opposite direction
                velocity = Vector3.Zero; // zero the velocity
            else // if the player is moving
                direction = -Vector3
                    .Normalize(velocity); // act as if they are trying to move in the opposite direction of their current direction
        }

        velocity += direction * Acceleration * delta;

        //velocity *= 1f - DRAG_COEFFICIENT * deltaSeconds;

        if (velocity.Length() > MaxVelocity)
        {
            velocity.Normalize();
            velocity *= MaxVelocity;
        }

        position += velocity * delta;


        return new PlayerState
        {
            Position = position,
            Velocity = velocity,
            Rotation = rotation
        };
    }
}
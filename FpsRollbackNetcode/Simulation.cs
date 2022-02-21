using Microsoft.Xna.Framework;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FpsRollbackNetcode
{
    public static class Simulation
    {
        public static GameState Next(float delta, GameState previous, PlayerInput[] playerInputs)
        {
            var next = new GameState {Players = new PlayerState[previous.Players.Length] };

            for (int i = 0; i < previous.Players.Length; i++)
            {               
                next.Players[i] = PlayerSimulation.Next(delta, previous.Players[i], playerInputs[i]);
            }

            return next;
        }
    }

    public static class PlayerSimulation
    {
        const float SPEED = 0.00005f;
        const float DRAG = 0.02f;

        public static PlayerState Next(float delta, PlayerState previous, PlayerInput playerInput)
        {
            var position = previous.Position;
            var velocity = previous.Velocity;

            velocity *= (1 - delta * DRAG);

            var playerActions = playerInput.playerActions;

            var direction = Vector3.Zero;

            if (playerActions != PlayerAction.None)
            {
                if (playerActions.HasFlag(PlayerAction.MoveForward))
                    direction.Y = 1f;

                if (playerActions.HasFlag(PlayerAction.MoveBackward))
                    direction.Y = -1f;

                if (playerActions.HasFlag(PlayerAction.MoveRight))
                    direction.X = 1f;

                if (playerActions.HasFlag(PlayerAction.MoveLeft))
                    direction.X = -1f;

                direction = Vector3.Normalize(direction);
            }

            velocity += (direction * SPEED) * delta;

            position += velocity * delta;


            return new PlayerState()
            {
                Position = position,
                Velocity = velocity,
            };
        }       
    }
}

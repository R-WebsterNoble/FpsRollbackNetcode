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
        private const float MOVEMENT_SPEED = 0.001f;

        public static PlayerState Next(float delta, PlayerState previous, PlayerInput playerInput)
        {
            PlayerState next = new PlayerState()
            {
                Position = previous.Position,
            };

            var playerActions = playerInput.playerActions;        

            if (playerActions.HasFlag(PlayerAction.MoveForward))
                next.Position.Y += 1 * delta * MOVEMENT_SPEED;

            if (playerActions.HasFlag(PlayerAction.MoveBackward))
                next.Position.Y -= 1 * delta * MOVEMENT_SPEED;

            if (playerActions.HasFlag(PlayerAction.MoveRight))
                next.Position.X += 1 * delta * MOVEMENT_SPEED;

            if (playerActions.HasFlag(PlayerAction.MoveLeft))
                next.Position.X -= 1 * delta * MOVEMENT_SPEED;

            return next;
        }
    }
}

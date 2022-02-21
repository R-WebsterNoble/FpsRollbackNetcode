using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Input;

namespace FpsRollbackNetcode
{
    public class Tick
    {
        public int TickNum;
        public GameState GameState;
        public PlayerInput[] PlayerInputs;

        public override string ToString()
        {
            return  $"{TickNum}:{(string.Join(',', PlayerInputs.Select((p, i) => $"{i}:{p.playerActions}")))}";
        }
    }

    public class GameState
    {
        public PlayerState[] Players;
    }

    public class PlayerInput
    {
        public PlayerAction playerActions;

        public static PlayerInput CreatePlayerInput(KeyboardState keyboard)
        {      
            var playerInput = new PlayerInput();

            if (keyboard.IsKeyDown(Keys.W))
                playerInput.playerActions = PlayerAction.MoveForward;

            if (keyboard.IsKeyDown(Keys.S))
                playerInput.playerActions |= PlayerAction.MoveBackward;

            if (keyboard.IsKeyDown(Keys.A))
                playerInput.playerActions |= PlayerAction.MoveLeft;

            if (keyboard.IsKeyDown(Keys.D))
                playerInput.playerActions |= PlayerAction.MoveRight;

            return playerInput;
        }

        public override string ToString()
        {
            return playerActions.ToString();
        }

        //public PlayerInput Combine(PlayerInput newPlayerInput)
        //{
        //    if (tickNum != newPlayerInput.tickNum)
        //        throw new InvalidOperationException("Unable to combine player inputs from diffrent ticks");

        //    return new PlayerInput 
        //    {
        //        tickNum = tickNum,
        //        playerActions = playerActions | newPlayerInput.playerActions
        //    };
        //}
    }

    [Flags]
    public enum PlayerAction
    {
        None = 0,
        MoveForward = 2,
        MoveBackward = 4,
        MoveLeft = 8,
        MoveRight = 16
    }

    public class PlayerState
    {
        public Vector3 Position = Vector3.Zero;

        public PlayerState()
        {
            Position = Vector3.Zero;
        }

        //private PlayerInput? lastPlayerInput;

        //public PlayerInput LastPlayerInput
        //{
        //    get
        //    {
        //        if(lastPlayerInput != null)
        //            return lastPlayerInput.Clone();

        //        return new PlayerInput { }; 
        //    } set => lastPlayerInput = value; 
        //}

        //public void PredictUpdate(float delta)
        //{
        //    PlayerAction playerActions = LastPlayerInput.playerActions;

        //    if (playerActions.HasFlag(PlayerAction.MoveForward))
        //        Position.X += 1 * delta;

        //    if (playerActions.HasFlag(PlayerAction.MoveBackward))
        //        Position.X -= 1 * delta;

        //    if (playerActions.HasFlag(PlayerAction.MoveRight))
        //        Position.Y += 1 * delta;

        //    if (playerActions.HasFlag(PlayerAction.MoveLeft))
        //        Position.Y += 1 * delta;
        //}

        public void Update(float delta, PlayerInput playerInput)
        {
            PlayerAction playerActions = playerInput.playerActions;

            if (playerActions.HasFlag(PlayerAction.MoveForward))
                Position.X += 1* delta;

            if (playerActions.HasFlag(PlayerAction.MoveBackward))
                Position.X -= 1 * delta;

            if (playerActions.HasFlag(PlayerAction.MoveRight))
                Position.Y += 1 * delta;

            if (playerActions.HasFlag(PlayerAction.MoveLeft))
                Position.Y += 1 * delta;

            //LastPlayerInput = playerInput;
        }
    }
}

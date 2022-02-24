using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Input;

namespace GameLogic;

public class Tick
{
    public GameState GameState;
    public PlayerInput[] PlayerInputs;
    public int TickNum;

    public override string ToString()
    {
        return $"{TickNum}:{string.Join(',', PlayerInputs.Select((p, i) => $"{i}:{p.PlayerActions}"))}";
    }
}

public class GameState
{
    public PlayerState[] Players;
}

public class PlayerInput
{
    public PlayerAction PlayerActions;

    public static PlayerInput CreatePlayerInput(KeyboardState keyboard)
    {
        var playerInput = new PlayerInput();

        if (keyboard.IsKeyDown(Keys.W))
            playerInput.PlayerActions = PlayerAction.MoveForward;

        if (keyboard.IsKeyDown(Keys.S))
            playerInput.PlayerActions |= PlayerAction.MoveBackward;

        if (keyboard.IsKeyDown(Keys.A))
            playerInput.PlayerActions |= PlayerAction.MoveLeft;

        if (keyboard.IsKeyDown(Keys.D))
            playerInput.PlayerActions |= PlayerAction.MoveRight;

        return playerInput;
    }

    public override string ToString()
    {
        return PlayerActions.ToString();
    }

    public static PlayerInput[] Empty(int count)
    {
        return Enumerable.Range(0, count).Select(_ => new PlayerInput()).ToArray();
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
    public Vector3 Velocity = Vector3.Zero;
}
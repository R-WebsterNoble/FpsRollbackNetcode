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

    public static PlayerInput CreatePlayerInput(KeyboardState keyboard, Point mouseDelta)
    {
        var playerInput = new PlayerInput
        {
            MouseDelta = new Point(mouseDelta.X, mouseDelta.Y)
        };

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

    public Point MouseDelta { get; set; } = Point.Zero;
    public static PlayerInput None => new();

    public static PlayerInput operator +(PlayerInput value1, PlayerInput value2)
    {
        value1.PlayerActions |= value2.PlayerActions;
        value1.MouseDelta += value2.MouseDelta;
        
        return value1;
    }

    public static PlayerInput[] Empty(int count)
    {
        return Enumerable.Range(0, count).Select(_ => new PlayerInput()).ToArray();
    }

    public override string ToString()
    {
        return PlayerActions.ToString();
    }
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
    public Vector2 Rotation = Vector2.Zero;
}
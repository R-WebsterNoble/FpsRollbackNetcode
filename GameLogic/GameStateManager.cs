using CircularBuffer;
using Microsoft.Xna.Framework;

namespace GameLogic;

public class GameStateManager
{
    private readonly CircularBuffer<PlayerInput[]> _delayedPlayerActionsBuffer;
    private readonly float _wiggleFrequency;
    private GameState _lastRealTimeTick;

    private float _timeRemainingAfterProcessingFixedTicks;

    public CircularBuffer<Tick> GamesStates;

    public GameStateManager(float tickDuration, float maxRollbackTime, int playerCount, float wiggleFrequency)
    {
        TickDuration = tickDuration;
        MaxRollbackTime = maxRollbackTime;
        PlayerCount = playerCount;
        _wiggleFrequency = wiggleFrequency;
        // var rand = new Random(0);

        MaxRollbackTicks = (int)MathF.Floor(MaxRollbackTime / TickDuration);
        GamesStates = new CircularBuffer<Tick>(MaxRollbackTicks + 1);

        var gameState = new GameState
        {
            //Players = Enumerable.Range(0, playerCount).Select(_ => new PlayerState() { Position = new Vector3((float)rand.NextDouble() - 0.5f, (float)rand.NextDouble() - 0.5f, 0f) }).ToArray()
            Players = Enumerable.Range(0, playerCount)
                .Select((_, i) => new PlayerState { Position = new Vector3 { X = 5f / playerCount * i } }).ToArray()
        };

        var playerInputs = Enumerable.Range(0, playerCount)
            .Select(_ => new PlayerInput { PlayerActions = PlayerAction.None }).ToArray();

        for (var i = 0; i <= MaxRollbackTicks; i++)
            GamesStates.PushFront(new Tick { TickNum = i, GameState = gameState, PlayerInputs = playerInputs });

        _lastRealTimeTick = Latest;

        _delayedPlayerActionsBuffer = new CircularBuffer<PlayerInput[]>(MaxRollbackTicks);
        for (var i = 0; i < MaxRollbackTicks; i++) _delayedPlayerActionsBuffer.PushBack(PlayerInput.Empty(PlayerCount));
    }

    public GameState Latest => GamesStates.Front().GameState;

    public PlayerInput[] LatestInputs => GamesStates.Front().PlayerInputs;

    public int TickNum { get; private set; }
    public float TickDuration { get; }
    public float MaxRollbackTime { get; }
    public int PlayerCount { get; }

    public int MaxRollbackTicks { get; }

    public GameState GetDelayedGameState(int delayTicks)
    {
        return GamesStates[delayTicks].GameState;
    }

    public void UpdateRollbackGameState(int frameNum, int playerNum, PlayerInput playerInput)
    {
        var ticksToRollback = TickNum - frameNum;

        if (ticksToRollback > MaxRollbackTicks)
            throw new InvalidOperationException(
                "Unaable to roll back to frame because this would require that we roll back more than MaxRollbackTicks")
            {
                Data =
                {
                    ["frameNum"] = frameNum,
                    ["ticksToRollback"] = ticksToRollback,
                    ["MaxRollbackTicks"] = MaxRollbackTicks
                }
            };

        for (var i = ticksToRollback - 1; i >= 0; i--)
        {
            var gameTickToUpdate = GamesStates[i];

            gameTickToUpdate.PlayerInputs[playerNum] = playerInput;

            GamesStates[i] = new Tick
            {
                TickNum = TickNum - i,
                GameState = Simulation.Next(TickDuration, GamesStates[i + 1].GameState, gameTickToUpdate.PlayerInputs),
                PlayerInputs = gameTickToUpdate.PlayerInputs
            };
        }
    }


    public (GameState, GameState) UpdateCurrentGameState(float gameTime, PlayerInput playerInput, bool controlAll)
    {
        if (gameTime == 0)
            return (Latest, Latest);

        _timeRemainingAfterProcessingFixedTicks += gameTime;

        var playerInputs = new PlayerInput[PlayerCount];
        Array.Copy(LatestInputs, playerInputs, PlayerCount);
        playerInputs[0] = playerInput;

        PlayerInput GetInput(int tickNum)
        {
            if (controlAll)
                return new PlayerInput{PlayerActions = playerInput.PlayerActions , MouseDelta = playerInput.MouseDelta};
            return new PlayerInput
            {
                PlayerActions = tickNum * TickDuration % _wiggleFrequency < _wiggleFrequency / 2f
                    ? PlayerAction.MoveForward
                    : PlayerAction.MoveBackward,
                MouseDelta = Point.Zero
            };
        }

        PlayerInput[] MakeInputs(PlayerInput input)
        {
            return Enumerable.Range(0, PlayerCount).Select(_ => new PlayerInput { PlayerActions = input.PlayerActions, MouseDelta = input.MouseDelta}).ToArray();
        }

        var input = GetInput(TickNum);
        var inputs = MakeInputs(input);
        while (_timeRemainingAfterProcessingFixedTicks >= TickDuration)
        {
            _lastRealTimeTick = Simulation.Next(TickDuration, _lastRealTimeTick, inputs);

            _delayedPlayerActionsBuffer.PushFront(inputs);

            for (var i = 1; i < PlayerCount; i++)
            {
                var playerRollbackTicks = GetDelayedPlayerRollbackTicks(i);
                var playerRollbackInputs = _delayedPlayerActionsBuffer[playerRollbackTicks];
                UpdateRollbackGameState(TickNum - playerRollbackTicks, i, playerRollbackInputs[i]);
            }

            var tickPlayerInputs = new PlayerInput[PlayerCount];
            Array.Copy(playerInputs, tickPlayerInputs, PlayerCount);

            var latest = Simulation.Next(TickDuration, Latest, tickPlayerInputs);
            GamesStates.PushFront(
                new Tick
                {
                    TickNum = TickNum,
                    GameState = latest,
                    PlayerInputs = tickPlayerInputs
                });


            TickNum++;

            input = GetInput(TickNum);
            inputs = MakeInputs(input);

            _timeRemainingAfterProcessingFixedTicks -= TickDuration;
        }
        //Console.WriteLine(string.Join(',', GamesStates.Select(s=>$"{s.PlayerInputs[9].MouseDelta.X,3}")));
        //Console.WriteLine(GamesStates.Select(s => s.PlayerInputs[9].MouseDelta.X).Sum());
        return (Simulation.Next(_timeRemainingAfterProcessingFixedTicks, Latest, playerInputs),
            Simulation.Next(_timeRemainingAfterProcessingFixedTicks, _lastRealTimeTick, inputs));
    }

    public int GetDelayedPlayerRollbackTicks(int playerNum)
    {
        return MaxRollbackTicks / PlayerCount * playerNum;
    }
}
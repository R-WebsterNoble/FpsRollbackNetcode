using Microsoft.Xna.Framework;

namespace GameLogic;

public class GameStateManager
{
    public GameState Latest => GamesStates.Front().GameState;

    public PlayerInput[] LatestInputs => GamesStates.Front().PlayerInputs;

    public int TickNum { get; private set; }
    public float TickDuration { get; }
    public float MaxRollbackTime { get; }
    public int PlayerCount { get; }

    public int MaxRollbackTicks { get; private set; }

    public CircularBuffer<Tick> GamesStates;

    private float _timeRemainingAfterProcessingFixedTicks;
    private GameState _lastRealTimeTick;

    private CircularBuffer<PlayerInput[]> _delayedPlayerActionsBuffer;
    private readonly float _wiggleFrequency;

    public GameStateManager(float tickDuration, float maxRollbackTime, int playerCount, float wiggleFrequency)
    {
        TickDuration = tickDuration;
        MaxRollbackTime = maxRollbackTime;
        PlayerCount = playerCount;
        _wiggleFrequency = wiggleFrequency;
        // var rand = new Random(0);

        MaxRollbackTicks = (int)MathF.Floor(MaxRollbackTime / TickDuration);
        GamesStates = new CircularBuffer<Tick>(MaxRollbackTicks + 1);

        var gameState = new GameState()
        {
            //Players = Enumerable.Range(0, playerCount).Select(_ => new PlayerState() { Position = new Vector3((float)rand.NextDouble() - 0.5f, (float)rand.NextDouble() - 0.5f, 0f) }).ToArray()
            Players = Enumerable.Range(0, playerCount).Select((_, i) => new PlayerState() { Position = new Vector3 { X = ((5f / playerCount) * i) } }).ToArray()
        };

        var playerInputs = Enumerable.Range(0, playerCount).Select(_ => new PlayerInput() { PlayerActions = PlayerAction.None }).ToArray();

        for (var i = 0; i <= MaxRollbackTicks; i++)
        {
            GamesStates.PushFront(new Tick { TickNum = i, GameState = gameState, PlayerInputs = playerInputs });
        }

        _lastRealTimeTick = Latest;

        _delayedPlayerActionsBuffer = new CircularBuffer<PlayerInput[]>(MaxRollbackTicks);
        for (var i = 0; i < MaxRollbackTicks; i++)
        {
            _delayedPlayerActionsBuffer.PushBack(PlayerInput.Empty(PlayerCount));
        }
    }     

    public void UpdateRollbackGameState(int frameNum, int playerNum, PlayerInput playerInput)
    {
        var ticksToRollback = TickNum - frameNum;

        if (ticksToRollback > MaxRollbackTicks)
            throw new InvalidOperationException("Unaable to roll back to frame because this would require that we roll back more than MaxRollbackTicks")
            {
                Data = {
                    ["frameNum"] = frameNum,
                    ["ticksToRollback"] = ticksToRollback,
                    ["MaxRollbackTicks"] = MaxRollbackTicks
                }
            };         

        // if (ticksToRollback != 0)
        // {
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
        // }
        // else
        //     GamesStates[0].PlayerInputs[playerNum] = playerInput;
    }


    public (GameState, GameState) UpdateCurrentGameState(float gameTime, PlayerInput playerInput, bool controllAll)
    {
        if (gameTime == 0)
            return (Latest, Latest);

        _timeRemainingAfterProcessingFixedTicks += gameTime;

        var playerInputs = new PlayerInput[PlayerCount];
        Array.Copy(LatestInputs, playerInputs, PlayerCount);
        playerInputs[0] = playerInput;

        PlayerAction GetAction(int tickNum)
        {
            if (controllAll)
                return playerInput.PlayerActions;
            return (tickNum * TickDuration) % _wiggleFrequency < _wiggleFrequency/2f ? PlayerAction.MoveForward : PlayerAction.MoveBackward;
        }

        PlayerInput[] MakeInputs(PlayerAction action)
        {
            return Enumerable.Range(0, PlayerCount).Select(_ => new PlayerInput() { PlayerActions = action }).ToArray();
        }

        var action = GetAction(TickNum);
        var inputs = MakeInputs(action);
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

            action = GetAction(TickNum);
            inputs = MakeInputs(action);

            _timeRemainingAfterProcessingFixedTicks -= TickDuration;
        }

        return (Simulation.Next(_timeRemainingAfterProcessingFixedTicks, Latest, playerInputs), Simulation.Next(_timeRemainingAfterProcessingFixedTicks, _lastRealTimeTick, inputs));


    }

    public int GetDelayedPlayerRollbackTicks(int playerNum)
    {
        return (MaxRollbackTicks / PlayerCount) * playerNum;
    }
}
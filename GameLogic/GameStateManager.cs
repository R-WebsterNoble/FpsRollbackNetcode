using Microsoft.Xna.Framework;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameLogic
{
    public class GameStateManager
    {
        public GameState Latest { get => GamesStates.Front().GameState; }

        public PlayerInput[] LatestInputs { get => GamesStates.Front().PlayerInputs; }

        private readonly int maxRollbackTicks;

        public int TickNum { get; private set; } = 0;
        public float TickDuration { get; }
        public float MaxRollbackTime { get; }
        public int PlayerCount { get; }

        public int MaxRollbackTicks
        {
            get
            {   
                // oldest game tick which has a previous state recorded
                return GamesStates.Size -1; 
            }
        }

        public CircularBuffer<Tick> GamesStates;

        public GameStateManager(float tickDuration, float maxRollbackTime, int playerCount)
        {
            TickDuration = tickDuration;
            MaxRollbackTime = maxRollbackTime;
            PlayerCount = playerCount;
            var rand = new Random(0);

            maxRollbackTicks = (int)MathF.Floor(MaxRollbackTime / TickDuration);
            GamesStates = new CircularBuffer<Tick>(maxRollbackTicks + 1);
            GamesStates.PushFront(
                new Tick
                {
                    TickNum = 0,
                    GameState = new GameState()
                    {
                        //Players = Enumerable.Range(0, playerCount).Select(_ => new PlayerState() { Position = new Vector3((float)rand.NextDouble() - 0.5f, (float)rand.NextDouble() - 0.5f, 0f) }).ToArray()
                        Players = Enumerable.Range(0, playerCount).Select(_ => new PlayerState() { Position = Vector3.Zero }).ToArray()
                    },
                    PlayerInputs = Enumerable.Range(0, playerCount).Select(_ => new PlayerInput() { playerActions = PlayerAction.None }).ToArray()
                }
            );
        }

        public void Start()
        {
            //new System.Timers.Timer()
            //{
            //    AutoReset = true,
            //    Interval = tickRate,
            //    Enabled = true,                
            //}.Elapsed += (object? sender, System.Timers.ElapsedEventArgs e) => Tick();            
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

            ////if (framesToRollback > MaxRollbackTicks)
            ////    throw new InvalidOperationException("Cannot rollback more than MaxRollbackTicks tick") { Data = { ["MaxRollbackTicks"] = MaxRollbackTicks } };

            //var o = new System.Runtime.Serialization.ObjectIDGenerator();

            //for (int i = 0; i < framesToRollback; i++)
            //{
            //    GamesStates.PopFront();
            //}

            //GamesStates.Front().PlayerInputs[playerNum] = playerInput;

            ////var thing = GamesStates.Select(s => (s.playerInputs.ContainsKey(1) ? o.GetId(s.playerInputs[1].playerActions, out var _) : 0, s.playerInputs.ContainsKey(1) ? s.playerInputs[1].playerActions : PlayerAction.None, s.gameState.Players[1].Position.Y));

            //for (int i = 0; i < framesToRollback; i++)
            //{
            //    var previousGameState = GamesStates.Front();
            //    var newGameState = Simulation.Next(TickRate, previousGameState.GameState, previousGameState.PlayerInputs);
            //    Tick newTick = new Tick 
            //    {
            //        TickNum = TickNum - i,
            //        GameState = newGameState,
            //        PlayerInputs = previousGameState.PlayerInputs 
            //    };
            //    GamesStates.PushFront(newTick);
            //}
            ////GamesStates.Select(s => (s.playerInputs[1].playerActions, s.gameState.Players[1].Position.Y))

            //GamesStates[framesToRollback].PlayerInputs[playerNum] = playerInput;

            if (ticksToRollback != 0)
            {
                var blah = 0;
                for (var i = ticksToRollback-1; i >= 0; i--)
                    blah++;

                for (var i = ticksToRollback-1; i >= 0; i--)
                {
                    var gameTickToUpdate = GamesStates[i];

                    //Update this ticks PlayerInputs to include the new
                    gameTickToUpdate.PlayerInputs[playerNum] = playerInput;

                    //                PropogateUpdatedPlayerInputs(gameStateToUpdate.PlayerInputs, playerNum, playerInput);

                    GamesStates[i] = new Tick
                    {
                        TickNum = TickNum - i,
                        GameState = Simulation.Next(TickDuration, GamesStates[i+1].GameState, gameTickToUpdate.PlayerInputs),
                        PlayerInputs = gameTickToUpdate.PlayerInputs
                    };
                }
            }
            else
                GamesStates[0].PlayerInputs[playerNum] = playerInput;
        }

        float _timeRamainingAfterProcessingFixedTicks = 0f;
        public GameState UpdateCurrentGameState(float gameTime, PlayerInput playerInput)
        {
            if (gameTime == 0)
                return Latest;

            _timeRamainingAfterProcessingFixedTicks += gameTime;

            PlayerInput[] playerInputs = new PlayerInput[PlayerCount];
            Array.Copy(LatestInputs, playerInputs, PlayerCount);
            playerInputs[0] = playerInput;


            while (_timeRamainingAfterProcessingFixedTicks >= TickDuration)
            {
                PlayerInput[] tickPlayerInputs = new PlayerInput[PlayerCount];
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

                _timeRamainingAfterProcessingFixedTicks -= TickDuration;
            }

            return Simulation.Next(_timeRamainingAfterProcessingFixedTicks, Latest, playerInputs);
        }

        //public void Tick() 
        //{
        //    int thisTicksTickNum;
        //    PlayerInput thisTicksPlayerInput;

        //    lock (this)
        //    {
        //        thisTicksTickNum = TickNum++;
        //        thisTicksPlayerInput = playerInput = new PlayerInput() 
        //        {
        //            tickNum = TickNum 
        //        };
        //    }

        //    Latest = Simulation.Next(tickRate, Latest, thisTicksPlayerInput);
        //}
    }
}

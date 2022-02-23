using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameLogic
{
    public class PlayerSimulator
    {
        public GameStateManager GameStateManager { get; }

        public static Random rand = new Random(0);

        public PlayerSimulator(GameStateManager gameStateManager)
        {
            GameStateManager = gameStateManager;
        }

        //int i = 0;

        public void SimulatePlayers()
        {
            //i = (i % (GameStateManager.PlayerCount - 1) + 1);
            var gameState = GameStateManager.Latest;

            for (int i = 1; i < GameStateManager.PlayerCount; i++)
            {
                if (rand.Next(50) < 1)
                {

                    var randomAction = (rand.Next(4)) switch
                    //var randomAction = (i%4) switch
                    {
                        0 => PlayerAction.MoveForward,
                        1 => PlayerAction.MoveBackward,
                        2 => PlayerAction.MoveLeft,
                        3 => PlayerAction.MoveRight,
                        _ => throw new NotImplementedException(),
                    };

                    var newPlayerState = new PlayerInput(){ playerActions = GameStateManager.LatestInputs[i].playerActions ^ randomAction };

                    //newPlayerState.playerActions = lastPlayerActions ^ randomAction;

                    //var updateTickNum = GameStateManager.TickNum - rand.Next(GameStateManager.MaxRollbackTicks);
                    var updateTickNum = GameStateManager.TickNum - GameStateManager.MaxRollbackTicks;

                    GameStateManager.UpdateRollbackGameState(updateTickNum, i, newPlayerState);
                }                
            }
        }
    }
}

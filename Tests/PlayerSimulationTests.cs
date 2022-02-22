using Xunit;
using FpsRollbackNetcode;
using System.Linq;
using Xunit.Abstractions;
using System;

namespace Tests;

public class PlayerSimulationTests
{
    private readonly ITestOutputHelper output;

    public PlayerSimulationTests(ITestOutputHelper output)
    {
        this.output = output;
    }

    [Fact]
    public void Test_Next_MaintainsConsistencyIndependantOfTickRate()
    {
        //var playerState = new PlayerState();
        const int testRuns = 11;
        var results = new float[testRuns];


        for (int i = 0; i < testRuns; i++)
        {
            var ticksPerSecond = MathF.Pow(2, i+1);

            float tickDuration = 1000f / ticksPerSecond;

            float timeElapsed = tickDuration;

            const float totalSimulationTime = (10f * 1000f) + 7919f; // add a prime for a bit of randomness 

            var playerState = new PlayerState();
            do
            {
                bool moveForward = ((int)timeElapsed % 1000) <= 500;
                var playerInput = new PlayerInput
                {
                    playerActions = moveForward ? PlayerAction.MoveForward : PlayerAction.None
                };
                playerState = PlayerSimulation.Next(tickDuration, playerState, playerInput);

                timeElapsed += tickDuration;
            }
            while (timeElapsed <= totalSimulationTime - tickDuration);

            //output.WriteLine((totalSimulationTime - timeElapsed).ToString());
            playerState = PlayerSimulation.Next(totalSimulationTime - timeElapsed, playerState, new PlayerInput());

            results[i] = playerState.Position.Y;

            output.WriteLine($"{i}: ticksPerSecond = {ticksPerSecond}, Position = {playerState.Position.Y}");
        }

        var average = results[results.Length/2]; //median

        const float tolerance = 0.1f;

        output.WriteLine($"average = {average}");
        for (int i = 0; i < testRuns; i++)
        {
            float result = results[i];
            output.WriteLine($"{i} = {result}");
            Assert.InRange(result, average - tolerance, average + tolerance);
        }        
    }
}

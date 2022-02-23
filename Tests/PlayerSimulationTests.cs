using Xunit;
using GameLogic;
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
        const int testRuns = 11;
        var results = new float[testRuns];

        for (int i = 0; i < testRuns; i++)
        {
            var ticksPerSecond = MathF.Pow(2, i+1);

            float tickDuration = 1024f / ticksPerSecond;

            float timeElapsed = 0;

            const float totalSimulationTime = 1024f;

            var playerState = new PlayerState();
            while (timeElapsed < totalSimulationTime)
            {
                bool moveForward = ((int)timeElapsed % 1000) <= 500;
                var playerInput = new PlayerInput
                {
                    playerActions = moveForward ? PlayerAction.MoveForward : PlayerAction.None
                };
                playerState = PlayerSimulation.Next(tickDuration, playerState, playerInput);

                timeElapsed += tickDuration;
            }

            results[i] = playerState.Position.Y;

            output.WriteLine($"{i+1:00}: ticksPerSecond = {ticksPerSecond:0000}, Position = {playerState.Position.Y:00.00}");
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

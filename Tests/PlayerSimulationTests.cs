using Xunit;
using GameLogic;
using Xunit.Abstractions;
using System;

namespace Tests;

public class PlayerSimulationTests
{
    private readonly ITestOutputHelper _output;

    public PlayerSimulationTests(ITestOutputHelper output)
    {
        _output = output;
    }

    [Fact]
    public void Test_Next_MaintainsConsistencyInterdependentOfTickRate()
    {
        const int testRuns = 11;
        var results = new float[testRuns];

        for (var i = 0; i < testRuns; i++)
        {
            var ticksPerSecond = MathF.Pow(2, i+1);

            var tickDuration = 1024f / ticksPerSecond;

            float timeElapsed = 0;

            const float totalSimulationTime = 1024f;

            var playerState = new PlayerState();
            while (timeElapsed < totalSimulationTime)
            {
                var moveForward = ((int)timeElapsed % 1000) <= 500;
                var playerInput = new PlayerInput
                {
                    PlayerActions = moveForward ? PlayerAction.MoveForward : PlayerAction.None
                };
                playerState = PlayerSimulation.Next(tickDuration, playerState, playerInput);

                timeElapsed += tickDuration;
            }

            results[i] = playerState.Position.Y;

            _output.WriteLine($"{i+1:00}: ticksPerSecond = {ticksPerSecond:0000}, Position = {playerState.Position.Y:00.00}");
        }

        var average = results[results.Length/2]; //median

        const float tolerance = 0.1f;

        _output.WriteLine($"average = {average}");
        for (var i = 0; i < testRuns; i++)
        {
            var result = results[i];
            _output.WriteLine($"{i} = {result}");
            Assert.InRange(result, average - tolerance, average + tolerance);
        }        
    }
}

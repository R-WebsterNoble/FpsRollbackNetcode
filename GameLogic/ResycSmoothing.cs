using Microsoft.Xna.Framework;

namespace GameLogic;

public class ResycSmoothing
{
    private readonly float _lerp;
    private readonly float _linear;

    public ResycSmoothing(PlayerState[] initialState, float lerp, float linear)
    {
        SmoothedPlayers = initialState;
        _lerp = lerp;
        _linear = linear;
    }

    private PlayerState[] SmoothedPlayers { get; }

    public PlayerState[] SmoothPlayers(PlayerState[] players, float deltaTime)
    {
        for (var i = 1; i < players.Length; i++)
        {
            var smoothPos = SmoothedPlayers[i].Position;
            var smoothRot = SmoothedPlayers[i].Rotation;
            var targetPos = players[i].Position;
            var targetRot = players[i].Rotation;

            var amountPos = MathF.Min(deltaTime * _lerp, 1f); // clamp lerp amount to a max of 1
            SmoothedPlayers[i].Position = Vector3.Lerp(smoothPos, targetPos, amountPos);

            var amountRot = MathF.Min(deltaTime * 0.01f, 1f); // clamp lerp amount to a max of 1
            SmoothedPlayers[i].Rotation = Vector2.Lerp(smoothRot, targetRot, amountRot);

            {
                var linearDirection = targetPos - smoothPos;
                if (linearDirection != Vector3.Zero)
                {
                    var totalLinearDistanceToCover = linearDirection.Length();

                    var linearDistance = _linear * deltaTime;
                    var clampedDistance =
                        MathF.Min(linearDistance,
                            totalLinearDistanceToCover); // Clamp to prevent overshooting and oscillating
                    var smoothMovement = Vector3.Normalize(linearDirection) * clampedDistance;
                    SmoothedPlayers[i].Position += smoothMovement;
                }
            }

            {
                var linearDirection = targetRot - smoothRot;
                if (linearDirection != Vector2.Zero)
                {
                    var totalLinearDistanceToCover = linearDirection.Length();

                    var linearDistance = 0.005f * deltaTime;
                    var clampedDistance =
                        MathF.Min(linearDistance,
                            totalLinearDistanceToCover); // Clamp to prevent overshooting and oscillating
                    var smoothMovement = Vector2.Normalize(linearDirection) * clampedDistance;
                    SmoothedPlayers[i].Rotation += smoothMovement;
                }
            }
        }
       

        return SmoothedPlayers;


    }
}
using Microsoft.Xna.Framework;

namespace GameLogic;

public class ResycSmoothing
{
    private readonly float _lerp;
    private readonly float _linear;
    private readonly float _rotLerp;
    private readonly float _rotLin;

    public ResycSmoothing(PlayerState[] initialState, float lerp, float linear, float rotLerp, float rotLin)
    {
        SmoothedPlayers = initialState;
        _lerp = lerp;
        _linear = linear;
        _rotLerp = rotLerp;
        _rotLin = rotLin;
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

            var rotationLerp = new Vector2(
                AngleMath.LerpAngle(smoothRot.X, targetRot.X, deltaTime * _rotLerp),
                AngleMath.LerpAngle(smoothRot.Y, targetRot.Y, deltaTime * _rotLerp)
                );

            var rotationLerpAndLin = new Vector2(
                AngleMath.MoveTowardsAngle(rotationLerp.X, targetRot.X, deltaTime * _rotLin),
                AngleMath.MoveTowardsAngle(rotationLerp.Y, targetRot.Y, deltaTime * _rotLin)
                );

            SmoothedPlayers[i].Rotation = rotationLerpAndLin;
        }
        
        return SmoothedPlayers;
    }
}
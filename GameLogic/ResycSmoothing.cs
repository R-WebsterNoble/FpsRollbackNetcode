using Microsoft.Xna.Framework;

namespace GameLogic;

public class ResycSmoothing
{
    private PlayerState[] SmoothedPlayers { get; }

    private readonly float _lerp;
    private readonly float _linear;

    public ResycSmoothing(PlayerState[] initialState, float lerp, float linear)
    {
        SmoothedPlayers = initialState;
        _lerp = lerp;
        _linear = linear;
    }

    public PlayerState[] SmoothPlayers(PlayerState[] players, float deltaTime) 
    {
        for (var i = 1; i < players.Length; i++)
        {
            var smootheePos = SmoothedPlayers[i].Position;
            var targetPos = players[i].Position;

            var amount = MathF.Min(deltaTime * _lerp, 1f);// clamp lerp amount to a max of 1
            SmoothedPlayers[i].Position = Vector3.Lerp(smootheePos, targetPos, amount);

            var linearDirection = targetPos - smootheePos;
            if (linearDirection == Vector3.Zero)
            {
                continue;
            }

            var totalLinearDistanceToCover = linearDirection.Length();

            var linearDistance = _linear * deltaTime;
            var clampedDistance = MathF.Min(linearDistance, totalLinearDistanceToCover); // Clamp to prevent overshooting and ocilating
            var smootheeMovement = Vector3.Normalize(linearDirection) * clampedDistance;
            SmoothedPlayers[i].Position += smootheeMovement;
        }

        return SmoothedPlayers;
    }
}
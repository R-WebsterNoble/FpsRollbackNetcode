using Microsoft.Xna.Framework;

namespace GameLogic
{
    public class ResycSmoothing
    {
        PlayerState[] _smoothedPlayers { get; }

        private readonly float _lerp;
        private readonly float _linear;

        public ResycSmoothing(PlayerState[] initialState, float lerp, float linear)
        {
            _smoothedPlayers = initialState;
            _lerp = lerp;
            _linear = linear;
        }

        public PlayerState[] SmoothPlayers(PlayerState[] players, float deltaTime) 
        {
            for (int i = 1; i < players.Length; i++)
            {
                var smootheePos = _smoothedPlayers[i].Position;
                var targetPos = players[i].Position;

                float amount = MathF.Min(deltaTime * _lerp, 1f);// clamp lerp amount to a max of 1
                _smoothedPlayers[i].Position = Vector3.Lerp(smootheePos, targetPos, amount);

                var linearDirection = targetPos - smootheePos;
                if (linearDirection == Vector3.Zero)
                {
                    continue;
                }

                var totalLinearDistanceToCover = linearDirection.Length();

                float linearDistance = _linear * deltaTime;
                float clampedDistance = MathF.Min(linearDistance, totalLinearDistanceToCover); // Clamp to prevent overshooting and ocilating
                Vector3 smootheeMovement = Vector3.Normalize(linearDirection) * clampedDistance;
                _smoothedPlayers[i].Position += smootheeMovement;
            }

            return _smoothedPlayers;
        }
    }
}

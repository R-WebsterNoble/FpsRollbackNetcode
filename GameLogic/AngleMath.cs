namespace GameLogic;

public static class AngleMath
{
    // Clamps value between min and max and returns value.
    // Set the position of the transform to be that of the time
    // but never less than 1 or more than 3
    //
    private static float Clamp(float value, float min, float max)
    {
        if (value < min)
            value = min;
        else if (value > max)
            value = max;
        return value;
    }

    // Clamps value between 0 and 1 and returns value
    public static float Clamp01(float value)
    {
        if (value < 0F)
            return 0F;
        else if (value > 1F)
            return 1F;
        else
            return value;
    }

    // Loops the value t, so that it is never larger than length and never smaller than 0.
    public static float Repeat(float t, float length)
    {
        return Clamp(t - MathF.Floor(t / length) * length, 0.0f, length);
    }

    // Calculates the shortest difference between two given angles.
    public static float LerpAngle(float a, float b, float t)
    {
        var delta = Repeat((b - a), MathF.PI * 2f);
        if (delta > MathF.PI)
            delta -= MathF.PI * 2f;
        return a + delta * Clamp01(t);
    }

    // Calculates the shortest difference between two given angles.
    public static float DeltaAngle(float current, float target)
    {
        var delta = Repeat((target - current), MathF.PI * 2f);
        if (delta > MathF.PI)
            delta -= MathF.PI * 2f;
        return delta;
    }

    // Moves a value /current/ towards /target/.
    public static float MoveTowards(float current, float target, float maxDelta)
    {
        if (MathF.Abs(target - current) <= maxDelta)
            return target;
        return current + MathF.Sign(target - current) * maxDelta;
    }


    // Same as MoveTowards but makes sure the values interpolate correctly when they wrap around a full rotation.
    public static float MoveTowardsAngle(float current, float target, float maxDelta)
    {
        var deltaAngle = DeltaAngle(current, target);
        if (-maxDelta < deltaAngle && deltaAngle < maxDelta)
            return target;
        target = current + deltaAngle;
        return MoveTowards(current, target, maxDelta);
    }
}
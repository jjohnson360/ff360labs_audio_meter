#pragma once
#include <cmath>
#include <algorithm>

namespace ff360_labs
{
    /** Smooth spring-damper follower for needle and dial animations */
    class SpringDamper
    {
    public:
        SpringDamper (float initialValue = 0.0f, float stiffness = 180.0f, float damping = 22.0f)
            : current (initialValue), target (initialValue), velocity (0.0f),
              stiffnessCoeff (stiffness), dampingCoeff (damping)
        {}

        void reset (float value = 0.0f)
        {
            current = value;
            target = value;
            velocity = 0.0f;
        }

        void setTarget (float newTarget)
        {
            target = newTarget;
        }

        void setParameters (float stiffness, float damping)
        {
            stiffnessCoeff = stiffness;
            dampingCoeff = damping;
        }

        float update (float dt)
        {
            // Clamp dt to avoid physics instability on frame hitch
            float clampedDt = std::min(dt, 0.05f);
            
            float force = stiffnessCoeff * (target - current) - dampingCoeff * velocity;
            velocity += force * clampedDt;
            current += velocity * clampedDt;

            return current;
        }

        float getCurrent() const { return current; }
        float getTarget() const { return target; }

    private:
        float current { 0.0f };
        float target { 0.0f };
        float velocity { 0.0f };
        float stiffnessCoeff { 180.0f };
        float dampingCoeff { 22.0f };
    };

    /** Ballistic follower with asymmetric attack and release decay rates */
    class BallisticFollower
    {
    public:
        BallisticFollower (float initialValue = 0.0f, float attackSpeed = 0.4f, float releaseSpeed = 0.08f)
            : current (initialValue), target (initialValue),
              attackRate (attackSpeed), releaseRate (releaseSpeed)
        {}

        void reset (float value = 0.0f)
        {
            current = value;
            target = value;
        }

        void setTarget (float newTarget)
        {
            target = newTarget;
        }

        float update()
        {
            if (target > current)
                current += (target - current) * attackRate;
            else
                current += (target - current) * releaseRate;

            return current;
        }

        float getCurrent() const { return current; }

    private:
        float current { 0.0f };
        float target { 0.0f };
        float attackRate { 0.4f };
        float releaseRate { 0.08f };
    };
}

#pragma once

#include <glm/glm.hpp>

namespace XrayEngine
{
    static const float PI = 3.14159265358979323846264338327950288f;

    struct AmbientLight
    {
        glm::vec3 irradiance;
    };

    struct DirectionLight
    {
        glm::vec3 direction;
        glm::vec3 color;
    };

    struct PointLight
    {
        glm::vec3 position{0.0f};
        glm::vec3 intensity{1.0f};

        float calculateRadius() const
        {
            const float INTENSITY_CUTOFF = 1.0f;
            const float ATTENTUATION_CUTOFF = 0.05f;
            float maxIntensity = std::max({intensity.x, intensity.y, intensity.z});
            float attenuation = std::max(INTENSITY_CUTOFF, ATTENTUATION_CUTOFF * maxIntensity) / maxIntensity;
            return 1.0f / sqrtf(attenuation);
        }
    };

    struct PointLightList
    {
        std::vector<PointLight> lights;
    };
}
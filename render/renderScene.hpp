#pragma once

#include <vulkan/vulkan.hpp>
#include "light.hpp"

namespace XrayEngine
{
    class RenderScene
    {
    public:
        AmbientLight ambientLight;
        DirectionLight directionLight;
        PointLightList pointLightList;
    };
}
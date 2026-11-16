#pragma once

#include <vulkan/vulkan.hpp>
#include "render/object/sceneObject.hpp"
#include "render/light.hpp"

namespace XrayEngine
{
    class RenderScene
    {
    public:
        AmbientLight ambientLight;
        DirectionLight directionLight;
        PointLightList pointLightList;

        std::vector<SceneObject> objects;
    };
}
#pragma once

#include <string>

namespace XrayEngine
{
    class Material
    {
    public:
        std::string baseColorTexture;
        std::string metallicTexture;
        std::string roughnessTexture;
        std::string normalTexture;
        std::string occlusionTexture;
    };
}
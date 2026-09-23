#include "render/renderPass.hpp"

namespace XrayEngine
{
    void RenderPass::Initialize(const RenderPassInitInfo* initInfo)
    {
        vulkanRHI = initInfo->vulkanRHI;
        renderResource = initInfo->renderResource;
    }
}
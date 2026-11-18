#pragma once

#include "render/renderHelper.hpp"
#include "render/renderPass.hpp"

namespace XrayEngine
{
    struct MainCameraPassInitInfo : RenderPassInitInfo
    {
    };

    class MainCameraPass : public RenderPass
    {
        enum MainCameraPassAttachment : uint8_t
        {
            eMainCameraPassAttachmentGBufferA = 0,
            eMainCameraPassAttachmentGBufferB = 1,
            eMainCameraPassAttachmentGBufferC = 2,
            eMainCameraPassAttachmentSwapchainImage = 3,
            eMainCameraPassAttachmentDepth = 4,
            eMainCameraPassAttachmentCount = 5,
            eMainCameraPassCustomAttachmentCount = 3,
        };

        enum MainCameraSubpass : uint8_t
        {
            eMainCameraSubpassBasePass,
            eMainCameraSubpassDeferredLighting,
            eMainCameraSubpassCount,
        };

        enum RenderPipelineType : uint8_t
        {
            eRenderPipelineTypeMeshGBuffer,
            eRenderPipelineTypeDeferredLighting,
            eRenderPipelineTypeCount,
        };

        enum DescriptorSetLayoutType : uint8_t
        {
            eDescriptorSetLayoutPerFrame,
            eDescriptorSetLayoutMeshPerMaterial,
            eDescriptorSetLayoutDeferredLighting,
            eDescriptorSetLayoutCount,
        };

    public:
        std::vector<RenderPipeline> renderPipelines;
        std::vector<vk::Framebuffer> swapchainFramebuffers;

        void Initialize(const RenderPassInitInfo *initInfo) override final;
        void Quit();

        void PreparePassData();
        void Draw();

    private:
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        std::vector<vk::DescriptorSet> descriptorSets;
        vk::Sampler sampler;

        void SetupAttachment();
        void SetupRenderPass();
        void SetupPipelines();
        void SetupSwapchainFramebuffers();
        void SetupDescriptorSetLayout();
        void SetupDescriptorSet();
        void DrawEntity(const vk::CommandBuffer &commandBuffer, uint32_t entityIndex, uint32_t perFrameDynamicOffset);
    };
}
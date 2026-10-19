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
            eMainCameraSubpassBasePass = 0,
            eMainCameraSubpassDeferredLighting = 1,
            eMainCameraSubpassCount = 2,
        };

        enum RenderPipelineType : uint8_t
        {
            eRenderPipelineTypeMeshGBuffer = 0,
            eRenderPipelineTypeDeferredLighting = 1,
            eRenderPipelineTypeCount = 2,
        };

        enum DescriptorSetLayoutType : uint8_t
        {
            eDescriptorSetLayoutPerFrame = 0,
            eDescriptorSetLayoutMeshPerMaterial = 1,
            eDescriptorSetLayoutDeferredLighting = 2,
            eDescriptorSetLayoutCount = 3,
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
    };
}
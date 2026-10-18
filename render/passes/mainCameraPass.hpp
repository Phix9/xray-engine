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
            eMainCameraPassAttachmentSwapchainImage = 0,
            eMainCameraPassAttachmentDepth = 1,
            eMainCameraPassAttachmentCount = 2,
        };

        enum MainCameraSubpass : uint8_t
        {
            eMainCameraSubpassBasePass = 0,
            eMainCameraSubpassCount = 1,
        };

        enum RenderPipelineType : uint8_t
        {
            eRenderPipelineTypeMesh = 0,
            eRenderPipelineTypeCount = 1,
        };

        enum DescriptorSetLayoutType : uint8_t
        {
            eDescriptorSetLayoutPerFrame = 0,
            eDescriptorSetLayoutMeshPerMaterial = 1,
            eDescriptorSetLayoutCount = 2,
        };

    public:
        vk::RenderPass renderPass;
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

        void SetupRenderPass();
        void SetupPipelines();
        void SetupSwapchainFramebuffers();
        void SetupDescriptorSetLayout();
        void SetupDescriptorSet();
    };
}
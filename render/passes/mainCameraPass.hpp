#pragma once

#include "render/renderPass.hpp"
#include "render/renderMesh.hpp"

namespace XrayEngine
{
    struct MainCameraPassInitInfo : RenderPassInitInfo
    {
    };

    class MainCameraPass : public RenderPass
    {
        enum
        {
            eMainCameraPassAttachmentSwapchainImage = 0,
            eMainCameraPassAttachmentCount = 1,
        };

        enum
        {
            eMainCameraSubpassBasePass = 0,
            eMainCameraSubpassCount = 1,
        };

        enum
        {
            eRenderPipelineTypeMesh = 0,
            eRenderPipelineTypeCount = 1,
        };

    public:
        vk::RenderPass renderPass;
        std::vector<RenderPipeline> renderPipelines;
        std::vector<vk::Framebuffer> swapchainFramebuffers;

        void Initialize(const RenderPassInitInfo *initInfo) override final;
        void Quit();

        void PreparePassData(const vk::Buffer &uniformBuffer);
        void Draw();

    private:
        vk::Buffer vertexBuffer;
        vk::Buffer indexBuffer;
        vk::Buffer uniformBuffer;
        vk::DescriptorSetLayout descriptorSetLayout;
        vk::DescriptorSet descriptorSet;
        vk::Sampler sampler;

        void SetupRenderPass();
        void SetupPipelines();
        void SetupSwapchainFramebuffers();
        void SetupDescriptorSetLayout();
        void SetupDescriptorSet();
    };
}
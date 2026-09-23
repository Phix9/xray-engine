#define STB_IMAGE_IMPLEMENTATION

#include "render/renderResource.hpp"

namespace XrayEngine
{
    void RenderResource::Initialize(RenderResourceInitInfo initInfo)
    {
        vulkanRHI=initInfo.vulkanRHI;
    }

    void RenderResource::Quit()
    {
        if (mesh.vertexBuffer)
        {
            vulkanRHI->DestroyBuffer(mesh.vertexBuffer);
            vulkanRHI->FreeMemory(mesh.vertexBufferMemory);
        }

        if (mesh.indexBuffer)
        {
            vulkanRHI->DestroyBuffer(mesh.indexBuffer);
            vulkanRHI->FreeMemory(mesh.indexBufferMemory);
        }

        if (texture.imageView)
        {
            vulkanRHI->DestroyImageView(texture.imageView);
            vulkanRHI->DestroyImage(texture.image);
            vulkanRHI->FreeMemory(texture.memory);
        }

        for (Mesh &m : meshes)
        {
            vulkanRHI->DestroyBuffer(m.vertexBuffer);
            vulkanRHI->FreeMemory(m.vertexBufferMemory);
            vulkanRHI->DestroyBuffer(m.indexBuffer);
            vulkanRHI->FreeMemory(m.indexBufferMemory);
        }
        meshes.clear();

        for (Texture &t : textures)
        {
            vulkanRHI->DestroyImageView(t.imageView);
            vulkanRHI->DestroyImage(t.image);
            vulkanRHI->FreeMemory(t.memory);
        }
        textures.clear();
    }

    TextureData RenderResource::LoadTexture(std::string filePath)
    {
        int width, height, nrChannel;
        stbi_uc* pixels = stbi_load(filePath.data(), &width, &height, &nrChannel, STBI_rgb_alpha);

        if (!pixels)
        {
            throw std::runtime_error("Load image failed");
        }

        TextureData textureData;
        textureData.width = width;
        textureData.height = height;
        textureData.pixels = pixels;

        return textureData;
    }

    void RenderResource::CreateMeshes(const std::vector<MeshVertex> &meshData)
    {
        vk::DeviceSize meshSize = meshData.size() * sizeof(MeshVertex);
        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;

        vk::BufferCreateInfo createInfo;
        createInfo.setSize(meshSize)
            .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
            .setSharingMode(vk::SharingMode::eExclusive);
        stagingBuffer = vulkanRHI->CreateBuffer(createInfo, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBufferMemory);

        createInfo.setSize(meshSize)
            .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
            .setSharingMode(vk::SharingMode::eExclusive);
        mesh.vertexBuffer = vulkanRHI->CreateBuffer(createInfo, vk::MemoryPropertyFlagBits::eDeviceLocal, mesh.vertexBufferMemory);

        void *ptr = vulkanRHI->MapMemory(stagingBufferMemory, 0, meshSize);
        memcpy(ptr, meshData.data(), meshSize);
        vulkanRHI->UnmapMemory(stagingBufferMemory);

        vk::CommandBuffer commandBuffer = vulkanRHI->BeginOneTimeCommandBuffer();

        vk::BufferCopy region;
        region.setSrcOffset(0)
            .setDstOffset(0)
            .setSize(meshSize);

        vulkanRHI->CommandCopyBuffer(commandBuffer, stagingBuffer, mesh.vertexBuffer, region);

        vulkanRHI->EndOneTimeCommandBuffer(commandBuffer);

        vulkanRHI->DestroyBuffer(stagingBuffer);
        vulkanRHI->FreeMemory(stagingBufferMemory);
    }

    void RenderResource::CreateIndices(const std::vector<uint16_t> &indices)
    {
        vk::DeviceSize indexSize = indices.size() * sizeof(uint16_t);

        vk::BufferCreateInfo createInfo;
        createInfo.setSize(indexSize)
            .setUsage(vk::BufferUsageFlagBits::eIndexBuffer)
            .setSharingMode(vk::SharingMode::eExclusive);
        mesh.indexBuffer = vulkanRHI->CreateBuffer(createInfo, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, mesh.indexBufferMemory);

        void *ptr = vulkanRHI->MapMemory(mesh.indexBufferMemory, 0, indexSize);
        memcpy(ptr, indices.data(), indexSize);
        vulkanRHI->UnmapMemory(mesh.indexBufferMemory);
    }

    void RenderResource::CreateTextures(const TextureData &textureData)
    {
        vk::DeviceSize textureSize = textureData.width * textureData.height * 4;

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;
        
        vk::BufferCreateInfo stagingBufferCreateInfo;
        stagingBufferCreateInfo.setSize(textureSize)
            .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
            .setSharingMode(vk::SharingMode::eExclusive);
        stagingBuffer = vulkanRHI->CreateBuffer(stagingBufferCreateInfo, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBufferMemory);

        void *ptr = vulkanRHI->MapMemory(stagingBufferMemory, 0, textureSize);
        memcpy(ptr, textureData.pixels, textureSize);
        vulkanRHI->UnmapMemory(stagingBufferMemory);

        vk::ImageCreateInfo createInfo;
        createInfo.setImageType(vk::ImageType::e2D)
            .setArrayLayers(1)
            .setMipLevels(1)
            .setExtent({textureData.width, textureData.height, 1})
            .setFormat(vk::Format::eR8G8B8A8Srgb)
            .setTiling(vk::ImageTiling::eOptimal)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
            .setSamples(vk::SampleCountFlagBits::e1);

        texture.image = vulkanRHI->CreateImage(createInfo, vk::MemoryPropertyFlagBits::eDeviceLocal, texture.memory, texture.imageView);
        
        TransitionImageLayout(texture.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        vk::CommandBuffer commandBuffer = vulkanRHI->BeginOneTimeCommandBuffer();

        vk::ImageSubresourceLayers layers;
        layers.setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setMipLevel(0)
            .setBaseArrayLayer(0)
            .setLayerCount(1);
        vk::BufferImageCopy region;
        region.setBufferOffset(0)
            .setBufferRowLength(0)
            .setBufferImageHeight(0)
            .setImageOffset({0, 0, 0})
            .setImageExtent({textureData.width, textureData.height, 1})
            .setImageSubresource(layers);

        vulkanRHI->CommandCopyBufferToImage(commandBuffer, stagingBuffer, texture.image, vk::ImageLayout::eTransferDstOptimal ,region);

        vulkanRHI->EndOneTimeCommandBuffer(commandBuffer);
        
        TransitionImageLayout(texture.image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    
        vulkanRHI->DestroyBuffer(stagingBuffer);
        vulkanRHI->FreeMemory(stagingBufferMemory);
    }

    void RenderResource::TransitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
    {
        vk::CommandBuffer commandBuffer = vulkanRHI->BeginOneTimeCommandBuffer();

        vk::ImageSubresourceRange range;
        range.setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setLayerCount(1)
            .setLevelCount(1);
        vk::ImageMemoryBarrier barrier;
        barrier.setImage(image)
            .setOldLayout(oldLayout)
            .setNewLayout(newLayout)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSubresourceRange(range);

        vk::PipelineStageFlags srcStage;
        vk::PipelineStageFlags dstStage;

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits::eNone)
                .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
            
            srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
            dstStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
                 newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

            srcStage = vk::PipelineStageFlagBits::eTransfer;
            dstStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        // for getGuidAndDepthOfMouseClickOnRenderSceneForUI() get depthimage
        else if (oldLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal &&
                 newLayout == vk::ImageLayout::eTransferSrcOptimal)
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                .setDstAccessMask(vk::AccessFlagBits::eTransferRead);

            srcStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
            dstStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal &&
                 newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
                .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);

            srcStage = vk::PipelineStageFlagBits::eTransfer;
            dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        }
        // for generating mipmapped image
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
                 newLayout == vk::ImageLayout::eTransferSrcOptimal)
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eTransferRead);

            srcStage = vk::PipelineStageFlagBits::eTransfer;
            dstStage = vk::PipelineStageFlagBits::eTransfer;
        }
        
        vulkanRHI->CommandPipelineBarrier(commandBuffer, srcStage, dstStage, {}, {}, {}, {barrier});

        vulkanRHI->EndOneTimeCommandBuffer(commandBuffer);
    }
}
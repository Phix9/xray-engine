#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include "render/renderResource.hpp"

namespace XrayEngine
{
    void RenderResource::Initialize(RenderResourceInitInfo initInfo)
    {
        vulkanRHI = initInfo.vulkanRHI;
        sampler = vulkanRHI->CreateDefaultLinearSampler();

        TextureData defaultTextureData;
        defaultTextureData.width = 1;
        defaultTextureData.height = 1;
        defaultTextureData.format = vk::Format::eR8G8B8A8Unorm;

        unsigned char whitePixel[4] = {255, 255, 255, 255};
        defaultTextureData.pixels = std::malloc(4);
        std::memcpy(defaultTextureData.pixels, whitePixel, 4);

        defalutTexture.push_back(CreateTexture(defaultTextureData));
        std::free(defaultTextureData.pixels);
    }

    void RenderResource::Quit()
    {
        if (storage.ringBuffer)
        {
            if (storage.mappedMemory)
            {
                vulkanRHI->UnmapMemory(storage.ringBufferMemory);
                storage.mappedMemory = nullptr;
            }
            vulkanRHI->DestroyBuffer(storage.ringBuffer);
            vulkanRHI->FreeMemory(storage.ringBufferMemory);
        }

        for (RHIMaterial &material : materials)
        {
            if (material.materialUniformBuffer)
            {
                vulkanRHI->DestroyBuffer(material.materialUniformBuffer);
                vulkanRHI->FreeMemory(material.materialUniformBufferMemory);
            }

            if (material.descriptorSetLayout)
            {
                vulkanRHI->DestroyDescriptorSetLayout(material.descriptorSetLayout);
            }
        }

        for (RHIMesh &mesh : meshes)
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
        }

        for (RHITexture &texture : texturePool)
        {
            if (texture.imageView)
            {
                vulkanRHI->DestroyImageView(texture.imageView);
            }

            if (texture.image)
            {
                vulkanRHI->DestroyImage(texture.image);
                vulkanRHI->FreeMemory(texture.memory);
            }
        }

        for (TextureData &textureData : textureDatas)
        {
            if (textureData.pixels)
            {
                std::free(textureData.pixels);
                textureData.pixels = nullptr;
            }
        }

        if (sampler)
        {
            vulkanRHI->DestroySampler(sampler);
        }
    }

    void RenderResource::CreateModel(std::string filePath)
    {
        AssetLoader::LoadModel(filePath, renderObjects, renderEntities, meshDatas, materialDatas, textureDatas);

        for (MeshData &meshData : meshDatas)
        {
            meshes.push_back(CreateMesh(meshData));
        }
        
        for (TextureData &textureData : textureDatas)
        {
            texturePool.push_back(CreateTexture(textureData));
        }

        for (MaterialData &materialData : materialDatas)
        {
            materials.push_back(CreateMaterial(materialData));
        }

        CreateRenderQueue();
    }

    RHIMesh RenderResource::CreateMesh(const MeshData &meshData)
    {
        RHIMesh mesh;

        {
            mesh.vertexCount = static_cast<uint32_t>(meshData.vertices.size());
            vk::DeviceSize meshSize = meshData.vertices.size() * sizeof(MeshVertex);
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
            memcpy(ptr, meshData.vertices.data(), meshSize);
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

        {
            mesh.indexCount = static_cast<uint32_t>(meshData.indices.size());

            vk::DeviceSize indexSize = meshData.indices.size() * sizeof(uint32_t);

            vk::BufferCreateInfo createInfo;
            createInfo.setSize(indexSize)
                .setUsage(vk::BufferUsageFlagBits::eIndexBuffer)
                .setSharingMode(vk::SharingMode::eExclusive);
            mesh.indexBuffer = vulkanRHI->CreateBuffer(createInfo, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, mesh.indexBufferMemory);

            void *ptr = vulkanRHI->MapMemory(mesh.indexBufferMemory, 0, indexSize);
            memcpy(ptr, meshData.indices.data(), indexSize);
            vulkanRHI->UnmapMemory(mesh.indexBufferMemory);
        }

        return mesh;
    }

    RHITexture RenderResource::CreateTexture(const TextureData &textureData)
    {
        RHITexture texture;

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
            .setFormat(textureData.format)
            .setTiling(vk::ImageTiling::eOptimal)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
            .setSamples(vk::SampleCountFlagBits::e1);

        texture.image = vulkanRHI->CreateImage(createInfo, vk::MemoryPropertyFlagBits::eDeviceLocal, texture.memory);
        
        vk::ImageSubresourceRange range;
        range.setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setLevelCount(1)
            .setBaseMipLevel(0);
        vk::ImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.setImage(texture.image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(createInfo.format)
            .setSubresourceRange(range);
        
        texture.imageView = vulkanRHI->CreateImageView(imageViewCreateInfo);

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

        return texture;
    }

    RHIMaterial RenderResource::CreateMaterial(const MaterialData &materialData)
    {
        RHIMaterial material;

        {
            for (const Parameter<TextureInfoData> &texture : materialData.textures)
            {
                material.textureReferences.push_back(texture.value.textureIndex >= 0 ? texture.value.textureIndex : -1);
            }

            MeshPerMaterialUniformBufferObject meshPerMaterialUniformBufferObject;

            for (const Parameter<MaterialData::Vector> &vector : materialData.vectors)
            {
                meshPerMaterialUniformBufferObject.uniforms.push_back(vector.value.x);
                meshPerMaterialUniformBufferObject.uniforms.push_back(vector.value.y);
                meshPerMaterialUniformBufferObject.uniforms.push_back(vector.value.z);
                meshPerMaterialUniformBufferObject.uniforms.push_back(vector.value.w);
            }

            for (const Parameter<MaterialData::Scalar> &scalar : materialData.scalars)
            {
                meshPerMaterialUniformBufferObject.uniforms.push_back(scalar.value);
            }

            const vk::DeviceSize uboSize = meshPerMaterialUniformBufferObject.uniforms.size() * sizeof(float);

            vk::BufferCreateInfo createInfo;
            createInfo.setSize(uboSize)
                .setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
                .setSharingMode(vk::SharingMode::eExclusive);
            material.materialUniformBuffer = vulkanRHI->CreateBuffer(createInfo, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, material.materialUniformBufferMemory);

            void *ptr = vulkanRHI->MapMemory(material.materialUniformBufferMemory, 0, uboSize);
            memcpy(ptr, meshPerMaterialUniformBufferObject.uniforms.data(), uboSize);
            vulkanRHI->UnmapMemory(material.materialUniformBufferMemory);
        }

        {
            const uint32_t textureCount = static_cast<uint32_t>(material.textureReferences.size());

            std::vector<vk::DescriptorSetLayoutBinding> meshPerMaterialBindings;
            meshPerMaterialBindings.reserve(static_cast<size_t>(textureCount) + 1);

            for (uint32_t i = 0; i < textureCount; i++)
            {
                meshPerMaterialBindings.push_back(vk::DescriptorSetLayoutBinding().setBinding(i).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(1).setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute));
            }

            meshPerMaterialBindings.push_back(vk::DescriptorSetLayoutBinding().setBinding(textureCount).setDescriptorType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(1).setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment));

            vk::DescriptorSetLayoutCreateInfo createInfo;
            createInfo.setBindings(meshPerMaterialBindings);

            material.descriptorSetLayout = vulkanRHI->CreateDescriptorSetLayout(createInfo);
        }

        {
            vk::DescriptorSetAllocateInfo allocateInfo;
            allocateInfo.setDescriptorPool(vulkanRHI->GetDescriptorPool())
                .setDescriptorSetCount(1)
                .setSetLayouts(material.descriptorSetLayout);

            material.descriptorSet = vulkanRHI->AllocateDescriptorSets(allocateInfo);

            const uint32_t textureCount = static_cast<uint32_t>(material.textureReferences.size());

            std::vector<vk::DescriptorImageInfo> imageInfos;
            imageInfos.reserve(textureCount);
            for (uint32_t i = 0; i < textureCount; i++)
            {
                const vk::ImageView imageView = material.textureReferences[i] >= 0
                                                    ? texturePool[material.textureReferences[i]].imageView
                                                    : defalutTexture[0].imageView;

                imageInfos.push_back(vk::DescriptorImageInfo().setSampler(sampler)
                                         .setImageView(imageView)
                                         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
            }

            std::vector<vk::WriteDescriptorSet> writes;
            writes.reserve(static_cast<size_t>(textureCount) + 1);

            for (uint32_t i = 0; i < textureCount; i++)
            {
                writes.push_back(vk::WriteDescriptorSet().setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                                     .setImageInfo(imageInfos[i])
                                     .setDstSet(material.descriptorSet)
                                     .setDstBinding(i)
                                     .setDescriptorCount(1));
            }

            vk::DescriptorBufferInfo bufferInfo;
            bufferInfo.setOffset(0)
                .setRange(vk::WholeSize)
                .setBuffer(material.materialUniformBuffer);

            writes.push_back(vk::WriteDescriptorSet().setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                 .setBufferInfo(bufferInfo)
                                 .setDstSet(material.descriptorSet)
                                 .setDstBinding(textureCount)
                                 .setDescriptorCount(1));

            vulkanRHI->UpdateDescriptorSets(writes, {});
        }

        return material;
    }

    void RenderResource::CreateStorageBuffer()
    {
        vk::PhysicalDeviceProperties properties = vulkanRHI->GetPhysicalDeviceProperties();
        storage.minUniformBufferOffsetAlignment = static_cast<uint32_t>(properties.limits.minUniformBufferOffsetAlignment);
        storage.minStorageBufferOffsetAlignment = static_cast<uint32_t>(properties.limits.minStorageBufferOffsetAlignment);
        storage.maxStorageBufferRange = properties.limits.maxStorageBufferRange;

        const uint32_t bufferSize = 1024 * 1024 * 128;

        vk::BufferCreateInfo createInfo;
        createInfo.setSize(bufferSize)
            .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
            .setSharingMode(vk::SharingMode::eExclusive);
        storage.ringBuffer = vulkanRHI->CreateBuffer(createInfo, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, storage.ringBufferMemory);

        uint32_t framesInFlight = vulkanRHI->GetFramesInFlightCount();
        storage.ringBufferBegins.resize(framesInFlight);
        storage.ringBufferEnds.resize(framesInFlight);
        storage.ringBufferSizes.resize(framesInFlight);

        for(uint32_t i = 0; i < framesInFlight; i++)
        {
            storage.ringBufferBegins[i] = (bufferSize * i) / framesInFlight;
            storage.ringBufferSizes[i] = (bufferSize * (i + 1)) / framesInFlight - (bufferSize * i) / framesInFlight;
        }

        storage.mappedMemory = vulkanRHI->MapMemory(storage.ringBufferMemory, 0, bufferSize);
    }

    void RenderResource::CreateRenderQueue()
    {
        for (std::vector<uint32_t> &queue : renderQueue)
        {
            queue.clear();
        }

        for (size_t i = 0; i < renderEntities.size(); i++)
        {
            const RenderEntity &renderEntity = renderEntities[i];

            if (renderEntity.meshIndex < 0 || renderEntity.materialIndex < 0)
            {
                continue;
            }

            renderQueue[materialDatas[renderEntity.materialIndex].domain].push_back(static_cast<uint32_t>(i));
        }

        for (std::vector<uint32_t> &queue : renderQueue)
        {
            std::stable_sort(queue.begin(), queue.end(),
                [this](uint32_t left, uint32_t right)
                {
                    return renderEntities[left].materialIndex < renderEntities[right].materialIndex;
                });
        }
    }

    void RenderResource::ResetRingBufferOffset()
    {
        storage.ringBufferEnds[vulkanRHI->GetCurrentFrameIndex()] = storage.ringBufferBegins[vulkanRHI->GetCurrentFrameIndex()];
    }

    void RenderResource::UpdatePerFramebuffer(std::shared_ptr<RenderScene> renderScene, std::shared_ptr<RenderCamera> camera)
    {
        glm::mat4 viewMatrix = camera->GetViewMatrix();
        glm::mat4 projectMatrix = camera->GetProjectMatrix();
        glm::vec3 cameraPosition = camera->GetPosition();
        glm::vec3 ambientLight = renderScene->ambientLight.irradiance;
        uint32_t pointLightCount = static_cast<uint32_t>(renderScene->pointLightList.lights.size());
        
        for(uint32_t i = 0; i< pointLightCount; i++)
        {
            glm::vec3 pointLightPositon = renderScene->pointLightList.lights[i].position;
            glm::vec3 pointLightIntensity = renderScene->pointLightList.lights[i].intensity;
            float pointLightRadius = renderScene->pointLightList.lights[i].calculateRadius();

            meshPerFrameStorageBufferObject.pointLights[i].position = pointLightPositon;
            meshPerFrameStorageBufferObject.pointLights[i].radius = pointLightRadius;
            meshPerFrameStorageBufferObject.pointLights[i].intensity = pointLightIntensity;
        }

        glm::mat4 projectViewMatrix = projectMatrix * viewMatrix;

        meshPerFrameStorageBufferObject.projectViewMatrix = projectViewMatrix;
        meshPerFrameStorageBufferObject.inverseProjectViewMatrix = glm::inverse(projectViewMatrix);
        meshPerFrameStorageBufferObject.cameraPosition = cameraPosition;
        meshPerFrameStorageBufferObject.ambientLight = ambientLight;
        meshPerFrameStorageBufferObject.pointLightCount = pointLightCount;
        meshPerFrameStorageBufferObject.directionalLight.direcion = renderScene->directionLight.direction;
        meshPerFrameStorageBufferObject.directionalLight.color = renderScene->directionLight.color;
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
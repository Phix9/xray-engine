#include "assetLoader.hpp"

namespace XrayEngine
{
    void AssetLoader::LoadModel(std::string filePath,
                                std::vector<uint32_t> &renderObjects,
                                std::vector<RenderEntity> &renderEntities,
                                std::vector<MeshData> &meshDatas,
                                std::vector<MaterialData> &materialDatas,
                                std::vector<TextureData> &textureDatas)
    {
        const size_t dotPosition = filePath.find_last_of('.');
        std::string extension = dotPosition == std::string::npos ? std::string() : filePath.substr(dotPosition);

        for (char &character : extension)
        {
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }

        if (extension == ".gltf" || extension == ".glb")
        {
            GltfModel gltfModel = LoadGltfModel(filePath, materialDatas, meshDatas, textureDatas);

            const int rootIndex = ProcessGltfModel(gltfModel, gltfModel.scene.node, glm::mat4(1.0f), renderEntities);
            if (rootIndex >= 0)
            {
                renderObjects.push_back(static_cast<uint32_t>(rootIndex));
            }
        }
    }

    GltfModel AssetLoader::LoadGltfModel(std::string filePath, std::vector<MaterialData> &materialDatas, std::vector<MeshData> &meshDatas, std::vector<TextureData> &textureDatas)
    {
        GltfModel result;

        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        const bool isBinary = filePath.size() >= 4 && filePath.compare(filePath.size() - 4, 4, ".glb") == 0;

        const bool loaded = isBinary ? loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filePath)
                                     : loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filePath);

        if (!loaded)
        {
            throw std::runtime_error("glTF load failed: " + filePath + "\n" + err);
        }

        if (gltfModel.scenes.empty())
        {
            throw std::runtime_error("glTF: \"" + filePath + "\" defines no scene!");
        }

        const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

        if (scene.nodes.empty())
        {
            throw std::runtime_error("glTF: scene \"" + scene.name + "\" has no root node!");
        }

        const int rootNodeIndex = scene.nodes[0];
        if (rootNodeIndex < 0 || rootNodeIndex >= static_cast<int>(gltfModel.nodes.size()))
        {
            throw std::runtime_error("glTF: scene \"" + scene.name + "\" references node " + std::to_string(rootNodeIndex) + " which does not exist!");
        }

        result.scene.node = rootNodeIndex;

        const int meshCount = static_cast<int>(gltfModel.meshes.size());
        result.meshes.resize(meshCount);
        for (int i = 0; i < meshCount; i++)
        {
            result.meshes[i] = LoadGltfMesh(gltfModel, i, meshDatas);
        }

        const int nodeCount = static_cast<int>(gltfModel.nodes.size());
        result.nodes.resize(nodeCount);
        for (int i = 0; i < nodeCount; i++)
        {
            result.nodes[i] = LoadGltfNode(gltfModel, i);
        }

        const int textureCount = static_cast<int>(gltfModel.textures.size());
        std::vector<bool> textureIsSRGB(textureCount, false);

        for (const tinygltf::Material &gltfMaterial : gltfModel.materials)
        {
            for (int textureIndex : {gltfMaterial.pbrMetallicRoughness.baseColorTexture.index, gltfMaterial.emissiveTexture.index})
            {
                if (textureIndex > -1 && textureIndex < textureCount)
                {
                    textureIsSRGB[textureIndex] = true;
                }
            }
        }

        result.textures.resize(textureCount);
        for (int i = 0; i < textureCount; i++)
        {
            result.textures[i] = LoadGltfTexture(gltfModel, i, textureIsSRGB[i], textureDatas);
        }

        const int materialCount = static_cast<int>(gltfModel.materials.size());
        result.materials.resize(materialCount);
        for (int i = 0; i < materialCount; i++)
        {
            result.materials[i] = LoadGltfMaterial(gltfModel, i, result.textures, materialDatas);
        }

        return result;
    }

    GltfTexture AssetLoader::LoadGltfTexture(const tinygltf::Model &gltfModel, uint32_t index, bool isSRGB, std::vector<TextureData> &textureDatas)
    {
        const tinygltf::Texture &gltfTexture = gltfModel.textures[index];

        GltfTexture texture;

        const int imageIndex = gltfTexture.source;
        if (imageIndex < 0 || imageIndex >= static_cast<int>(gltfModel.images.size()))
        {
            throw std::runtime_error("glTF: texture \"" + gltfTexture.name + "\" has no valid image source!");
        }

        const tinygltf::Image &gltfImage = gltfModel.images[imageIndex];

        if (gltfImage.width <= 0 || gltfImage.height <= 0 || gltfImage.image.empty())
        {
            throw std::runtime_error("glTF: image \"" + gltfImage.name + "\" has no decoded pixel data!");
        }

        if (gltfImage.component != 4)
        {
            throw std::runtime_error("glTF: expected RGBA8 images, got " + std::to_string(gltfImage.component) + " channels!");
        }

        const size_t byteSize = static_cast<size_t>(gltfImage.width) * static_cast<size_t>(gltfImage.height) * 4;
        if (gltfImage.image.size() != byteSize)
        {
            throw std::runtime_error("glTF: image \"" + gltfImage.name + "\" is not tightly packed 8-bit RGBA!");
        }

        void *pixels = std::malloc(byteSize);
        if (pixels == nullptr)
        {
            throw std::runtime_error("glTF: out of memory while copying image \"" + gltfImage.name + "\"!");
        }
        std::memcpy(pixels, gltfImage.image.data(), byteSize);

        TextureData textureData;
        textureData.width = static_cast<uint32_t>(gltfImage.width);
        textureData.height = static_cast<uint32_t>(gltfImage.height);
        textureData.pixels = pixels;
        textureData.format = isSRGB ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;

        texture.textureDataIndex = static_cast<int>(textureDatas.size());

        textureDatas.push_back(textureData);

        return texture;
    }

    GltfMesh AssetLoader::LoadGltfMesh(const tinygltf::Model &gltfModel, uint32_t index, std::vector<MeshData> &meshDatas)
    {
        const tinygltf::Mesh &gltfMesh = gltfModel.meshes[index];
        GltfMesh mesh;

        mesh.name = gltfMesh.name;
        mesh.primitives.resize(gltfMesh.primitives.size());

        for (int i = 0; i < gltfMesh.primitives.size(); i++)
        {
            const tinygltf::Primitive &gltfPrimitive = gltfMesh.primitives[i];
            GltfPrimitive primitive;
            MeshData meshData;

            bool hasTangent;
            bool hasSkin;
            bool hasIndices = gltfPrimitive.indices > -1;

            glm::vec3 positionMin{};
            glm::vec3 positionMax{};

            const float *bufferPositions = nullptr;
            const float *bufferNormals = nullptr;
            const float *bufferTangents = nullptr;
            const float *bufferTexCoordSet0 = nullptr;
            const float *bufferTexCoordSet1 = nullptr;
            const float *bufferColorSet0 = nullptr;
            const void *bufferJoints = nullptr;
            const float *bufferWeights = nullptr;

            int positionByteStride;
            int normalByteStride;
            int tangentByteStride;
            int uv0ByteStride;
            int uv1ByteStride;
            int color0ByteStride;
            int jointByteStride;
            int weightByteStride;

            int jointComponentType;

            if (gltfPrimitive.mode != -1 && gltfPrimitive.mode != TINYGLTF_MODE_TRIANGLES)
            {
                throw std::runtime_error("glTF: only triangle primitives are supported");
            }

            if (gltfPrimitive.attributes.find("POSITION") == gltfPrimitive.attributes.end())
            {
                throw std::runtime_error("glTF: primitive has no POSITION attribute");
            }

            const tinygltf::Accessor &positionAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("POSITION")->second];
            const tinygltf::BufferView positionBufferView = gltfModel.bufferViews[positionAccessor.bufferView];
            bufferPositions = reinterpret_cast<const float *>(&(gltfModel.buffers[positionBufferView.buffer].data[positionAccessor.byteOffset + positionBufferView.byteOffset]));
            positionByteStride = positionAccessor.ByteStride(positionBufferView) ? (positionAccessor.ByteStride(positionBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
            
            positionMin = glm::vec3(positionAccessor.minValues[0], positionAccessor.minValues[1], positionAccessor.minValues[2]);
            positionMax = glm::vec3(positionAccessor.maxValues[0], positionAccessor.maxValues[1], positionAccessor.maxValues[2]);
            
            meshData.vertices.resize(positionAccessor.count);

            if (gltfPrimitive.attributes.find("NORMAL") != gltfPrimitive.attributes.end())
            {
                const tinygltf::Accessor &normalAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("NORMAL")->second];
                const tinygltf::BufferView &normalBufferView = gltfModel.bufferViews[normalAccessor.bufferView];
                bufferNormals = reinterpret_cast<const float *>(&(gltfModel.buffers[normalBufferView.buffer].data[normalAccessor.byteOffset + normalBufferView.byteOffset]));
                normalByteStride = normalAccessor.ByteStride(normalBufferView) ? (normalAccessor.ByteStride(normalBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
            }

            if (gltfPrimitive.attributes.find("TANGENT") != gltfPrimitive.attributes.end())
            {
                const tinygltf::Accessor &tangentAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("TANGENT")->second];
                const tinygltf::BufferView &tangentBufferView = gltfModel.bufferViews[tangentAccessor.bufferView];
                bufferTangents = reinterpret_cast<const float *>(&(gltfModel.buffers[tangentBufferView.buffer].data[tangentAccessor.byteOffset + tangentBufferView.byteOffset]));
                tangentByteStride = tangentAccessor.ByteStride(tangentBufferView) ? (tangentAccessor.ByteStride(tangentBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
            }

            if (gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end())
            {
                const tinygltf::Accessor &uvAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView &uvBufferView = gltfModel.bufferViews[uvAccessor.bufferView];
                bufferTexCoordSet0 = reinterpret_cast<const float *>(&(gltfModel.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
                uv0ByteStride = uvAccessor.ByteStride(uvBufferView) ? (uvAccessor.ByteStride(uvBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
            }

            // TODO:: Add those unused data to process (maybe)
            if (gltfPrimitive.attributes.find("TEXCOORD_1") != gltfPrimitive.attributes.end())
            {
                const tinygltf::Accessor &uvAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("TEXCOORD_1")->second];
                const tinygltf::BufferView &uvBufferView = gltfModel.bufferViews[uvAccessor.bufferView];
                bufferTexCoordSet1 = reinterpret_cast<const float *>(&(gltfModel.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
                uv1ByteStride = uvAccessor.ByteStride(uvBufferView) ? (uvAccessor.ByteStride(uvBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
            }

            if (gltfPrimitive.attributes.find("COLOR_0") != gltfPrimitive.attributes.end())
            {
                const tinygltf::Accessor &colorAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("COLOR_0")->second];
                const tinygltf::BufferView &colorBufferView = gltfModel.bufferViews[colorAccessor.bufferView];
                bufferColorSet0 = reinterpret_cast<const float *>(&(gltfModel.buffers[colorBufferView.buffer].data[colorAccessor.byteOffset + colorBufferView.byteOffset]));
                color0ByteStride = colorAccessor.ByteStride(colorBufferView) ? (colorAccessor.ByteStride(colorBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
            }

            if (gltfPrimitive.attributes.find("JOINTS_0") != gltfPrimitive.attributes.end())
            {
                const tinygltf::Accessor &jointAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("JOINTS_0")->second];
                const tinygltf::BufferView &jointBufferView = gltfModel.bufferViews[jointAccessor.bufferView];
                bufferJoints = &(gltfModel.buffers[jointBufferView.buffer].data[jointAccessor.byteOffset + jointBufferView.byteOffset]);
                jointComponentType = jointAccessor.componentType;
                jointByteStride = jointAccessor.ByteStride(jointBufferView) ? (jointAccessor.ByteStride(jointBufferView) / tinygltf::GetComponentSizeInBytes(jointComponentType)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
            }

            if (gltfPrimitive.attributes.find("WEIGHTS_0") != gltfPrimitive.attributes.end())
            {
                const tinygltf::Accessor &weightAccessor = gltfModel.accessors[gltfPrimitive.attributes.find("WEIGHTS_0")->second];
                const tinygltf::BufferView &weightBufferView = gltfModel.bufferViews[weightAccessor.bufferView];
                bufferWeights = reinterpret_cast<const float *>(&(gltfModel.buffers[weightBufferView.buffer].data[weightAccessor.byteOffset + weightBufferView.byteOffset]));
                weightByteStride = weightAccessor.ByteStride(weightBufferView) ? (weightAccessor.ByteStride(weightBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
            }

            hasTangent = (bufferTangents != nullptr);
            hasSkin = (bufferJoints && bufferWeights);

            for (size_t v = 0; v < positionAccessor.count; v++)
            {
                MeshVertex vertex;

                vertex.position = glm::make_vec3(&bufferPositions[v * positionByteStride]);
                vertex.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normalByteStride]) : glm::vec3(0.0f)));
                vertex.texCoord = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec2(0.0f);
                
                if(hasTangent)
                {
                    vertex.tangent = glm::make_vec4(&bufferTangents[v * tangentByteStride]);
                }

                meshData.vertices[v] = vertex;
            }

            if (hasIndices)
            {
                const tinygltf::Accessor &indexAccessor = gltfModel.accessors[gltfPrimitive.indices > -1 ? gltfPrimitive.indices : 0];
                const tinygltf::BufferView &indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer &indexBuffer = gltfModel.buffers[indexBufferView.buffer];
                const void *bufferIndices = &(indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset]);

                meshData.indices.resize(static_cast<uint32_t>(indexAccessor.count));

                switch (indexAccessor.componentType)
                {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                {
                    const uint32_t *buffer = static_cast<const uint32_t *>(bufferIndices);
                    for (size_t index = 0; index < indexAccessor.count; index++)
                    {
                        meshData.indices[index] = buffer[index];
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                {
                    const uint16_t *buffer = static_cast<const uint16_t *>(bufferIndices);
                    for (size_t index = 0; index < indexAccessor.count; index++)
                    {
                        meshData.indices[index] = static_cast<uint32_t>(buffer[index]);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                {
                    const uint8_t *buffer = static_cast<const uint8_t *>(bufferIndices);
                    for (size_t index = 0; index < indexAccessor.count; index++)
                    {
                        meshData.indices[index] = static_cast<uint32_t>(buffer[index]);
                    }
                    break;
                }
                default:
                    throw std::runtime_error("Index component type " + std::to_string(indexAccessor.componentType) + " not supported!");
                }
            }
            else
            {
                meshData.indices.resize(meshData.vertices.size());
                for (size_t index = 0; index < meshData.indices.size(); index++)
                {
                    meshData.indices[index] = static_cast<uint32_t>(index);
                }
            }

            if (!hasTangent)
            {
                std::vector<glm::vec3> tangentAccumulators(meshData.vertices.size(), glm::vec3(0.0f));
                std::vector<glm::vec3> bitangentAccumulators(meshData.vertices.size(), glm::vec3(0.0f));

                for (size_t i = 0; i + 2 < meshData.indices.size(); i += 3)
                {
                    const uint32_t i0 = meshData.indices[i + 0];
                    const uint32_t i1 = meshData.indices[i + 1];
                    const uint32_t i2 = meshData.indices[i + 2];

                    const MeshVertex &v0 = meshData.vertices[i0];
                    const MeshVertex &v1 = meshData.vertices[i1];
                    const MeshVertex &v2 = meshData.vertices[i2];

                    const glm::vec3 edge1 = v1.position - v0.position;
                    const glm::vec3 edge2 = v2.position - v0.position;
                    const glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
                    const glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;

                    const float determinant = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
                    if (std::abs(determinant) < 1e-8f)
                    {
                        continue;
                    }

                    const float r = 1.0f / determinant;
                    const glm::vec3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * r;
                    const glm::vec3 bitangent = (edge2 * deltaUV1.x - edge1 * deltaUV2.x) * r;

                    tangentAccumulators[i0] += tangent;
                    tangentAccumulators[i1] += tangent;
                    tangentAccumulators[i2] += tangent;

                    bitangentAccumulators[i0] += bitangent;
                    bitangentAccumulators[i1] += bitangent;
                    bitangentAccumulators[i2] += bitangent;
                }

                for (size_t i = 0; i < meshData.vertices.size(); ++i)
                {
                    glm::vec3 tangent = tangentAccumulators[i];
                    const glm::vec3 &normal = meshData.vertices[i].normal;

                    if (glm::dot(tangent, tangent) > 1e-12f)
                    {
                        tangent -= normal * glm::dot(normal, tangent);
                    }

                    const float length = glm::length(tangent);
                    if (length > 1e-6f)
                    {
                        tangent /= length;
                    }
                    else
                    {
                        const glm::vec3 fallback = std::abs(normal.x) < 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                        tangent = glm::normalize(fallback - normal * glm::dot(normal, fallback));
                    }

                    const float handedness = glm::dot(glm::cross(normal, tangent), bitangentAccumulators[i]) < 0.0f ? -1.0f : 1.0f;
                    meshData.vertices[i].tangent = glm::vec4(tangent, handedness);
                }
            }

            primitive.boundingBox.min = positionMin;
            primitive.boundingBox.max = positionMax;
            primitive.boundingBox.valid = true;

            primitive.material = gltfPrimitive.material;

            primitive.meshDataIndex = static_cast<int>(meshDatas.size());

            meshDatas.push_back(std::move(meshData));

            mesh.primitives[i] = primitive;
        }

        for (GltfPrimitive& primitive : mesh.primitives) {
            if (primitive.boundingBox.valid && !mesh.boundingBox.valid) {
                mesh.boundingBox = primitive.boundingBox;
                mesh.boundingBox.valid = true;
            }
            mesh.boundingBox.min = glm::min(mesh.boundingBox.min, primitive.boundingBox.min);
            mesh.boundingBox.max = glm::max(mesh.boundingBox.max, primitive.boundingBox.max);
        }

        return mesh;
    }

    GltfNode AssetLoader::LoadGltfNode(const tinygltf::Model &gltfModel, uint32_t index)
    {
        const tinygltf::Node &gltfNode = gltfModel.nodes[index];
        GltfNode node;

        node.index = index;
        node.name = gltfNode.name;
        node.matrix = glm::mat4(1.0f);

        glm::vec3 translation = glm::vec3(0.0f);
        if (gltfNode.translation.size() == 3)
        {
            translation = glm::make_vec3(gltfNode.translation.data());
        }

        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        if (gltfNode.rotation.size() == 4)
        {
            rotation = glm::make_quat(gltfNode.rotation.data());
        }

        glm::vec3 scale = glm::vec3(1.0f);
        if (gltfNode.scale.size() == 3)
        {
            scale = glm::make_vec3(gltfNode.scale.data());
        }

        if (gltfNode.matrix.size() == 16)
        {
            node.matrix = glm::make_mat4x4(gltfNode.matrix.data());
        }
        else
        {
            node.matrix = glm::translate(glm::mat4(1.0f), translation) *
                          glm::mat4_cast(rotation) *
                          glm::scale(glm::mat4(1.0f), scale);
        }

        if (gltfNode.mesh > -1)
        {
            if (gltfNode.mesh < gltfModel.meshes.size())
            {
                node.mesh = gltfNode.mesh;
            }
            else
            {
                throw std::runtime_error("glTF: node \"" + gltfNode.name + "\" references mesh " + std::to_string(gltfNode.mesh) + " which does not exist!");
            }
        }

        node.children = gltfNode.children;

        return node;
    }

    GltfMaterial AssetLoader::LoadGltfMaterial(const tinygltf::Model &gltfModel, uint32_t index, const std::vector<GltfTexture> &textures, std::vector<MaterialData> &materialDatas)
    {
        const tinygltf::Material &gltfMaterial = gltfModel.materials[index];

        GltfMaterial material;
        material.name = gltfMaterial.name;

        const tinygltf::PbrMetallicRoughness &pbr = gltfMaterial.pbrMetallicRoughness;

        if (pbr.baseColorFactor.size() == 4)
        {
            material.baseColorFactor = glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
        }

        material.metallicFactor = static_cast<float>(pbr.metallicFactor);
        material.roughnessFactor = static_cast<float>(pbr.roughnessFactor);

        material.baseColorTexture = pbr.baseColorTexture.index;
        material.metallicRoughnessTexture = pbr.metallicRoughnessTexture.index;

        material.normalTexture = gltfMaterial.normalTexture.index;
        material.normalScale = static_cast<float>(gltfMaterial.normalTexture.scale);

        material.occlusionTexture = gltfMaterial.occlusionTexture.index;
        material.occlusionStrength = static_cast<float>(gltfMaterial.occlusionTexture.strength);

        material.emissiveTexture = gltfMaterial.emissiveTexture.index;
        if (gltfMaterial.emissiveFactor.size() == 3)
        {
            material.emissiveFactor = glm::vec3(gltfMaterial.emissiveFactor[0], gltfMaterial.emissiveFactor[1], gltfMaterial.emissiveFactor[2]);
        }

        for (int textureIndex : {material.baseColorTexture, material.metallicRoughnessTexture, material.normalTexture, material.occlusionTexture, material.emissiveTexture})
        {
            if (textureIndex >= static_cast<int>(gltfModel.textures.size()))
            {
                throw std::runtime_error("glTF: material \"" + gltfMaterial.name + "\" references texture " + std::to_string(textureIndex) + " which does not exist!");
            }
        }

        MaterialData materialData;
        materialData.name = gltfMaterial.name;
        materialData.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
        materialData.doubleSided = gltfMaterial.doubleSided ? 1u : 0u;

        if (gltfMaterial.alphaMode == "MASK")
        {
            materialData.alphaMode = MaterialAlphaMode::eMask;
        }
        else if (gltfMaterial.alphaMode == "BLEND")
        {
            materialData.alphaMode = MaterialAlphaMode::eBlend;
        }
        else
        {
            materialData.alphaMode = MaterialAlphaMode::eOpaque;
        }

        materialData.setVector("baseColorFactor", material.baseColorFactor);
        materialData.setScalar("metallicFactor", material.metallicFactor);
        materialData.setScalar("roughnessFactor", material.roughnessFactor);
        materialData.setScalar("normalScale", material.normalScale);
        materialData.setScalar("occlusionStrength", material.occlusionStrength);
        materialData.setVector("emissiveFactor",
            glm::vec4(material.emissiveFactor.x, material.emissiveFactor.y, material.emissiveFactor.z, 0.0f));

        const auto textureInfo = [&textures](int gltfTextureIndex)
        {
            TextureInfoData info;
            info.textureIndex = gltfTextureIndex >= 0 ? textures[gltfTextureIndex].textureDataIndex : -1;
            return info;
        };

        materialData.setTexture("baseColorTexture", textureInfo(material.baseColorTexture));
        materialData.setTexture("metallicRoughnessTexture", textureInfo(material.metallicRoughnessTexture));
        materialData.setTexture("normalTexture", textureInfo(material.normalTexture));
        materialData.setTexture("occlusionTexture", textureInfo(material.occlusionTexture));
        materialData.setTexture("emissiveTexture", textureInfo(material.emissiveTexture));

        materialDatas.push_back(std::move(materialData));

        return material;
    }
    
    int AssetLoader::ProcessGltfModel(const GltfModel &model, int nodeIndex, const glm::mat4 &parentWorld, std::vector<RenderEntity> &renderEntities)
    {
        const GltfNode &node = model.nodes[nodeIndex];
        const glm::mat4 worldMatrix = parentWorld * node.matrix;

        RenderEntity entity;
        entity.modelMatrix = worldMatrix;

        if (node.mesh > -1)
        {
            const GltfMesh &mesh = model.meshes[node.mesh];

            if (mesh.primitives.size() == 1)
            {
                const GltfPrimitive &primitive = mesh.primitives[0];
                entity.meshIndex = primitive.meshDataIndex;
                entity.materialIndex = primitive.material;
                entity.boundingBox = primitive.boundingBox;
            }
            else
            {
                for (const GltfPrimitive &primitive : mesh.primitives)
                {
                    RenderEntity leaf;
                    leaf.modelMatrix = worldMatrix;
                    leaf.meshIndex = primitive.meshDataIndex;
                    leaf.materialIndex = primitive.material;
                    leaf.boundingBox = primitive.boundingBox;

                    renderEntities.push_back(std::move(leaf));
                    entity.children.push_back(static_cast<uint32_t>(renderEntities.size() - 1));
                }
            }
        }

        for (int childIndex : node.children)
        {
            const int childEntityIndex = ProcessGltfModel(model, childIndex, worldMatrix, renderEntities);
            if (childEntityIndex >= 0)
            {
                entity.children.push_back(static_cast<uint32_t>(childEntityIndex));
            }
        }

        if (node.mesh < 0 && entity.children.empty())
        {
            return -1;
        }

        renderEntities.push_back(std::move(entity));
        return static_cast<int>(renderEntities.size() - 1);
    }
}
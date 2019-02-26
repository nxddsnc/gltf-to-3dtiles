#pragma once
#include "MyMesh.h"
#include <vector>
#include <unordered_map>

namespace tinygltf
{
    class Model;
    class Material;
    class Primitive;
    class TinyGLTF;
    class Texture;
    class Sampler;
    class Image;
}
enum AccessorType
{
    POSITION,
    NORMAL,
    UV,
    INDEX,
    BATCH_ID
};

class GltfExporter
{
public:
    GltfExporter(vector<MyMeshInfo> myMeshInfos, string bufferName);
    ~GltfExporter();
    void ConstructNewModel();
    tinygltf::Model* GetNewModel() { return m_pNewModel; }
    bool GltfExporter::DoExport(std::string filepath, bool binary);
private:
    int addMesh(tinygltf::Material* material, MyMesh* myMesh);
    void addPrimitive(tinygltf::Primitive* primitive);
    int addAccessor(AccessorType type);
    int addBufferView(AccessorType type, size_t& byteOffset);
    int addBuffer(AccessorType type);

    int addMaterial(tinygltf::Material* material);
    int addTexture(tinygltf::Texture* texture);
    int addSampler(tinygltf::Sampler* sampler);
    int addImage(tinygltf::Image* image);
private:
    std::string m_bufferName;
    tinygltf::Model* m_pNewModel;
    tinygltf::Model* m_pModel;
    std::vector<MyMeshInfo> m_myMeshInfos;
    float m_positionMax[3];
    float m_positionMin[3];
    std::unordered_map<MyVertex*, uint32_t> m_vertexUintMap;
    std::unordered_map<MyVertex*, uint16_t> m_vertexUshortMap;
    std::unordered_map<tinygltf::Material*, int> m_materialIndexMap;
    std::unordered_map<tinygltf::Texture*, int> m_textureIndexMap;
    std::unordered_map<tinygltf::Sampler*, int> m_samplerIndexMap;
    std::unordered_map<tinygltf::Image*, int> m_imageIndexMap;
    std::vector<unsigned char> m_currentAttributeBuffer;
    std::vector<unsigned char> m_currentTexBuffer;
    std::vector<unsigned char> m_currentUShortIndexBuffer;
    std::vector<unsigned char> m_currentUIntIndexBuffer;
    std::vector<unsigned char> m_currentBatchIdBuffer;

    MyMesh* m_currentMesh;

    tinygltf::TinyGLTF* m_pTinyGTLF;
};
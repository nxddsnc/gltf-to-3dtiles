#pragma once
#include <vector>
#include "MyMesh.h"
#include <unordered_map>
#include <stdio.h>
#include "json.hpp"
#define MIN_FACE_NUM 10 // If face number is less then this, don't decimate then. 


enum AccessorType
{
    POSITION,
    NORMAL,
    UV,
    INDEX,
    BATCH_ID
};
namespace tinygltf
{
    class Model;
    class Node;
    class TinyGLTF;
	class Mesh;
	class Material;
    class Primitive;
    class BufferView;
}
class LodExporter
{
public:
    LodExporter(tinygltf::Model* model, std::vector<MyMesh*> myMeshes, tinygltf::TinyGLTF* tinyGLTF);
    ~LodExporter();
    void ExportLods(std::vector<TileInfo> lodInfos, int level);
    void SetTileInfo(TileInfo* tileInfo) { m_pTileInfo = tileInfo; }
    bool ExportTileset();
private:
    void getMeshIdxs(std::vector<int> nodeIdxs, std::vector<int>& meshIdxs);
    void traverseNode(tinygltf::Node* node, std::vector<int>& meshIdxs);

	int addNode(tinygltf::Node* node);
	int addMesh(tinygltf::Mesh* mesh);
	int addMaterial(int material);
	void addPrimitive(tinygltf::Primitive* primitive, tinygltf::Mesh* mesh);
	int addAccessor(AccessorType type);
    int addBufferView(AccessorType type, size_t& byteOffset);
    int addBuffer(AccessorType type);
	std::unordered_map<int, int> m_materialCache; // map between old material and new material;
    std::string getOutputFilePath(int level, int index);
    void traverseExportTile(TileInfo* tileInfo);
    nlohmann::json traverseExportTileSetJson(TileInfo* tileInfo);
private:
    tinygltf::Model* m_pModel;
	tinygltf::Model* m_pNewModel;
    std::vector<MyMesh*> m_myMeshes;
    TileInfo* m_pTileInfo;
    int m_currentTileLevel;
    TriEdgeCollapseQuadricParameter* m_pParams;
    tinygltf::TinyGLTF* m_pTinyGTLF;
    MyMesh* m_pCurrentMesh;
    float m_currentBatchId;
    nlohmann::json m_tilesetJson;
    std::vector<unsigned char> m_currentAttributeBuffer;
    std::vector<unsigned char> m_currentBatchIdBuffer;
    std::vector<unsigned char> m_currentIndexBuffer;

    std::unordered_map<MyVertex*, uint32_t> m_vertexUintMap;
    std::unordered_map<MyVertex*, uint16_t> m_vertexUshortMap;

    std::unordered_map<int, int> m_levelAccumMap;
    float m_positionMax[3];
    float m_positionMin[3];
};


#pragma once
#include <vector>
#include "MyMesh.h"

namespace tinygltf
{
    class Model;
    class Node;
    class TinyGLTF;
}
class LodExporter
{
public:
    LodExporter(tinygltf::Model* model, std::vector<MyMesh*> myMeshes, tinygltf::TinyGLTF* tinyGLTF);
    ~LodExporter();
    void ExportLods(std::vector<LodInfo> lodInfos, int level);
private:
    void getMeshIdxs(std::vector<int> nodeIdxs, std::vector<int>& meshIdxs);
    void traverseNode(tinygltf::Node* node, std::vector<int>& meshIdxs);
private:
    tinygltf::Model* m_pModel;
    std::vector<MyMesh*> m_myMeshes;
    TriEdgeCollapseQuadricParameter* m_qparams;
    tinygltf::TinyGLTF* m_tinyGTLF;
};


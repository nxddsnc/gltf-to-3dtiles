#pragma once
#include <vector>
#include "MyMesh.h"

namespace tinygltf
{
    class Model;
}
class LodExporter
{
public:
    LodExporter(tinygltf::Model* model, std::vector<MyMesh*> myMeshes);
    ~LodExporter();
    void ExportLods(std::vector<LodInfo> lodInfos, int level);
private:
    void GetMeshIdxs(std::vector<int> nodeIdxs, std::vector<int>& meshIdxs);
private:
    tinygltf::Model* m_pModel;
    std::vector<MyMesh*> m_myMeshes;
    TriEdgeCollapseQuadricParameter m_qparams;
};


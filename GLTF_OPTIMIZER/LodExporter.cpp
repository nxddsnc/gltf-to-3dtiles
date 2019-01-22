#include "LodExporter.h"
using namespace tinygltf;

LodExporter::LodExporter(tinygltf::Model* model, vector<MyMesh*> myMeshes)
{
    m_pModel = model;
    m_myMeshes = myMeshes;
}


LodExporter::~LodExporter()
{

}

void LodExporter::ExportLods(vector<LodInfo> lodInfos, int level)
{

}
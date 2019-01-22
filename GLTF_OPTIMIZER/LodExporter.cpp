#include "LodExporter.h"
#include "tiny_gltf.h"
using namespace tinygltf;

LodExporter::LodExporter(tinygltf::Model* model, vector<MyMesh*> myMeshes, tinygltf::TinyGLTF* tinyGLTF)
{
    m_pModel = model;
    m_myMeshes = myMeshes;

    m_qparams = new TriEdgeCollapseQuadricParameter();
    m_qparams->QualityThr = .3;
    m_qparams->PreserveBoundary = true; // Perserve mesh boundary
    m_qparams->PreserveTopology = false;
    m_tinyGTLF = tinyGLTF;
}

LodExporter::~LodExporter()
{
    if (m_qparams != NULL)
    {
        delete m_qparams;
        m_qparams = NULL;
    }
}

void LodExporter::getMeshIdxs(std::vector<int> nodeIdxs, std::vector<int>& meshIdxs)
{
    for (int i = 0; i < nodeIdxs.size(); ++i)
    {
        traverseNode(&(m_pModel->nodes[nodeIdxs[i]]), meshIdxs);
    }
}

void LodExporter::traverseNode(tinygltf::Node* node, std::vector<int>& meshIdxs)
{
    if (node->children.size() > 0) 
    {
        for (int i = 0; i < node->children.size(); ++i)
        {
            traverseNode(&(m_pModel->nodes[node->children[i]]), meshIdxs);
        }
    }
    if (node->mesh != -1)
    {
        meshIdxs.push_back(node->mesh);
    }
}

void LodExporter::ExportLods(vector<LodInfo> lodInfos, int level)
{
    for (int i = 0; i < lodInfos.size(); ++i)
    {
        std::vector<int> meshIdxs;
        getMeshIdxs(lodInfos[i].nodes, meshIdxs);

        float geometryError = 0;
        if (level > 0)
        {
            for (int j = 0; j < meshIdxs.size(); ++j)
            {
                MyMesh* myMesh = m_myMeshes[meshIdxs[j]];
                // decimator initialization
                vcg::LocalOptimization<MyMesh> deciSession(*myMesh, m_qparams);
                deciSession.Init<MyTriEdgeCollapse>();
                uint32_t finalSize = myMesh->fn * 0.5;
                deciSession.SetTargetSimplices(myMesh->fn * 0.5); // Target face number;
                deciSession.SetTimeBudget(0.5f); // Time budget for each cycle
                do
                {
                    deciSession.DoOptimization();
                } while (myMesh->fn > finalSize);
                geometryError += deciSession.currMetric;
            }
        }

        

    }
    /**********************   decimation    *******************************/

}
#include "SpatialTree.h"
#include "globals.h"
#include "tiny_gltf.h"
using namespace tinygltf;
using namespace std;
using namespace vcg;

SpatialTree::SpatialTree(Model* model, vector<MyMesh*> meshes)
{
    m_pModel = model;
    m_meshes = meshes;
    m_currentDepth = 0;
    m_pTileRoot = new TileInfo;
}


SpatialTree::~SpatialTree()
{
    deleteMyTreeNode(m_pRoot);
    deleteTileInfo(m_pTileRoot);
}

void SpatialTree::deleteTileInfo(TileInfo* tileInfo)
{
    if (tileInfo != NULL)
    {
        for (int i = 0; i < tileInfo->children.size(); ++i)
        {
            deleteTileInfo(tileInfo->children[i]);
        }
        delete tileInfo;
        tileInfo = NULL;
    }
}

TileInfo* SpatialTree::GetTilesetInfo()
{
    if (m_treeDepth < g_settings.tileLevel - 1)
    {
        for (int i = 0; i < g_settings.tileLevel - m_treeDepth - 1; ++i)
        {
            TileInfo* tileInfo = new TileInfo;
            tileInfo->boundingBox = m_pTileRoot->boundingBox;
            tileInfo->myMeshInfos = m_pTileRoot->myMeshInfos;
            tileInfo->children.push_back(m_pTileRoot);
            m_pTileRoot = tileInfo;
        }
        m_treeDepth = g_settings.tileLevel - 1;
    }
    recomputeTileBox(m_pTileRoot);
    return m_pTileRoot;
}

void SpatialTree::recomputeTileBox(TileInfo* parent)
{
    for (int i = 0; i < parent->children.size(); ++i)
    {
        recomputeTileBox(parent->children[i]);
    }
    parent->boundingBox->min.X() = parent->boundingBox->min.Y() = parent->boundingBox->min.Z() = INFINITY;
    parent->boundingBox->max.X() = parent->boundingBox->max.Y() = parent->boundingBox->max.Z() = -INFINITY;
    for (int i = 0; i < parent->myMeshInfos.size(); ++i)
    {
        parent->boundingBox->Add(parent->myMeshInfos[i].myMesh->bbox);
    }

    // Clear vector to save memory, but I don't think it neccessary;
    if (parent->children.size() > 0)
    {
        parent->myMeshInfos.clear();
    }
}

void SpatialTree::deleteMyTreeNode(MyTreeNode* node)
{
    if (node != NULL)
    {
        if (node->left != NULL) 
        {
            deleteMyTreeNode(node->left);
        }
        if (node->right != NULL)
        {
            deleteMyTreeNode(node->right);
        }
        delete node;
        node = NULL;
    }
}

void SpatialTree::Initialize()
{
    Box3f* sceneBox = new Box3f();
    sceneBox->min.X() = sceneBox->min.Y() = sceneBox->min.Z() = INFINITY;
    sceneBox->max.X() = sceneBox->max.Y() = sceneBox->max.Z() = -INFINITY;
    
    m_pTileRoot = new TileInfo;
    
    for (int i = 0; i < m_meshes.size(); ++i)
    {
        sceneBox->Add(m_meshes[i]->bbox);
        MyMeshInfo meshInfo;
        meshInfo.myMesh = m_meshes[i];
        meshInfo.material = &(m_pModel->materials[m_pModel->meshes[i].primitives[0].material]);
        m_pTileRoot->myMeshInfos.push_back(meshInfo);
    }
    m_pTileRoot->boundingBox = sceneBox;

    splitTreeNode(m_pTileRoot);
}

// FIXME: template?
bool myCompareX(MyMeshInfo& a, MyMeshInfo& b)
{
    return a.myMesh->bbox.Center().V()[0] < b.myMesh->bbox.Center().V()[0];
}

bool myCompareY(MyMeshInfo& a, MyMeshInfo& b)
{
    return a.myMesh->bbox.Center().V()[1] < b.myMesh->bbox.Center().V()[1];
}

bool myCompareZ(MyMeshInfo& a, MyMeshInfo& b)
{
    return a.myMesh->bbox.Center().V()[2] < b.myMesh->bbox.Center().V()[2];
}


void SpatialTree::splitTreeNode(TileInfo* parentTile)
{
    if (m_currentDepth > m_treeDepth) 
    {
        m_treeDepth = m_currentDepth;
    }

    m_currentDepth++;
    Point3f dim = parentTile->boundingBox->Dim();
    
    int i;
    int totalVertexCount = 0;
    for (i = 0; i < parentTile->myMeshInfos.size(); ++i)
    {
        totalVertexCount += parentTile->myMeshInfos[i].myMesh->vn;
    }
    parentTile->originalVertexCount = totalVertexCount;

    if (parentTile->myMeshInfos.size() < MIN_TREE_NODE || m_currentDepth > g_settings.maxTreeDepth || totalVertexCount <= g_settings.vertexCountPerTile)
    {
        m_currentDepth--;
        return;
    }

    TileInfo* pLeft = new TileInfo;
    TileInfo* pRight = new TileInfo;
    pLeft->boundingBox = new Box3f(*parentTile->boundingBox);
    pRight->boundingBox  = new Box3f(*parentTile->boundingBox);

    vector<MyMeshInfo> meshInfos = parentTile->myMeshInfos;
    int vertexCount = 0;

    if (dim.X() > dim.Y() && dim.X() > dim.Z())
    {
        // Split X
        sort(meshInfos.begin(), meshInfos.end(), myCompareX);
        for (i = 0; i < meshInfos.size(); ++i)
        {
            vertexCount += meshInfos[i].myMesh->vn;
            if (vertexCount < totalVertexCount / 2)
            {
                pLeft->myMeshInfos.push_back(meshInfos[i]);
            }
            else
            {
                pRight->myMeshInfos.push_back(meshInfos[i]);
            }
        }
    }
    else if (dim.Y() > dim.X() && dim.Y() > dim.Z())
    {
        // Split Y
        sort(meshInfos.begin(), meshInfos.end(), myCompareY);
        for (i = 0; i < meshInfos.size(); ++i)
        {
            vertexCount += meshInfos[i].myMesh->vn;
            if (vertexCount < totalVertexCount / 2)
            {
                pLeft->myMeshInfos.push_back(meshInfos[i]);
            }
            else
            {
                pRight->myMeshInfos.push_back(meshInfos[i]);
            }
        }
    }
    else
    {
        // Split Z
        sort(meshInfos.begin(), meshInfos.end(), myCompareZ);
        for (i = 0; i < meshInfos.size(); ++i)
        {
            vertexCount += meshInfos[i].myMesh->vn;
            if (vertexCount < totalVertexCount / 2)
            {
                pLeft->myMeshInfos.push_back(meshInfos[i]);
            }
            else
            {
                pRight->myMeshInfos.push_back(meshInfos[i]);
            }
        }
    }

    // delete if children is empty
    if (pLeft->myMeshInfos.size() == 0)
    {
        delete pLeft;
        pLeft = NULL;
    }
    else
    {
        splitTreeNode(pLeft);
        parentTile->children.push_back(pLeft);
    }

    // delete if children is empty
    if (pRight->myMeshInfos.size() == 0)
    {
        delete pRight;
        pRight = NULL;
    }
    else
    {
        splitTreeNode(pRight);
        parentTile->children.push_back(pRight);
    }

    m_currentDepth--;
}

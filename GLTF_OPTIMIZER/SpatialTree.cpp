#include "SpatialTree.h"
#include "globals.h"
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
            tileInfo->nodes = m_pTileRoot->nodes;
            tileInfo->children.push_back(m_pTileRoot);
            m_pTileRoot = tileInfo;
        }
        m_treeDepth = g_settings.tileLevel - 1;
    }
    return m_pTileRoot;
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

Box3f SpatialTree::getNodeBBox(Node* node)
{
    Box3f result;
    result.min.X() = result.min.Y() = result.min.Z() = INFINITY;
    result.max.X() = result.max.Y() = result.max.Z() = -INFINITY;

    if (node->mesh != -1)
    {
        MyMesh* mesh = m_meshes[node->mesh];
        vcg::tri::UpdateBounding<MyMesh>::Box(*mesh);
        Box3f box = mesh->bbox;
        if (node->matrix.size() > 0)
        {
            float matrixValues[16];
            for (int k = 0; k < 16; ++k)
            {
                matrixValues[k] = (float)node->matrix[k];
            }
            result.Add(matrixValues, box);
        }
        else
        {
            result.Add(box);
        }
    }
    for (int i = 0; i < node->children.size(); ++i)
    {
        Box3f box = getNodeBBox(&(m_pModel->nodes[node->children[i]]));
        if (m_pModel->nodes[node->children[i]].matrix.size() > 0)
        {
            float matrixValues[16];
            for (int k = 0; k < 16; ++k)
            {
                int row = k / 4;
                int col = k % 4;
                matrixValues[k] = (float)m_pModel->nodes[node->children[i]].matrix[col * 4 + row];
            }
            result.Add(matrixValues, box);
        }
        else
        {
            result.Add(box);
        }
    }
    return result;
}

void SpatialTree::Initialize()
{
    // Travese nodes and calculate bounding box for each element;
    // The tree structure of the gltf is corresponding to revit:
    // -scene_root
    //      -childElement
    //          -childMesh
    //      -childElement
    //          -childInstance
    //              -childMesh
    Node root = m_pModel->nodes[0];
    for (int i = 0; i < root.children.size(); i++)
    {
        Box3f box = getNodeBBox(&(m_pModel->nodes[root.children[i]]));
        m_nodeBoxMap.insert(make_pair(root.children[i], box));
    }
    
    int totalNodeSize = m_pModel->nodes[0].children.size();
    Box3f* sceneBox = new Box3f();
    sceneBox->min.X() = sceneBox->min.Y() = sceneBox->min.Z() = INFINITY;
    sceneBox->max.X() = sceneBox->max.Y() = sceneBox->max.Z() = -INFINITY;
    for (int i = 0; i < totalNodeSize; ++i)
    {
        sceneBox->Add(m_nodeBoxMap[i]);
    }
    m_pRoot = new MyTreeNode;
    m_pRoot->nodes = m_pModel->nodes[0].children;
    m_pRoot->boundingBox = sceneBox;

    splitTreeNode(m_pRoot, m_pTileRoot);
}

void SpatialTree::splitTreeNode(MyTreeNode* father, TileInfo* parentTile)
{
    parentTile->boundingBox = father->boundingBox;
    parentTile->nodes = father->nodes;

    if (m_currentDepth > m_treeDepth) 
    {
        m_treeDepth = m_currentDepth;
    }

    m_currentDepth++;
    Point3f dim = father->boundingBox->Dim();
    if (father->nodes.size() < MIN_TREE_NODE || m_currentDepth > g_settings.maxTreeDepth)
    {
        m_currentDepth--;
        return;
    }

    MyTreeNode* pLeft = new MyTreeNode();
    MyTreeNode* pRight = new MyTreeNode();
    pLeft->boundingBox = new Box3f(*father->boundingBox);
    pRight->boundingBox  = new Box3f(*father->boundingBox);

    if (dim.X() > dim.Y() && dim.X() > dim.Z())
    {
        // Split X
        pLeft->boundingBox->max.X() = pRight->boundingBox->min.X() = father->boundingBox->Center().X();
    }
    else if (dim.Y() > dim.X() && dim.Y() > dim.Z())
    {
        // Split Y
        pLeft->boundingBox->max.Y() = pRight->boundingBox->min.Y() = father->boundingBox->Center().Y();
    }
    else
    {
        // Split Z
        pLeft->boundingBox->max.Z() = pRight->boundingBox->min.Z() = father->boundingBox->Center().Z();
    }

    for (int i = 0; i < father->nodes.size(); ++i)
    {
        int nodeIdx = father->nodes[i];
        Box3f childBox = m_nodeBoxMap.at(nodeIdx);
        if (pLeft->boundingBox->IsInEx(childBox.Center()))
        {
            pLeft->nodes.push_back(nodeIdx);
        }
        else
        {
            pRight->nodes.push_back(nodeIdx);
        }
    }

    // delete if children is empty
    if (pLeft->nodes.size() == 0)
    {
        delete pLeft;
        pLeft = NULL;
    }
    else
    {
        TileInfo* pLeftTile = new TileInfo;
        parentTile->children.push_back(pLeftTile);
        splitTreeNode(pLeft, pLeftTile);
        father->left = pLeft;
    }

    // delete if children is empty
    if (pRight->nodes.size() == 0)
    {
        delete pRight;
        pRight = NULL;
    }
    else
    {
        TileInfo* pRightTile = new TileInfo;
        parentTile->children.push_back(pRightTile);
        splitTreeNode(pRight, pRightTile);
        father->right = pRight;
    }

    m_currentDepth--;
}

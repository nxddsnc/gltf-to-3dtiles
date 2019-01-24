#include "SpatialTree.h"
using namespace tinygltf;
using namespace std;
using namespace vcg;

SpatialTree::SpatialTree(Model* model, vector<MyMesh*> meshes)
{
    m_pModel = model;
    m_meshes = meshes;
    m_currentDepth = 0;
}


SpatialTree::~SpatialTree()
{
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

    SplitTreeNode(m_pRoot);
}

void SpatialTree::SplitTreeNode(MyTreeNode* father)
{
    LodInfo lodInfo;
    lodInfo.level = m_currentDepth;
    lodInfo.nodes = father->nodes;
    lodInfo.boundingBox = father->boundingBox;

    if (m_levelLodInfosMap.count(MAX_DEPTH - m_currentDepth) > 0)
    {
        m_levelLodInfosMap.at(MAX_DEPTH - m_currentDepth).push_back(lodInfo);
    }
    else 
    {
        std::vector<LodInfo> lodInfos;
        lodInfos.push_back(lodInfo);
        m_levelLodInfosMap.insert(make_pair(MAX_DEPTH - m_currentDepth, lodInfos));
    }

    m_currentDepth++;
    Point3f dim = father->boundingBox->Dim();
    if (father->nodes.size() < MIN_TREE_NODE || m_currentDepth > MAX_DEPTH)
    {
        m_currentDepth--;
        return;
    }

    MyTreeNode* pLeft = new MyTreeNode();
    MyTreeNode* pRight = new MyTreeNode();
    pLeft->boundingBox = new Box3f(*father->boundingBox);
    pRight->boundingBox  = new Box3f(*father->boundingBox);
    father->left = pLeft;
    father->right = pRight;
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

    SplitTreeNode(pLeft);
    SplitTreeNode(pRight);
    m_currentDepth--;
}

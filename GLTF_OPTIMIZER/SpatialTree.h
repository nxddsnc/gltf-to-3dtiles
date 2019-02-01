#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include "vcg/space//box3.h"
#include "tiny_gltf.h"
#include "MyMesh.h"
#define MIN_TREE_NODE 8
#define MAX_DEPTH 4


class MyTreeNode
{
public:
    MyTreeNode() {}
    ~MyTreeNode()
    {
        if (boundingBox != NULL)
        {
            delete boundingBox;
            boundingBox = NULL;
        }
    }
    std::vector<int> nodes;
    vcg::Box3f* boundingBox = NULL;
    MyTreeNode* left = NULL;
    MyTreeNode* right = NULL;
};

namespace tinygltf
{
    class Node;
}

class MyMesh;

class SpatialTree
{
public:
    SpatialTree(tinygltf::Model* model, std::vector<MyMesh*> meshes);
    ~SpatialTree();

    MyTreeNode* GetRoot() { return m_pRoot; }
    void Initialize();
    TileInfo* GetTilesetInfo();
private: 
    tinygltf::Model* m_pModel;
    std::vector<MyMesh*> m_meshes;
    MyTreeNode* m_pRoot;
    std::unordered_map<int, vcg::Box3f> m_nodeBoxMap; // Start from the sceond node of the scene.
    int m_currentDepth;
    int m_treeDepth;
    TileInfo* m_pTileRoot;
    Box3f getNodeBBox(tinygltf::Node* node);

    void deleteMyTreeNode(MyTreeNode* node);
    void deleteTileInfo(TileInfo* tileInfo);
    void recomputeTileBox(TileInfo* parent);
private:
    void splitTreeNode(MyTreeNode* father, TileInfo* parentTile);
};


#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include "vcg/space//box3.h"
#include "tiny_gltf.h"
#include "MyMesh.h"
#define MIN_TREE_NODE 8
#define MAX_DEPTH 4


struct MyTreeNode
{
    std::vector<int> nodes;
    vcg::Box3f* boundingBox;
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
    std::map<int, std::vector<TileInfo>> GetLodInfosMap() { return m_levelLodInfosMap; }
    void Initialize();
    void SetMaxTreeDepth(int maxDepth) { m_maxTreeDepth = maxDepth; }
    void SetTileTotalLevels(int  tileLevels) { m_tileLevels = tileLevels; }
private: 
    tinygltf::Model* m_pModel;
    std::vector<MyMesh*> m_meshes;
    MyTreeNode* m_pRoot;
    std::unordered_map<int, vcg::Box3f> m_nodeBoxMap; // Start from the sceond node of the scene.
    int m_currentDepth;
    int m_maxTreeDepth;
    int m_tileLevels;
    std::map<int, std::vector<TileInfo>> m_levelLodInfosMap;
    Box3f getNodeBBox(tinygltf::Node* node);

    void deleteMyTreeNode(MyTreeNode* node);
private:
    void SplitTreeNode(MyTreeNode* father);
};


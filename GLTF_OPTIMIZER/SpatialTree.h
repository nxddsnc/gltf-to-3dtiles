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

class MyMesh;

class SpatialTree
{
public:
    SpatialTree(tinygltf::Model* model, std::vector<MyMesh*> meshes);
    ~SpatialTree();

    MyTreeNode* GetRoot() { return m_pRoot; }
    std::map<int, std::vector<LodInfo>> GetLodInfosMap() { return m_levelLodInfosMap; }
    void Initialize();
private: 
    tinygltf::Model* m_pModel;
    std::vector<MyMesh*> m_meshes;
    MyTreeNode* m_pRoot;
    std::unordered_map<int, vcg::Box3f> m_nodeBoxMap;
    int m_currentDepth;
    std::map<int, std::vector<LodInfo>> m_levelLodInfosMap;
private:
    void SplitTreeNode(MyTreeNode* father);
};


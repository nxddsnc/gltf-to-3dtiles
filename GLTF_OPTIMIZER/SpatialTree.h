#pragma once
#include <vector>
#include <unordered_map>
#include "vcg/space//box3.h"
#include "tiny_gltf.h"
#define MIN_TREE_NODE 8
#define MIN_BOX_SIZE 1.0


struct MyTreeNode
{
    std::vector<int> nodes;
    vcg::Box3f* boundingBox;
    MyTreeNode* left;
    MyTreeNode* right;
};

class MyMesh;

class SpatialTree
{
public:
    SpatialTree(tinygltf::Model* model, std::vector<MyMesh*> meshes);
    ~SpatialTree();

    MyTreeNode* GetRoot() { return m_pRoot; }
    void Initialize();
private: 
    tinygltf::Model* m_pModel;
    std::vector<MyMesh*> m_meshes;
    MyTreeNode* m_pRoot;
    std::unordered_map<int, vcg::Box3f> m_nodeBoxMap;
private:
    void SplitTreeNode(MyTreeNode* father);
};


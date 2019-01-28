#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include "tiny_gltf.h"
#define MATERIAL_EPS 0.01
//
//namespace tinygltf
//{
//	class Model;
//}

class MyMesh;
class MyVertex;
enum AccessorType
{
    POSITION,
    NORMAL,
    UV,
    INDEX,
    BATCH_ID
};
// If the hash compare goes slow, optimize this function.
struct material_hash_fn {
    unsigned long operator()(const tinygltf::Material& material) const {
        unsigned long hash = 0;
        hash += 31 * material.values.size();
        std::map<std::string, tinygltf::Parameter>::const_iterator it = material.values.begin();
        // assume the color values are from 0-1.
        for (it = material.values.begin(); it != material.values.end(); ++it)
        {
            if (it->second.has_number_value)
            {
                hash += 31 * it->second.number_value;
            }
            else
            {
                for (int i = 0; i < it->second.number_array.size(); ++i)
                {
                    hash += int(it->second.number_array[i] * 255.0) * 31;
                }
            }
        }
        return hash;
    }
};

struct material_equal_fn {
    bool operator()(const tinygltf::Material& m1, const tinygltf::Material& m2) const {
        if (m1.values.size() != m2.values.size())
        {
            return false;
        }
        std::map<std::string, tinygltf::Parameter>::const_iterator it;
        for (it = m1.values.begin(); it != m1.values.end(); ++it)
        {
            std::string name = it->first;
            if (m2.values.count(name) < 1)
                return false;
            tinygltf::Parameter values = m2.values.at(name);

            if (it->second.has_number_value != values.has_number_value ||
                it->second.bool_value != values.bool_value)
            {
                return false;
            }
            if (std::abs(it->second.number_value - values.number_value) > MATERIAL_EPS)
            {
                return false;
            }
            if (it->second.number_array.size() != values.number_array.size())
            {
                return false;
            }
            for (int i = 0; i < it->second.number_array.size(); ++i)
            {
                if (std::abs(it->second.number_array[i] - values.number_array[i]) > MATERIAL_EPS)
                {
                    return false;
                }
            }
        }
        return true;
    }
};

class MergeMesh
{
public:
	MergeMesh(tinygltf::Model* model, tinygltf::Model* newModel, std::vector<MyMesh*> myMeshes, std::vector<int> nodesToMerge, std::string bufferName);
	~MergeMesh();

	void DoMerge();
private :
    bool meshComparenFunction(int meshIdx1, int meshIdx2);
    void addMergedMeshesToNewModel(int materialIdx, std::vector<MyMesh*> meshes);

    void MergeMesh::addPrimitive(tinygltf::Primitive* primitive);
    int MergeMesh::addAccessor(AccessorType type);
    int MergeMesh::addBufferView(AccessorType type, size_t& byteOffset);
    int addBuffer(AccessorType type);
private:
	tinygltf::Model* m_pModel;
    tinygltf::Model* m_pNewModel;
	std::vector<MyMesh*> m_myMeshes;
    std::vector<int> m_nodesToMerge;
    std::string m_bufferName;
    std::unordered_map<tinygltf::Material, std::vector<MyMesh*>, material_hash_fn, material_equal_fn> m_materialMeshMap;

    std::unordered_map<MyVertex*, uint32_t> m_vertexUintMap;
    std::unordered_map<MyVertex*, uint16_t> m_vertexUshortMap;
    std::vector<unsigned char> m_currentAttributeBuffer;
    std::vector<unsigned char> m_currentBatchIdBuffer;
    std::vector<unsigned char> m_currentIndexBuffer;
    float m_positionMax[3];
    float m_positionMin[3];
    int m_currentMeshIdx;

    int m_totalVertex;
    int m_totalFace;

    std::vector<MyMesh*> m_currentMeshes;
};
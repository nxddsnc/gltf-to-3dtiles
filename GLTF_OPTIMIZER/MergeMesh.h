#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include "tiny_gltf.h"
#include "MyMesh.h"
#define MATERIAL_EPS 0.01
#define MIN_FACE_NUM 10 // If face number is less then this, don't decimate then. 

//
//namespace tinygltf
//{
//	class Model;
//}

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

class MeshOptimizer
{
public:
	MeshOptimizer(std::vector<MyMeshInfo> meshInfos);
	~MeshOptimizer();

	void DoMerge();
    float DoDecimation(float targetPercetage);
private:
    void mergeSameMaterialMeshes(tinygltf::Material* material, std::vector<MyMesh*> meshes);
private:
    std::unordered_map<tinygltf::Material, std::vector<MyMesh*>, material_hash_fn, material_equal_fn> m_materialMeshMap;

    std::vector<MyMeshInfo> m_meshInfos;
	std::vector<MyMeshInfo> m_mergeMeshInfos;

    TriEdgeCollapseQuadricParameter* m_pParams;
};
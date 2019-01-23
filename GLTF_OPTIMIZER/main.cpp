#pragma once
#include "SpatialTree.h"
#include "MyMesh.h"
#include <stdint.h>
#include "tiny_gltf.h"
#include "LodExporter.h"
using namespace tinygltf;
using namespace std;

int main(int argc, char *argv[])
{
    char* inputPath = NULL;
    char* outputPath = NULL;
    uint32_t idBegin = -1;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "-i") == 0)
        {
            if (i + 1 >= argc)
            {
                std::printf("Input error\n");
                return -1;
            }
            inputPath = argv[i + 1];
        }
        else if (std::strcmp(argv[i], "-o") == 0)
        {
            if (i + 1 >= argc)
            {
                std::printf("Input error\n");
                return -1;
            }
            outputPath = argv[i + 1];
        }
        else if (std::strcmp(argv[i], "-idBegin") == 0)
        {

            if (i + 1 >= argc)
            {
                std::printf("Input error\n");
                return -1;
            }
            idBegin = atoi(argv[i + 1]);
        }
    }

    std::printf("intput file path: %s\n", inputPath);
    
    /****************************************  step0. Read gltf into vcglib mesh.  ******************************************************/
    Model *model = new Model;
    vector<MyMesh*> myMeshes;
    TinyGLTF* tinyGltf = new TinyGLTF;
    std::string err;
    std::string warn;
    bool ret = tinyGltf->LoadASCIIFromFile(model, &err, &warn, inputPath);

    for (int i = 0; i < model->meshes.size(); ++i)
    {
        MyMesh* myMesh = new MyMesh();
        myMeshes.push_back(myMesh);

        Mesh mesh = model->meshes[i];
        int positionAccessorIdx = mesh.primitives[0].attributes.at("POSITION");
        int normalAccessorIdx = mesh.primitives[0].attributes.at("NORMAL");
        int indicesAccessorIdx = mesh.primitives[0].indices;
        Accessor positionAccessor = model->accessors[positionAccessorIdx];
        Accessor normalAccessor = model->accessors[normalAccessorIdx];
        Accessor indicesAccessor = model->accessors[indicesAccessorIdx];
        BufferView positionBufferView = model->bufferViews[positionAccessor.bufferView];
        BufferView normalBufferView = model->bufferViews[normalAccessor.bufferView];
        BufferView indicesBufferView = model->bufferViews[indicesAccessor.bufferView];
        std::vector<unsigned char> positionBuffer = model->buffers[positionBufferView.buffer].data;
        std::vector<unsigned char> normalBuffer = model->buffers[normalBufferView.buffer].data;
        std::vector<unsigned char> indicesBuffer = model->buffers[indicesBufferView.buffer].data;
        
        float* positions = (float *)(model->buffers[positionBufferView.buffer].data.data() +
            positionBufferView.byteOffset +
            positionAccessor.byteOffset);
        float* normals = (float*)(model->buffers[normalBufferView.buffer].data.data() +
            normalBufferView.byteOffset +
            normalAccessor.byteOffset);
        uint16_t* indices = (uint16_t*)(model->buffers[indicesBufferView.buffer].data.data() +
            indicesBufferView.byteOffset +
            indicesAccessor.byteOffset);

        std::vector<VertexPointer> index;
        VertexIterator vi = Allocator<MyMesh>::AddVertices(*myMesh, positionAccessor.count);
        
        for (int j = 0; j < positionAccessor.count; ++j)
        {
            (*vi).P()[0] = positions[j * 3 + 0];
            (*vi).P()[1] = positions[j * 3 + 1];
            (*vi).P()[2] = positions[j * 3 + 2];

            (*vi).N()[0] = normals[j * 3 + 0];
            (*vi).N()[1] = normals[j * 3 + 1];
            (*vi).N()[2] = normals[j * 3 + 2];

            //(*vi).T().P().X() = va.u;
            //(*vi).T().P().Y() = va.v;
            ++vi;
        }

        index.resize(positionAccessor.count);
        vi = myMesh->vert.begin();
        for (int j = 0; j < positionAccessor.count; ++j, ++vi)
            index[j] = &*vi;

        int faceNum = indicesAccessor.count / 3;
        FaceIterator fi = Allocator<MyMesh>::AddFaces(*myMesh, faceNum);

        for (int j = 0; j < faceNum; ++j)
        {
            (*fi).V(0) = index[indices[j * 3 + 0]];
            (*fi).V(1) = index[indices[j * 3 + 1]];
            (*fi).V(2) = index[indices[j * 3 + 2]];
            ++fi;
        }

        //char testOutputPath[1024];
        //sprintf(testOutputPath, "../data/after-%d.ply", i);
        //vcg::tri::io::ExporterPLY<MyMesh>::Save(myMesh, testOutputPath);
    }

    /****************************************  step0. Read gltf into vcglib mesh.  ******************************************************/

    SpatialTree spatialTree = SpatialTree(model, myMeshes);
    spatialTree.Initialize();

    LodExporter lodExporter = LodExporter(model, myMeshes, tinyGltf);
    lodExporter.SetOutputDir(outputPath);
    std::map<int, std::vector<LodInfo>> lodInfosMap = spatialTree.GetLodInfosMap();
    std::map<int, std::vector<LodInfo>>::reverse_iterator it = lodInfosMap.rbegin();
    int maxLevel = it->first;
    for (; it != lodInfosMap.rend(); ++it)
    {
        vector<LodInfo> lodInfos = it->second;
        lodExporter.ExportLods(lodInfos, it->first);
    }

    /***********************  step1. Figure out the material with different ids and have the same value. ********************************/
    // TODO:
	/*for (int i = 0;i < model->meshes.size(); ++i)
	{
		int materialIdx = model->meshes[i].primitives[0].material;
		
	}*/
	
    /***********************  step1. Figure out the material with different ids and have the same value. ********************************/

    /***********************  step2. Simpilify meshes. ( This should be done before the mesh is merged.  ********************************/
    // Since we use quadratic error method, the gaps between maybe closed. 
    
    
    /***********************  step2. Simpilify meshes. ( This should be done before the mesh is merged.  ********************************/
    // TODO: 

    /**********************   step3. Merge the meshes and add "batchId" in vertex attributes.   *****************************************/
    // TODO:

    /**********************   step3. Merge the meshes and add "batchId" in vertex attributes.   *****************************************/
    return 0;
}
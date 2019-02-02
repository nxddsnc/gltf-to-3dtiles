#pragma once
#include "SpatialTree.h"
#include "MyMesh.h"
#include <stdint.h>
#include "tiny_gltf.h"
#include "LodExporter.h"
#include "globals.h"
#include "GltfUtils.h"
#include <ctime>
#define TILE_LEVEL 8
using namespace tinygltf;
using namespace std;

GlobalSetting g_settings;
// arguments:
// -i Input gltf file path.
// -o Output directory/
// -d Max depth of the binary tree.
// -l Lod number.
// -b output glb instead of gltf.
int main(int argc, char *argv[])
{
	// TODO: Make these variables global.
	g_settings.inputPath = NULL;
	g_settings.outputPath = NULL;
	g_settings.maxTreeDepth = MAX_DEPTH;
	g_settings.tileLevel = TILE_LEVEL;
	g_settings.writeBinary = false;
    g_settings.printLog = true;
    for (int i = 1; i < argc; ++i)
	{
        if (std::strcmp(argv[i], "-i") == 0)
        {
            if (i + 1 >= argc)
            {
                std::printf("Input error\n");
                return -1;
            }
			g_settings.inputPath = argv[i + 1];
        }
        else if (std::strcmp(argv[i], "-o") == 0)
        {
            if (i + 1 >= argc)
            {
                std::printf("Input error\n");
                return -1;
            }
			g_settings.outputPath = argv[i + 1];
        }
        else if (std::strcmp(argv[i], "-d") == 0)
        {
            if (i + 1 >= argc)
            {
                std::printf("Input error\n");
                return -1;
            }
			g_settings.maxTreeDepth = atoi(argv[i + 1]);
        }
        else if (std::strcmp(argv[i], "-l") == 0)
        {
            if (i + 1 >= argc)
            {
                std::printf("Input error\n");
                return -1;
            }
			g_settings.tileLevel = atoi(argv[i + 1]);
        }
		else if (std::strcmp(argv[i], "-b") == 0)
		{
			g_settings.writeBinary = true;
		}
    }

    std::printf("intput file path: %s\n", g_settings.inputPath);
	std::printf("output file dir: %s\n", g_settings.outputPath);
    
    /****************************************  step0. Read gltf into vcglib mesh.  ******************************************************/
    Model *model = new Model;
    vector<MyMesh*> myMeshes;
    TinyGLTF* tinyGltf = new TinyGLTF;
    std::string err;
    std::string warn;
    bool ret = tinyGltf->LoadASCIIFromFile(model, &err, &warn, g_settings.inputPath);

    if (g_settings.printLog)
    {
        std::printf("************************************************\n");
        std::printf("mesh number: %d\n", model->meshes.size());
        std::printf("material number: %d\n", model->materials.size());
        std::printf("************************************************\n");
    }

    for (int i = 0; i < model->meshes.size(); ++i)
    {
        MyMesh* myMesh = new MyMesh();
        myMeshes.push_back(myMesh);

        Mesh* mesh = &(model->meshes[i]);
        int positionAccessorIdx = mesh->primitives[0].attributes.at("POSITION");
        int normalAccessorIdx = mesh->primitives[0].attributes.at("NORMAL");
        int indicesAccessorIdx = mesh->primitives[0].indices;
        Accessor* positionAccessor = &(model->accessors[positionAccessorIdx]);
        Accessor* normalAccessor = &(model->accessors[normalAccessorIdx]);
        Accessor* indicesAccessor = &(model->accessors[indicesAccessorIdx]);
        BufferView* positionBufferView = &(model->bufferViews[positionAccessor->bufferView]);
        BufferView* normalBufferView = &(model->bufferViews[normalAccessor->bufferView]);
        BufferView* indicesBufferView = &(model->bufferViews[indicesAccessor->bufferView]);
        std::vector<unsigned char>& positionBuffer = model->buffers[positionBufferView->buffer].data;
        std::vector<unsigned char>& normalBuffer = model->buffers[normalBufferView->buffer].data;
        std::vector<unsigned char>& indicesBuffer = model->buffers[indicesBufferView->buffer].data;
        
        float* positions = (float *)(model->buffers[positionBufferView->buffer].data.data() +
            positionBufferView->byteOffset +
            positionAccessor->byteOffset);
        float* normals = (float*)(model->buffers[normalBufferView->buffer].data.data() +
            normalBufferView->byteOffset +
            normalAccessor->byteOffset);
        uint16_t* indices = (uint16_t*)(model->buffers[indicesBufferView->buffer].data.data() +
            indicesBufferView->byteOffset +
            indicesAccessor->byteOffset);

        std::vector<VertexPointer> index;
        VertexIterator vi = Allocator<MyMesh>::AddVertices(*myMesh, positionAccessor->count);
        

        for (int j = 0; j < positionAccessor->count; ++j)
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

        index.resize(positionAccessor->count);
        vi = myMesh->vert.begin();
        for (int j = 0; j < positionAccessor->count; ++j, ++vi)
            index[j] = &*vi;

        int faceNum = indicesAccessor->count / 3;
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

    std::printf("My Meshes created\n");
    /****************************************  step0. Read gltf into vcglib mesh.  ******************************************************/

	/****************************************  step1. Write batchIds to mesh color component  ******************************************************/

    g_settings.batchLength = model->nodes[0].children.size();
	for (int i = 0; i < model->nodes[0].children.size(); ++i)
	{
		int nodeIdx = model->nodes[0].children[i];
		Node* node = &(model->nodes[nodeIdx]);
		std::vector<int> meshIdxs;
		GetNodeMeshIdx(model, node, meshIdxs);
		for (int j = 0; j < meshIdxs.size(); ++j)
		{
			MyMesh* mesh = myMeshes[meshIdxs[j]];

            float nodeId = i;//  atoi(node->name.c_str());
			// Write the batchIds into vertex color component in little endian.
            // TODO: Check out what happens when two vertices merged into one.
            vector<MyVertex>::iterator it;
            unsigned char* batchId = (unsigned char*)&nodeId;
            for (it = mesh->vert.begin(); it != mesh->vert.end(); ++it)
            {
                it->C().X() = batchId[0];
                it->C().Y() = batchId[1];
                it->C().Z() = batchId[2];
                it->C().W() = batchId[3];
            }
		}
	}

    std::printf("batch id written\n");

    SpatialTree spatialTree = SpatialTree(model, myMeshes);
    spatialTree.Initialize();

    std::printf("sptial tree created\n");

    TileInfo* tileInfo = spatialTree.GetTilesetInfo();

    std::printf("tileset info generated\n");
    LodExporter lodExporter = LodExporter(model, myMeshes, tinyGltf);
    lodExporter.SetTileInfo(tileInfo);
    bool success = lodExporter.ExportTileset();
    
    if (success)
    {
        printf("export tileset to %s successfully\n", g_settings.outputPath);
    }
    else
    {
        printf("export error\n");
    }

    for (int i = 0; i < myMeshes.size(); ++i)
    {
        delete myMeshes[i];
    }
    delete model;
    model = NULL;
    return 0;
}
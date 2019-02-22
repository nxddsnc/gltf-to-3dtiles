#include "LodExporter.h"
#include "globals.h"
#include "vcg\complex\algorithms\clean.h"
#include "gltfExporter.h"
#include "MergeMesh.h"
#include "tiny_gltf.h"
#include <fstream>
#include <windows.h>
using namespace tinygltf;

LodExporter::LodExporter(tinygltf::Model* model, vector<MyMesh*> myMeshes, tinygltf::TinyGLTF* tinyGLTF)
{
    m_pModel = model;
    m_pTinyGTLF = tinyGLTF;
    m_currentTileLevel = 0;
    m_batchLegnthsJson = nlohmann::json({});
    m_maxGeometricError = 0;
}

LodExporter::~LodExporter()
{

}

void LodExporter::getMeshIdxs(std::vector<int> nodeIdxs, std::vector<int>& meshIdxs)
{
    for (int i = 0; i < nodeIdxs.size(); ++i)
    {
        traverseNode(&(m_pModel->nodes[nodeIdxs[i]]), meshIdxs);
    }
}

void LodExporter::traverseNode(tinygltf::Node* node, std::vector<int>& meshIdxs)
{
    if (node->children.size() > 0) 
    {
        for (int i = 0; i < node->children.size(); ++i)
        {
            traverseNode(&(m_pModel->nodes[node->children[i]]), meshIdxs);
        }
    }
    if (node->mesh != -1)
    {
        meshIdxs.push_back(node->mesh);
    }
}

bool LodExporter::ExportTileset()
{
    traverseExportTile(m_pTileInfo);
    
    nlohmann::json tilesetJson = nlohmann::json({});
    nlohmann::json version = nlohmann::json({});
    version["version"] = "1.0";
    tilesetJson["asset"] = version;
    tilesetJson["geometricError"] = std::to_string(m_maxGeometricError * 1.1);
    nlohmann::json root = nlohmann::json({});
    tilesetJson["root"] = traverseExportTileSetJson(m_pTileInfo);

	// add a dummy transform to root to make the tileset position at globe surface.
	nlohmann::json dummyTransform = nlohmann::json::array();
	dummyTransform.push_back(1);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(1);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(1);
	dummyTransform.push_back(0);
	dummyTransform.push_back(1234215.482895546);
	dummyTransform.push_back(-5086096.594671654);
	dummyTransform.push_back(3633390.4238980953);
	dummyTransform.push_back(1);
	tilesetJson["root"]["transform"] = dummyTransform;
    char filepath[1024];
    sprintf(filepath, "%s/tileset.json", g_settings.outputPath);
    std::ofstream file(filepath);
    file << tilesetJson;

    sprintf(filepath, "%s/batchLength.json", g_settings.outputPath);
    std::ofstream batchLengthJson(filepath);
    m_batchLegnthsJson["batchLength"] = g_settings.batchLength;
    batchLengthJson << m_batchLegnthsJson;

    return true;
}

nlohmann::json LodExporter::traverseExportTileSetJson(TileInfo* tileInfo)
{
    nlohmann::json parent = nlohmann::json({});
    Point3f center = tileInfo->boundingBox->Center();
    float radius = tileInfo->boundingBox->Diag() * 0.5;
    nlohmann::json boundingSphere = nlohmann::json::array();
    boundingSphere.push_back(center.X());
    boundingSphere.push_back(-center.Z());
    boundingSphere.push_back(center.Y());
    boundingSphere.push_back(center.Z());
    boundingSphere.push_back(radius);
    parent["boundingVolume"] = nlohmann::json({});
    parent["boundingVolume"]["sphere"] = boundingSphere;
    parent["geometricError"] = tileInfo->geometryError;
    parent["refine"] = "REPLACE";

    nlohmann::json content = nlohmann::json({});
    content["uri"] = tileInfo->contentUri;
    content["boundingVolume"] = nlohmann::json({}); 
    content["boundingVolume"]["sphere"] = boundingSphere;
    parent["content"] = content;
    
    if (tileInfo->children.size() > 0)
    {
        nlohmann::json children = nlohmann::json::array();
        for (int i = 0; i < tileInfo->children.size(); ++i)
        {
            nlohmann::json child = traverseExportTileSetJson(tileInfo->children[i]);
            children.push_back(child);
        }
        parent["children"] = children;
    }

    return parent;
}

void LodExporter::traverseExportTile(TileInfo* tileInfo)
{
    tileInfo->level = m_currentTileLevel++;
    for (int i = 0; i < tileInfo->children.size(); ++i)
    {
        traverseExportTile(tileInfo->children[i]);
        tileInfo->myMeshInfos.insert(tileInfo->myMeshInfos.end(), tileInfo->children[i]->myMeshInfos.begin(), tileInfo->children[i]->myMeshInfos.end());
    }

    char bufferName[1024];
    int fileIdx = 0;
    if (m_levelAccumMap.count(tileInfo->level) > 0)
    {
        fileIdx = m_levelAccumMap.at(tileInfo->level);
        fileIdx++;
        m_levelAccumMap.at(tileInfo->level)++;
    }
    else
    {
        m_levelAccumMap.insert(make_pair(tileInfo->level, fileIdx));
    }
    sprintf(bufferName, "%d-%d.bin", tileInfo->level, fileIdx);

    MeshOptimizer meshOptimizer = MeshOptimizer(tileInfo->myMeshInfos);
    meshOptimizer.DoMerge();
	meshOptimizer.DoDecimation(std::pow(0.5, g_settings.tileLevel - m_currentTileLevel));
	
	GltfExporter gltfExporter = GltfExporter(meshOptimizer.GetMergedMeshInfos(), bufferName);
	gltfExporter.ConstructNewModel();
	Model* pNewModel = gltfExporter.GetNewModel();
    // output
    char contentUri[1024];
    sprintf(contentUri, "%d/%d-%d.b3dm", tileInfo->level, tileInfo->level, fileIdx);
    tileInfo->contentUri = string(contentUri);
    string outputFilePath = getOutputFilePath(tileInfo->level, fileIdx);

    if (outputFilePath.size() > 0)
    {
		bool bSuccess = false;
		if (g_settings.writeBinary)
		{
			bSuccess = m_pTinyGTLF->WriteGltfSceneToFile(pNewModel, outputFilePath, false, false, true, true);
		}
		else
		{
			bSuccess = m_pTinyGTLF->WriteGltfSceneToFile(pNewModel, outputFilePath, false, false, true, false);
		}
        if (bSuccess)
        {
            printf("export gltf success: %s\n", outputFilePath.c_str());
        }
        else
        {
            printf("gltf write error\n");
        }
    }
    else
    {
        printf("cannot create output filepath\n");
    }

    m_currentTileLevel--;
}

//TODO: Add a class for manipulate the direcotry.
std::string LodExporter::getOutputFilePath(int level, int index)
{
    char outputDir[1024];
    sprintf(outputDir, "%s/%d", g_settings.outputPath, level);

    wchar_t wOutputDir[1024];
    mbstowcs(wOutputDir, outputDir, strlen(outputDir) + 1); // Plus null
    LPWSTR pWOutputDir = wOutputDir;

    if (CreateDirectory(pWOutputDir, NULL) || ERROR_ALREADY_EXISTS == GetLastError())
    {
        char outputFilePath[1024];
		if (g_settings.writeBinary)
		{
			sprintf(outputFilePath, "%s/%d/%d-%d.glb", g_settings.outputPath, level, level, index);
		}
		else
		{
			sprintf(outputFilePath, "%s/%d/%d.gltf", g_settings.outputPath, level, index);
		}
        return string(outputFilePath);
    }
    else
    {
        printf("cannot crate folder %s\n", outputDir);
        return "";
    }
}

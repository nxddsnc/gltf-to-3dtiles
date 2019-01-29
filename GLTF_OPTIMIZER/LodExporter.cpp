#include "LodExporter.h"
#include "tiny_gltf.h"
#include "globals.h"
#include "MergeMesh.h"
using namespace tinygltf;

LodExporter::LodExporter(tinygltf::Model* model, vector<MyMesh*> myMeshes, tinygltf::TinyGLTF* tinyGLTF)
{
    m_pModel = model;
    m_myMeshes = myMeshes;

    m_pParams = new TriEdgeCollapseQuadricParameter();
    m_pParams->QualityThr = .3;
    m_pParams->PreserveBoundary = false; // Perserve mesh boundary
    m_pParams->PreserveTopology = false;
    //m_pParams->QualityThr = .3;
    //m_pParams->QualityCheck = true;
    //m_pParams->HardQualityCheck = false;
    //m_pParams->NormalCheck = false;
    //m_pParams->AreaCheck = false;
    //m_pParams->OptimalPlacement = false;
    //m_pParams->ScaleIndependent = false;
    //m_pParams->PreserveBoundary = true; // Perserve mesh boundary
    //m_pParams->PreserveTopology = false;
    //m_pParams->QualityQuadric = false;
    //m_pParams->QualityWeight = false;

    m_pTinyGTLF = tinyGLTF;
    m_currentTileLevel = 0;
    m_batchLegnthsJson = nlohmann::json({});
}

LodExporter::~LodExporter()
{
    if (m_pParams != NULL)
    {
        delete m_pParams;
        m_pParams = NULL;
    }
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
    tilesetJson["geometricError"] = "200000";
    nlohmann::json root = nlohmann::json({});
    tilesetJson["root"] = traverseExportTileSetJson(m_pTileInfo);

	// add a dummy transform to root to make the tileset position at globe surface.
	nlohmann::json dummyTransform = nlohmann::json::array();
	dummyTransform.push_back(0.9717966035675396);
	dummyTransform.push_back(0.23582061253120834);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(-0.13509253137814733);
	dummyTransform.push_back(0.5567047839944483);
	dummyTransform.push_back(0.8196522381129321);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0.19329089285436749);
	dummyTransform.push_back(-0.7965352611046796);
	dummyTransform.push_back(0.5728614217735919);
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

    sprintf(filepath, "%s/batchLengthes.json", g_settings.outputPath);
    std::ofstream batchLengthJson(filepath);
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
    }

    std::vector<int> meshIdxs;
    // FIXME: Cache it if there's performance  issue.
    getMeshIdxs(tileInfo->nodes, meshIdxs);

    // decimation
    float geometryError = 0;
	if (m_currentTileLevel != g_settings.tileLevel)
	{
		for (int j = 0; j < meshIdxs.size(); ++j)
		{
			MyMesh* myMesh = m_myMeshes[meshIdxs[j]];
			if (myMesh->fn <= MIN_FACE_NUM)
			{
				continue;
			}
			// decimator initialization
			vcg::LocalOptimization<MyMesh> deciSession(*myMesh, m_pParams);
			deciSession.Init<MyTriEdgeCollapse>();
			uint32_t finalSize = myMesh->fn * 0.5;
			deciSession.SetTargetSimplices(finalSize); // Target face number;
			deciSession.SetTimeBudget(0.5f); // Time budget for each cycle
			deciSession.SetTargetOperations(100000);
			int maxTry = 100;
			int currentTry = 0;
			do
			{
				deciSession.DoOptimization();
				currentTry++;
			} while (myMesh->fn > finalSize && currentTry < maxTry);

            geometryError += deciSession.currMetric;
		}
        geometryError /= meshIdxs.size();
	}
    
    tileInfo->geometryError = geometryError;


    char bufferName[1024];
    int fileIdx = 0;
    if (m_levelAccumMap.count(tileInfo->level) > 0)
    {
        fileIdx = m_levelAccumMap.at(tileInfo->level);
        m_levelAccumMap.at(tileInfo->level)++;
    }
    else
    {
        m_levelAccumMap.insert(make_pair(tileInfo->level, fileIdx));
    }
    sprintf(bufferName, "%d-%d.bin", tileInfo->level, fileIdx);
    
    Model* pNewModel = new Model;
    MergeMesh mergeMesh = MergeMesh(m_pModel, pNewModel, m_myMeshes, tileInfo->nodes, bufferName);
    mergeMesh.DoMerge();
    
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
			bSuccess = m_pTinyGTLF->WriteGltfSceneToFile(pNewModel, outputFilePath);
		}
        if (bSuccess)
        {
            printf("export gltf success: %s\n", outputFilePath.c_str());
        }
        else
        {
            printf("gltf write error\n");
        }

        if (g_settings.writeBinary)
        {
            char key[1024];
            sprintf(key, "%d-%d.glb", tileInfo->level, fileIdx);
            m_batchLegnthsJson[key] = tileInfo->nodes.size();
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

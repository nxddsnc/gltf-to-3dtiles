#include "LodExporter.h"
#include "tiny_gltf.h"
#include "globals.h"
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

			if (deciSession.currMetric > geometryError)
			{
				// TODO: Look into geometricError.
				geometryError += deciSession.currMetric / m_myMeshes[meshIdxs[j]]->VertexNumber();
			}
		}
	}
    
    tileInfo->geometryError = geometryError;

    m_currentAttributeBuffer.clear();
    m_currentBatchIdBuffer.clear();
    m_currentIndexBuffer.clear();
    m_vertexUintMap.clear();
    m_vertexUshortMap.clear();
    m_materialCache.clear();

    m_pNewModel = new Model(*m_pModel);

    {
        // FIXME: Support more than 3 bufferviews.
        BufferView arraybufferView;
        arraybufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        arraybufferView.byteLength = 0;
        arraybufferView.buffer = 0;
        arraybufferView.byteStride = 12;
        arraybufferView.byteOffset = 0;

        BufferView batchIdArrayBufferView;
        batchIdArrayBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        batchIdArrayBufferView.byteLength = 0;
        batchIdArrayBufferView.buffer = 0;
        batchIdArrayBufferView.byteStride = 4;
        batchIdArrayBufferView.byteOffset = 0;

        BufferView elementArraybufferView;
        elementArraybufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
        elementArraybufferView.byteLength = 0;
        elementArraybufferView.buffer = 0;

        m_pNewModel->bufferViews.push_back(arraybufferView);
        m_pNewModel->bufferViews.push_back(batchIdArrayBufferView);
        m_pNewModel->bufferViews.push_back(elementArraybufferView);
    }

    Node node;
    node.children = tileInfo->nodes;
    node.name = m_pModel->nodes[0].name;

    addNode(&node);

    m_pNewModel->scenes[0].nodes[0] = m_pNewModel->nodes.size() - 1;

    {
        // FIXME: Support more than 3 bufferviews.
        m_pNewModel->bufferViews[1].byteOffset = m_currentAttributeBuffer.size();
        m_pNewModel->bufferViews[2].byteOffset = m_currentAttributeBuffer.size() + m_currentBatchIdBuffer.size();
    }

    Buffer buffer;
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
    buffer.uri = string(bufferName);
    buffer.data.resize(m_currentAttributeBuffer.size() + m_currentIndexBuffer.size() + m_currentBatchIdBuffer.size());
    memcpy(buffer.data.data(), m_currentAttributeBuffer.data(), m_currentAttributeBuffer.size());
    memcpy(buffer.data.data() + m_currentAttributeBuffer.size(), m_currentBatchIdBuffer.data(), m_currentBatchIdBuffer.size());
    memcpy(buffer.data.data() + m_currentAttributeBuffer.size() + m_currentBatchIdBuffer.size(),
        m_currentIndexBuffer.data(), m_currentIndexBuffer.size());
    m_pNewModel->buffers.push_back(buffer);
    
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
			bSuccess = m_pTinyGTLF->WriteGltfSceneToFile(m_pNewModel, outputFilePath, false, false, true, true);
		}
		else
		{
			bSuccess = m_pTinyGTLF->WriteGltfSceneToFile(m_pNewModel, outputFilePath);
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


// Deprecated
void LodExporter::ExportLods(vector<TileInfo> lodInfos, int level)
{
    float targetError = level * 100;
    for (int i = 0; i < lodInfos.size(); ++i)
    {
        std::vector<int> meshIdxs;
        getMeshIdxs(lodInfos[i].nodes, meshIdxs);

        float geometryError = 0;
        if (level > 0)
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
                //vcg::tri::UpdateBounding<MyMesh>::Box(*myMesh);
                geometryError += deciSession.currMetric;
            }
        }

        m_currentAttributeBuffer.clear();
        m_currentBatchIdBuffer.clear();
        m_currentIndexBuffer.clear();
        m_vertexUintMap.clear();
        m_vertexUshortMap.clear();
        m_materialCache.clear();

        m_pNewModel = new Model(*m_pModel);

        {
            // FIXME: Support more than 3 bufferviews.
            BufferView arraybufferView;
            arraybufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            arraybufferView.byteLength = 0;
            arraybufferView.buffer = 0;
            arraybufferView.byteStride = 12;
            arraybufferView.byteOffset = 0;

            BufferView batchIdArrayBufferView;
            batchIdArrayBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            batchIdArrayBufferView.byteLength = 0;
            batchIdArrayBufferView.buffer = 0;
            batchIdArrayBufferView.byteStride = 4;
            batchIdArrayBufferView.byteOffset = 0;

            BufferView elementArraybufferView;
            elementArraybufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
            elementArraybufferView.byteLength = 0;
            elementArraybufferView.buffer = 0;

            m_pNewModel->bufferViews.push_back(arraybufferView);
            m_pNewModel->bufferViews.push_back(batchIdArrayBufferView);
            m_pNewModel->bufferViews.push_back(elementArraybufferView);
        }

        Node node;
        node.children = lodInfos[i].nodes;
        node.name = m_pModel->nodes[0].name;

        addNode(&node);

        m_pNewModel->scenes[0].nodes[0] = m_pNewModel->nodes.size() - 1;

        {
            // FIXME: Support more than 3 bufferviews.
            m_pNewModel->bufferViews[1].byteOffset = m_currentAttributeBuffer.size();
            m_pNewModel->bufferViews[2].byteOffset = m_currentAttributeBuffer.size() + m_currentBatchIdBuffer.size();
        }
        Buffer buffer;
        char bufferName[1024];
        sprintf(bufferName, "%d.bin", i);
        buffer.uri = string(bufferName);
        buffer.data.resize(m_currentAttributeBuffer.size() + m_currentIndexBuffer.size() + m_currentBatchIdBuffer.size());
        memcpy(buffer.data.data(), m_currentAttributeBuffer.data(), m_currentAttributeBuffer.size());
        memcpy(buffer.data.data() + m_currentAttributeBuffer.size(), m_currentBatchIdBuffer.data(), m_currentBatchIdBuffer.size());
        memcpy(buffer.data.data() + m_currentAttributeBuffer.size() + m_currentBatchIdBuffer.size(),
            m_currentIndexBuffer.data(), m_currentIndexBuffer.size());
        m_pNewModel->buffers.push_back(buffer);
        // output

        string outputFilePath = getOutputFilePath(level, i);
        if (outputFilePath.size() > 0)
        {
            bool bSuccess = m_pTinyGTLF->WriteGltfSceneToFile(m_pNewModel, outputFilePath);
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
    }
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

int LodExporter::addNode(Node* node)
{
	Node newNode;
	newNode.name = node->name;
	newNode.matrix = node->matrix;
	for (int i = 0; i < node->children.size(); ++i)
	{
		int idx = addNode(&(m_pModel->nodes[node->children[i]]));
		newNode.children.push_back(idx);
	}

	if (node->mesh != -1)
	{
		MyMesh* myMesh = m_myMeshes[node->mesh];
        m_pCurrentMesh = myMesh;
        m_currentBatchId = node->mesh;
        m_vertexUshortMap.clear();
		int idx = addMesh(&(m_pModel->meshes[node->mesh]));
		newNode.mesh = idx;
	}

	m_pNewModel->nodes.push_back(newNode);
	return m_pNewModel->nodes.size() - 1;
}

int LodExporter::addMesh(Mesh* mesh)
{
	Mesh newMesh;
	newMesh.name = mesh->name;
	Primitive newPrimitive;

	addPrimitive(&newPrimitive, mesh);
	newMesh.primitives.push_back(newPrimitive);

	m_pNewModel->meshes.push_back(newMesh);
	return m_pNewModel->meshes.size() - 1;
}

void LodExporter::addPrimitive(Primitive* primitive, Mesh* mesh)
{
	primitive->mode = mesh->primitives[0].mode;
	int mtlIdx = addMaterial(mesh->primitives[0].material);
	primitive->material = mtlIdx;
	std::map<std::string, int>::iterator it;
	for (it = mesh->primitives[0].attributes.begin(); it != mesh->primitives[0].attributes.end(); ++it)
	{
        int idx = -1;
        if (strcmp(&it->first[0], "POSITION") == 0)
        {
            idx = addAccessor(POSITION);
        }
        else if (strcmp(&it->first[0], "NORMAL") == 0)
        {
            idx = addAccessor(NORMAL);
        }
        else if (strcmp(&it->first[0], "UV") == 0)
        {
            idx = addAccessor(UV);
        }
        assert(idx != -1);
		primitive->attributes.insert(make_pair(it->first, idx));
	}
    int batchIdAccessorIdx = addAccessor(BATCH_ID);
    primitive->attributes.insert(make_pair("_BATCHID", batchIdAccessorIdx));
	int idx = addAccessor(INDEX);
	primitive->indices = idx;
}

int LodExporter::addAccessor(AccessorType type)
{
    Accessor newAccessor;
    switch (type)
    {
    case BATCH_ID:
        newAccessor.type = TINYGLTF_TYPE_SCALAR;
        newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        newAccessor.count = m_pCurrentMesh->vn;
        break;
    case POSITION:
        newAccessor.type = TINYGLTF_TYPE_VEC3;
        newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        newAccessor.count = m_pCurrentMesh->vn;
        break;
    case NORMAL:
        newAccessor.type = TINYGLTF_TYPE_VEC3;
        newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        newAccessor.count = m_pCurrentMesh->vn;
        break;
    case UV:
        newAccessor.type = TINYGLTF_TYPE_VEC2;
        newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        newAccessor.count = m_pCurrentMesh->vn;
        break;
    case INDEX:
        newAccessor.type = TINYGLTF_TYPE_SCALAR;
        if (m_pCurrentMesh->vn > 65535)
        {
            newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
        }
        else
        {
            newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
        }
        newAccessor.count = m_pCurrentMesh->fn * 3;
        break;
    default:
        assert(-1);
        break;
    }
    
    newAccessor.bufferView = addBufferView(type, newAccessor.byteOffset);
    if (type == POSITION)
    {
        newAccessor.minValues.push_back(m_positionMin[0]);
        newAccessor.minValues.push_back(m_positionMin[1]);
        newAccessor.minValues.push_back(m_positionMin[2]);
        newAccessor.maxValues.push_back(m_positionMax[0]);
        newAccessor.maxValues.push_back(m_positionMax[1]);
        newAccessor.maxValues.push_back(m_positionMax[2]);
    }
    m_pNewModel->accessors.push_back(newAccessor);
    return m_pNewModel->accessors.size() - 1;
}

int LodExporter::addBufferView(AccessorType type, size_t& byteOffset)
{
    // FIXME: We have not consider the uv coordinates yet.
    // And we only have one .bin file currently.
    int byteLength = addBuffer(type);
    switch (type)
    {
    case POSITION:
    case NORMAL:
        byteOffset = m_pNewModel->bufferViews[0].byteLength;
        m_pNewModel->bufferViews[0].byteLength += byteLength;
        return 0;
    case UV:
        // TODO: implement UV
        break;
    case INDEX:
        byteOffset = m_pNewModel->bufferViews[2].byteLength;
        m_pNewModel->bufferViews[2].byteLength += byteLength;
        return 2;
        break;
    case BATCH_ID:
        byteOffset = m_pNewModel->bufferViews[1].byteLength;
        m_pNewModel->bufferViews[1].byteLength += byteLength;
        return 1;
        break;
    default:
        break;
    }
}

int LodExporter::addBuffer(AccessorType type)
{
    int byteLength = 0;
    int index = 0;
    unsigned char* temp = NULL;
    switch (type)
    {
    case BATCH_ID:
        for (vector<MyVertex>::iterator it = m_pCurrentMesh->vert.begin(); it != m_pCurrentMesh->vert.end(); ++it)
        {
            if (it->IsD())
            {
                continue;
            }
            temp = (unsigned char*)&(m_currentBatchId);
            m_currentBatchIdBuffer.push_back(temp[0]);
            m_currentBatchIdBuffer.push_back(temp[1]);
            m_currentBatchIdBuffer.push_back(temp[2]);
            m_currentBatchIdBuffer.push_back(temp[3]);
        }
        byteLength = m_pCurrentMesh->vn * sizeof(unsigned int);
        break;
    case POSITION:
        m_positionMin[0] = m_positionMin[1] = m_positionMin[2] = INFINITY;
        m_positionMax[0] = m_positionMax[1] = m_positionMax[2] = -INFINITY;
        for (vector<MyVertex>::iterator it = m_pCurrentMesh->vert.begin(); it != m_pCurrentMesh->vert.end(); ++it)
        {
            if (it->IsD())
            {
                continue;
            }
            for (int i = 0; i < 3; ++i)
            {
                if (m_positionMin[i] > it->P()[i])
                {
                    m_positionMin[i] = it->P()[i];
                }
                if (m_positionMax[i] < it->P()[i])
                {
                    m_positionMax[i] = it->P()[i];
                }

                temp = (unsigned char*)&(it->P()[i]);
                m_currentAttributeBuffer.push_back(temp[0]);
                m_currentAttributeBuffer.push_back(temp[1]);
                m_currentAttributeBuffer.push_back(temp[2]);
                m_currentAttributeBuffer.push_back(temp[3]);
            }

            m_vertexUshortMap.insert(make_pair(&(*it), index));
            index++;
        }
        byteLength = m_pCurrentMesh->vn * 3 * sizeof(float);
        break;
    case NORMAL:
        for (vector<MyVertex>::iterator it = m_pCurrentMesh->vert.begin(); it != m_pCurrentMesh->vert.end(); ++it)
        {
            if (it->IsD())
            {
                continue;
            }
            for (int i = 0; i < 3; ++i)
            {
                temp = (unsigned char*)&(it->N()[i]);
                m_currentAttributeBuffer.push_back(temp[0]);
                m_currentAttributeBuffer.push_back(temp[1]);
                m_currentAttributeBuffer.push_back(temp[2]);
                m_currentAttributeBuffer.push_back(temp[3]);
            }
        }
        byteLength = m_pCurrentMesh->vn * 3 * sizeof(float); 
        //assert(m_pCurrentMesh->vert.size() == m_pCurrentMesh->vn);
        break;
    case UV:
        // FIXME: Implement UV
        break;
    case INDEX:
        for (vector<MyFace>::iterator it = m_pCurrentMesh->face.begin(); it != m_pCurrentMesh->face.end(); ++it)
        {
            if (it->IsD())
            {
                continue;
            }
            for (int i = 0; i < 3; ++i)
            {
                temp = (unsigned char*)&(m_vertexUshortMap.at(it->V(i)));
                m_currentIndexBuffer.push_back(temp[0]);
                m_currentIndexBuffer.push_back(temp[1]);
            }
        }
        // FIXME: Add uint32 support
        byteLength = m_pCurrentMesh->fn * 3 * sizeof(uint16_t);
        //assert(m_pCurrentMesh->face.size() == m_pCurrentMesh->fn);
        break;
    default:
        break;
    }
    return byteLength;
}

int LodExporter::addMaterial(int material)
{
	if (m_materialCache.count(material) > 0) 
	{
		return m_materialCache.at(material);
	}
	Material newMaterial;
	newMaterial = m_pModel->materials[material];
	m_pNewModel->materials.push_back(newMaterial);
	int idx = m_pNewModel->materials.size() - 1;
	
	m_materialCache.insert(make_pair(material, idx));

    return idx;
}

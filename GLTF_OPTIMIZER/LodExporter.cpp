#include "LodExporter.h"
#include "tiny_gltf.h"
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

void LodExporter::ExportLods(vector<LodInfo> lodInfos, int level)
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
        m_currentIndexBuffer.clear();
        m_vertexUintMap.clear();
        m_vertexUshortMap.clear();
        m_materialCache.clear();

        m_pNewModel = new Model(*m_pModel);

        {
            // FIXME: Support more than 2 bufferviews.
            BufferView arraybufferView;
            arraybufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            arraybufferView.byteLength = 0;
            arraybufferView.buffer = 0;
            arraybufferView.byteStride = 12;
            arraybufferView.byteOffset = 0;

            BufferView elementArraybufferView;
            elementArraybufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
            elementArraybufferView.byteLength = 0;
            elementArraybufferView.buffer = 0;

            m_pNewModel->bufferViews.push_back(arraybufferView);
            m_pNewModel->bufferViews.push_back(elementArraybufferView);
        }

        Node node;
        node.children = lodInfos[i].nodes;
        node.name = m_pModel->nodes[0].name;

        addNode(&node);

        m_pNewModel->scenes[0].nodes[0] = m_pNewModel->nodes.size() - 1;

        {
            // FIXME: Support more than 2 bufferviews.
            m_pNewModel->bufferViews[1].byteOffset = m_currentAttributeBuffer.size();
        }
        Buffer buffer;
        char bufferName[1024];
        sprintf(bufferName, "%d.bin", i);
        buffer.uri = string(bufferName);
        buffer.data.resize(m_currentAttributeBuffer.size() + m_currentIndexBuffer.size());
        memcpy(buffer.data.data(), m_currentAttributeBuffer.data(), m_currentAttributeBuffer.size());
        memcpy(buffer.data.data() + m_currentAttributeBuffer.size(), m_currentIndexBuffer.data(), m_currentIndexBuffer.size());
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

std::string LodExporter::getOutputFilePath(int level, int index)
{
    char outputDir[1024];
    sprintf(outputDir, "%s/%d", m_outputDir.c_str(), level);

    wchar_t wOutputDir[1024];
    mbstowcs(wOutputDir, outputDir, strlen(outputDir) + 1); // Plus null
    LPWSTR pWOutputDir = wOutputDir;

    if (CreateDirectory(pWOutputDir, NULL) || ERROR_ALREADY_EXISTS == GetLastError())
    {
        char outputFilePath[1024];
        sprintf(outputFilePath, "%s/%d/%d.gltf", m_outputDir.c_str(), level, index);
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
	int idx = addAccessor(INDEX);
	primitive->indices = idx;
}

int LodExporter::addAccessor(AccessorType type)
{
    Accessor newAccessor;
    switch (type)
    {
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
    if (type == INDEX) 
    {
        byteOffset = m_pNewModel->bufferViews[1].byteLength;
        m_pNewModel->bufferViews[1].byteLength += byteLength;
        return 1;
    }
    else
    {
        byteOffset = m_pNewModel->bufferViews[0].byteLength;
        m_pNewModel->bufferViews[0].byteLength += byteLength;
        return 0;
    }
}

int LodExporter::addBuffer(AccessorType type)
{
    int byteLength = 0;
    int index = 0;
    unsigned char* temp = NULL;
    switch (type)
    {
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

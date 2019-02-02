#include "MergeMesh.h"
#include "MyMesh.h"
#include <algorithm>
#include "vcg/complex/algorithms/clean.h"
using namespace tinygltf;
using namespace std;

MergeMesh::MergeMesh(tinygltf::Model* model, tinygltf::Model* newModel, std::vector<MyMesh*> myMeshes, std::vector<int> nodesToMerge, std::string bufferName)
{
    m_bufferName = bufferName;
	m_pModel = model;
    m_pNewModel = newModel;
	m_myMeshes = myMeshes;
    m_nodesToMerge = nodesToMerge; 
    m_currentMeshIdx = 0;

    m_pParams = new TriEdgeCollapseQuadricParameter();
    m_pParams->QualityThr = .3;
    m_pParams->PreserveBoundary = false; // Perserve mesh boundary
    m_pParams->PreserveTopology = false;
}

MergeMesh::~MergeMesh()
{
    std::unordered_map<int, std::vector<MyMesh*>>::iterator it;
    for (it = m_materialNewMeshesMap.begin(); it != m_materialNewMeshesMap.end(); ++it)
    {
        for (int i = 0; i < it->second.size(); ++i)
        {
            delete it->second[i];
        }
    }
    delete m_pParams;
}


struct mesh_compare_fn
{
    inline bool operator() (const MyMesh* myMesh1, const MyMesh* myMesh2)
    {
        return myMesh1->vn < myMesh2->vn;
    }
};

void MergeMesh::ConstructNewModel()
{
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

    Node root;
    root.name = "scene_root";
    m_pNewModel->nodes.push_back(root);
    Scene scene;
    scene.name = "scene";
    scene.nodes.push_back(0);
    m_pNewModel->scenes.push_back(scene);
    m_pNewModel->asset.version = "2.0";
    std::unordered_map<int, std::vector<MyMesh*>>::iterator it;
    int index = 0;
    for (it = m_materialNewMeshesMap.begin(); it != m_materialNewMeshesMap.end(); ++it)
    {
        std::vector<MyMesh*> myMeshes = it->second;

        for (int i = 0; i < myMeshes.size(); ++i)
        {
            Node node;

        addMergedMeshesToNewModel(m_pNewModel->materials.size() - 1, myMeshes);
    }

    {
        // FIXME: Support more than 3 bufferviews.
        m_pNewModel->bufferViews[1].byteOffset = m_currentAttributeBuffer.size();
        m_pNewModel->bufferViews[2].byteOffset = m_currentAttributeBuffer.size() + m_currentBatchIdBuffer.size();
    }

    Buffer buffer;
    buffer.uri = m_bufferName;
    buffer.data.resize(m_currentAttributeBuffer.size() + m_currentIndexBuffer.size() + m_currentBatchIdBuffer.size());
    memcpy(buffer.data.data(), m_currentAttributeBuffer.data(), m_currentAttributeBuffer.size());
    memcpy(buffer.data.data() + m_currentAttributeBuffer.size(), m_currentBatchIdBuffer.data(), m_currentBatchIdBuffer.size());
    memcpy(buffer.data.data() + m_currentAttributeBuffer.size() + m_currentBatchIdBuffer.size(),
        m_currentIndexBuffer.data(), m_currentIndexBuffer.size());
    m_pNewModel->buffers.push_back(buffer);
}

float MergeMesh::DoDecimation(float targetPercentage)
{
    std::unordered_map<int, std::vector<MyMesh*>>::iterator it;
    float geometryError = 0;
    for (it = m_materialNewMeshesMap.begin(); it != m_materialNewMeshesMap.end(); ++it)
    {
        for (int i = 0; i < it->second.size(); ++i)
        {
            MyMesh* myMesh = it->second[i];
            // decimator initialization
            vcg::LocalOptimization<MyMesh> deciSession(*myMesh, m_pParams);
            deciSession.Init<MyTriEdgeCollapse>();
            // FIXME: If the mesh bbox is large and it's face number is ralatively few, we should not do decimation.
            uint32_t finalSize = myMesh->fn * targetPercentage;
            finalSize = finalSize < MIN_FACE_NUM ? MIN_FACE_NUM : finalSize;
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
        
            tri::Clean<MyMesh>::RemoveDuplicateVertex(*myMesh);
            tri::Clean<MyMesh>::RemoveUnreferencedVertex(*myMesh);
        }
    }
    return geometryError;
}

void MergeMesh::DoMerge()
{
	for (int i = 0; i < m_nodesToMerge.size(); ++i)
	{
		Node* node = &(m_pModel->nodes[m_nodesToMerge[i]]);
		std::vector<MeshInfo> meshInfos;
        GetNodeMeshInfos(m_pModel, node, meshInfos);
        
        for (int j = 0; j < meshInfos.size(); ++j)
        {
            Mesh* mesh = &(m_pModel->meshes[meshInfos[j].meshIdx]);

            if (m_meshMatrixMap.count(m_myMeshes[meshInfos[j].meshIdx]) > 0)
            {
                printf("error detected\n");
            }
            else if (meshInfos[j].matrix != NULL)
            {
                m_meshMatrixMap.insert(make_pair(m_myMeshes[meshInfos[j].meshIdx], *meshInfos[j].matrix));
            }
            int materialIdx = mesh->primitives[0].material;
            Material material = m_pModel->materials[materialIdx];
            if (m_materialMeshMap.count(material) > 0)
            {
                m_materialMeshMap.at(material).push_back(m_myMeshes[meshInfos[j].meshIdx]);
            }
            else
            {
                std::vector<MyMesh*> myMeshesToMerge;
                myMeshesToMerge.push_back(m_myMeshes[meshInfos[j].meshIdx]);
                m_materialMeshMap.insert(make_pair(material, myMeshesToMerge));
            }
        }
	}

    std::unordered_map<tinygltf::Material, std::vector<MyMesh*>, material_hash_fn, material_equal_fn>::iterator it;
    for (it = m_materialMeshMap.begin(); it != m_materialMeshMap.end(); ++it)
    {
        m_pNewModel->materials.push_back(it->first);

        std::vector<MyMesh*> myMeshes = it->second;

        std::sort(myMeshes.begin(), myMeshes.end(), mesh_compare_fn());

        mergeSameMaterialMeshes(m_pNewModel->materials.size() - 1, myMeshes);
    }
}

void MergeMesh::createMyMesh(int materialIdx, std::vector<MyMesh*> myMeshes)
{
    MyMesh* newMesh = new MyMesh();

    if (m_materialNewMeshesMap.count(materialIdx) > 0) 
    {
        m_materialNewMeshesMap.at(materialIdx).push_back(newMesh);
    }
    else
    {
        std::vector<MyMesh*> newMeshes;
        newMeshes.push_back(newMesh);
        m_materialNewMeshesMap.insert(make_pair(materialIdx, newMeshes));
    }

    int vertexCount;
    int faceCount;
    std::unordered_map<VertexPointer, VertexPointer> vertexMap;

    for (int i = 0; i < myMeshes.size(); ++i)
    {
        vertexMap.clear();
        VertexIterator vi = Allocator<MyMesh>::AddVertices(*newMesh, myMeshes[i]->vn);
        vertexCount = myMeshes[i]->vn;
        faceCount = myMeshes[i]->fn;

        if (m_meshMatrixMap.count(myMeshes[i]) > 0)
        {
            Matrix44f* matrix = &(m_meshMatrixMap.at(myMeshes[i]));
            Matrix33f normalMatrix = Matrix33f(*matrix, 3);
            normalMatrix = Inverse(normalMatrix);
            for (int j = 0; j < vertexCount; ++j)
            {
                Point4f position(myMeshes[i]->vert[j].P()[0], myMeshes[i]->vert[j].P()[1], myMeshes[i]->vert[j].P()[2], 1.0);
                Point3f normal(myMeshes[i]->vert[j].N()[0], myMeshes[i]->vert[j].N()[1], myMeshes[i]->vert[j].N()[2]);
                position = *matrix * position;
                normal = normalMatrix * normal;
                (*vi).P()[0] = position[0];
                (*vi).P()[1] = position[1];
                (*vi).P()[2] = position[2];

                (*vi).N()[0] = normal[0];
                (*vi).N()[1] = normal[1];
                (*vi).N()[2] = normal[2];

                (*vi).C()[0] = myMeshes[i]->vert[j].C()[0];
                (*vi).C()[1] = myMeshes[i]->vert[j].C()[1];
                (*vi).C()[2] = myMeshes[i]->vert[j].C()[2];
                (*vi).C()[3] = myMeshes[i]->vert[j].C()[3];
                //(*vi).T().P().X() = va.u;
                //(*vi).T().P().Y() = va.v;

                vertexMap.insert(make_pair(&(myMeshes[i]->vert[j]), &*vi));
                ++vi;
            }
        }
        else
        {
            for (int j = 0; j < vertexCount; ++j)
            {
                (*vi).P()[0] = myMeshes[i]->vert[j].P()[0];
                (*vi).P()[1] = myMeshes[i]->vert[j].P()[1];
                (*vi).P()[2] = myMeshes[i]->vert[j].P()[2];

                (*vi).N()[0] = myMeshes[i]->vert[j].N()[0];
                (*vi).N()[1] = myMeshes[i]->vert[j].N()[1];
                (*vi).N()[2] = myMeshes[i]->vert[j].N()[2];

                (*vi).C()[0] = myMeshes[i]->vert[j].C()[0];
                (*vi).C()[1] = myMeshes[i]->vert[j].C()[1];
                (*vi).C()[2] = myMeshes[i]->vert[j].C()[2];
                (*vi).C()[3] = myMeshes[i]->vert[j].C()[3];
                //(*vi).T().P().X() = va.u;
                //(*vi).T().P().Y() = va.v;

                vertexMap.insert(make_pair(&(myMeshes[i]->vert[j]), &*vi));
                ++vi;
            }
        }

        FaceIterator fi = Allocator<MyMesh>::AddFaces(*newMesh, faceCount);
        for (int j = 0; j < faceCount; ++j)
        {
            (*fi).V(0) = vertexMap.at(myMeshes[i]->face[j].V(0));
            (*fi).V(1) = vertexMap.at(myMeshes[i]->face[j].V(1));
            (*fi).V(2) = vertexMap.at(myMeshes[i]->face[j].V(2));
            ++fi;
        }
    }
}

void MergeMesh::mergeSameMaterialMeshes(int materialIdx, std::vector<MyMesh*> myMeshes)
{
    m_totalVertex = 0;
    m_totalFace = 0;
    std::vector<MyMesh*> meshesToMerge;
    for (int i = 0; i < myMeshes.size(); ++i)
    {
        MyMesh* myMesh = myMeshes[i];
        if (m_totalVertex + myMesh->vn > 65536 || (m_totalVertex + myMesh->vn < 65536 && i == myMeshes.size() - 1))
        {
            if (m_totalVertex + myMesh->vn < 65536 && i == myMeshes.size() - 1)
            {
                meshesToMerge.push_back(myMesh);
                m_totalVertex += myMesh->vn;
                m_totalFace += myMesh->fn;
            }
            // add mesh node
            Node node;
            Mesh newMesh;
            char meshName[1024];
            sprintf(meshName, "%d", m_currentMeshIdx);
            node.name = string(meshName);
            newMesh.name = string(meshName);
            
            Primitive newPrimitive;
            newPrimitive.mode = 4; // currently only support triangle mesh.
            newPrimitive.material = materialIdx;
            m_currentMeshes = meshesToMerge;
            addPrimitive(&newPrimitive);
            newMesh.primitives.push_back(newPrimitive);

            createMyMesh(materialIdx, meshesToMerge);
        }

        meshesToMerge.push_back(myMesh);
        m_totalVertex += myMesh->vn;
        m_totalFace += myMesh->fn;
    }
}

int MergeMesh::addMesh(int materialIdx, MyMesh* myMesh)
{
    Mesh newMesh;
    //newMesh.name = mesh->name;
    Primitive newPrimitive;
    m_currentMesh = myMesh;

    m_totalVertex = 0;
    m_totalFace = 0;
    for (vector<MyVertex>::iterator it = myMesh->vert.begin(); it != myMesh->vert.end(); ++it)
    {
        if (it->IsD())
        {
            continue;
        }
        m_totalVertex++;
    }
    for (vector<MyFace>::iterator it = myMesh->face.begin(); it != myMesh->face.end(); ++it)
    {
        if (it->IsD())
        {
            continue;
        }
        m_totalFace++;
    }

    addPrimitive(&newPrimitive);
    newPrimitive.mode = 4;
    newPrimitive.material = materialIdx;
    newMesh.primitives.push_back(newPrimitive);

    m_pNewModel->meshes.push_back(newMesh);
    return m_pNewModel->meshes.size() - 1;
}
void MergeMesh::addPrimitive(Primitive* primitive)
{
    int positionAccessorIdx = addAccessor(POSITION);
    int normalAccessorIdx = addAccessor(NORMAL);
    //int uvAccessorIdx = addAccessor(UV);
    int batchIdAccessorIdx = addAccessor(BATCH_ID);
    int indicesAccessorIdx = addAccessor(INDEX);

    primitive->attributes.insert(make_pair("POSITION", positionAccessorIdx));
    primitive->attributes.insert(make_pair("NORMAL", normalAccessorIdx));
    primitive->attributes.insert(make_pair("_BATCHID", batchIdAccessorIdx));

    primitive->indices = indicesAccessorIdx;
}

int MergeMesh::addAccessor(AccessorType type)
{
    Accessor newAccessor;
    switch (type)
    {
    case BATCH_ID:
        newAccessor.type = TINYGLTF_TYPE_SCALAR;
        newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        newAccessor.count = m_totalVertex;
        break;
    case POSITION:
        newAccessor.type = TINYGLTF_TYPE_VEC3;
        newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        newAccessor.count = m_totalVertex;
        break;
    case NORMAL:
        newAccessor.type = TINYGLTF_TYPE_VEC3;
        newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        newAccessor.count = m_totalVertex;
        break;
    case UV:
        newAccessor.type = TINYGLTF_TYPE_VEC2;
        newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        newAccessor.count = m_totalVertex;
        break;
    case INDEX:
        newAccessor.type = TINYGLTF_TYPE_SCALAR;
        if (m_totalVertex > 65536)
        {
            newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
        }
        else
        {
            newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
        }
        newAccessor.count = m_totalFace * 3;
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

int MergeMesh::addBufferView(AccessorType type, size_t& byteOffset)
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

int MergeMesh::addBuffer(AccessorType type)
{
    int byteLength = 0;
    uint32_t index = 0;
    unsigned char* temp = NULL;
    MyMesh* myMesh = m_currentMesh;
    switch (type)
    {
    case BATCH_ID:
        for (vector<MyVertex>::iterator it = myMesh->vert.begin(); it != myMesh->vert.end(); ++it)
        {
            if (it->IsD())
            {
                continue;
            }
            m_currentBatchIdBuffer.push_back(it->C().X());
            m_currentBatchIdBuffer.push_back(it->C().Y());
            m_currentBatchIdBuffer.push_back(it->C().Z());
            m_currentBatchIdBuffer.push_back(it->C().W());
        }
        byteLength = m_totalVertex * sizeof(unsigned int);
        break;
    case POSITION:
        m_vertexUintMap.clear();
        m_positionMin[0] = m_positionMin[1] = m_positionMin[2] = INFINITY;
        m_positionMax[0] = m_positionMax[1] = m_positionMax[2] = -INFINITY;

        for (vector<MyVertex>::iterator it = myMesh->vert.begin(); it != myMesh->vert.end(); ++it)
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
            if (m_totalVertex > 65536)
            {
                m_vertexUintMap.insert(make_pair(&(*it), index));
            }
            else
            {
                m_vertexUshortMap.insert(make_pair(&(*it), index));
            }
            index++;
        }
        byteLength = m_totalVertex * 3 * sizeof(float);
        break;
    case NORMAL:
        for (vector<MyVertex>::iterator it = myMesh->vert.begin(); it != myMesh->vert.end(); ++it)
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
        byteLength = m_totalVertex * 3 * sizeof(float);
        break;
    case UV:
        // FIXME: Implement UV
        break;
    case INDEX:
        for (vector<MyFace>::iterator it = myMesh->face.begin(); it != myMesh->face.end(); ++it)
        {
            if (it->IsD())
            {
                continue;
            }
            for (int i = 0; i < 3; ++i)
            {
                if (m_totalVertex > 65536)
                {
                    temp = (unsigned char*)&(m_vertexUintMap.at(it->V(i)));
                    m_currentIndexBuffer.push_back(temp[0]);
                    m_currentIndexBuffer.push_back(temp[1]);
                    m_currentIndexBuffer.push_back(temp[2]);
                    m_currentIndexBuffer.push_back(temp[3]);
                }
                else
                {
                    temp = (unsigned char*)&(m_vertexUshortMap.at(it->V(i)));
                    m_currentIndexBuffer.push_back(temp[0]);
                    m_currentIndexBuffer.push_back(temp[1]);
                }
            }
        }
        byteLength = m_totalFace * 3 * (m_totalVertex > 65536 ? sizeof(uint32_t) : sizeof(uint16_t));
        break;
    default:
        break;
    }
    return byteLength;
}

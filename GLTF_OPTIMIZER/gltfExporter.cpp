#include "GltfExporter.h"
using namespace std;
using namespace tinygltf;

GltfExporter::GltfExporter(vector<MyMeshInfo> myMeshInfos, string bufferName)
{
	m_bufferName = bufferName;
	m_pNewModel = new Model();
	m_myMeshInfos = myMeshInfos;
}


GltfExporter::~GltfExporter()
{
	if (m_pNewModel != NULL)
	{
		delete m_pNewModel;
	}
}

void GltfExporter::ConstructNewModel()
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

	for (int i = 0; i < m_myMeshInfos.size(); ++i)
	{	
		MyMesh* myMesh = m_myMeshInfos[i].myMesh;
		Material* material = m_myMeshInfos[i].material;
		addMesh(material, myMesh);
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

int GltfExporter::addMesh(Material* material, MyMesh* myMesh)
{
	int materialIdx;
	if (m_materialIndexMap.count(material) > 0)
	{
		materialIdx = m_materialIndexMap.at(material);
	}
	else
	{
		m_pNewModel->materials.push_back(*material);
		materialIdx = m_pNewModel->materials.size() - 1;
		m_materialIndexMap.insert(make_pair(material, materialIdx));
	}

	Mesh newMesh;
	Primitive newPrimitive;
	m_currentMesh = myMesh;
	addPrimitive(&newPrimitive);
	newPrimitive.mode = 4;
	newPrimitive.material = materialIdx;
	newMesh.primitives.push_back(newPrimitive);

	m_pNewModel->meshes.push_back(newMesh);
	return m_pNewModel->meshes.size() - 1;
}
void GltfExporter::addPrimitive(Primitive* primitive)
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

int GltfExporter::addAccessor(AccessorType type)
{
	Accessor newAccessor;
	switch (type)
	{
	case BATCH_ID:
		newAccessor.type = TINYGLTF_TYPE_SCALAR;
		newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		newAccessor.count = m_currentMesh->vn;
		break;
	case POSITION:
		newAccessor.type = TINYGLTF_TYPE_VEC3;
		newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		newAccessor.count = m_currentMesh->vn;
		break;
	case NORMAL:
		newAccessor.type = TINYGLTF_TYPE_VEC3;
		newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		newAccessor.count = m_currentMesh->vn;
		break;
	case UV:
		newAccessor.type = TINYGLTF_TYPE_VEC2;
		newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		newAccessor.count = m_currentMesh->vn;
		break;
	case INDEX:
		newAccessor.type = TINYGLTF_TYPE_SCALAR;
		if (m_currentMesh->vn > 65536)
		{
			newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
		}
		else
		{
			newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
		}
		newAccessor.count = m_currentMesh->fn * 3;
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

int GltfExporter::addBufferView(AccessorType type, size_t& byteOffset)
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

int GltfExporter::addBuffer(AccessorType type)
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
		byteLength = m_currentMesh->vn * sizeof(unsigned int);
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
			if (myMesh->vn > 65536)
			{
				m_vertexUintMap.insert(make_pair(&(*it), index));
			}
			else
			{
				m_vertexUshortMap.insert(make_pair(&(*it), index));
			}
			index++;
		}
		byteLength = myMesh->vn * 3 * sizeof(float);
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
		byteLength = myMesh->vn * 3 * sizeof(float);
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
				if (myMesh->vn > 65536)
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
		byteLength = myMesh->fn * 3 * (myMesh->vn > 65536 ? sizeof(uint32_t) : sizeof(uint16_t));
		break;
	default:
		break;
	}
	return byteLength;
}

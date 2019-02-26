#include "GltfExporter.h"
#include "tiny_gltf.h"
#include "globals.h"
#include "utils.h"
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
    bool hasUv = false;
    bool hasUShortIndex = false;
    bool hasUIntIndex = false;
    {
        // FIXME: Support more than 3 bufferviews.
        BufferView arraybufferView;
        arraybufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        arraybufferView.byteLength = 0;
        arraybufferView.buffer = 0;
        arraybufferView.byteStride = 12;
        arraybufferView.byteOffset = 0;

        BufferView arraybufferViewTex;
        arraybufferViewTex.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        arraybufferViewTex.byteLength = 0;
        arraybufferViewTex.buffer = 0;
        arraybufferViewTex.byteStride = 8;
        arraybufferViewTex.byteOffset = 0;

        BufferView batchIdArrayBufferView;
        batchIdArrayBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        batchIdArrayBufferView.byteLength = 0;
        batchIdArrayBufferView.buffer = 0;
        batchIdArrayBufferView.byteStride = 4;
        batchIdArrayBufferView.byteOffset = 0;

        BufferView elementArrayUShortbufferView;
        elementArrayUShortbufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
        elementArrayUShortbufferView.byteLength = 0;
        elementArrayUShortbufferView.buffer = 0;

        BufferView elementArrayUIntbufferView;
        elementArrayUIntbufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
        elementArrayUIntbufferView.byteLength = 0;
        elementArrayUIntbufferView.buffer = 0;

        for (int i = 0; i < m_myMeshInfos.size(); ++i)
        {
            if (m_myMeshInfos[i].myMesh->hasUv)
            {
                hasUv = true;
            }
            if (m_myMeshInfos[i].myMesh->vn > 65536)
            {
                hasUIntIndex = true;
            }
            if (m_myMeshInfos[i].myMesh->vn <= 65536)
            {
                hasUShortIndex = true;
            }
        }
        m_pNewModel->bufferViews.push_back(arraybufferView);
        m_currentArrayBVIdx = m_pNewModel->bufferViews.size() - 1;
        if (hasUv)
        {
            m_pNewModel->bufferViews.push_back(arraybufferViewTex);
            m_currentArrayBVTexIdx = m_pNewModel->bufferViews.size() - 1;
        }
        m_pNewModel->bufferViews.push_back(batchIdArrayBufferView);
        m_currentBatchIdBVIdx = m_pNewModel->bufferViews.size() - 1;
        if (hasUIntIndex)
        {
            m_pNewModel->bufferViews.push_back(elementArrayUIntbufferView);
            m_currentUIntBVIdx = m_pNewModel->bufferViews.size() - 1;
        }
        if (hasUShortIndex)
        {
            m_pNewModel->bufferViews.push_back(elementArrayUShortbufferView);
            m_currentUShortBVIdx = m_pNewModel->bufferViews.size() - 1;
        }
    }

    Scene scene;
    scene.name = "scene";
    //scene.nodes.push_back(0);
    m_pNewModel->scenes.push_back(scene);
    m_pNewModel->asset.version = "2.0";
    std::unordered_map<int, std::vector<MyMesh*>>::iterator it;
    int index = 0;

    for (int i = 0; i < m_myMeshInfos.size(); ++i)
    {
        MyMesh* myMesh = m_myMeshInfos[i].myMesh;
        if (myMesh->vn == 0 || myMesh->fn == 0) 
        {
            continue;
        }
        Material* material = m_myMeshInfos[i].material;
        int meshIdx = addMesh(material, myMesh);
        Node node;
        node.name = "test";
        node.mesh = meshIdx;
        m_pNewModel->nodes.push_back(node);
        m_pNewModel->scenes[0].nodes.push_back(m_pNewModel->nodes.size() - 1);
    }

    {
        if (hasUv)
        {
            m_pNewModel->bufferViews[1].byteOffset = m_currentAttributeBuffer.size();
            m_pNewModel->bufferViews[2].byteOffset = m_currentAttributeBuffer.size() + m_currentTexBuffer.size();
            if (hasUIntIndex)
            {
                m_pNewModel->bufferViews[3].byteOffset = m_currentAttributeBuffer.size() + m_currentTexBuffer.size() + m_currentBatchIdBuffer.size();
                if (hasUShortIndex)
                {
                    m_pNewModel->bufferViews[4].byteOffset = m_currentAttributeBuffer.size() + m_currentTexBuffer.size() + m_currentBatchIdBuffer.size() + m_currentUIntIndexBuffer.size();
                }
            }
            else
            {
                m_pNewModel->bufferViews[3].byteOffset = m_currentAttributeBuffer.size() + m_currentTexBuffer.size() + m_currentBatchIdBuffer.size();
            }
        }
        else
        {
            m_pNewModel->bufferViews[1].byteOffset = m_currentAttributeBuffer.size();
            if (hasUIntIndex)
            {
                m_pNewModel->bufferViews[2].byteOffset = m_currentAttributeBuffer.size() + m_currentBatchIdBuffer.size();
                if (hasUShortIndex)
                {
                    m_pNewModel->bufferViews[3].byteOffset = m_currentAttributeBuffer.size() + m_currentBatchIdBuffer.size() + m_currentUIntIndexBuffer.size();
                }
            }
            else
            {
                m_pNewModel->bufferViews[2].byteOffset = m_currentAttributeBuffer.size() + m_currentBatchIdBuffer.size();
            }
        }
    }

    Buffer buffer;
    buffer.uri = m_bufferName;
    buffer.data.resize(m_currentAttributeBuffer.size() + m_currentUShortIndexBuffer.size() + m_currentBatchIdBuffer.size() + m_currentTexBuffer.size() + m_currentUIntIndexBuffer.size());
    memcpy(buffer.data.data(), m_currentAttributeBuffer.data(), m_currentAttributeBuffer.size());
    memcpy(buffer.data.data() + m_currentAttributeBuffer.size(), m_currentTexBuffer.data(),
        m_currentTexBuffer.size());
    memcpy(buffer.data.data() + m_currentAttributeBuffer.size() + m_currentTexBuffer.size(),
        m_currentBatchIdBuffer.data(), m_currentBatchIdBuffer.size());
    memcpy(buffer.data.data() + m_currentAttributeBuffer.size() + m_currentTexBuffer.size() +
        m_currentBatchIdBuffer.size(), m_currentUIntIndexBuffer.data(), m_currentUIntIndexBuffer.size());
    memcpy(buffer.data.data() + m_currentAttributeBuffer.size() + m_currentTexBuffer.size() +
        m_currentBatchIdBuffer.size() + m_currentUIntIndexBuffer.size(), m_currentUShortIndexBuffer.data(),
        m_currentUShortIndexBuffer.size());
    m_pNewModel->buffers.push_back(buffer);
}

bool GltfExporter::DoExport(std::string filepath, bool binary)
{
    bool bSuccess = false;
    if (binary)
    {
        bSuccess = m_pTinyGTLF->WriteGltfSceneToFile(m_pNewModel, filepath, false, false, true, true);
    }
    else
    {
        bSuccess = m_pTinyGTLF->WriteGltfSceneToFile(m_pNewModel, filepath, false, false, true, false);
    }
    return bSuccess;
}

int GltfExporter::addMaterial(Material* material)
{
    int materialIdx;
    if (m_materialIndexMap.count(material) > 0)
    {
        materialIdx = m_materialIndexMap.at(material);
    }
    else
    {
        Material* newMaterial = new Material;
        newMaterial->name = material->name;
        newMaterial->values = material->values;
        newMaterial->additionalValues = material->additionalValues;
        if (newMaterial->values.count("baseColorTexture"))
        {
            tinygltf::Texture* texture = &(m_pModel->textures[material->values.at("baseColorTexture").TextureIndex()]);
            int textureIdx = addTexture(texture);
            newMaterial->values.at("baseColorTexture").json_double_value.at("index") = textureIdx;
        }
        m_pNewModel->materials.push_back(*newMaterial);
        delete newMaterial;
        materialIdx = m_pNewModel->materials.size() - 1;
        m_materialIndexMap.insert(make_pair(material, materialIdx));
    }
    return materialIdx;
}

int GltfExporter::addTexture(Texture* texture)
{
    int textureIdx;
    if (m_textureIndexMap.count(texture) > 0)
    {
        textureIdx = m_textureIndexMap.at(texture);
    }
    else
    {
        Sampler* sampler = &(m_pModel->samplers[texture->sampler]);
        int samplerIdx = addSampler(sampler);

        Image* image = &(m_pModel->images[texture->source]);
        int imageIdx = addImage(image);

        Texture* newTexture = new Texture();
        newTexture->name = texture->name;
        newTexture->sampler = samplerIdx;
        newTexture->source = imageIdx;

        m_pNewModel->textures.push_back(*newTexture);

        delete newTexture;

        textureIdx = m_pNewModel->textures.size() - 1;
        m_textureIndexMap.insert(std::make_pair(texture, textureIdx));
    }
    return textureIdx;
}

int GltfExporter::addSampler(Sampler* sampler)
{
    int samplerIdx;
    if (m_samplerIndexMap.count(sampler) > 0)
    {
        samplerIdx = m_samplerIndexMap.at(sampler);
    }
    else
    {
        Sampler* newSampler = new Sampler();
        newSampler->magFilter = sampler->magFilter;
        newSampler->minFilter = sampler->minFilter;
        newSampler->name = sampler->name;
        newSampler->wrapT = sampler->wrapT;
        newSampler->wrapS = sampler->wrapS;

        m_pNewModel->samplers.push_back(*newSampler);
        delete newSampler;

        samplerIdx = m_pNewModel->samplers.size() - 1;
        m_samplerIndexMap.insert(std::make_pair(sampler, samplerIdx));
    }
    return samplerIdx;
}

int GltfExporter::addImage(Image* image)
{
    int imageIdx;
    if (m_imageIndexMap.count(image) > 0)
    {
        imageIdx = m_imageIndexMap.at(image);
    }
    else
    {
        if (image->uri.length() > 0)
        {
            string outputPath = g_settings.outputPath;

            size_t found = outputPath.find_last_of("/\\");
            string outputDir = outputPath.substr(0, found);

            string inputPath = g_settings.inputPath;
            found = inputPath.find_last_of("/\\");
            string inputDir = inputPath.substr(0, found);

            string filename = image->uri.substr(image->uri.find_last_of("/\\") + 1);
            char dest[1024];
            std::sprintf(dest, "%s/%s", outputDir.c_str(), filename.c_str());
            char source[1024];
            std::sprintf(source, "%s/%s", inputDir.c_str(), filename.c_str());
            //Copyfile(source, dest);

            Image newImage;
            newImage.name = image->name;
            newImage.uri = dest;
            newImage.width = image->width;
            newImage.height = image->height;
            newImage.component = image->component;
            newImage.as_is = image->as_is;
            newImage.image = image->image;

            m_pNewModel->images.push_back(newImage);

            imageIdx = m_pNewModel->images.size() - 1;
            m_imageIndexMap.insert(std::make_pair(image, imageIdx));
        }
    }
    return imageIdx;
}

int GltfExporter::addMesh(Material* material, MyMesh* myMesh)
{
    int materialIdx = addMaterial(material);

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
    if (m_currentMesh->hasUv)
    {
        int uvAccessorIdx = addAccessor(UV);
        primitive->attributes.insert(make_pair("TEXCOORD_0", uvAccessorIdx));
    }
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
        byteOffset = m_pNewModel->bufferViews[m_currentArrayBVIdx].byteLength;
        m_pNewModel->bufferViews[m_currentArrayBVIdx].byteLength += byteLength;
        return m_currentArrayBVIdx;
    case UV:
        byteOffset = m_pNewModel->bufferViews[m_currentArrayBVTexIdx].byteLength;
        m_pNewModel->bufferViews[m_currentArrayBVTexIdx].byteLength += byteLength;
        return m_currentArrayBVTexIdx;
    case INDEX:
        if (m_currentMesh->vn > 65536)
        {
            byteOffset = m_pNewModel->bufferViews[m_currentUIntBVIdx].byteLength;
            m_pNewModel->bufferViews[m_currentUIntBVIdx].byteLength += byteLength;
            return m_currentUIntBVIdx;
        }
        else
        {
            byteOffset = m_pNewModel->bufferViews[m_currentUShortBVIdx].byteLength;
            m_pNewModel->bufferViews[m_currentUShortBVIdx].byteLength += byteLength;
            return m_currentUShortBVIdx;
        }
    case BATCH_ID:
        byteOffset = m_pNewModel->bufferViews[m_currentBatchIdBVIdx].byteLength;
        m_pNewModel->bufferViews[m_currentBatchIdBVIdx].byteLength += byteLength;
        return m_currentBatchIdBVIdx;
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
        for (vector<MyVertex>::iterator it = myMesh->vert.begin(); it != myMesh->vert.end(); ++it)
        {
            if (it->IsD())
            {
                continue;
            }

            float t = it->T().U();
            temp = (unsigned char*)&(t);
            m_currentTexBuffer.push_back(temp[0]);
            m_currentTexBuffer.push_back(temp[1]);
            m_currentTexBuffer.push_back(temp[2]);
            m_currentTexBuffer.push_back(temp[3]);

            t = it->T().V();
            temp = (unsigned char*)&(t);
            m_currentTexBuffer.push_back(temp[0]);
            m_currentTexBuffer.push_back(temp[1]);
            m_currentTexBuffer.push_back(temp[2]);
            m_currentTexBuffer.push_back(temp[3]);
        }
        byteLength = myMesh->vn * 2 * sizeof(float);
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
                    m_currentUIntIndexBuffer.push_back(temp[0]);
                    m_currentUIntIndexBuffer.push_back(temp[1]);
                    m_currentUIntIndexBuffer.push_back(temp[2]);
                    m_currentUIntIndexBuffer.push_back(temp[3]);
                }
                else
                {
                    temp = (unsigned char*)&(m_vertexUshortMap.at(it->V(i)));
                    m_currentUShortIndexBuffer.push_back(temp[0]);
                    m_currentUShortIndexBuffer.push_back(temp[1]);
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
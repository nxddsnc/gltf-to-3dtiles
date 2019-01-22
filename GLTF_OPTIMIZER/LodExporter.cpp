#include "LodExporter.h"
#include "tiny_gltf.h"
using namespace tinygltf;

LodExporter::LodExporter(tinygltf::Model* model, vector<MyMesh*> myMeshes, tinygltf::TinyGLTF* tinyGLTF)
{
    m_pModel = model;
    m_myMeshes = myMeshes;

    m_qparams = new TriEdgeCollapseQuadricParameter();
    m_qparams->QualityThr = .3;
    m_qparams->PreserveBoundary = true; // Perserve mesh boundary
    m_qparams->PreserveTopology = false;
    m_tinyGTLF = tinyGLTF;
}

LodExporter::~LodExporter()
{
    if (m_qparams != NULL)
    {
        delete m_qparams;
        m_qparams = NULL;
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
                // decimator initialization
                vcg::LocalOptimization<MyMesh> deciSession(*myMesh, m_qparams);
                deciSession.Init<MyTriEdgeCollapse>();
                uint32_t finalSize = myMesh->fn * 0.5;
                deciSession.SetTargetSimplices(myMesh->fn * 0.5); // Target face number;
                deciSession.SetTimeBudget(0.5f); // Time budget for each cycle
                do
                {
                    deciSession.DoOptimization();
                } while (myMesh->fn > finalSize);
                geometryError += deciSession.currMetric;
            }
        }

        m_pNewModel = new Model(*m_pModel);
        for (int j = 0; j < lodInfos[i].nodes.size(); ++j)
        {
            int nodeIdx = lodInfos[i].nodes[j];
            m_pModel->nodes[nodeIdx];
        }
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
		int idx = addMesh(&(m_pModel->meshes[node->mesh]), myMesh);
		newNode.mesh = idx;
	}

	m_pNewModel->nodes.push_back(newNode);
	return m_pNewModel->nodes.size();
}

int LodExporter::addMesh(Mesh* mesh, MyMesh* myMesh)
{
	Mesh newMesh;
	newMesh.name = mesh->name;
	Primitive newPrimitive;

	addPrimitive(&newPrimitive, mesh, myMesh);
	newMesh.primitives.push_back(newPrimitive);

	m_pNewModel->meshes.push_back(newMesh);
	return m_pNewModel->meshes.size();
}

void LodExporter::addPrimitive(Primitive* primitive, Mesh* mesh, MyMesh* myMesh)
{
	primitive->mode = mesh->primitives[0].mode;
	int mtlIdx = addMaterial(mesh->primitives[0].material);
	primitive->material = mtlIdx;
	std::map<std::string, int>::iterator it;
	for (it = mesh->primitives[0].attributes.begin(); it != mesh->primitives[0].attributes.end(); ++it)
	{
		int idx = addAccessor();
		primitive->attributes.insert(make_pair(it->first, idx));
	}
	int idx = addAccessor();
	primitive->indices = idx;
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
	int idx = m_pNewModel->materials.size();
	
	m_materialCache.insert(make_pair(material, idx));
}

int LodExporter::addAccessor()
{
	return 0;
}


// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include <iostream>
// stuff to define the mesh
#include "vcg/complex/complex.h"
#include "wrap/io_trimesh/export_ply.h"
// local optimization
#include <vcg/complex/algorithms/local_optimization.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>
#include "tiny_gltf.h"
#include <vcg/complex/algorithms/clean.h>
using namespace tinygltf;

using namespace vcg;
using namespace tri;
using namespace std;

// The class prototypes.
class MyVertex;
class MyEdge;
class MyFace;

struct MyUsedTypes : public UsedTypes<Use<MyVertex>::AsVertexType, Use<MyEdge>::AsEdgeType, Use<MyFace>::AsFaceType> {};

class MyVertex : public Vertex< MyUsedTypes,
    vertex::VFAdj,
    vertex::Coord3f,
    vertex::Normal3f,
    vertex::Mark,
    vertex::Qualityf,
    vertex::BitFlags  > {
public:
    vcg::math::Quadric<double> &Qd() { return q; }
private:
    math::Quadric<double> q;
};

class MyEdge : public Edge< MyUsedTypes> {};

typedef BasicVertexPair<MyVertex> VertexPair;

class MyFace : public Face< MyUsedTypes,
    face::VFAdj,
    face::VertexRef,
    face::BitFlags > {};

// the main mesh class
class MyMesh : public vcg::tri::TriMesh<std::vector<MyVertex>, std::vector<MyFace> > {};


class MyTriEdgeCollapse : public vcg::tri::TriEdgeCollapseQuadric< MyMesh, VertexPair, MyTriEdgeCollapse, QInfoStandard<MyVertex>  > {
public:
    typedef  vcg::tri::TriEdgeCollapseQuadric< MyMesh, VertexPair, MyTriEdgeCollapse, QInfoStandard<MyVertex>  > TECQ;
    typedef  MyMesh::VertexType::EdgeType EdgeType;
    inline MyTriEdgeCollapse(const VertexPair &p, int i, BaseParameterClass *pp) :TECQ(p, i, pp) {}
};


typedef typename MyMesh::VertexPointer VertexPointer;
typedef typename MyMesh::ScalarType ScalarType;
typedef typename MyMesh::VertexType VertexType;
typedef typename MyMesh::FaceType FaceType;
typedef typename MyMesh::VertexIterator VertexIterator;
typedef typename MyMesh::FaceIterator FaceIterator;
typedef typename MyMesh::EdgeIterator EdgeIterator;

int main(int argc, char *argv[])
{
    char* inputPath = NULL;
    char* outputPath = NULL;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "-i") == 0)
        {
            if (i + 1 >= argc)
            {
                printf("Input error\n");
                return -1;
            }
            inputPath = argv[i + 1];
        }
        else if (std::strcmp(argv[i], "-o") == 0)
        {
            if (i + 1 >= argc)
            {
                printf("Input error\n");
                return -1;
            }
            outputPath = argv[i + 1];
        }
    }

    printf("intput file path: %s\n", inputPath);
    
    Model *model = new Model;
    TinyGLTF loader;
    std::string err;
    std::string warn;
    bool ret = loader.LoadASCIIFromFile(model, &err, &warn, inputPath);
    
	MyMesh mergedMesh;
	int indexCount = 0;
    for (int i = 0; i < model->meshes.size(); ++i)
    {
        MyMesh myMesh;

        Mesh mesh = model->meshes[i];

        vcg::tri::Clean<MyMesh>::RemoveUnreferencedVertex(myMesh);

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
        VertexIterator vi = Allocator<MyMesh>::AddVertices(mergedMesh, positionAccessor.count);
        
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
        vi = myMesh.vert.begin();
        for (int j = 0; j < positionAccessor.count; ++j, ++vi)
            index[j] = &*vi;

        int faceNum = indicesAccessor.count / 3;
        FaceIterator fi = Allocator<MyMesh>::AddFaces(mergedMesh, faceNum);

        for (int j = 0; j < faceNum; ++j)
        {
            (*fi).V(0) = index[indices[j * 3 + 0]];
            (*fi).V(1) = index[indices[j * 3 + 1]];
            (*fi).V(2) = index[indices[j * 3 + 2]];
            ++fi;
        }
    }

	/**********************   decimation    *******************************/
	TriEdgeCollapseQuadricParameter qparams;
	qparams.QualityThr = .3;
	qparams.PreserveBoundary = false;
	qparams.PreserveTopology = false;
	qparams.OptimalPlacement = false;
	//double TargetError = std::numeric_limits<double >::max();
	//double TargetError = 0.5;
	//qparams.QualityCheck = true;
	//qparams.HardQualityCheck = false;
	//qparams.NormalCheck = false;
	//qparams.AreaCheck = false;
	//qparams.OptimalPlacement = false;
	//qparams.ScaleIndependent = false;
	//qparams.PreserveBoundary = false;
	//qparams.PreserveTopology = false;
	//qparams.QualityQuadric = false;
	//qparams.QualityWeight = false;
	//qparams.QualityQuadricWeight = atof(argv[i] + 2);
	//qparams.QualityWeightFactor = atof(argv[i] + 2);
	//qparams.QualityThr = atof(argv[i] + 2);
	//qparams.HardQualityThr = atof(argv[i] + 2);
	//qparams.NormalThrRad = math::ToRad(atof(argv[i] + 2));
	//qparams.BoundaryQuadricWeight = atof(argv[i] + 2);
	//qparams.QuadricEpsilon = atof(argv[i] + 2);

	float FinalSize = 0.5 * 0.5 * mergedMesh.fn;
	bool CleaningFlag = true;

	vcg::tri::UpdateBounding<MyMesh>::Box(mergedMesh);

	// decimator initialization
	vcg::LocalOptimization<MyMesh> DeciSession(mergedMesh, &qparams);

	int t1 = clock();
	DeciSession.Init<MyTriEdgeCollapse>();
	int t2 = clock();
	printf("Initial Heap Size %i\n", int(DeciSession.h.size()));

	DeciSession.SetTargetSimplices(FinalSize);
	DeciSession.SetTimeBudget(0.5f);
	DeciSession.SetTargetOperations(100000);
	//if (TargetError< std::numeric_limits<float>::max()) DeciSession.SetTargetMetric(TargetError);

	int counter = 0;
	do {
		DeciSession.DoOptimization();
		counter++;
	} while (mergedMesh.fn > FinalSize && counter < 100);
	//DeciSession.DoOptimization();
	//while (DeciSession.DoOptimization() && myMesh.fn>FinalSize && DeciSession.currMetric < TargetError)
	//    printf("Current Mesh size %7i heap sz %9i err %9g \n", myMesh.fn, int(DeciSession.h.size()), DeciSession.currMetric);

	printf("mesh  %d %d Error %g \n", mergedMesh.vn, mergedMesh.fn, DeciSession.currMetric);
	//printf("\nCompleted in (%5.3f+%5.3f) sec\n", float(t2 - t1) / CLOCKS_PER_SEC, float(t3 - t2) / CLOCKS_PER_SEC);

	char testOutputPath[1024];
	sprintf(testOutputPath, "../data/after.ply");
	vcg::tri::io::ExporterPLY<MyMesh>::Save(mergedMesh, testOutputPath);
    // step0. Read gltf into vcglib mesh.
 
    // step1. Figure out the material with different ids and have the same value.
 
    // step2. Simpilify meshes. ( This should be done before the mesh is merged. 
    // Since we use quadratic error method, the gaps between maybe closed. 

    // step3. Merge the meshes and add "batchId" in vertex attributes.

    return 0;
}
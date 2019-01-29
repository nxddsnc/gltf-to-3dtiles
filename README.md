# GLTF-Optimizer
A command line tool to convert gltf(glb) file to 3d tiles format.

#Usage example
GLTF_OPTIMIZER.exe -i "../data/test.gltf" -o "../data/output"

#Build
git clone -b devel https://github.com/cnr-isti-vclab/vcglib.git
Set environment VCG_LIB to root of vcglib.
git clone https://github.com/nxddsnc/GLTF-Optimizer.git
Build it with vs2015 community.
# Overview
There are serveral step for the conversion.
Step1. Import gltf file use [tinygltf](https://github.com/syoyo/tinygltf).
Step2. Create hierachical binary tree and insert nodes to boxes according to it's bounding box center.
Step3. Generate tileset structure according to the result of step2.
Step4. Do decimation on each level of the tree with the powerful mesh processing library [vcglib](http://vcg.isti.cnr.it/vcglib/).
Step5. Output the tileset.json file and corresponding gltf/glb(contains batchId) files.

After conversion, we can use 3dtiles-tools to convert glb to b3dm so that it can be viewed in cesium.js. Or use the command called "glbsToB3dms" in my fork (https://github.com/nxddsnc/3d-tiles-tools).

# Notice
##### It is just a test project yet and the gltf file format is limited to the structure as it was in https://github.com/nxddsnc/GLTF-Optimizer/tree/master/data with no textures supported yet. And currently I am working on branch dev#mergeMesh.
##### If you have any other question, please contact me(fanqileiOGL@163.com).



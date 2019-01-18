// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"
#include <iostream>

using namespace tinygltf;
//bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, argv[1]);
//bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb) 

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
            outputPath = argv[i + 1];
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
    
    Model model;
    TinyGLTF loader;
    std::string err;
    std::string warn;
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, inputPath);
    
    printf("output file path: %s\n", outputPath);

    return 0;
}
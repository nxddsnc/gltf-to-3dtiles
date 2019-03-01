#pragma once
struct GlobalSetting
{
	char* inputPath;
	char* outputPath;
	int maxTreeDepth;
	int tileLevel;
	bool writeBinary;
    bool printLog;
    uint32_t batchLength;
    int faceCountPerTile;
    float mergeVertexThr = 0.001;
};

extern GlobalSetting g_settings;
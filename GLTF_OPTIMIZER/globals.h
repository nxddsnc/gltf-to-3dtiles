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
};

extern GlobalSetting g_settings;
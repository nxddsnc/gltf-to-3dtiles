#pragma once
struct GlobalSetting
{
	char* inputPath;
	char* outputPath;
	int maxTreeDepth;
	int tileLevel;
	bool writeBinary;
    bool printLog;
};

extern GlobalSetting g_settings;
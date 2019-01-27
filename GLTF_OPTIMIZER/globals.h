#pragma once
struct GlobalSetting
{
	char* inputPath;
	char* outputPath;
	int maxTreeDepth;
	int tileLevel;
	bool writeBinary;
};

extern GlobalSetting g_settings;
#pragma once
struct GlobalSetting
{
	char* inputPath;
	char* outputPath;
	int maxTreeDepth;
	int tileLevel;
	bool writeBinary;
};

static GlobalSetting g_settings;
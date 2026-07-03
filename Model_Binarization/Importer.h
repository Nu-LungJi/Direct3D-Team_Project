#pragma once
#include "pch.h"


class CImporter
{
public:
	enum class MODEL_CATEGORY
	{
		STATIC,
		SKELETAL,
		ANIMATION
	};

public:
	CImporter();
	~CImporter();

public:


	HRESULT ImportFBXFolder(const std::string& strLevelName,const std::string& strSourceFolder);

};


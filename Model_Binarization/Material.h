#pragma once

#include "pch.h"


class CMaterial
{
public:
	CMaterial();

	~CMaterial();



	uint32_t m_materialNum;
	std::vector<std::vector<TEXTUREINFO>> m_textures;

};


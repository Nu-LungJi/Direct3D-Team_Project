#pragma once
#include "pch.h"

class CMesh
{
public:
	CMesh();

	~CMesh();




	std::string m_name;
	

	uint32_t m_materialIndex;

	XMFLOAT3 m_min;
	XMFLOAT3 m_max;


	std::shared_ptr<std::vector<VTXMESH>> m_vertices;
	std::shared_ptr<std::vector<VTXANIMMESH>> m_animvertices;
	std::shared_ptr<std::vector<uint32_t>> m_indices;

	uint32_t		m_iNumBones = {}; /* 이 메시가 이용하는 뼈의 갯수. */

	/*  이 메시에 영향을 주는 뼈들의 전체뼈기준의 인덱스 */
	std::shared_ptr<std::vector<uint32_t>>	m_BoneIndices;
	std::shared_ptr<std::vector<XMFLOAT4X4>>	m_BoneMatrices;
	std::shared_ptr<std::vector<XMFLOAT4X4>>	m_OffsetMatrices;
};


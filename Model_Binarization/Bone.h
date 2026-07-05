#pragma once
#include "pch.h"

class CBone
{
public:
	CBone();

	~CBone();


	bool Compare_Name(std::string pBoneName) {
		if (pBoneName == Bone.m_name) {
			return true;
		}
		else {
			return false;
		}
	}

	BONEINFO Bone;
	//std::string m_name;
	//int32_t m_patrentBoneIndex;
	//XMFLOAT4X4 m_TransformationMatrix;
};



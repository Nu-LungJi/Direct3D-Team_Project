#pragma once

#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResModelBone final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResModelBone, CResource)
public:
	typedef struct tagDesc {
		_char* ptr;
	}DESC;
private:
	explicit CResModelBone(const _string& sPath);
	~CResModelBone() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;


public:
	_matrix Get_CombinedTransformationMatrix() {
		return XMLoadFloat4x4(&m_CombinedTransformationMatrix);
	}

	const _float4x4* Get_CombinedTransformationMatrixPtr() {
		return &m_CombinedTransformationMatrix;
	}

	void Set_TransformationMatrix(_fmatrix TransformationMatrix) {
		XMStoreFloat4x4(&m_TransformationMatrix, TransformationMatrix);
	}

public:
	void Update_CombinedTransformationMatrix(const std::vector<SPtr<CResModelBone>>& Bones, _fmatrix PreTransformMatrix);

public:
	_bool Compare_Name(const _char* pBoneName) {
		return !strcmp(pBoneName, m_szName);
	}
private:
	_char			m_szName[MAX_PATH] = {  };
	_float4x4		m_TransformationMatrix = { }; /* 이 뼈만의 상태행렬 */
	_float4x4		m_CombinedTransformationMatrix = {}; /* 부모 뼈의 상태를 포함한 최종 행렬 */
	int32_t			m_iParentBoneIndex = { -1 };

public:
	static SPtr<CResModelBone> Create(const _string& sPath = {});
};

NS_END
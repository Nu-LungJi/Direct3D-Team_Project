#pragma once

#include "ResVIBuffer.h"
struct aiMesh;
NS_BEGIN(Engine)

class CResTestModel;
class CResTestModelBone;

class ENGINE_DLL CResTestModelMesh final : public CResVIBuffer
{
public:
	DECLARE_DERIVED_TYPE(CResTestModelMesh, CResVIBuffer)
public:
	typedef struct tagDesc {
		MODEL eType;
		CResTestModel* pModel;
		const aiMesh* pAIMesh;
		_matrix PreTransformMatrix;
	}DESC;
private:
	explicit CResTestModelMesh(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResTestModelMesh() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	
private:
	HRESULT Ready_NonAnimMesh(const aiMesh* pAIMesh, _fmatrix PreTransformMatrix);
	HRESULT	Ready_AnimMesh(class CResTestModel* pModel, const aiMesh* pAIMesh);


public:
	uint32_t Get_MaterialIndex() const {return m_iMaterialIndex;}
	uint32_t Get_BoneIndex() const {return m_iNumBones;}

public:
	std::vector<uint32_t>& GetBoneIndices() { return m_BoneIndices; }
	std::vector<_float4x4>& GetBoneMatrices() { return m_BoneMatrices; }
	std::vector<_float4x4>& GetOffsetMatrices() { return m_OffsetMatrices; }
	ComPtr<ID3D11Buffer> GetCBBones() { return m_pCBBones; }
private:
	ComPtr<ID3D11Buffer> m_pCBBones;

private:
	uint32_t		m_iMaterialIndex = {};
	uint32_t		m_iNumBones = {}; /* 이 메시가 이용하는 뼈의 갯수. */

	/*  이 메시에 영향을 주는 뼈들의 전체뼈기준의 인덱스 */
	_char				m_szName[MAX_PATH] = {};
	std::vector<uint32_t>	m_BoneIndices;
	std::vector<_float4x4>	m_BoneMatrices;
	std::vector<_float4x4>	m_OffsetMatrices;

public:
	static SPtr<CResTestModelMesh> Create();
};

NS_END
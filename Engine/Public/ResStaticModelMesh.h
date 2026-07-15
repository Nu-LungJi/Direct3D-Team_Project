

#pragma once

#include "ResVIBuffer.h"

NS_BEGIN(Engine)

class CResStaticModel;
//class CResTestModelBone;

class ENGINE_DLL CResStaticModelMesh final : public CResVIBuffer
{
public:
	DECLARE_DERIVED_TYPE(CResStaticModelMesh, CResVIBuffer)
public:
	typedef struct tagDesc {
		MODEL eType;
		CResStaticModel* pModel;
		_char* ptr;
		_matrix PreTransformMatrix;
	}DESC;
private:
	explicit CResStaticModelMesh(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResStaticModelMesh() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	const XMFLOAT3& GetMinPos() const { return m_vMinPos; }
	const XMFLOAT3& GetMaxPos() const { return m_vMaxPos; }

public:
	HRESULT LoadAssimp(std::string name, uint32_t materialIndex, const XMFLOAT3& minPos, const XMFLOAT3& maxPos, std::vector<VTXMESH>&& vertices, std::vector<uint32_t>&& indices, _fmatrix PreTransformMatrix);
private:
	HRESULT Ready_NonAnimMesh(_char* pPoint, _fmatrix PreTransformMatrix);

public:
	uint32_t Get_MaterialIndex() const { return m_iMaterialIndex; }

private:
	uint32_t		m_iMaterialIndex = {};

	XMFLOAT3 m_vMinPos{};
	XMFLOAT3 m_vMaxPos{};

public:
	static SPtr<CResStaticModelMesh> Create();
};

NS_END

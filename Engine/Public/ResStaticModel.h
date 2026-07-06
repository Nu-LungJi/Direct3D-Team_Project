#pragma once

#include "Resource.h"
#include "ResStaticModelMesh.h"
#include "ResModelMaterial.h"


NS_BEGIN(Engine)

class ENGINE_DLL CResStaticModel final : public CResource
{

public:
	DECLARE_DERIVED_TYPE(CResStaticModel, CResource)
public:
	typedef struct tagDesc {
		_matrix PreTransformMatrix;
	}DESC;
private:
	explicit CResStaticModel(const _string& sPath);
	~CResStaticModel() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;


private:

	HRESULT Ready_Meshes(_char* ptr);
	HRESULT Ready_Materials(const _string& strModelFilePath, _char* ptr);
public:
	uint32_t Get_NumMeshes() const { return m_iNumMeshes; }
	const _float4x4& Get_PreTransformMatrix() { return m_PreTransformMatrix; }




public:
	std::vector<SPtr<CResStaticModelMesh>>& GetMeshes() { return m_Meshes; }
	std::vector<SPtr<CResModelMaterial>>& GetMaterials() { return m_Materials; }


protected:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};


private:
	MODEL						m_eModelType = {};
	uint32_t					m_iNumMeshes = {};
	std::vector<SPtr<CResStaticModelMesh>>	m_Meshes;

	uint32_t						m_iNumMaterials;
	std::vector<SPtr<CResModelMaterial>>	m_Materials;


private:
	_float4x4				m_PreTransformMatrix = {};

public:
	static SPtr<CResStaticModel> Create(const _string& sPath);
};

NS_END
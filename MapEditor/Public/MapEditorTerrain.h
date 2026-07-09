#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComConstantBuffer;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;
NS_END

NS_BEGIN(Client)
class CResMapEditorTerrainVIBuffer;
class CMapEditorTerrain final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CMapEditorTerrain, CGameObject)

public:
	typedef struct tagTerrainDesc : public CGameObject::GAMEOBJECT_DESC
	{
	}DESC;

private:
	CMapEditorTerrain();
	~CMapEditorTerrain() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	const std::vector<VTX_NORMAL_TEX>& GetVertices() const;
	const std::vector<uint32_t>& GetIndices() const;

private:
	SPtr<CResMapEditorTerrainVIBuffer> m_pResMapEditorTerrainVIBuffer{};
	SPtr<CResTexture2D> m_pResTerrainTexture2D{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResSamplerState> m_pResSamplerState{};
	CComConstantBuffer* m_pComCBufferPerObject{};

public:
	static E::UPtr<CMapEditorTerrain> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

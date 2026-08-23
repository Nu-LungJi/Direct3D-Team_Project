#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CResVertexShader;
class CResPixelShader;
class CComModelInstance;
NS_END

NS_BEGIN(Client)

class CPlayer_Broom final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Broom, CGameObject)

	struct BROOM_DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_string sResourceTag{};
		_string sLevelTag{};
		CHandle hParent{};
		int32_t iSocketBoneIndex{ -1 };
		_float3 vScale{ 4.f, 4.f, 4.f };
		_bool bVisible{};
	};

private:
	CPlayer_Broom() = default;
	~CPlayer_Broom() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	bool GetShadowBounds(BoundingBox& outBounds) const override;

	void SetVisible(_bool bVisible) { m_bVisible = bVisible; }
	_bool IsVisible() const { return m_bVisible; }
	void SetMovementRatio(_float fRatio);
	void SetBoostEffectRatio(_float fRatio);

	static UPtr<CPlayer_Broom> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	CComModelInstance* m_pModelInstance{};
	CComConstantBuffer* m_pObjectBuffer{};
	SPtr<CResVertexShader> m_pVertexShader{};
	SPtr<CResPixelShader> m_pPixelShader{};
	CHandle m_hParent{};
	int32_t m_iSocketBoneIndex{ -1 };
	_float4x4 m_ParentMatrix{};
	_bool m_bVisible{};
	_float m_fCurrentHeightOffset{ -1.f };
	_float m_fTargetHeightOffset{ -1.f };
	_float m_fHeightBlendResponse{ 7.f };
	_float m_fBoostEffectRatio{};
	EFFECT_INSTANCE_ID m_iSpeedLineEffectID{ INVALID_EFFECT_INSTANCE_ID };
	_float m_fSpawnTime{ 0.f };
	int32_t m_iBroomEndBoneIndex{0};
};

NS_END

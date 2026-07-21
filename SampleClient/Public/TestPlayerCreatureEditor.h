#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComPxCharacterController;
class CComCharacterMoveIntent;
class CComCharacterMotor;
NS_END

NS_BEGIN(Client)

class CTestPlayerCreatureEditor final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestPlayerCreatureEditor, CGameObject)

public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPosition{ 10.f, 50.f, 10.f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CTestPlayerCreatureEditor();
	CTestPlayerCreatureEditor(const CTestPlayerCreatureEditor& rhs);
	~CTestPlayerCreatureEditor() override;

public:
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;

private:
	CComPxCharacterController* m_pCharacterController{};
	CComCharacterMoveIntent* m_pMoveIntent{};
	CComCharacterMotor* m_pCharacterMotor{};

public:
	static UPtr<CTestPlayerCreatureEditor> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END

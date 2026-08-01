#pragma once
#include "Mon_Weapon.h"
#include "Client_Defines.h"
NS_BEGIN(Client)
class CBossMace final : public CMon_Weapon
{
public:
	DECLARE_DERIVED_TYPE(CBossMace, CMon_Weapon)

public:
	typedef struct tagWeapondesc : public CMon_Weapon::GAMEOBJECT_DESC
	{
		_float3 vScale{ 1.f,1.f,1.f };
		_string	WeaponName{}, LevelTag{};
		CHandle ParentHandle{};
		int32_t iBoneIndex{ -1 };
	}WEAPON_DESC;

private:
	CBossMace();
	~CBossMace() override;

public:
	void UpdateGUI() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;

	/*----------- 광윤 추가 -----------*/
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	/*---------------------------------*/
public:
	void OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;
private:
	void	Enable_Emissive( _float fTimeDelta);
	void	Disable_Emissive(_float fTimeDelta);

	_float		m_fTime{ 3.f };
public:
	static E::UPtr<CBossMace> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

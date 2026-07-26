#pragma once
#include "Monster.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CTmbGurdian final : public CMonster
{
public:
	struct TMBGURDIAN_DESC :public  CMonster::MONSTER_DESC
	{
		_string WeaponResourceName{};
		_string WeaponProtoName{};
	};
public:
	DECLARE_DERIVED_TYPE(CTmbGurdian, CMonster)

private:
	CTmbGurdian();
	~CTmbGurdian() override;

public:
	void UpdateGUI() override;
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void FixedUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;

private:
	//std::vector<UPtr<class CTmbGurdianDead>>				m_DeadMeshes;


public:
	static E::UPtr<CTmbGurdian> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

#pragma once
#include "Monster.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CEnderDragon final : public CMonster
{
public:
	DECLARE_DERIVED_TYPE(CEnderDragon, CMonster)

public:
	typedef struct tagDragonDesc : public CMonster::MONSTER_DESC
	{

	}DRAGON_DESC;

private:
	CEnderDragon();
	~CEnderDragon() override;

public:
	void UpdateGUI() override;
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void FixedUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT			Ready_Fsm(const _string& LevelTag);

	virtual _bool				Update_BT()override;

private:
	void						Update_BBToFsm();
private:
	class CEnderDragon_State* m_pFsm{ nullptr };
	
public:
	static E::UPtr<CEnderDragon> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

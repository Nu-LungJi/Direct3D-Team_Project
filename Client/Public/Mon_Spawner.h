#pragma once
#include "Client_Defines.h"
#include "GameObject.h"
NS_BEGIN(Client)
class CMon_Spawner : public CGameObject
{
public:
	typedef struct strmonspawner : public CGameObject::GAMEOBJECT_DESC
	{
		CHandle handle{};
		_string LevelTag{};
	}MON_SPAWNER_DESC;
public:
	DECLARE_DERIVED_TYPE(CMon_Spawner, CGameObject)

private:
	explicit CMon_Spawner();
	explicit CMon_Spawner(const CMon_Spawner& rhs);
	~CMon_Spawner() override;
public:
	void UpdateGUI() override;
public:
	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initialize(void* pArg) override;
	void						PriorityUpdate(E::_float fTimeDelta) override;
	void						FixedUpdate(E::_float fTimeDelta) override;
	void						Update(E::_float fTimeDelta) override;
	void						LateUpdate(E::_float fTimeDelta) override;

private:
	void						Picking();
	void						Picking_TerrainMon();
private:
	std::vector<CHandle>		m_Monsters;
	_string						m_LeveTag{};
	std::list<_float3>			m_SpawnPos;
	CHandle						m_Handle{};
	_bool						m_bPick{ false };
public:
	static E::UPtr<CMon_Spawner> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};
NS_END

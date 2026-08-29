#pragma once
#include "WorldAgent.h"
#include "Client_Defines.h"


NS_BEGIN(Client)
class CNpcRagdollController;

class CWorldNpc final : public CWorldAgent
{
public:
	DECLARE_DERIVED_TYPE(CWorldNpc, CWorldAgent)

private:
	CWorldNpc();
	CWorldNpc(const CWorldNpc& Prototype);
	~CWorldNpc() override;

public:
	void UpdateGUI() override;
public:
	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initialize(void* pArg) override;
	void						PriorityUpdate(E::_float fTimeDelta) override;
	void						FixedUpdate(E::_float fTimeDelta) override;
	void						Update(E::_float fTimeDelta) override;
	void						LateUpdate(E::_float fTimeDelta) override;
	HRESULT						Ready_Fsm(const _string& LevelTag);
	void						Ready_BBKeyValue(WORLD_AGENT_DESC* pDesc);
	int32_t						Find_AnimIndex(const _string& AnimName);

public:
	void						Set_Gravity(_bool bGravity);
	_bool						Check_Table(PLAYER_SKILL_TYPE eType) override;
	void						Set_Dissolve(_float fDissolve) { m_fDissolve = fDissolve; }
	_bool						RequestRagdollActivation(
		const _float3& vLinearVelocity = {},
		const _float3& vAngularVelocityRadians = {});
	_bool						ResetRagdoll();
	_bool						IsRagdollActive() const;

private:
	UPtr<CNpcRagdollController>	m_pRagdollController{};

public:
	static E::UPtr<CWorldNpc> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

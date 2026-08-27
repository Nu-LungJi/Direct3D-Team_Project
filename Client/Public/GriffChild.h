#pragma once
#include "WorldAgent.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CGriffChild final : public CWorldAgent
{
public:
	DECLARE_DERIVED_TYPE(CGriffChild, CWorldAgent)

private:
	CGriffChild();
	~CGriffChild() override;

public:
	void UpdateGUI() override;
public:
	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initialize(void* pArg) override;
	void						PriorityUpdate(E::_float fTimeDelta) override;
	void						FixedUpdate(E::_float fTimeDelta) override;
	void						Update(E::_float fTimeDelta) override;
	void						LateUpdate(E::_float fTimeDelta) override;
	
public:
	void						Set_Gravity(_bool bGravity);
private:
	void						Chase_Leader(_float fTimeDelta);
private:
	_float3						m_vSpreadDir{};
	_float3						m_vCurDir{};
	_float3						m_vOffsetPos{};
public:
	static E::UPtr<CGriffChild> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

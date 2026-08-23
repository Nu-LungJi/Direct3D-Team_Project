#pragma once
#include "Animal.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CGriffChild final : public CAnimal
{
public:
	DECLARE_DERIVED_TYPE(CGriffChild, CAnimal)

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

	void						Set_Neighbor(std::vector<CHandle>& Neighbors);
public:
	void						Set_Gravity(_bool bGravity);
private:
	void						Chase_Leader(_float fTimeDelta);
private:
	_float3						m_vSpreadDir{};
	_float3						m_vCurDir{};
	_float3						m_vOffsetPos{};
	std::vector<CHandle>		m_Neighbors;
public:
	static E::UPtr<CGriffChild> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

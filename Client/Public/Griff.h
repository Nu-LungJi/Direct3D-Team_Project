#pragma once
#include "WorldAgent.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CGriff final : public CWorldAgent
{
public:
	typedef struct strGriffdesc : CWorldAgent::WORLD_AGENT_DESC
	{
		_string WayName{};
		_string ChildModelTag{};
		_string ChildObjectTag{};
	}GRIFF_DESC;

public:
	DECLARE_DERIVED_TYPE(CGriff, CWorldAgent)

private:
	CGriff();
	~CGriff() override;

public:
	void UpdateGUI() override;
public:
	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initialize(void* pArg) override;
	void						PriorityUpdate(E::_float fTimeDelta) override;
	void						FixedUpdate(E::_float fTimeDelta) override;
	void						Update(E::_float fTimeDelta) override;
	void						LateUpdate(E::_float fTimeDelta) override;
	std::vector<CHandle>&		Get_Neighbor() { return m_ChildHandles; }
public:
	void						Set_Gravity(_bool bGravity);
	void						Set_Child();
	private:
		std::vector<CHandle>		m_ChildHandles;
	std::vector<_float3>		m_WayPoint;
	_string	m_ChildModelTag{}, m_ChildObjectTag{};
	int32_t						m_iIndex{0};
	_bool						m_bLoop{ false };
public:
	static E::UPtr<CGriff> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

#pragma once
#include "Client_Defines.h"
#include "BTAnimRoot.h"

NS_BEGIN(Client)
class CBTAnimNpc final : public CBTAnimRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTAnimNpc, CBTAnimRoot)

private:
	CBTAnimNpc();

	CBTAnimNpc(const CBTAnimNpc& rhs);
	~CBTAnimNpc() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT							InitializePrototype(void* pArg = nullptr) override;
	HRESULT							Initalize(void* pArg)override;
public:
	EVALUATE						Evaluate(_float fTimeDelta) override;
	virtual void					Update_Gui() override;
	void							Abort() override;
	virtual nlohmann::json			Save_Node()override;
	HRESULT							Load_json(const nlohmann::json& j) override;
private:
	void OnEnter()override;
	void OnExit(EVALUATE eResult)override;
private:
	MOVE				m_eMove{ MOVE::STRAIGHT };
	int32_t				m_iAnimIndex{ -1 };
	_float3				m_vLastPos{}, m_vLastDir{};
	_float2				m_vRatio{}, m_vRotRatio{};
	_float				m_fDis{}, m_fTime{};
	_string				m_AnimName{};
public:
	static UPtr<CBTAnimNpc> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

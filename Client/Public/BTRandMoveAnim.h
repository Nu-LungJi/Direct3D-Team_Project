#pragma once
#include "Client_Defines.h"
#include "BTAnimRoot.h"

NS_BEGIN(Client)
class CBTRandMoveAnim final : public CBTAnimRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTRandMoveAnim, CBTAnimRoot)
private:
	CBTRandMoveAnim();
	CBTRandMoveAnim(const CBTRandMoveAnim& rhs);
	~CBTRandMoveAnim() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	void							Abort()override;
	virtual nlohmann::json			Save_Node()override;
	HRESULT							Load_json(const nlohmann::json& j) override;
	EVALUATE						Evaluate(_float fTimeDelta) override;
	virtual void					Update_Gui() override;
private:
	void							RandomDirSelect();
	EVALUATE						Move(_float fTimeDelta);
private:
	_float3							m_vDir{}, m_vFinishPos{};
	_bool							m_bInit{ false };
	_float							m_fDis{}, m_fClamp{};
public:
	static UPtr<CBTRandMoveAnim> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END


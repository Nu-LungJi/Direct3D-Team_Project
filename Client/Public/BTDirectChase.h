#pragma once
#include "Client_Defines.h"
#include "BTAnimRoot.h"

NS_BEGIN(Client)
class CBTDirectChase final : public CBTAnimRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTDirectChase, CBTAnimRoot)
private:
	CBTDirectChase();

	CBTDirectChase(const CBTDirectChase& rhs);
	~CBTDirectChase() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT					InitializePrototype(void* pArg = nullptr) override;
	HRESULT					Initalize(void* pArg)override;
public:
	EVALUATE				Evaluate(_float fTimeDelta) override;
	virtual void			Update_Gui() override;
	void					Abort()override;
	virtual nlohmann::json	Save_Node()override;
	HRESULT					Load_json(const nlohmann::json& j) override;

	virtual void		OnEnter() override;
	virtual void		OnExit(EVALUATE eResult) override;
private:
	_bool			m_bUseCurAnim{ false };
	_float          m_fSpeed{}, m_fAngle{}, m_fDist{};
public:
	static UPtr<CBTDirectChase> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

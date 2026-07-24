#pragma once
#include "Client_Defines.h"
#include "BTAnimRoot.h"

NS_BEGIN(Client)
class CBTAnimation final : public CBTAnimRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTAnimation, CBTAnimRoot)
private:
	CBTAnimation();

	CBTAnimation(const CBTAnimation& rhs);
	~CBTAnimation() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT	InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initalize(void* pArg)override;
public:
	EVALUATE				Evaluate(_float fTimeDelta) override;
	virtual void			Update_Gui() override;
	void					Abort()override;
	virtual nlohmann::json	Save_Node()override;
	HRESULT					Load_json(const nlohmann::json& j) override;
private:
	uint32_t			m_iEndFlag{}, m_iStartFlag{};
public:
	static UPtr<CBTAnimation> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

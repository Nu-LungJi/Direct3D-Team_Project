#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTTeleport final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTTeleport, CBTActionNode)
private:
	CBTTeleport();
	CBTTeleport(const CBTTeleport& rhs);
	~CBTTeleport() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;
	virtual void		Update_Gui() override;
private:
	MOVE						m_eMove{};
public:
	static UPtr<CBTTeleport> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END


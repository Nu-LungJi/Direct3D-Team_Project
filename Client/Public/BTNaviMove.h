#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTNaviMove final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTNaviMove, CBTActionNode)
private:
	CBTNaviMove();
	CBTNaviMove(const CBTNaviMove& rhs);
	~CBTNaviMove() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	virtual nlohmann::json			Save_Node()override;
	HRESULT							Load_json(const nlohmann::json& j) override;
	EVALUATE						Evaluate(_float fTimeDelta) override;
	virtual void					Update_Gui() override;

private:
	void						Abort()override;
	void						OnEnter()override;
	void						OnExit(EVALUATE eResult)override;
	_bool						Sweep(_vector vNextDir, _vector vCurDir,_float3 vCurPos,_float fDist);
private:
	MOVE						m_eMove{};
	int32_t						m_iNaviPathIndex;
	_float3						m_vLastDir{}, m_vSlideDir{};
	_bool						m_bMoveToEnd{ true }, m_bSweep{ false};
	std::vector<_float3>		m_NaviPath;
	std::vector<_float3>		m_Separations;
public:
	static UPtr<CBTNaviMove> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END


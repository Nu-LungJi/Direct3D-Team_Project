#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)
class CHandle;
NS_END

NS_BEGIN(Client)

class CFlightRing final : public CGameObject
{
public:
	struct CFlightRing_DESC : public CGameObject::GAMEOBJECT_DESC
	{

	};
public:
	DECLARE_DERIVED_TYPE(CFlightRing, CGameObject)

private:
	CFlightRing();
	~CFlightRing() override;

public:
	void UpdateGUI() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;

	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
public:
	_bool PassCheck(CHandle hPlayer);
	const _bool IsCheckComplete() const { return m_bCheckComplete; }

private:
	_float3 m_prevPlayerPos{}; // 이전 프레임 플레이어 포지션
	_float3 m_curPlayerPos{}; // 현재 프레임 플레이어 포지션

	const _float m_fRingRadius = 5.f;

	_bool m_bCheckComplete = false;

public:
	static E::UPtr<CFlightRing> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
